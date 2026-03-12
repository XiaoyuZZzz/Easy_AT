# AT 命令库 - 用户指南

## 📖 概述

这是一个基于**链接段**和**X 宏**实现的 AT 命令客户端库，支持编译时静态注册 AT 命令。

### 特性

- ✅ **编译时注册**：零 RAM 开销，命令表存储在 Flash 中
- ✅ **自动执行**：按注册顺序自动执行所有命令
- ✅ **灵活配置**：每个命令可独立配置超时时间
- ✅ **回调机制**：收到期望响应时自动调用用户回调
- ✅ **易于移植**：只需实现 2 个移植层接口

---

## 🚀 快速开始

### 1. 文件结构

```
AT/
├── Inc/
│   ├── at_kernel.h      # 核心头文件（包含 API 和宏定义）
│   ├── at_register.h    # 用户注册文件（在此注册你的命令）
│   └── at_port.h        # 移植层接口（声明）
├── Src/
│   ├── at_fms.c         # 核心实现（命令执行引擎）
│   └── at_port.c        # 移植层实现（需要修改）
└── README.md            # 本文档
```

### 2. 集成步骤

#### Step 1: 修改 Makefile

在 `Makefile` 中添加源文件和头文件路径：

```makefile
# 添加 AT 库源文件
C_SOURCES += \
AT/Src/at_fms.c \
AT/Src/at_port.c

# 添加头文件路径
C_INCLUDES += \
-IAT/Inc
```

#### Step 2: 修改链接器脚本

`STM32F103XX_FLASH.ld` 已添加 `.at_cmd_table` 段支持。

#### Step 3: 实现移植层接口

修改 `AT/Src/at_port.c`：

```c
#include "at_port.h"
#include "main.h"

// 实现发送函数（根据实际使用的串口修改）
void AT_Port_Send(const uint8_t *data, uint16_t len)
{
    HAL_UART_Transmit(&huart1, data, len, 1000);
}

// 实现获取 tick 函数
uint32_t AT_Port_GetTick(void)
{
    return HAL_GetTick();
}
```

#### Step 4: 注册你的 AT 命令

修改 `AT/Inc/at_register.h`：

```c
#ifndef __AT_REGISTER_H__
#define __AT_REGISTER_H__

#include "at_kernel.h"

// 声明你的回调函数
static void My_Cmd_Callback(void *user_data);

// 注册命令
// 参数：名称，AT 命令字符串，期望响应，回调函数，用户数据，超时时间 (ms)
AT_CMD_REGISTER(MY_CMD, "AT+TEST", "OK", My_Cmd_Callback, NULL, 1000);

// 回调函数实现
static void My_Cmd_Callback(void *user_data)
{
    // 命令执行成功的处理逻辑
}

#endif
```

#### Step 5: 在主程序中调用

```c
#include "at_kernel.h"

int main(void)
{
    HAL_Init();
    SystemClock_Config();
    MX_GPIO_Init();
    
    // 初始化 AT 系统
    AT_Init();
    
    // 启动命令执行
    AT_Start();
    
    while (1)
    {
        // 处理 AT 命令（必须在主循环中调用）
        AT_Process();
    }
}
```

#### Step 6: 串口接收中断中调用

```c
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == USART1) {
        uint8_t rx_data = huart->Instance->DR;
        
        // 将接收到的数据传递给 AT 库
        AT_ReceiveData(&rx_data, 1);
        
        // 重新启动接收
        HAL_UART_Receive_IT(&huart1, &rx_data, 1);
    }
}
```

---

## 📋 API 参考

### 核心函数

| 函数 | 说明 |
|------|------|
| `AT_Init()` | 初始化 AT 命令系统 |
| `AT_Start()` | 启动命令执行（按顺序执行所有注册命令） |
| `AT_Stop()` | 停止命令执行 |
| `AT_Process()` | 主处理函数（需在主循环中调用） |
| `AT_ReceiveData(data, len)` | 接收串口数据（在中断中调用） |

### 注册宏

```c
AT_CMD_REGISTER(cmd_name, cmd_str, expected_resp, cb, user_data, timeout)
```

**参数说明：**

| 参数 | 说明 | 示例 |
|------|------|------|
| `cmd_name` | 命令名称（标识符，用于区分） | `WIFI_CONNECT` |
| `cmd_str` | AT 命令字符串（实际发送） | `"AT+CWJAP=\"SSID\",\"PWD\""` |
| `expected_resp` | 期望响应（收到此响应触发回调） | `"OK"` |
| `cb` | 回调函数指针 | `My_Callback` |
| `user_data` | 用户数据指针（传递给回调） | `NULL` 或 `(void*)0x1234` |
| `timeout` | 超时时间（毫秒） | `1000` |

### 查询函数

```c
AT_CmdState_t AT_GetCmdState(const char *cmd_name);  // 获取命令状态
uint16_t AT_GetCurrentIndex(void);                   // 获取当前索引
uint16_t AT_GetCmdCount(void);                       // 获取命令总数
void AT_SetTimeoutCallback(void (*cb)(const char*)); // 设置超时回调
```

---

## 💡 使用示例

### 示例 1：简单测试

```c
// 注册一个简单的测试命令
AT_CMD_REGISTER(TEST, "AT", "OK", Test_Callback, NULL, 1000);

static void Test_Callback(void *user_data)
{
    // AT 命令测试成功
}
```

### 示例 2：WiFi 连接

```c
AT_CMD_REGISTER(WIFI_CONNECT, 
                "AT+CWJAP=\"MyWiFi\",\"Password123\"", 
                "WIFI GOT IP", 
                WiFi_Connect_Callback, 
                NULL, 
                10000);  // 10 秒超时

static void WiFi_Connect_Callback(void *user_data)
{
    // WiFi 连接成功，可以点亮指示灯
    HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, GPIO_PIN_SET);
}
```

### 示例 3：发送数据

```c
AT_CMD_REGISTER(SEND_DATA, 
                "AT+CIPSEND", 
                "SEND OK", 
                Send_Callback, 
                NULL, 
                2000);

static void Send_Callback(void *user_data)
{
    // 数据发送成功
}
```

### 示例 4：传递用户数据

```c
// 传递参数给回调函数
AT_CMD_REGISTER(CUSTOM, 
                "AT+CUSTOM=123", 
                "OK", 
                Custom_Callback, 
                (void*)0x1234,  // 用户数据指针
                1000);

static void Custom_Callback(void *user_data)
{
    uint32_t param = (uint32_t)user_data;
    // 使用 param = 0x1234
}
```

### 示例 5：条件编译

```c
#ifdef USE_WIFI_MODULE
    AT_CMD_REGISTER(WIFI_INIT, "AT+GMR", "OK", WiFi_Init_Callback, NULL, 1000);
    AT_CMD_REGISTER(WIFI_CONNECT, "AT+CWJAP=\"SSID\",\"PWD\"", "OK", WiFi_Connect_Callback, NULL, 5000);
#endif

#ifdef USE_BLE_MODULE
    AT_CMD_REGISTER(BLE_INIT, "AT+BLEINIT=1", "OK", BLE_Init_Callback, NULL, 1000);
#endif
```

---

## ⚠️ 注意事项

1. **命令名称唯一性**：每个 `AT_CMD_REGISTER` 的 `cmd_name` 必须是全局唯一的
2. **回调执行上下文**：回调函数在串口中断上下文中执行，避免长时间操作
3. **主循环调用**：必须在主循环中调用 `AT_Process()`
4. **及时传递数据**：串口接收中断中及时调用 `AT_ReceiveData()`
5. **超时处理**：合理设置超时时间，避免命令卡死
6. **Flash 空间**：每个命令约占用 20-30 字节 Flash 空间

---

## 🔧 高级功能

### 超时回调

```c
void Timeout_Handler(const char *cmd_name)
{
    printf("Command timeout: %s\r\n", cmd_name);
}

// 在初始化时设置
AT_SetTimeoutCallback(Timeout_Handler);
```

### 查询命令状态

```c
AT_CmdState_t state = AT_GetCmdState("WIFI_CONNECT");

if (state == AT_CMD_STATE_COMPLETED) {
    // 命令执行完成
}
```

### 手动控制执行流程

```c
// 不自动启动，手动控制
AT_Init();

// 在某个条件下启动
if (need_send_at) {
    AT_Start();
}

// 随时可以停止
AT_Stop();
```

---

## 📝 常见问题

### Q1: 如何修改串口？
A: 修改 `AT/Src/at_port.c` 中的 `AT_Port_Send()` 函数，使用你的串口句柄。

### Q2: 如何添加更多命令？
A: 在 `AT/Inc/at_register.h` 中添加 `AT_CMD_REGISTER` 宏调用。

### Q3: 命令不执行怎么办？
A: 检查：
   - 链接器脚本是否添加了 `.at_cmd_table` 段
   - 是否正确调用了 `AT_Init()` 和 `AT_Start()`
   - 主循环中是否调用了 `AT_Process()`

### Q4: 如何调试？
A: 重定向 `printf` 到串口，查看 AT 库的调试信息。

---

## 📄 License

本库采用 MIT 许可证，可自由使用和修改。

---

## 🤝 贡献

欢迎提交 Issue 和 Pull Request！

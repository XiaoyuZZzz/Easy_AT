# AT 命令库实现总结

## ✅ 完成情况

已成功创建基于**链接段**和**X 宏**的 AT 命令客户端库，支持编译时静态注册。

---

## 📁 文件结构

```
AT/
├── Inc/
│   ├── at_kernel.h      ✅ 核心头文件（API 和宏定义）
│   ├── at_register.h    ✅ 用户注册文件（示例）
│   └── at_port.h        ✅ 移植层接口声明
├── Src/
│   ├── at_fms.c         ✅ 核心实现（命令执行引擎）
│   └── at_port.c        ✅ 移植层实现（需修改）
├── at_user_example.c    ✅ 使用示例
└── README.md            ✅ 详细文档
```

---

## 🔧 集成步骤

### 1. Makefile 已更新

```makefile
# 已添加 AT 库源文件
C_SOURCES += \
AT/Src/at_fms.c \
AT/Src/at_port.c

# 已添加头文件路径
C_INCLUDES += \
-IAT/Inc
```

### 2. 链接器脚本已更新

`STM32F103XX_FLASH.ld` 已添加 `.at_cmd_table` 段：

```ld
.at_cmd_table :
{
    . = ALIGN(4);
    __start_at_cmd_table = .;
    KEEP(*(.at_cmd_table))
    . = ALIGN(4);
    __stop_at_cmd_table = .;
} >FLASH
```

### 3. 编译测试通过

```
text    data     bss     dec     hex filename
3404      20    1572    4996    1384 build/Basic_project.elf
```

---

## 📖 使用方法

### 1. 注册 AT 命令

在 `AT/Inc/at_register.h` 中注册：

```c
#include "at_kernel.h"

// 声明回调函数
static void My_Callback(void *user_data);

// 注册命令
AT_CMD_REGISTER(MY_CMD, "AT+TEST", "OK", My_Callback, NULL, 1000);

// 实现回调
static void My_Callback(void *user_data)
{
    // 命令执行成功后的处理
}
```

### 2. 实现移植层

修改 `AT/Src/at_port.c`：

```c
void AT_Port_Send(const uint8_t *data, uint16_t len)
{
    HAL_UART_Transmit(&huart1, data, len, 1000);
}

uint32_t AT_Port_GetTick(void)
{
    return HAL_GetTick();
}
```

### 3. 在主程序中调用

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
        // 主循环中调用
        AT_Process();
    }
}
```

### 4. 串口中断中接收数据

```c
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == USART1) {
        uint8_t rx_data = huart->Instance->DR;
        AT_ReceiveData(&rx_data, 1);
        HAL_UART_Receive_IT(&huart1, &rx_data, 1);
    }
}
```

---

## 🎯 核心特性

### ✅ 编译时注册（零 RAM 开销）

- 命令表存储在 Flash 中（const）
- 运行时状态存储在 RAM 中（独立数组）
- 无需动态注册，启动即可用

### ✅ 灵活的命令配置

每个命令可独立配置：
- 命令字符串（实际发送的内容）
- 期望响应（触发回调的条件）
- 超时时间（毫秒）
- 回调函数
- 用户数据指针

### ✅ 自动顺序执行

- 按注册顺序依次执行所有命令
- 收到期望响应后自动切换到下一个
- 超时后自动切换到下一个

### ✅ 完善的错误处理

- 超时检测
- 超时回调
- 状态查询

---

## 📋 API 参考

### 核心函数

| 函数 | 说明 |
|------|------|
| `AT_Init()` | 初始化 AT 系统 |
| `AT_Start()` | 启动命令执行 |
| `AT_Stop()` | 停止命令执行 |
| `AT_Process()` | 主处理函数（主循环调用） |
| `AT_ReceiveData(data, len)` | 接收数据（中断调用） |

### 注册宏

```c
AT_CMD_REGISTER(cmd_name, cmd_str, expected_resp, cb, user_data, timeout)
```

| 参数 | 说明 | 示例 |
|------|------|------|
| `cmd_name` | 命令名称（标识符） | `WIFI_CONNECT` |
| `cmd_str` | AT 命令字符串 | `"AT+CWJAP=\"SSID\",\"PWD\""` |
| `expected_resp` | 期望响应 | `"OK"` 或 `"WIFI GOT IP"` |
| `cb` | 回调函数 | `My_Callback` |
| `user_data` | 用户数据指针 | `NULL` 或 `(void*)0x1234` |
| `timeout` | 超时时间（ms） | `1000` |

### 查询函数

```c
AT_CmdState_t AT_GetCmdState(const char *cmd_name);  // 获取状态
uint16_t AT_GetCurrentIndex(void);                   // 当前索引
uint16_t AT_GetCmdCount(void);                       // 命令总数
void AT_SetTimeoutCallback(void (*cb)(const char*)); // 超时回调
```

---

## 💡 使用示例

### 示例 1：简单测试

```c
AT_CMD_REGISTER(TEST, "AT", "OK", Test_Callback, NULL, 1000);

static void Test_Callback(void *user_data)
{
    // AT 测试成功
}
```

### 示例 2：WiFi 连接

```c
AT_CMD_REGISTER(WIFI_CONNECT, 
                "AT+CWJAP=\"MyWiFi\",\"Password\"", 
                "WIFI GOT IP", 
                WiFi_Callback, 
                NULL, 
                10000);

static void WiFi_Callback(void *user_data)
{
    // WiFi 连接成功
}
```

### 示例 3：传递用户数据

```c
AT_CMD_REGISTER(CUSTOM, 
                "AT+CUSTOM=123", 
                "OK", 
                Custom_Callback, 
                (void*)0x1234, 
                1000);

static void Custom_Callback(void *user_data)
{
    uint32_t param = (uint32_t)user_data;
    // 使用 param = 0x1234
}
```

---

## ⚠️ 注意事项

1. **命令名称唯一**：每个 `AT_CMD_REGISTER` 的 `cmd_name` 必须全局唯一
2. **回调执行上下文**：回调在串口中断上下文中执行，避免长时间操作
3. **主循环调用**：必须在主循环调用 `AT_Process()`
4. **及时传递数据**：串口中断及时调用 `AT_ReceiveData()`
5. **超时设置**：合理设置超时时间，避免命令卡死
6. **RAM 限制**：运行时状态数组最多支持 64 个命令（可修改）

---

## 🔍 调试技巧

1. **重定向 printf**：查看 AT 库的调试信息
2. **查询状态**：使用 `AT_GetCmdState()` 检查命令状态
3. **超时回调**：设置超时回调捕获超时命令
4. **串口日志**：在回调中添加串口打印

---

## 📝 下一步建议

1. **修改 at_port.c**：根据实际硬件实现串口发送
2. **添加命令**：在 `at_register.h` 中添加你的 AT 命令
3. **测试通信**：连接串口查看 AT 命令执行情况
4. **扩展功能**：根据需要添加更多高级功能

---

## 🎉 总结

✅ AT 命令库已成功集成到项目中  
✅ 编译测试通过，无错误  
✅ 支持编译时静态注册，零 RAM 开销  
✅ 使用简单，只需注册命令和回调  
✅ 文档完善，包含详细使用说明  

**现在你可以开始使用这个 AT 命令库了！** 🚀

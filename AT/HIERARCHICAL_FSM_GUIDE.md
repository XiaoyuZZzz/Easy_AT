# AT 命令库 - 分层状态机使用指南

## 🎯 概述

本 AT 命令库支持**分层状态机**，可以灵活管理：
- **一次性命令**：只执行一次（如初始化命令 AT、ATE0 等）
- **周期性命令**：定时重复执行（如 CSQ、CELL 等）

---

## 📋 工作原理

### 两个阶段

```
┌─────────────────────────────────────────────────────────┐
│  阶段 1: 初始化阶段 (AT_PHASE_INIT)                      │
│  - 执行所有一次性命令 (AT_CMD_TYPE_ONCE)                 │
│  - AT, ATE0, AT+CGMR 等                                 │
│  - 完成后自动进入循环阶段                                │
└─────────────────────────────────────────────────────────┘
                          ↓
┌─────────────────────────────────────────────────────────┐
│  阶段 2: 循环阶段 (AT_PHASE_LOOP)                        │
│  - 定时执行周期性命令 (AT_CMD_TYPE_PERIODIC)             │
│  - CSQ (每 5 秒), CELL (每 10 秒) 等                      │
│  - 持续循环执行                                          │
└─────────────────────────────────────────────────────────┘
```

---

## 🔧 使用方法

### 1. 注册命令

#### 注册一次性命令（只执行一次）

```c
#include "at_kernel.h"

/* 方法 1：使用便捷宏 */
AT_CMD_ONCE(INIT, "AT", "OK", at_init_callback, 0, 1000);
AT_CMD_ONCE(ECHO_OFF, "ATE0", "OK", at_init_callback, 0, 1000);
AT_CMD_ONCE(CGMR, "AT+CGMR", "OK", at_version_callback, 0, 1000);

/* 方法 2：使用完整宏 */
AT_CMD_REGISTER(INIT, "AT", "OK", at_init_callback, 0, 1000, 
                AT_CMD_TYPE_ONCE, 0);
```

#### 注册周期性命令（定时重复执行）

```c
/* 方法 1：使用便捷宏 */
/* 每 5 秒查询一次信号强度 */
AT_CMD_PERIODIC(CSQ, "AT+CSQ", "OK", at_csq_callback, 0, 1000, 5000);

/* 每 10 秒查询一次基站信息 */
AT_CMD_PERIODIC(CELL, "AT+CELL", "OK", at_cell_callback, 0, 1000, 10000);

/* 方法 2：使用完整宏 */
AT_CMD_REGISTER(CSQ, "AT+CSQ", "OK", at_csq_callback, 0, 1000, 
                AT_CMD_TYPE_PERIODIC, 5000);
```

### 2. 实现回调函数

```c
#include "at_register.h"
#include <stdio.h>

/* 初始化回调 */
void at_init_callback(const char *response, void *user_data)
{
    (void)response;
    (void)user_data;
    
    // 初始化完成的处理逻辑
    // 例如：点亮指示灯
}

/* 版本查询回调 */
void at_version_callback(const char *response, void *user_data)
{
    (void)user_data;
    
    // 处理版本号
    printf("Version: %s\r\n", response);
}

/* 信号强度查询回调（周期性执行） */
void at_csq_callback(const char *response, void *user_data)
{
    (void)user_data;
    
    // 每 5 秒自动执行一次
    // 解析信号强度：+CSQ: <rssi>,<ber>
    int rssi, ber;
    if (sscanf(response, "+CSQ: %d,%d", &rssi, &ber) == 2) {
        printf("Signal Strength: %d (0-31)\r\n", rssi);
    }
}

/* 基站信息查询回调（周期性执行） */
void at_cell_callback(const char *response, void *user_data)
{
    (void)user_data;
    
    // 每 10 秒自动执行一次
    // 处理基站信息
    printf("Cell Info: %s\r\n", response);
}
```

### 3. 主程序调用

```c
#include "at_kernel.h"

int main(void)
{
    /* 系统初始化 */
    HAL_Init();
    SystemClock_Config();
    
    /* AT 库初始化 */
    at_init();
    
    /* 启动命令执行 */
    at_start();
    
    while (1) {
        /* AT 库处理（必须在主循环中调用） */
        at_process();
        
        /* 其他任务... */
        
        /* 延时（建议 10-50ms） */
        HAL_Delay(10);
    }
}

/* 串口中断处理 */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == USART1) {
        uint8_t rx_byte;
        // 读取数据
        // ...
        
        /* 传递给 AT 库处理 */
        at_receive_data(&rx_byte, 1);
    }
}
```

---

## 📊 执行流程示例

### 注册命令
```c
AT_CMD_ONCE(INIT, "AT", "OK", at_init_callback, 0, 1000);
AT_CMD_ONCE(CGMR, "AT+CGMR", "OK", at_version_callback, 0, 1000);
AT_CMD_PERIODIC(CSQ, "AT+CSQ", "OK", at_csq_callback, 0, 1000, 5000);
AT_CMD_PERIODIC(CELL, "AT+CELL", "OK", at_cell_callback, 0, 1000, 10000);
```

### 执行顺序
```
时间线：
  ↓
  0ms:    [INIT] 阶段开始
          ├─ 执行 AT → OK ✓
          ├─ 执行 AT+CGMR → 版本号 ✓
          └─ [INIT] 阶段完成，进入 [LOOP] 阶段
  
  0ms:    [LOOP] 阶段开始
          ├─ 检查 CSQ (间隔 5000ms) → 时间到，执行 → OK ✓
          └─ 检查 CELL (间隔 10000ms) → 时间到，执行 → OK ✓
  
  5000ms: [LOOP] 阶段
          └─ 检查 CSQ (间隔 5000ms) → 时间到，执行 → OK ✓
  
  10000ms: [LOOP] 阶段
           ├─ 检查 CSQ (间隔 5000ms) → 时间到，执行 → OK ✓
           └─ 检查 CELL (间隔 10000ms) → 时间到，执行 → OK ✓
  
  15000ms: [LOOP] 阶段
           └─ 检查 CSQ (间隔 5000ms) → 时间到，执行 → OK ✓
  
  ... 持续循环
```

---

## 🎨 高级功能

### 1. 获取当前阶段

```c
AT_PHASE_T phase = at_get_current_phase();

if (phase == AT_PHASE_INIT) {
    printf("正在初始化...\r\n");
} else if (phase == AT_PHASE_LOOP) {
    printf("正在循环执行...\r\n");
}
```

### 2. 手动触发周期性命令

```c
/* 立即执行一次 CSQ 查询（不管时间间隔） */
at_trigger_periodic_cmd("CSQ");
```

### 3. 混合使用

```c
/* 初始化命令 */
AT_CMD_ONCE(INIT, "AT", "OK", at_init_callback, 0, 1000);
AT_CMD_ONCE(ECHO_OFF, "ATE0", "OK", at_init_callback, 0, 1000);
AT_CMD_ONCE(CMUX, "AT+CMUX=0", "OK", at_init_callback, 0, 1000);

/* 周期性命令 */
AT_CMD_PERIODIC(CSQ, "AT+CSQ", "OK", at_csq_callback, 0, 1000, 5000);
AT_CMD_PERIODIC(CELL, "AT+CELL", "OK", at_cell_callback, 0, 1000, 10000);
AT_CMD_PERIODIC(BATT, "AT+BATT", "OK", at_batt_callback, 0, 1000, 30000);  // 30 秒
```

---

## ⚠️ 注意事项

### 1. 命令执行顺序

- **初始化阶段**：按照注册顺序执行所有一次性命令
- **循环阶段**：周期性命令按照注册顺序依次检查，一次只执行一个

### 2. 时间间隔设置

```c
/* 推荐设置 */
AT_CMD_PERIODIC(CSQ, "AT+CSQ", "OK", at_csq_callback, 0, 1000, 5000);   // 5 秒
AT_CMD_PERIODIC(CELL, "AT+CELL", "OK", at_cell_callback, 0, 1000, 10000); // 10 秒

/* 避免过短间隔 */
AT_CMD_PERIODIC(FAST, "AT+FAST", "OK", callback, 0, 1000, 100);  // ❌ 100ms 太短
```

### 3. 超时处理

```c
/* 设置合适的超时时间 */
AT_CMD_ONCE(INIT, "AT", "OK", at_init_callback, 0, 1000);  // 1 秒超时
AT_CMD_PERIODIC(CSQ, "AT+CSQ", "OK", at_csq_callback, 0, 3000, 5000);  // 3 秒超时
```

### 4. 主循环调用频率

```c
/* 推荐：10-50ms */
while (1) {
    at_process();  // 频繁调用
    HAL_Delay(10);
}

/* 不推荐：间隔太长 */
while (1) {
    at_process();
    HAL_Delay(1000);  // ❌ 可能导致响应处理延迟
}
```

---

## 📝 完整示例

### at_register.h
```c
#ifndef __AT_REGISTER_H__
#define __AT_REGISTER_H__

#include "at_kernel.h"

/* 回调函数声明 */
void at_init_callback(const char *response, void *user_data);
void at_csq_callback(const char *response, void *user_data);

/* 注册命令 */
AT_CMD_ONCE(INIT, "AT", "OK", at_init_callback, 0, 1000);
AT_CMD_PERIODIC(CSQ, "AT+CSQ", "OK", at_csq_callback, 0, 1000, 5000);

#endif
```

### at_register.c
```c
#include "at_register.h"
#include <stdio.h>

void at_init_callback(const char *response, void *user_data)
{
    printf("Init completed: %s\r\n", response);
}

void at_csq_callback(const char *response, void *user_data)
{
    int rssi, ber;
    if (sscanf(response, "+CSQ: %d,%d", &rssi, &ber) == 2) {
        printf("Signal: %d (每 5 秒自动更新)\r\n", rssi);
    }
}
```

### main.c
```c
#include "at_kernel.h"

int main(void)
{
    HAL_Init();
    at_init();
    at_start();
    
    while (1) {
        at_process();
        HAL_Delay(10);
    }
}
```

---

## 🎯 总结

### 优势
✅ **自动管理**：一次性命令自动执行一次，周期性命令定时执行  
✅ **灵活配置**：可以为每个命令设置不同的执行间隔  
✅ **阶段分离**：初始化和循环阶段自动切换  
✅ **易于扩展**：添加新命令只需一行宏定义  

### 适用场景
- ✅ 模块初始化（AT、ATE0 等）
- ✅ 定期查询（CSQ、CELL、BATT 等）
- ✅ 混合任务（先初始化，后循环监控）

**现在你的 AT 库支持分层状态机了！** 🎉

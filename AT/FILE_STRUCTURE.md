# AT 命令库 - 文件结构说明

## 📁 正确的文件组织

### 头文件 vs 源文件

**问题：** 之前 `at_register.h` 中包含了函数实现，这是不正确的。

**解决方案：** 将声明和实现分离到不同的文件。

---

## ✅ 正确的文件结构

### 1. `AT/Inc/at_register.h` - 头文件

**作用：** 只包含**声明**和**注册宏**

```c
#ifndef __AT_REGISTER_H__
#define __AT_REGISTER_H__

#include "at_kernel.h"

/* 1. 声明回调函数（只有声明，没有实现） */
void AT_Init_Callback(void *user_data);
void AT_Version_Callback(void *user_data);
void User_Cmd_Callback(void *user_data);

/* 2. 注册命令（使用宏） */
AT_CMD_REGISTER(INIT, "AT", "OK", AT_Init_Callback, 0, 1000);
AT_CMD_REGISTER(CGMR, "AT+CGMR", "OK", AT_Version_Callback, 0, 1000);
AT_CMD_REGISTER(USER_CMD, "AT+USER", "OK", User_Cmd_Callback, 0, 1000);

#endif
```

**关键点：**
- ✅ 只声明函数原型
- ✅ 只使用注册宏
- ❌ **不包含函数实现**

---

### 2. `AT/Src/at_register.c` - 源文件

**作用：** 实现所有回调函数

```c
#include "at_register.h"
#include "gpio.h"

/* 实现默认命令回调 */
void AT_Init_Callback(void *user_data)
{
    (void)user_data;
    // 初始化完成的处理逻辑
}

void AT_Version_Callback(void *user_data)
{
    (void)user_data;
    // 版本查询完成的处理逻辑
}

/* 实现用户自定义命令回调 */
void User_Cmd_Callback(void *user_data)
{
    (void)user_data;
    // 用户自定义命令的处理逻辑
}
```

**关键点：**
- ✅ 包含 `at_register.h` 获取声明
- ✅ 实现所有回调函数
- ✅ 可以包含其他头文件（如 `gpio.h`）

---

## 📋 用户使用流程

### Step 1: 在 `at_register.h` 中声明和注册

```c
// 声明回调函数
void My_LED_Callback(void *user_data);
void My_WIFI_Callback(void *user_data);

// 注册命令
AT_CMD_REGISTER(LED_CTRL, "AT+LED", "OK", My_LED_Callback, 0, 1000);
AT_CMD_REGISTER(WIFI_CONN, "AT+CWJAP=\"SSID\",\"PWD\"", "OK", My_WIFI_Callback, 0, 10000);
```

### Step 2: 在 `at_register.c` 中实现

```c
#include "at_register.h"
#include "gpio.h"

void My_LED_Callback(void *user_data)
{
    // 实现 LED 控制逻辑
    HAL_GPIO_TogglePin(LED_GPIO_Port, LED_Pin);
}

void My_WIFI_Callback(void *user_data)
{
    // 实现 WiFi 连接成功后的逻辑
    HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, GPIO_PIN_SET);
}
```

---

## 🔧 为什么这样设计？

### 分离声明和实现的好处

1. **符合 C 语言规范**
   - 头文件只包含声明
   - 源文件包含实现
   - 避免重复定义错误

2. **编译效率**
   - 头文件被多个文件包含时不会重复编译函数体
   - 减少编译时间

3. **代码组织**
   - 清晰的职责分离
   - 易于维护和查找

4. **避免链接错误**
   - 如果头文件包含函数实现，多个 `.c` 文件包含时会导致重复定义

---

## ⚠️ 常见错误

### ❌ 错误做法

```c
// at_register.h - 错误示例

void My_Callback(void *user_data)  // 在头文件中实现函数
{
    // 实现代码
}

AT_CMD_REGISTER(MY_CMD, "AT", "OK", My_Callback, 0, 1000);
```

**问题：**
- 多个文件包含此头文件时会报"重复定义"错误
- 不符合 C 语言最佳实践

### ✅ 正确做法

```c
// at_register.h - 正确示例

void My_Callback(void *user_data);  // 只声明

AT_CMD_REGISTER(MY_CMD, "AT", "OK", My_Callback, 0, 1000);

// at_register.c - 正确示例

#include "at_register.h"

void My_Callback(void *user_data)  // 在源文件中实现
{
    // 实现代码
}
```

---

## 📊 编译后的内存布局

```
Flash (代码 + 常量):
├── .text (代码段)
│   └── at_register.c 中的回调函数
├── .rodata (只读数据)
│   └── at_register.h 中注册的命令表
└── .at_cmd_table (AT 命令表)
    └── 所有 AT_CMD_REGISTER 注册的命令

RAM (变量):
├── .data (已初始化数据)
├── .bss (未初始化数据)
└── g_runtime_states (AT 命令运行时状态)
```

---

## 🎯 总结

| 文件 | 内容 | 作用 |
|------|------|------|
| `at_register.h` | 函数声明 + 注册宏 | 定义有哪些 AT 命令 |
| `at_register.c` | 函数实现 | 实现命令的处理逻辑 |

**记忆口诀：**
- 头文件只**声明**
- 源文件来**实现**
- 宏在头文件**注册**
- 函数在源文件**写**

这样设计既符合 C 语言规范，又保持了代码的清晰和可维护性！👍

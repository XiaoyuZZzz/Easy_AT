# AT 命令库 - 命名规范

## 📋 代码风格约定

为了保持代码的一致性和可读性，本库遵循以下命名规范：

---

## 🎯 命名规则

### 1. 宏定义 - 全大写 + 下划线

```c
#define AT_RX_BUFFER_SIZE       256
#define AT_MAX_RESPONSE_LEN     128
#define AT_CMD_REGISTER(...)
```

### 2. 类型定义（struct/enum） - 全大写 + 下划线

```c
typedef enum {
    AT_CMD_STATE_IDLE = 0,      // 枚举值：全大写 + 下划线
    AT_CMD_STATE_SENDING,
} AT_CMD_STATE_T;

typedef struct {
    const char *name;
    const char *cmd_string;
} AT_CMD_T;

typedef struct {
    AT_CMD_STATE_T state;
    uint32_t start_tick;
} AT_CMD_RUNTIME_T;

typedef struct {
    const AT_CMD_T *cmd_table;
    uint16_t cmd_count;
} AT_MANAGER_T;
```

### 3. 函数名 - 全小写 + 下划线（前缀 `at_`）

#### 公共 API 函数
```c
void at_init(void);
void at_start(void);
void at_stop(void);
void at_process(void);
void at_receive_data(const uint8_t *data, uint16_t len);
AT_CMD_STATE_T at_get_cmd_state(const char *cmd_name);
uint16_t at_get_current_index(void);
uint16_t at_get_cmd_count(void);
void at_set_timeout_callback(void (*cb)(const char *cmd_name));
```

#### 移植层函数
```c
void at_port_send(const uint8_t *data, uint16_t len);
uint32_t at_port_get_tick(void);
```

#### 私有函数
```c
static void at_parse_response(void);
static bool at_check_response(const char *response, const char *expected);
static void at_execute_next_command(void);
static void at_send_command(const AT_CMD_T *cmd);
```

### 4. 变量名 - 全小写 + 下划线

#### 全局变量（前缀 `g_`）
```c
static AT_CMD_RUNTIME_T g_runtime_states[64];
static AT_MANAGER_T g_at_manager = {0};
static AT_RX_BUFFER_T g_rx_buffer = {0};
static const AT_CMD_T *g_current_cmd = NULL;
```

#### 局部变量
```c
uint16_t runtime_index = g_at_manager.current_index - 1;
AT_CMD_RUNTIME_T *runtime = &g_runtime_states[runtime_index];
uint32_t current_tick = at_port_get_tick();
```

#### 结构体成员
```c
typedef struct {
    uint8_t buffer[AT_RX_BUFFER_SIZE];
    uint16_t write_index;
    uint16_t read_index;
    char response[AT_MAX_RESPONSE_LEN];
    uint16_t resp_index;
} AT_RX_BUFFER_T;
```

### 5. 回调函数 - 全小写 + 下划线

```c
void at_init_callback(const char *response, void *user_data);
void at_version_callback(const char *response, void *user_data);
void user_cmd_callback(const char *response, void *user_data);
```

### 6. 宏参数 - 全小写 + 下划线

```c
#define AT_CMD_REGISTER(cmd_name, cmd_str, expected_resp, cb, user_data, timeout) \
    __attribute__((used, section(".at_cmd_table"))) \
    const AT_CMD_T at_cmd_##cmd_name = { \
        #cmd_name, \
        cmd_str, \
        expected_resp, \
        cb, \
        (void*)(user_data), \
        timeout \
    }
```

---

## 📊 命名对比表

| 类型 | 旧命名 | 新命名 | 说明 |
|------|--------|--------|------|
| 枚举类型 | `atCmdState_t` | `AT_CMD_STATE_T` | 全大写 + 下划线 |
| 结构体 | `atCmd_t` | `AT_CMD_T` | 全大写 + 下划线 |
| 结构体 | `atCmdRuntime_t` | `AT_CMD_RUNTIME_T` | 全大写 + 下划线 |
| 结构体 | `atManager_t` | `AT_MANAGER_T` | 全大写 + 下划线 |
| 函数 | `atInit()` | `at_init()` | 全小写 + 下划线 |
| 函数 | `atStart()` | `at_start()` | 全小写 + 下划线 |
| 函数 | `atProcess()` | `at_process()` | 全小写 + 下划线 |
| 函数 | `atPortSend()` | `at_port_send()` | 全小写 + 下划线 |
| 函数 | `atReceiveData()` | `at_receive_data()` | 全小写 + 下划线 |
| 回调 | `atInitCallback()` | `at_init_callback()` | 全小写 + 下划线 |
| 变量 | `g_rxBuffer` | `g_rx_buffer` | 全小写 + 下划线 |
| 变量 | `g_currentCmd` | `g_current_cmd` | 全小写 + 下划线 |
| 成员 | `cmdString` | `cmd_string` | 全小写 + 下划线 |
| 成员 | `userData` | `user_data` | 全小写 + 下划线 |
| 成员 | `timeoutMs` | `timeout_ms` | 全小写 + 下划线 |
| 成员 | `startTick` | `start_tick` | 全小写 + 下划线 |

---

## ✅ 使用示例

### 注册命令
```c
// at_register.h
void at_init_callback(const char *response, void *user_data);
AT_CMD_REGISTER(INIT, "AT", "OK", at_init_callback, 0, 1000);
```

### 实现回调
```c
// at_register.c
void at_init_callback(const char *response, void *user_data)
{
    (void)response;
    (void)user_data;
    
    // 处理逻辑
}
```

### 主程序调用
```c
// main.c
#include "at_kernel.h"

int main(void)
{
    at_init();
    at_start();
    
    while (1) {
        at_process();
    }
}
```

---

## 🎨 代码风格总结

### ✅ DO（推荐）
- ✅ 宏定义：全大写 + 下划线 `AT_CMD_REGISTER`
- ✅ 类型名：全大写 + 下划线 `AT_CMD_T`
- ✅ 函数名：全小写 + 下划线 `at_init()`
- ✅ 变量名：全小写 + 下划线 `cmd_count`
- ✅ 全局变量：`g_` 前缀 + 全小写 + 下划线 `g_at_manager`
- ✅ 结构体成员：全小写 + 下划线 `write_index`

### ❌ DON'T（避免）
- ❌ 宏定义：小写 `at_cmd_register`
- ❌ 类型名：小写驼峰 `atCmd_t`
- ❌ 函数名：驼峰 `atInit()`
- ❌ 变量名：驼峰 `cmdCount`
- ❌ 混用风格 `AT_cmd_T` 或 `atCMD_T`

---

## 📝 文件组织

```
AT/
├── Inc/
│   ├── at_kernel.h      # 核心 API（全小写 + 下划线）
│   ├── at_port.h        # 移植层接口（全小写 + 下划线）
│   └── at_register.h    # 用户注册（全小写 + 下划线）
├── Src/
│   ├── at_fms.c         # 核心实现（全小写 + 下划线）
│   ├── at_port.c        # 移植层实现（全小写 + 下划线）
│   └── at_register.c    # 用户回调（全小写 + 下划线）
└── at_user_example.c    # 使用示例（全小写 + 下划线）
```

---

## 🎯 总结

遵循统一的命名规范可以：
- ✅ 提高代码可读性
- ✅ 降低维护成本
- ✅ 保持代码一致性
- ✅ 便于团队协作

**所有新增代码请严格遵循此规范！** 🚀

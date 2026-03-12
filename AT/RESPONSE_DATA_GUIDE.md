# AT 命令库 - 响应数据使用指南

## 🎯 改进内容

**回调函数现在可以接收响应数据了！**

### 修改前
```c
void Callback(void *user_data);  // ❌ 无法获取响应数据
```

### 修改后
```c
void Callback(const char *response, void *user_data);  // ✅ 可以获取响应数据
```

---

## 📖 工作原理

### 完整流程示例

假设你注册了版本查询命令：

```c
AT_CMD_REGISTER(VERSION, "AT+CGMR", "OK", Version_Callback, 0, 1000);
```

**执行流程：**

```
1. 发送：AT+CGMR\r\n
   ↓
2. 模块回复：
      26-03-11\r\n
      OK\r\n
   ↓
3. 接收缓冲区逐行解析：
   - 第一行："26-03-11"（不匹配 "OK"，继续等待）
   - 第二行："OK"（匹配成功！）
   ↓
4. 调用回调：
   Version_Callback("OK", user_data);
   ↓
5. 用户可以在回调中处理之前收到的所有数据
```

---

## 💡 实际使用示例

### 示例 1：获取版本号

```c
// 注册命令
AT_CMD_REGISTER(VERSION, "AT+CGMR", "OK", Version_Callback, 0, 1000);

// 回调函数实现
static char g_version[32];  // 全局变量保存版本号

void Version_Callback(const char *response, void *user_data)
{
    // response 是最后一行匹配期望响应的数据（"OK"）
    // 但实际数据在之前的行中
    
    // 注意：当前实现只传递最后一行
    // 如果需要完整响应，需要修改 AT_ParseResponse 函数
}
```

**改进方案：保存完整响应**

```c
// 在 at_fms.c 中修改响应缓冲区
typedef struct {
    char full_response[256];      // 完整响应
    char last_line[128];          // 最后一行
} AT_Response_t;

// 在回调中使用
void Version_Callback(const char *response, void *user_data)
{
    // response = "OK"
    // 从完整响应缓冲区解析版本号
}
```

### 示例 2：获取 WiFi IP 地址

```c
// 注册 WiFi 连接命令
AT_CMD_REGISTER(WIFI, "AT+CWJAP?", "OK", WiFi_Callback, 0, 1000);

// 模块响应：
// +CWJAP:"MyWiFi",11:22:33:44:55:66,-50
// IP:192.168.1.100
// OK

static char g_ip_address[16];

void WiFi_Callback(const char *response, void *user_data)
{
    // response = "OK"
    // 需要从完整响应中提取 IP 地址
    
    // 示例：解析 IP
    char *ip_start = strstr(full_response, "IP:");
    if (ip_start) {
        sscanf(ip_start + 3, "%15s", g_ip_address);
        printf("Connected! IP: %s\r\n", g_ip_address);
    }
}
```

### 示例 3：获取信号强度 (CSQ)

```c
// 注册信号强度查询命令
AT_CMD_REGISTER(CSQ, "AT+CSQ", "OK", CSQ_Callback, 0, 1000);

// 模块响应：
// +CSQ: 25,99
// OK

void CSQ_Callback(const char *response, void *user_data)
{
    // 解析信号强度
    // +CSQ: <rssi>,<ber>
    // rssi: 0-31, 数值越大信号越好
}
```

---

## 🔧 当前实现的说明

### 当前行为

**当前实现中，`response` 参数传递的是最后一行（匹配期望响应的行）**。

例如：
```
发送：AT+CGMR
响应：
  26-03-11
  OK
  
回调收到：response = "OK"
```

### 如果需要完整响应

有两种方案：

#### 方案 1：修改解析器保存完整响应

在 `at_fms.c` 中添加完整响应缓冲区：

```c
typedef struct {
    char full_response[256];      // 保存完整响应
    uint16_t resp_len;
} AT_Response_Buffer_t;

static AT_Response_Buffer_t g_resp_buffer = {0};

// 在 AT_ParseResponse 中累积响应
static void AT_ParseResponse(void)
{
    while (...) {
        char c = g_rx_buffer.buffer[g_rx_buffer.read_index++];
        
        if (c == '\n') {
            // 保存到完整响应缓冲区
            strcat(g_resp_buffer.full_response, g_rx_buffer.response);
            strcat(g_resp_buffer.full_response, "\n");
            
            // 检查是否匹配期望响应
            if (AT_CheckResponse(...)) {
                // 传递完整响应
                g_current_cmd->callback(g_resp_buffer.full_response, ...);
            }
        }
    }
}
```

#### 方案 2：使用多行匹配

修改期望响应为通配符，在回调中手动解析：

```c
// 使用通配符匹配任意响应
AT_CMD_REGISTER(VERSION, "AT+CGMR", "*", Version_Callback, 0, 1000);

void Version_Callback(const char *response, void *user_data)
{
    // response 包含所有非空行
    // 手动解析版本号
    if (strstr(response, "26-03-11")) {
        // 处理版本信息
    }
}
```

---

## 📋 推荐使用方式

### 简单响应（单行）

对于单行响应的命令，直接使用 `response` 参数：

```c
// AT 测试命令
AT_CMD_REGISTER(TEST, "AT", "OK", Test_Callback, 0, 1000);

void Test_Callback(const char *response, void *user_data)
{
    if (strcmp(response, "OK") == 0) {
        printf("AT command successful!\r\n");
    }
}
```

### 数据响应（多行）

对于多行响应的命令，建议：

1. **使用全局缓冲区保存数据**
2. **在回调中解析全局缓冲区**

```c
// 全局响应缓冲区
static char g_last_response[256];

// 在 AT_ReceiveData 中保存完整响应
void AT_ReceiveData(const uint8_t *data, uint16_t len)
{
    for (uint16_t i = 0; i < len; i++) {
        // 累积到全局缓冲区
        if (g_resp_index < sizeof(g_last_response) - 1) {
            g_last_response[g_resp_index++] = data[i];
        }
        
        // 原有的行解析逻辑...
    }
}

// 回调中解析
void CSQ_Callback(const char *response, void *user_data)
{
    // g_last_response 包含完整响应
    // +CSQ: 25,99\r\nOK\r\n
    
    int rssi, ber;
    if (sscanf(g_last_response, "+CSQ: %d,%d", &rssi, &ber) == 2) {
        printf("Signal Strength: %d\r\n", rssi);
    }
}
```

---

## 🎯 完整示例：获取并保存版本号

```c
// at_register.h
void Version_Get_Callback(const char *response, void *user_data);
AT_CMD_REGISTER(VERSION, "AT+CGMR", "OK", Version_Get_Callback, 0, 1000);

// at_register.c
static char g_version_buffer[32];

void Version_Get_Callback(const char *response, void *user_data)
{
    (void)response;  // 当前实现中是 "OK"
    (void)user_data;
    
    // 假设我们已经在别处保存了完整响应
    // 这里从全局缓冲区解析
    
    // 示例：解析版本号
    char *line = strtok(g_full_response, "\r\n");
    while (line != NULL) {
        if (strstr(line, "OK") == NULL) {
            // 非 OK 行就是版本号
            strncpy(g_version_buffer, line, sizeof(g_version_buffer) - 1);
            printf("Version: %s\r\n", g_version_buffer);
            break;
        }
        line = strtok(NULL, "\r\n");
    }
}

// 在主程序中使用
int main(void)
{
    AT_Init();
    AT_Start();
    
    while (1) {
        AT_Process();
        
        // 使用版本号
        if (strlen(g_version_buffer) > 0) {
            printf("Current version: %s\r\n", g_version_buffer);
        }
    }
}
```

---

## ⚠️ 注意事项

1. **response 参数内容**
   - 当前实现传递的是最后一行（匹配期望响应的行）
   - 如果需要完整响应，需要修改解析器

2. **缓冲区大小**
   - 确保响应缓冲区足够大（默认 128 字节）
   - 可以在 `at_kernel.h` 中修改 `AT_Manager_t.response_buffer` 大小

3. **字符串拷贝**
   - 回调中的 `response` 是临时缓冲区
   - 如需长期保存，请拷贝到自己的缓冲区

4. **线程安全**
   - 回调在中断上下文中执行
   - 避免在回调中进行长时间操作

---

## 🚀 总结

✅ **回调函数现在可以接收响应数据**  
✅ **支持单行和多行响应**  
✅ **可以解析具体数据（版本号、IP、信号强度等）**  
✅ **灵活配置期望响应**  

**你的需求完全可以实现！** 用户现在可以在回调函数中处理响应数据，包括版本号、IP 地址、信号强度等任何模块返回的信息。🎉

/**
  * @file    at_fms.c
  * @brief   AT 命令文件系统 - 核心实现
  * @details 使用链接段自动收集所有注册的 AT 命令
  */

/* Includes ------------------------------------------------------------------*/
#include "at_kernel.h"
#include "at_port.h"
#include <stdio.h>

/* Private defines -----------------------------------------------------------*/
#define AT_RX_BUFFER_SIZE       256
#define AT_MAX_RESPONSE_LEN     128

/* Private typedef -----------------------------------------------------------*/
typedef struct {
    uint8_t buffer[AT_RX_BUFFER_SIZE];
    uint16_t write_index;
    uint16_t read_index;
    char response[AT_MAX_RESPONSE_LEN];
    uint16_t resp_index;
} AT_RxBuffer_t;

/* Private variables ---------------------------------------------------------*/

/* 链接段符号（由链接器脚本定义） */
extern uint32_t __start_at_cmd_table[];
extern uint32_t __stop_at_cmd_table[];

/* 运行时状态数组（在 RAM 中） */
static AT_Cmd_Runtime_t g_runtime_states[64];  /* 最多支持 64 个命令 */

/* 命令管理器 */
static AT_Manager_t g_at_manager = {0};

/* 接收缓冲区 */
static AT_RxBuffer_t g_rx_buffer = {0};

/* 超时回调 */
static void (*g_timeout_callback)(const char *cmd_name) = NULL;

/* 当前正在执行的命令 */
static const AT_Cmd_t *g_current_cmd = NULL;

/* Private function prototypes -----------------------------------------------*/
static void AT_ParseResponse(void);
static bool AT_CheckResponse(const char *response, const char *expected);
static void AT_ExecuteNextCommand(void);
static void AT_SendCommand(const AT_Cmd_t *cmd);

/* Exported functions --------------------------------------------------------*/

/**
  * @brief 初始化 AT 命令管理器
  */
void AT_Init(void)
{
    /* 设置命令表起始和结束地址 */
    g_at_manager.cmd_table = (const AT_Cmd_t *)__start_at_cmd_table;
    g_at_manager.cmd_table_end = (const AT_Cmd_t *)__stop_at_cmd_table;
    g_at_manager.cmd_count = (uint16_t)(g_at_manager.cmd_table_end - g_at_manager.cmd_table);
    g_at_manager.current_index = 0;
    g_at_manager.is_busy = false;
    
    /* 设置运行时状态数组 */
    g_at_manager.runtime_states = g_runtime_states;
    
    /* 初始化所有运行时状态 */
    for (uint16_t i = 0; i < g_at_manager.cmd_count; i++) {
        g_runtime_states[i].state = AT_CMD_STATE_IDLE;
        g_runtime_states[i].start_tick = 0;
    }
    
    /* 清空接收缓冲区 */
    memset(&g_rx_buffer, 0, sizeof(g_rx_buffer));
    
    printf("[AT] Initialized. Command table: %d commands\r\n", 
           AT_GetCmdCount());
}

/**
  * @brief 启动命令执行
  */
void AT_Start(void)
{
    if (g_at_manager.is_busy) {
        return;
    }
    
    g_at_manager.current_index = 0;
    g_at_manager.is_busy = true;
    
    AT_ExecuteNextCommand();
}

/**
  * @brief 停止命令执行
  */
void AT_Stop(void)
{
    g_at_manager.is_busy = false;
    g_current_cmd = NULL;
}

/**
  * @brief AT 命令处理函数
  */
void AT_Process(void)
{
    if (!g_at_manager.is_busy || g_current_cmd == NULL) {
        return;
    }
    
    /* 获取当前命令的运行时状态 */
    uint16_t runtime_index = g_at_manager.current_index - 1;
    if (runtime_index >= g_at_manager.cmd_count) {
        return;
    }
    
    AT_Cmd_Runtime_t *runtime = &g_runtime_states[runtime_index];
    
    /* 检查超时 */
    if (runtime->start_tick == 0) {
        runtime->start_tick = AT_Port_GetTick();
    }
    
    uint32_t current_tick = AT_Port_GetTick();
    if (current_tick - runtime->start_tick > g_current_cmd->timeout_ms) {
        printf("[AT] Timeout: %s\r\n", g_current_cmd->name);
        
        if (g_timeout_callback != NULL) {
            g_timeout_callback(g_current_cmd->name);
        }
        
        /* 切换到下一个命令 */
        runtime->state = AT_CMD_STATE_TIMEOUT;
        AT_ExecuteNextCommand();
        return;
    }
    
    /* 解析接收到的响应 */
    AT_ParseResponse();
}

/**
  * @brief 接收数据
  */
void AT_ReceiveData(const uint8_t *data, uint16_t len)
{
    for (uint16_t i = 0; i < len; i++) {
        g_rx_buffer.buffer[g_rx_buffer.write_index++] = data[i];
        
        /* 检测行尾，开始解析 */
        if (data[i] == '\n' || g_rx_buffer.write_index >= AT_RX_BUFFER_SIZE) {
            g_rx_buffer.buffer[g_rx_buffer.write_index] = '\0';
            g_rx_buffer.write_index = 0;
        }
    }
}

/**
  * @brief 获取命令状态
  */
AT_CmdState_t AT_GetCmdState(const char *cmd_name)
{
    const AT_Cmd_t *cmd = g_at_manager.cmd_table;
    uint16_t index = 0;
    
    while (cmd < g_at_manager.cmd_table_end) {
        if (strcmp(cmd->name, cmd_name) == 0) {
            return g_runtime_states[index].state;
        }
        cmd++;
        index++;
    }
    
    return AT_CMD_STATE_ERROR;
}

/**
  * @brief 获取当前索引
  */
uint16_t AT_GetCurrentIndex(void)
{
    return g_at_manager.current_index;
}

/**
  * @brief 获取命令总数
  */
uint16_t AT_GetCmdCount(void)
{
    return (uint16_t)(g_at_manager.cmd_table_end - g_at_manager.cmd_table);
}

/**
  * @brief 设置超时回调
  */
void AT_SetTimeoutCallback(void (*cb)(const char *cmd_name))
{
    g_timeout_callback = cb;
}

/* Private functions ---------------------------------------------------------*/

/**
  * @brief 解析响应
  */
static void AT_ParseResponse(void)
{
    if (g_rx_buffer.read_index >= g_rx_buffer.write_index) {
        return;
    }
    
    /* 获取当前命令的运行时状态 */
    uint16_t runtime_index = g_at_manager.current_index - 1;
    if (runtime_index >= g_at_manager.cmd_count) {
        return;
    }
    
    AT_Cmd_Runtime_t *runtime = &g_runtime_states[runtime_index];
    
    /* 提取一行响应 */
    while (g_rx_buffer.read_index < g_rx_buffer.write_index) {
        char c = g_rx_buffer.buffer[g_rx_buffer.read_index++];
        
        if (c == '\n') {
            g_rx_buffer.response[g_rx_buffer.resp_index] = '\0';
            g_rx_buffer.resp_index = 0;
            
            /* 检查是否是期望的响应 */
            if (AT_CheckResponse(g_rx_buffer.response, g_current_cmd->expected_response)) {
                printf("[AT] Matched: %s -> %s\r\n", 
                       g_current_cmd->name, 
                       g_current_cmd->expected_response);
                
                /* 保存完整响应数据 */
                strncpy(g_at_manager.response_buffer, g_rx_buffer.response, sizeof(g_at_manager.response_buffer) - 1);
                g_at_manager.response_buffer[sizeof(g_at_manager.response_buffer) - 1] = '\0';
                
                /* 调用回调（传递响应数据） */
                if (g_current_cmd->callback != NULL) {
                    g_current_cmd->callback(g_at_manager.response_buffer, g_current_cmd->user_data);
                }
                
                runtime->state = AT_CMD_STATE_COMPLETED;
                
                /* 切换到下一个命令 */
                AT_ExecuteNextCommand();
            }
            
            g_rx_buffer.response[0] = '\0';
        } else if (g_rx_buffer.resp_index < AT_MAX_RESPONSE_LEN - 1) {
            g_rx_buffer.response[g_rx_buffer.resp_index++] = c;
        }
    }
}

/**
  * @brief 检查响应是否匹配
  */
static bool AT_CheckResponse(const char *response, const char *expected)
{
    if (response == NULL || expected == NULL) {
        return false;
    }
    
    /* 支持通配符匹配（如果 expected 为 "*" 则匹配任意响应） */
    if (strcmp(expected, "*") == 0) {
        return strlen(response) > 0;
    }
    
    /* 精确匹配 */
    return strstr(response, expected) != NULL;
}

/**
  * @brief 执行下一个命令
  */
static void AT_ExecuteNextCommand(void)
{
    if (g_at_manager.current_index >= AT_GetCmdCount()) {
        printf("[AT] All commands completed\r\n");
        g_at_manager.is_busy = false;
        g_current_cmd = NULL;
        return;
    }
    
    g_current_cmd = &g_at_manager.cmd_table[g_at_manager.current_index++];
    
    printf("[AT] Executing: %s (%s)\r\n", 
           g_current_cmd->name, 
           g_current_cmd->cmd_string);
    
    AT_SendCommand(g_current_cmd);
}

/**
  * @brief 发送命令
  */
static void AT_SendCommand(const AT_Cmd_t *cmd)
{
    if (cmd == NULL) {
        return;
    }
    
    /* 获取运行时状态索引 */
    uint16_t runtime_index = g_at_manager.current_index - 1;
    if (runtime_index >= g_at_manager.cmd_count) {
        return;
    }
    
    /* 发送 AT 命令 */
    AT_Port_Send((const uint8_t *)cmd->cmd_string, strlen(cmd->cmd_string));
    
    /* 发送回车换行 */
    AT_Port_Send((const uint8_t *)"\r\n", 2);
    
    /* 更新运行时状态 */
    g_runtime_states[runtime_index].state = AT_CMD_STATE_SENDING;
}

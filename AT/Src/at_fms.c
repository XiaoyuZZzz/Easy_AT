/**
  * @file    at_fms.c
  * @brief   AT 命令文件系统 - 核心实现
  * @details 使用链接段自动收集所有注册的 AT 命令
  */

/* Includes ------------------------------------------------------------------*/
#include "at_kernel.h"
#include "at_port.h"
#include <stdio.h>
#include <string.h>

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
} AT_RX_BUFFER_T;

/* Private variables ---------------------------------------------------------*/

/* 链接段符号（由链接器脚本定义） */
extern uint32_t __start_at_cmd_table[];
extern uint32_t __stop_at_cmd_table[];

/* 运行时状态数组（在 RAM 中） */
static AT_CMD_RUNTIME_T g_runtime_states[64];  /* 最多支持 64 个命令 */

/* 命令管理器 */
static AT_MANAGER_T g_at_manager = {0};

/* 接收缓冲区 */
static AT_RX_BUFFER_T g_rx_buffer = {0};

/* 超时回调 */
static void (*g_timeout_callback)(const char *cmd_name) = NULL;

/* 当前正在执行的命令 */
static const AT_CMD_T *g_current_cmd = NULL;

/* Private function prototypes -----------------------------------------------*/
static void at_parse_response(void);
static bool at_check_response(const char *response, const char *expected);
static void at_execute_next_command(void);
static void at_send_command(const AT_CMD_T *cmd);

/* Exported functions --------------------------------------------------------*/

/**
  * @brief 初始化 AT 命令管理器
  */
void at_init(void)
{
    /* 设置命令表起始和结束地址 */
    g_at_manager.cmd_table = (const AT_CMD_T *)__start_at_cmd_table;
    g_at_manager.cmd_table_end = (const AT_CMD_T *)__stop_at_cmd_table;
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
           at_get_cmd_count());
}

/**
  * @brief 启动命令执行
  */
void at_start(void)
{
    if (g_at_manager.is_busy) {
        return;
    }
    
    g_at_manager.current_index = 0;
    g_at_manager.is_busy = true;
    
    at_execute_next_command();
}

/**
  * @brief 停止命令执行
  */
void at_stop(void)
{
    g_at_manager.is_busy = false;
    g_current_cmd = NULL;
}

/**
  * @brief AT 命令处理函数
  */
void at_process(void)
{
    if (!g_at_manager.is_busy || g_current_cmd == NULL) {
        return;
    }
    
    /* 获取当前命令的运行时状态 */
    uint16_t runtime_index = g_at_manager.current_index - 1;
    if (runtime_index >= g_at_manager.cmd_count) {
        return;
    }
    
    AT_CMD_RUNTIME_T *runtime = &g_runtime_states[runtime_index];
    
    /* 检查超时 */
    if (runtime->start_tick == 0) {
        runtime->start_tick = at_port_get_tick();
    }
    
    uint32_t current_tick = at_port_get_tick();
    if (current_tick - runtime->start_tick > g_current_cmd->timeout_ms) {
        printf("[AT] Timeout: %s\r\n", g_current_cmd->name);
        
        if (g_timeout_callback != NULL) {
            g_timeout_callback(g_current_cmd->name);
        }
        
        /* 切换到下一个命令 */
        runtime->state = AT_CMD_STATE_TIMEOUT;
        at_execute_next_command();
        return;
    }
    
    /* 解析接收到的响应 */
    at_parse_response();
}

/**
  * @brief 接收数据
  */
void at_receive_data(const uint8_t *data, uint16_t len)
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
AT_CMD_STATE_T at_get_cmd_state(const char *cmd_name)
{
    const AT_CMD_T *cmd = g_at_manager.cmd_table;
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
uint16_t at_get_current_index(void)
{
    return g_at_manager.current_index;
}

/**
  * @brief 获取命令总数
  */
uint16_t at_get_cmd_count(void)
{
    return g_at_manager.cmd_count;
}

/**
  * @brief 设置超时回调
  */
void at_set_timeout_callback(void (*cb)(const char *cmd_name))
{
    g_timeout_callback = cb;
}

/* Private functions ---------------------------------------------------------*/

/**
  * @brief 解析响应
  */
static void at_parse_response(void)
{
    if (g_rx_buffer.read_index >= g_rx_buffer.write_index) {
        return;
    }
    
    /* 获取当前命令的运行时状态 */
    uint16_t runtime_index = g_at_manager.current_index - 1;
    if (runtime_index >= g_at_manager.cmd_count) {
        return;
    }
    
    AT_CMD_RUNTIME_T *runtime = &g_runtime_states[runtime_index];
    
    /* 提取一行响应 */
    while (g_rx_buffer.read_index < g_rx_buffer.write_index) {
        char c = g_rx_buffer.buffer[g_rx_buffer.read_index++];
        
        if (c == '\n') {
            g_rx_buffer.response[g_rx_buffer.resp_index] = '\0';
            g_rx_buffer.resp_index = 0;
            
            /* 检查是否是期望的响应 */
            if (at_check_response(g_rx_buffer.response, g_current_cmd->expected_response)) {
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
                at_execute_next_command();
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
static bool at_check_response(const char *response, const char *expected)
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
static void at_execute_next_command(void)
{
    if (g_at_manager.current_index >= at_get_cmd_count()) {
        printf("[AT] All commands completed\r\n");
        g_at_manager.is_busy = false;
        g_current_cmd = NULL;
        return;
    }
    
    g_current_cmd = &g_at_manager.cmd_table[g_at_manager.current_index++];
    
    printf("[AT] Executing: %s (%s)\r\n", 
           g_current_cmd->name, 
           g_current_cmd->cmd_string);
    
    at_send_command(g_current_cmd);
}

/**
  * @brief 发送命令
  */
static void at_send_command(const AT_CMD_T *cmd)
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
    at_port_send((const uint8_t *)cmd->cmd_string, strlen(cmd->cmd_string));
    
    /* 发送回车换行 */
    at_port_send((const uint8_t *)"\r\n", 2);
    
    /* 更新运行时状态 */
    g_runtime_states[runtime_index].state = AT_CMD_STATE_SENDING;
}

/**
  * @file    at_kernel.h
  * @brief   AT 命令客户端内核 - 头文件
  * @details 使用链接段方式实现编译时注册
  */

#ifndef __AT_KERNEL_H__
#define __AT_KERNEL_H__

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

/* Exported types ------------------------------------------------------------*/

/**
  * @brief AT 命令状态
  */
typedef enum {
    AT_CMD_STATE_IDLE = 0,      /* 空闲状态 */
    AT_CMD_STATE_SENDING,       /* 发送中 */
    AT_CMD_STATE_WAITING,       /* 等待响应 */
    AT_CMD_STATE_COMPLETED,     /* 完成 */
    AT_CMD_STATE_TIMEOUT,       /* 超时 */
    AT_CMD_STATE_ERROR          /* 错误 */
} AT_CmdState_t;

/**
  * @brief AT 命令结构体（存储在 Flash 中）
  */
typedef struct {
    const char *name;                   /* 命令名称（用户定义） */
    const char *cmd_string;             /* AT 命令字符串（如 "AT"） */
    const char *expected_response;      /* 期望响应（如 "OK"） */
    void (*callback)(const char *response, void *user_data);  /* 回调函数（带响应数据） */
    void *user_data;                    /* 用户数据指针 */
    uint32_t timeout_ms;                /* 超时时间（ms） */
} AT_Cmd_t;

/**
  * @brief AT 命令运行时状态（存储在 RAM 中）
  */
typedef struct {
    AT_CmdState_t state;                /* 当前状态 */
    uint32_t start_tick;                /* 开始时间 */
} AT_Cmd_Runtime_t;

/**
  * @brief AT 命令管理器
  */
typedef struct {
    const AT_Cmd_t *cmd_table;          /* 命令表起始地址 */
    const AT_Cmd_t *cmd_table_end;      /* 命令表结束地址 */
    AT_Cmd_Runtime_t *runtime_states;   /* 运行时状态数组（RAM） */
    char response_buffer[128];          /* 响应数据缓冲区 */
    uint16_t current_index;             /* 当前命令索引 */
    uint16_t cmd_count;                 /* 命令总数 */
    bool is_busy;                       /* 忙标志 */
} AT_Manager_t;

/* Exported macros -----------------------------------------------------------*/

/**
  * @brief 使用链接段注册 AT 命令（GCC 专用）
  * @param  cmd_name: 命令名称（标识符）
  * @param  cmd_str: AT 命令字符串（如 "AT"）
  * @param  expected_resp: 期望响应（如 "OK"）
  * @param  cb: 回调函数
  * @param  user_data: 用户数据指针
  * @param  timeout: 超时时间（ms）
  */
#ifdef __GNUC__
    #define AT_CMD_REGISTER(cmd_name, cmd_str, expected_resp, cb, user_data, timeout) \
        __attribute__((used, section(".at_cmd_table"))) \
        const AT_Cmd_t at_cmd_##cmd_name = { \
            #cmd_name, \
            cmd_str, \
            expected_resp, \
            cb, \
            (void*)(user_data), \
            timeout \
        }
#else
    #error "Only GCC is supported for AT command registration"
#endif

/**
  * @brief X 宏定义（用于遍历命令表）
  * @note  用户可以在自己的文件中重新定义 AT_CMD_X 宏来使用命令表
  */
#define AT_CMD_TABLE_XMACRO() \
    AT_CMD_X(INIT, "AT_INIT", "READY", AT_Init_Callback, NULL, 1000) \
    AT_CMD_X(CGMR, "AT+CGMR", "OK", AT_Version_Callback, NULL, 1000) \
    AT_CMD_X(GMR, "AT+GMR", "OK", AT_Version_Callback, NULL, 1000) \
    /* 用户可以在这里添加更多默认命令 */

/* Exported functions prototypes ---------------------------------------------*/

/**
  * @brief 初始化 AT 命令管理器
  */
void AT_Init(void);

/**
  * @brief 启动命令执行（按顺序执行所有注册的命令）
  */
void AT_Start(void);

/**
  * @brief 停止命令执行
  */
void AT_Stop(void);

/**
  * @brief AT 命令处理函数（需要在主循环中调用）
  */
void AT_Process(void);

/**
  * @brief 接收数据（由串口中断调用）
  * @param  data: 数据缓冲区
  * @param  len: 数据长度
  */
void AT_ReceiveData(const uint8_t *data, uint16_t len);

/**
  * @brief 获取当前命令状态
  * @param  cmd_name: 命令名称
  * @retval 命令状态
  */
AT_CmdState_t AT_GetCmdState(const char *cmd_name);

/**
  * @brief 获取当前执行的命令索引
  * @retval 当前索引
  */
uint16_t AT_GetCurrentIndex(void);

/**
  * @brief 获取命令总数
  * @retval 命令总数
  */
uint16_t AT_GetCmdCount(void);

/**
  * @brief 设置超时回调
  * @param  cb: 超时回调函数
  */
void AT_SetTimeoutCallback(void (*cb)(const char *cmd_name));

/* 用户需要实现的移植层接口（在 at_port.h 中声明）---------------------------*/
/*
void AT_Port_Send(const uint8_t *data, uint16_t len);
uint32_t AT_Port_GetTick(void);
*/

#ifdef __cplusplus
}
#endif

#endif /* __AT_KERNEL_H__ */

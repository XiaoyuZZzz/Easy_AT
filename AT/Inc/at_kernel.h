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
    AT_CMD_STATE_IDLE = 0,          /* 空闲状态 */
    AT_CMD_STATE_SENDING,           /* 发送中 */
    AT_CMD_STATE_WAITING,           /* 等待响应 */
    AT_CMD_STATE_COMPLETED,         /* 完成 */
    AT_CMD_STATE_TIMEOUT,           /* 超时 */
    AT_CMD_STATE_ERROR              /* 错误 */
} AT_CMD_STATE_T;

/**
  * @brief AT 命令类型
  */
typedef enum {
    AT_CMD_TYPE_ONCE = 0,           /* 一次性命令（只执行一次） */
    AT_CMD_TYPE_PERIODIC            /* 周期性命令（定时重复执行） */
} AT_CMD_TYPE_T;

/**
  * @brief AT 命令阶段
  */
typedef enum {
    AT_PHASE_INIT = 0,              /* 初始化阶段 */
    AT_PHASE_LOOP                   /* 循环阶段 */
} AT_PHASE_T;

/**
  * @brief AT 命令结构体（存储在 Flash 中）
  */
typedef struct {
    const char *name;                       /* 命令名称（用户定义） */
    const char *cmd_string;                 /* AT 命令字符串（如 "AT"） */
    const char *expected_response;          /* 期望响应（如 "OK"） */
    void (*callback)(const char *response, void *user_data);  /* 回调函数（带响应数据） */
    void *user_data;                        /* 用户数据指针 */
    uint32_t timeout_ms;                    /* 超时时间（ms） */
    AT_CMD_TYPE_T type;                     /* 命令类型 */
    uint32_t interval_ms;                   /* 执行间隔（仅周期性命令有效） */
} AT_CMD_T;

/**
  * @brief AT 命令运行时状态（存储在 RAM 中）
  */
typedef struct {
    AT_CMD_STATE_T state;           /* 当前状态 */
    uint32_t start_tick;            /* 开始时间 */
    uint32_t last_exec_tick;        /* 上次执行时间 */
    bool executed;                  /* 是否已执行过 */
} AT_CMD_RUNTIME_T;

/**
  * @brief AT 命令管理器
  */
typedef struct {
    const AT_CMD_T *cmd_table;      /* 命令表起始地址 */
    const AT_CMD_T *cmd_table_end;  /* 命令表结束地址 */
    AT_CMD_RUNTIME_T *runtime_states;  /* 运行时状态数组（RAM） */
    char response_buffer[128];      /* 响应数据缓冲区 */
    uint16_t current_index;         /* 当前命令索引 */
    uint16_t cmd_count;             /* 命令总数 */
    bool is_busy;                   /* 忙标志 */
    AT_PHASE_T current_phase;       /* 当前阶段 */
    bool init_completed;            /* 初始化完成标志 */
} AT_MANAGER_T;

/* Exported macros -----------------------------------------------------------*/

/**
  * @brief 使用链接段注册 AT 命令（GCC 专用）
  * @param  cmd_name: 命令名称（标识符）
  * @param  cmd_str: AT 命令字符串（如 "AT"）
  * @param  expected_resp: 期望响应（如 "OK"）
  * @param  cb: 回调函数
  * @param  user_data: 用户数据指针
  * @param  timeout: 超时时间（ms）
  * @param  cmd_type: 命令类型（AT_CMD_TYPE_ONCE 或 AT_CMD_TYPE_PERIODIC）
  * @param  interval: 执行间隔（ms，仅周期性命令有效）
  */
#ifdef __GNUC__
    #define AT_CMD_REGISTER(cmd_name, cmd_str, expected_resp, cb, user_data, timeout, cmd_type, interval) \
        __attribute__((used, section(".at_cmd_table"))) \
        const AT_CMD_T at_cmd_##cmd_name = { \
            #cmd_name, \
            cmd_str, \
            expected_resp, \
            cb, \
            (void*)(user_data), \
            timeout, \
            cmd_type, \
            interval \
        }
#else
    #error "Only GCC is supported for AT command registration"
#endif

/**
  * @brief 便捷宏：注册一次性命令
  */
#define AT_CMD_ONCE(cmd_name, cmd_str, expected_resp, cb, user_data, timeout) \
    AT_CMD_REGISTER(cmd_name, cmd_str, expected_resp, cb, user_data, timeout, AT_CMD_TYPE_ONCE, 0)

/**
  * @brief 便捷宏：注册周期性命令
  */
#define AT_CMD_PERIODIC(cmd_name, cmd_str, expected_resp, cb, user_data, timeout, interval) \
    AT_CMD_REGISTER(cmd_name, cmd_str, expected_resp, cb, user_data, timeout, AT_CMD_TYPE_PERIODIC, interval)

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
void at_init(void);

/**
  * @brief 启动命令执行（按顺序执行所有注册的命令）
  */
void at_start(void);

/**
  * @brief 停止命令执行
  */
void at_stop(void);

/**
  * @brief AT 命令处理函数（需要在主循环中调用）
  */
void at_process(void);

/**
  * @brief 接收数据（由串口中断调用）
  * @param  data: 数据缓冲区
  * @param  len: 数据长度
  */
void at_receive_data(const uint8_t *data, uint16_t len);

/**
  * @brief 获取当前命令状态
  * @param  cmd_name: 命令名称
  * @retval 命令状态
  */
AT_CMD_STATE_T at_get_cmd_state(const char *cmd_name);

/**
  * @brief 获取当前执行的命令索引
  * @retval 当前索引
  */
uint16_t at_get_current_index(void);

/**
  * @brief 获取命令总数
  * @retval 命令总数
  */
uint16_t at_get_cmd_count(void);

/**
  * @brief 设置超时回调
  * @param  cb: 超时回调函数
  */
void at_set_timeout_callback(void (*cb)(const char *cmd_name));

/**
  * @brief 获取当前阶段
  * @retval 当前阶段（AT_PHASE_INIT 或 AT_PHASE_LOOP）
  */
AT_PHASE_T at_get_current_phase(void);

/**
  * @brief 手动触发周期性命令立即执行
  * @param  cmd_name: 命令名称
  */
void at_trigger_periodic_cmd(const char *cmd_name);

/* 用户需要实现的移植层接口（在 at_port.h 中声明）---------------------------*/
/*
void AT_Port_Send(const uint8_t *data, uint16_t len);
uint32_t AT_Port_GetTick(void);
*/

#ifdef __cplusplus
}
#endif

#endif /* __AT_KERNEL_H__ */

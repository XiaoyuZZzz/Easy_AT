/**
  * @file    at_register.h
  * @brief   AT 命令注册头文件
  * @details 用户在此文件中注册自己的 AT 命令
  */

#ifndef __AT_REGISTER_H__
#define __AT_REGISTER_H__

/* Includes ------------------------------------------------------------------*/
#include "at_kernel.h"

/* 用户回调函数声明 ----------------------------------------------------------*/

/* 声明回调函数 */
void at_init_callback(const char *response, void *user_data);
void at_version_callback(const char *response, void *user_data);
void at_csq_callback(const char *response, void *user_data);
void at_cell_callback(const char *response, void *user_data);

/* 注册命令 ------------------------------------------------------------------*/

/* 初始化阶段命令（只执行一次） */
AT_CMD_ONCE_OK_CB(INIT, "AT", at_init_callback, 0, 1000);
AT_CMD_ONCE_OK(ECHO_OFF, "ATE0", 1000);
AT_CMD_ONCE(CGMR, "AT+CGMR", "Easy_AT", at_version_callback, 0, 1000);

/* 周期性命令（定时重复执行） */
/* 每 5 秒查询一次信号强度 */
AT_CMD_PERIODIC(CSQ, "AT+CSQ", "+CSQ:", at_csq_callback, 0, 1000, 5000);

/* 每 10 秒查询一次基站信息 */
AT_CMD_PERIODIC(CELL, "AT+CELL", "+CELL:", at_cell_callback, 0, 1000, 10000);

/* 在此添加更多命令... */

#endif /* __AT_REGISTER_H__ */

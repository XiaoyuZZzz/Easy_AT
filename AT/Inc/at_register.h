/**
  * @file    at_register.h
  * @brief   AT 命令注册文件 - 用户在此声明和注册自己的 AT 命令
  * @details 使用链接段方式注册，只包含声明和注册宏
  * 
  * 使用说明：
  * 1. 在此文件中声明你的回调函数
  * 2. 使用 AT_CMD_REGISTER 宏注册命令
  * 3. 在 at_register.c 中实现回调函数
  * 
  * 参数说明：
  *    - cmd_name: 命令名称（标识符，用于区分不同命令）
  *    - cmd_str: AT 命令字符串（实际发送的内容，如 "AT"）
  *    - expected_resp: 期望响应（如 "OK"）
  *    - cb: 回调函数（收到期望响应时调用）
  *    - user_data: 用户数据指针（传递给回调函数）
  *    - timeout: 超时时间（毫秒）
  * 
  * 示例：
  * AT_CMD_REGISTER(MY_CMD, "AT+TEST", "OK", My_Callback, NULL, 1000);
  */

#ifndef __AT_REGISTER_H__
#define __AT_REGISTER_H__

/* Includes ------------------------------------------------------------------*/
#include "at_kernel.h"

/* ============================================================================
 * AT 命令注册表
 * 在此添加你的 AT 命令注册
 * ============================================================================*/

/* ----------------------------------------------------------------------------
 * 默认命令（库自带）
 * 用户可以根据需要删除或修改这些命令
 * ----------------------------------------------------------------------------*/

/* 声明回调函数 */
void at_init_callback(const char *response, void *user_data);
void at_version_callback(const char *response, void *user_data);

/* 注册命令 */
AT_CMD_REGISTER(INIT, "AT", "OK", at_init_callback, 0, 1000);
AT_CMD_REGISTER(CGMR, "AT+CGMR", "OK", at_version_callback, 0, 1000);

/* ----------------------------------------------------------------------------
 * 用户自定义命令注册区域
 * 
 * 使用步骤：
 * 1. 在此声明回调函数
 * 2. 使用 AT_CMD_REGISTER 注册命令
 * 3. 在 at_register.c 中实现回调函数
 * 
 * 示例：
 * void Wifi_Connect_Callback(void *user_data);
 * AT_CMD_REGISTER(WIFI_CONNECT, "AT+CWJAP=\"SSID\",\"PASSWORD\"", "OK", Wifi_Connect_Callback, 0, 5000);
 * ----------------------------------------------------------------------------*/

/* 示例：用户自定义命令 */
void user_cmd_callback(const char *response, void *user_data);
AT_CMD_REGISTER(USER_CMD, "AT+USER", "OK", user_cmd_callback, 0, 1000);

/* 在此添加更多命令... */

#endif /* __AT_REGISTER_H__ */

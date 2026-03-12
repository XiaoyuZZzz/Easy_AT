/**
  * @file    at_register.c
  * @brief   AT 命令注册实现
  * @details 用户在此文件中实现自己的 AT 命令回调函数
  */

/* Includes ------------------------------------------------------------------*/
#include "at_register.h"
#include "gpio.h"

/* 用户回调函数实现 ----------------------------------------------------------*/

/**
  * @brief 初始化命令回调
  * @param  response: 响应数据（如 "OK"）
  * @param  user_data: 用户数据指针
  */
void at_init_callback(const char *response, void *user_data)
{
    (void)response;
    (void)user_data;
    
    /* 初始化完成的处理逻辑 */
    // 例如：点亮指示灯表示 AT 系统初始化成功
    // HAL_GPIO_WritePin(LED0_GPIO_Port, LED0_Pin, GPIO_PIN_SET);
}

/**
  * @brief 版本查询命令回调
  * @param  response: 响应数据（如 "26-03-11" 或完整响应）
  * @param  user_data: 用户数据指针
  */
void at_version_callback(const char *response, void *user_data)
{
    (void)user_data;
    
    /* 版本查询完成的处理逻辑 */
    // response 包含完整的响应数据，例如："26-03-11"
    // 可以解析 response 获取版本号
}

/**
  * @brief 用户自定义命令回调示例
  * @param  response: 响应数据
  * @param  user_data: 用户数据指针
  */
void user_cmd_callback(const char *response, void *user_data)
{
    (void)response;
    (void)user_data;
    
    /* 用户自定义命令的处理逻辑 */
    // 例如：切换 LED 状态
    // HAL_GPIO_TogglePin(LED0_GPIO_Port, LED0_Pin);
}

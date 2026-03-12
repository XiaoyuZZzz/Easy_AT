/**
  * @file    at_register.c
  * @brief   AT 命令回调函数实现文件
  * @details 用户在此实现所有 AT 命令的回调函数
  */

/* Includes ------------------------------------------------------------------*/
#include "at_register.h"
#include "gpio.h"  /* 示例：用于 LED 控制 */

/* Private typedef -----------------------------------------------------------*/
/* Private defines -----------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/* Private function prototypes -----------------------------------------------*/
/* Private user code ---------------------------------------------------------*/

/* ============================================================================
 * 默认命令回调实现
 * ============================================================================*/

/**
  * @brief 初始化命令回调
  * @param  response: 响应数据（如 "OK"）
  * @param  user_data: 用户数据指针
  */
void AT_Init_Callback(const char *response, void *user_data)
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
void AT_Version_Callback(const char *response, void *user_data)
{
    (void)user_data;
    
    /* 版本查询完成的处理逻辑 */
    // response 包含完整的响应数据，例如："26-03-11"
    // 可以解析 response 获取版本号
}

/* ============================================================================
 * 用户自定义命令回调实现
 * ============================================================================*/

/**
  * @brief 用户自定义命令回调示例
  * @param  response: 响应数据
  * @param  user_data: 用户数据指针
  */
void User_Cmd_Callback(const char *response, void *user_data)
{
    (void)response;
    (void)user_data;
    
    /* 用户自定义命令的处理逻辑 */
    // 例如：切换 LED 状态
    // HAL_GPIO_TogglePin(LED0_GPIO_Port, LED0_Pin);
}

/* ============================================================================
 * 更多回调函数实现示例
 * ============================================================================*/

/*
// WiFi 连接命令回调示例
void Wifi_Connect_Callback(void *user_data)
{
    (void)user_data;
    
    // WiFi 连接成功的处理逻辑
    // HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, GPIO_PIN_SET);
}

// 发送数据命令回调示例
void Send_Data_Callback(void *user_data)
{
    (void)user_data;
    
    // 数据发送成功的处理逻辑
}

// 重启模块命令回调示例
void Restart_Callback(void *user_data)
{
    (void)user_data;
    
    // 模块重启完成的处理逻辑
}

// 带参数的回调示例
void Custom_Callback(void *user_data)
{
    uint32_t param = (uint32_t)user_data;
    
    // 使用传递的参数
    // if (param == 0x1234) { ... }
}
*/

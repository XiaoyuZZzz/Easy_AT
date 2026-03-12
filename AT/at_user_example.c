/**
  * @file    at_user_example.c
  * @brief   AT 命令用户示例
  * @details 展示如何注册和使用 AT 命令
  */

/* Includes ------------------------------------------------------------------*/
#include "at_register.h"

/* 示例回调函数实现 ----------------------------------------------------------*/

/**
  * @brief WiFi 连接命令回调示例
  * 
  * 注册示例：
  * AT_CMD_REGISTER(WIFI_CONNECT, 
  *                 "AT+CWJAP=\"MyWiFi\",\"Password\"", 
  *                 "WIFI GOT IP", 
  *                 wifi_connect_callback, 
  *                 NULL, 
  *                 10000);  // 10 秒超时
  */
static void wifi_connect_callback(const char *response, void *user_data)
{
    (void)response;
    (void)user_data;
    /* WiFi 连接成功的处理逻辑 */
    // LED_ON();  // 点亮指示灯
}

/**
  * @brief 发送数据命令回调示例
  * 
  * 注册示例：
  * AT_CMD_REGISTER(SEND_DATA, 
  *                 "AT+CIPSEND", 
  *                 "SEND OK", 
  *                 send_data_callback, 
  *                 NULL, 
  *                 2000);
  */
static void send_data_callback(const char *response, void *user_data)
{
    (void)response;
    (void)user_data;
    /* 数据发送成功的处理逻辑 */
}

/**
  * @brief 重启模块命令回调示例
  * 
  * 注册示例：
  * AT_CMD_REGISTER(RESTART, 
  *                 "AT+RST", 
  *                 "ready", 
  *                 restart_callback, 
  *                 NULL, 
  *                 3000);
  */
static void restart_callback(const char *response, void *user_data)
{
    (void)response;
    (void)user_data;
    /* 模块重启完成的处理逻辑 */
}

/**
  * @brief 自定义命令回调示例
  * 
  * 注册示例：
  * AT_CMD_REGISTER(CUSTOM, 
  *                 "AT+CUSTOM=123", 
  *                 "OK", 
  *                 custom_callback, 
  *                 (void*)0x1234,  // 传递自定义参数
  *                 1000);
  */
static void custom_callback(const char *response, void *user_data)
{
    (void)response;
    /* 使用传递的用户数据 */
    uint32_t param = (uint32_t)user_data;
    (void)param;
    
    /* 自定义命令的处理逻辑 */
}

/* 主程序使用示例 ------------------------------------------------------------*/

#if 0
/* 在 main.c 中的使用示例 */

#include "at_kernel.h"

int main(void)
{
    /* 初始化 */
    at_init();
    
    /* 启动命令执行 */
    at_start();
    
    while (1) {
        /* 处理 AT 命令 */
        at_process();
        
        /* 其他任务... */
    }
}

/* 串口中断处理示例 */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == USART1) {
        uint8_t rx_byte;
        // 读取数据
        at_receive_data(&rx_byte, 1);
    }
}

#endif

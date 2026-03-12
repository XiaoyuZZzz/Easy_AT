/**
  * @file    at_port.c
  * @brief   AT 命令移植层实现
  * @details 用户需要根据硬件修改此文件
  */

/* Includes ------------------------------------------------------------------*/
#include "at_port.h"
#include "main.h"  /* 包含 HAL 库头文件 */

/* 用户需要根据自己的硬件修改以下实现 ---------------------------------------*/

/**
  * @brief 发送数据实现
  * @note  请根据实际使用的串口修改
  */
void at_port_send(const uint8_t *data, uint16_t len)
{
    /* 示例：使用 UART1 发送 */
    /* 请根据实际情况修改 UART 句柄 */
    #if 0
    HAL_UART_Transmit(&huart1, data, len, 1000);
    #else
    /* 临时实现，用户需要替换为实际的串口发送 */
    for (uint16_t i = 0; i < len; i++) {
        /* 使用 ITM 或其他方式输出，用于调试 */
        ITM_SendChar(data[i]);
    }
    #endif
}

/**
  * @brief 获取系统 tick 实现
  */
uint32_t at_port_get_tick(void)
{
    return HAL_GetTick();
}

/* 如果需要重写串口中断接收，请在这里实现 -----------------------------------*/
/*
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == USART1) {
        // 将接收到的数据传递给 AT 库
        // at_receive_data(&rx_byte, 1);
    }
}
*/

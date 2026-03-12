/**
  * @file    at_port.h
  * @brief   AT 命令移植层接口
  * @details 用户需要根据硬件实现这些接口
  */

#ifndef __AT_PORT_H__
#define __AT_PORT_H__

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include <stdint.h>

/* Exported functions prototypes ---------------------------------------------*/

/**
  * @brief 发送数据（用户必须实现）
  * @param  data: 数据缓冲区
  * @param  len: 数据长度
  * 
  * @note 示例实现（使用 UART）：
  * void at_port_send(const uint8_t *data, uint16_t len)
  * {
  *     HAL_UART_Transmit(&huart1, data, len, 1000);
  * }
  */
void at_port_send(const uint8_t *data, uint16_t len);

/**
  * @brief 获取系统 tick（用户必须实现）
  * @retval 当前 tick 值（毫秒）
  * 
  * @note 示例实现：
  * uint32_t at_port_get_tick(void)
  * {
  *     return HAL_GetTick();
  * }
  */
uint32_t at_port_get_tick(void);

#ifdef __cplusplus
}
#endif

#endif /* __AT_PORT_H__ */

/**
  * @file    at_register.c
  * @brief   User AT command callbacks
  */

#include "at_register.h"
#include "gpio.h"
#include <stdio.h>

void at_init_callback(const char *response, void *user_data)
{
    (void)response;
    (void)user_data;

    HAL_GPIO_WritePin(LED0_GPIO_Port, LED0_Pin, GPIO_PIN_RESET);
}

void at_version_callback(const char *response, void *user_data)
{
    (void)user_data;
    printf("[AT] Version: %s\r\n", response);
}

void at_csq_callback(const char *response, void *user_data)
{
    (void)user_data;
    printf("[AT] CSQ: %s\r\n", response);
    HAL_GPIO_TogglePin(LED0_GPIO_Port, LED0_Pin);
}

void at_cell_callback(const char *response, void *user_data)
{
    (void)user_data;
    printf("[AT] CELL: %s\r\n", response);
    HAL_GPIO_TogglePin(LED0_GPIO_Port, LED0_Pin);
}

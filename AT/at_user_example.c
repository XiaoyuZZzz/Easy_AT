/**
  * @file    at_user_example.c
  * @brief   AT 命令库用户使用示例
  * @details 展示如何注册和使用自定义 AT 命令
  */

/* Includes ------------------------------------------------------------------*/
#include "at_kernel.h"
#include "at_register.h"
#include "gpio.h"  /* 用于 LED 控制示例 */

/* 用户自定义回调函数实现 =================================================*/

/**
  * @brief WiFi 连接命令回调示例
  * 
  * 注册示例：
  * AT_CMD_REGISTER(WIFI_CONNECT, 
  *                 "AT+CWJAP=\"MyWiFi\",\"Password\"", 
  *                 "WIFI GOT IP", 
  *                 WiFi_Connect_Callback, 
  *                 NULL, 
  *                 10000);  // 10 秒超时
  */
static void WiFi_Connect_Callback(const char *response, void *user_data)
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
  *                 Send_Data_Callback, 
  *                 NULL, 
  *                 2000);
  */
static void Send_Data_Callback(const char *response, void *user_data)
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
  *                 Restart_Callback, 
  *                 NULL, 
  *                 3000);
  */
static void Restart_Callback(const char *response, void *user_data)
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
  *                 Custom_Callback, 
  *                 (void*)0x1234,  // 传递自定义参数
  *                 1000);
  */
static void Custom_Callback(const char *response, void *user_data)
{
    (void)response;
    /* 使用传递的用户数据 */
    uint32_t param = (uint32_t)user_data;
    (void)param;
    
    /* 自定义命令的处理逻辑 */
}

/* 在 main.c 中的使用示例 =================================================*/

/*
// 包含头文件
#include "at_kernel.h"

int main(void)
{
    // 硬件初始化
    HAL_Init();
    SystemClock_Config();
    MX_GPIO_Init();
    
    // AT 命令系统初始化
    AT_Init();
    
    // 启动 AT 命令序列（会自动按顺序执行所有注册的命令）
    AT_Start();
    
    while (1)
    {
        // 在主循环中调用处理函数
        AT_Process();
        
        // 其他应用逻辑
        // ...
    }
}

// 如果使用串口中断接收数据，在中断回调中调用：
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == USART1) {  // 假设使用 USART1
        uint8_t rx_data = huart->Instance->DR;
        AT_ReceiveData(&rx_data, 1);
        
        // 重新启动接收
        HAL_UART_Receive_IT(&huart1, &rx_data, 1);
    }
}
*/

/* 高级用法：条件编译注册命令 =============================================*/

/*
#ifdef USE_WIFI_MODULE
    AT_CMD_REGISTER(WIFI_INIT, "AT+GMR", "OK", WiFi_Init_Callback, NULL, 1000);
    AT_CMD_REGISTER(WIFI_CONNECT, "AT+CWJAP=\"SSID\",\"PWD\"", "OK", WiFi_Connect_Callback, NULL, 5000);
#endif

#ifdef USE_BLE_MODULE
    AT_CMD_REGISTER(BLE_INIT, "AT+BLEINIT=1", "OK", BLE_Init_Callback, NULL, 1000);
    AT_CMD_REGISTER(BLE_START, "AT+BLESTART", "OK", BLE_Start_Callback, NULL, 2000);
#endif
*/

/* 注意事项 ===============================================================*/
/*
1. 每个 AT_CMD_REGISTER 必须是全局唯一的（cmd_name 不能重复）
2. 回调函数会在收到期望响应时立即调用（在中断上下文中）
3. 如果超时，会调用超时回调（如果设置了的话）
4. 命令按注册顺序依次执行
5. 确保在主循环中调用 AT_Process()
6. 确保及时调用 AT_ReceiveData() 传递串口接收到的数据
*/

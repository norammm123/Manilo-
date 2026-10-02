#include "esp8266.h"
#include "usart.h"
#include <string.h>
void ESP8266_Init(void)
{
    // 设置为Station模式，也就是连接别人的WiFi
    uart_printf(&huart3, "AT+CWMODE=1\r\n");
    HAL_Delay(100);

    // 连接STM32MP2开的WiFi热点
    

    // 查询ESP8266自己的IP地址
    uart_printf(&huart3, "AT+CIFSR\r\n");
    HAL_Delay(1000);

    // 设置为单连接模式
    uart_printf(&huart3, "AT+CIPMUX=0\r\n");
    HAL_Delay(1000);

    // 建立UDP连接
    // 192.168.4.1 是STM32MP2的IP
    // 8080 是STM32MP2监听的端口
    // 8081 是ESP8266本地端口
    uart_printf(&huart3, "AT+CIPSTART=\"UDP\",\"192.168.4.1\",8080,8081,0\r\n");
    HAL_Delay(2000);
}

void ESP8266_SendString(char *data)
{
    uint16_t len = strlen(data);

    uart_printf(&huart3, "AT+CIPSEND=%d\r\n", len);
    HAL_Delay(100);

    uart_printf(&huart3, "%s", data);
    HAL_Delay(100);
}

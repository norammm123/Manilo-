#ifndef __JY62_H
#define __JY62_H

#include "main.h"
#include <stdint.h>

#define JY62_FRAME_HEAD     0x55
#define JY62_FRAME_LEN      11

#define JY62_TYPE_ACC       0x51
#define JY62_TYPE_GYRO      0x52
#define JY62_TYPE_ANGLE     0x53

typedef struct
{
    int16_t ax_raw;
    int16_t ay_raw;
    int16_t az_raw;
    int16_t temp_raw;

    int16_t wx_raw;
    int16_t wy_raw;
    int16_t wz_raw;
    int16_t voltage_raw;

    int16_t roll_raw;
    int16_t pitch_raw;
    int16_t yaw_raw;
    int16_t version_raw;

    float ax;
    float ay;
    float az;

    float ax_ms2;
    float ay_ms2;
    float az_ms2;

    float wx;
    float wy;
    float wz;

    float roll;
    float pitch;
    float yaw;

    float temperature;
    float voltage;

    volatile uint8_t acc_flag;
    volatile uint8_t gyro_flag;
    volatile uint8_t angle_flag;

    volatile uint32_t frame_cnt;
    volatile uint32_t err_cnt;

} JY62_Data_t;

extern volatile JY62_Data_t jy62;

void JY62_Init(UART_HandleTypeDef *huart);
uint8_t JY62_UartRxCpltCallback(UART_HandleTypeDef *huart);
void JY62_ParseByte(uint8_t data);

void JY62_SendCmd(UART_HandleTypeDef *huart, uint8_t cmd);
void JY62_ZAxisZero(UART_HandleTypeDef *huart);
void JY62_AccCalibration(UART_HandleTypeDef *huart);
void JY62_SleepOrWake(UART_HandleTypeDef *huart);
void JY62_SetUartMode(UART_HandleTypeDef *huart);

#endif

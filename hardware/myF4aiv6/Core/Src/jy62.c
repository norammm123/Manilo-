#include "jy62.h"
#include "usart.h"
volatile JY62_Data_t jy62;

static UART_HandleTypeDef *jy62_uart = NULL;

static uint8_t jy62_rx_buf[JY62_FRAME_LEN];
static uint8_t jy62_rx_index = 0;

static int16_t JY62_GetShort(uint8_t low, uint8_t high)
{
    return (int16_t)(((uint16_t)high << 8) | low);
}

static uint8_t JY62_CheckSum(uint8_t *buf)
{
    uint8_t sum = 0;

    for (uint8_t i = 0; i < 10; i++)
    {
        sum += buf[i];
    }

    if (sum == buf[10])
    {
        return 1;
    }
    else
    {
        return 0;
    }
}

void JY62_Init(UART_HandleTypeDef *huart)
{
    jy62_uart = huart;
    jy62_rx_index = 0;

    HAL_UART_Receive_IT(jy62_uart, &uart6_rx, 1);
}

uint8_t JY62_UartRxCpltCallback(UART_HandleTypeDef *huart)
{
    

    HAL_UART_Receive_IT(jy62_uart, &uart6_rx, 1);

    return 1;
}

void JY62_ParseByte(uint8_t data)
{
    if (jy62_rx_index == 0)
    {
        if (data == JY62_FRAME_HEAD)
        {
            jy62_rx_buf[0] = data;
            jy62_rx_index = 1;
        }

        return;
    }

    jy62_rx_buf[jy62_rx_index] = data;
    jy62_rx_index++;

    if (jy62_rx_index == 2)
    {
        if ((jy62_rx_buf[1] != JY62_TYPE_ACC) &&
            (jy62_rx_buf[1] != JY62_TYPE_GYRO) &&
            (jy62_rx_buf[1] != JY62_TYPE_ANGLE))
        {
            jy62_rx_index = 0;
            jy62.err_cnt++;
            return;
        }
    }

    if (jy62_rx_index >= JY62_FRAME_LEN)
    {
        jy62_rx_index = 0;

        if (JY62_CheckSum(jy62_rx_buf) == 0)
        {
            jy62.err_cnt++;
            return;
        }

        jy62.frame_cnt++;

        switch (jy62_rx_buf[1])
        {
            case JY62_TYPE_ACC:
            {
                jy62.ax_raw   = JY62_GetShort(jy62_rx_buf[2], jy62_rx_buf[3]);
                jy62.ay_raw   = JY62_GetShort(jy62_rx_buf[4], jy62_rx_buf[5]);
                jy62.az_raw   = JY62_GetShort(jy62_rx_buf[6], jy62_rx_buf[7]);
                jy62.temp_raw = JY62_GetShort(jy62_rx_buf[8], jy62_rx_buf[9]);

                jy62.ax = (float)jy62.ax_raw / 32768.0f * 16.0f;
                jy62.ay = (float)jy62.ay_raw / 32768.0f * 16.0f;
                jy62.az = (float)jy62.az_raw / 32768.0f * 16.0f;

                jy62.ax_ms2 = jy62.ax * 9.8f;
                jy62.ay_ms2 = jy62.ay * 9.8f;
                jy62.az_ms2 = jy62.az * 9.8f;

                jy62.temperature = (float)jy62.temp_raw / 32768.0f * 96.38f + 36.53f;

                jy62.acc_flag = 1;
                break;
            }

            case JY62_TYPE_GYRO:
            {
                jy62.wx_raw      = JY62_GetShort(jy62_rx_buf[2], jy62_rx_buf[3]);
                jy62.wy_raw      = JY62_GetShort(jy62_rx_buf[4], jy62_rx_buf[5]);
                jy62.wz_raw      = JY62_GetShort(jy62_rx_buf[6], jy62_rx_buf[7]);
                jy62.voltage_raw = JY62_GetShort(jy62_rx_buf[8], jy62_rx_buf[9]);

                jy62.wx = (float)jy62.wx_raw / 32768.0f * 2000.0f;
                jy62.wy = (float)jy62.wy_raw / 32768.0f * 2000.0f;
                jy62.wz = (float)jy62.wz_raw / 32768.0f * 2000.0f;

                jy62.voltage = (float)jy62.voltage_raw / 100.0f;

                jy62.gyro_flag = 1;
                break;
            }

            case JY62_TYPE_ANGLE:
            {
                jy62.roll_raw    = JY62_GetShort(jy62_rx_buf[2], jy62_rx_buf[3]);
                jy62.pitch_raw   = JY62_GetShort(jy62_rx_buf[4], jy62_rx_buf[5]);
                jy62.yaw_raw     = JY62_GetShort(jy62_rx_buf[6], jy62_rx_buf[7]);
                jy62.version_raw = JY62_GetShort(jy62_rx_buf[8], jy62_rx_buf[9]);

                jy62.roll  = (float)jy62.roll_raw  / 32768.0f * 180.0f;
                jy62.pitch = (float)jy62.pitch_raw / 32768.0f * 180.0f;
                jy62.yaw   = (float)jy62.yaw_raw   / 32768.0f * 180.0f;

                jy62.angle_flag = 1;
                break;
            }

            default:
            {
                break;
            }
        }
    }
}

void JY62_SendCmd(UART_HandleTypeDef *huart, uint8_t cmd)
{
    uint8_t data[3];

    data[0] = 0xFF;
    data[1] = 0xAA;
    data[2] = cmd;

    HAL_UART_Transmit(huart, data, 3, 100);
}

void JY62_ZAxisZero(UART_HandleTypeDef *huart)
{
    JY62_SendCmd(huart, 0x52);
}

void JY62_AccCalibration(UART_HandleTypeDef *huart)
{
    JY62_SendCmd(huart, 0x67);
}

void JY62_SleepOrWake(UART_HandleTypeDef *huart)
{
    JY62_SendCmd(huart, 0x60);
}

void JY62_SetUartMode(UART_HandleTypeDef *huart)
{
    JY62_SendCmd(huart, 0x61);
}

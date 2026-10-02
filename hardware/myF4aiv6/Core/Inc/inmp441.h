#ifndef __INMP441_H
#define __INMP441_H

#include "main.h"
#include "i2s.h"
#include <stdint.h>

/* I2S DMA 接收缓冲区大小 */
#define INMP441_RX_BUF_SIZE      1024
#define INMP441_VOICE_FEATURE_COUNT 9

/* 声道选择 */
#define INMP441_CHANNEL_LEFT     0
#define INMP441_CHANNEL_RIGHT    1
#define INMP441_CHANNEL_AUTO     2

/*
 * 如果 INMP441 的 L/R 接 GND，选择 LEFT
 * 如果 INMP441 的 L/R 接 3.3V，选择 RIGHT
 */
#define INMP441_USE_CHANNEL      INMP441_CHANNEL_AUTO

typedef struct
{
    volatile uint8_t half_ready;
    volatile uint8_t full_ready;
    volatile uint8_t is_running;

    volatile int32_t last_sample;
    volatile int32_t last_left_sample;
    volatile int32_t last_right_sample;
    volatile uint32_t level;
    volatile uint32_t peak;
    volatile uint32_t energy;
    volatile uint32_t zcr;
    volatile uint32_t peak_avg_ratio;
    volatile uint32_t fft_low_energy;
    volatile uint32_t fft_mid_energy;
    volatile uint32_t fft_high_energy;
    volatile uint32_t spectral_centroid;
    volatile uint32_t dominant_freq;
    volatile uint32_t left_level;
    volatile uint32_t right_level;
    volatile uint32_t left_peak;
    volatile uint32_t right_peak;
    volatile int32_t left_dc;
    volatile int32_t right_dc;
    volatile uint32_t left_valid_count;
    volatile uint32_t right_valid_count;
    volatile uint32_t half_count;
    volatile uint32_t full_count;
    volatile uint32_t error_count;
    volatile uint32_t last_error;
    volatile uint32_t invalid_count;
    volatile uint32_t glitch_count;
    volatile uint8_t active_channel;
} INMP441_TypeDef;

extern INMP441_TypeDef inmp441;

/* 初始化 INMP441 软件状态 */
void INMP441_Init(void);

/* 启动 I2S DMA 采集 */
HAL_StatusTypeDef INMP441_Start(void);

/* 停止 I2S DMA 采集 */
HAL_StatusTypeDef INMP441_Stop(void);

/* 在 while(1) 中循环调用 */
void INMP441_Task(void);

/* 获取声音平均强度 */
uint32_t INMP441_GetLevel(void);

/* 获取声音峰值 */
uint32_t INMP441_GetPeak(void);

uint32_t INMP441_GetEnergy(void);

uint32_t INMP441_GetZcr(void);

uint32_t INMP441_GetPeakAvgRatio(void);

/* 获取最近一次原始采样值 */
int32_t INMP441_GetLastSample(void);

/* I2S DMA 半满回调 */
void INMP441_RxHalfCpltCallback(I2S_HandleTypeDef *hi2s);

/* I2S DMA 全满回调 */
void INMP441_RxCpltCallback(I2S_HandleTypeDef *hi2s);

void INMP441_ErrorCallback(I2S_HandleTypeDef *hi2s);

#endif

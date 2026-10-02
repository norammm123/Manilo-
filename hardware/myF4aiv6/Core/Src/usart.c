/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    usart.c
  * @brief   This file provides code for the configuration
  *          of the USART instances.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "usart.h"

/* USER CODE BEGIN 0 */
#include "jy62.h"
/* USER CODE END 0 */

UART_HandleTypeDef huart1;
UART_HandleTypeDef huart2;
UART_HandleTypeDef huart3;
UART_HandleTypeDef huart6;

/* USART1 init function */

void MX_USART1_UART_Init(void)
{

  /* USER CODE BEGIN USART1_Init 0 */

  /* USER CODE END USART1_Init 0 */

  /* USER CODE BEGIN USART1_Init 1 */

  /* USER CODE END USART1_Init 1 */
  huart1.Instance = USART1;
  huart1.Init.BaudRate = 9600;
  huart1.Init.WordLength = UART_WORDLENGTH_8B;
  huart1.Init.StopBits = UART_STOPBITS_1;
  huart1.Init.Parity = UART_PARITY_NONE;
  huart1.Init.Mode = UART_MODE_TX_RX;
  huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart1.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART1_Init 2 */

  /* USER CODE END USART1_Init 2 */

}
/* USART2 init function */

void MX_USART2_UART_Init(void)
{

  /* USER CODE BEGIN USART2_Init 0 */

  /* USER CODE END USART2_Init 0 */

  /* USER CODE BEGIN USART2_Init 1 */

  /* USER CODE END USART2_Init 1 */
  huart2.Instance = USART2;
  huart2.Init.BaudRate = 9600;
  huart2.Init.WordLength = UART_WORDLENGTH_8B;
  huart2.Init.StopBits = UART_STOPBITS_1;
  huart2.Init.Parity = UART_PARITY_NONE;
  huart2.Init.Mode = UART_MODE_TX_RX;
  huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart2.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART2_Init 2 */

  /* USER CODE END USART2_Init 2 */

}
/* USART3 init function */

void MX_USART3_UART_Init(void)
{

  /* USER CODE BEGIN USART3_Init 0 */

  /* USER CODE END USART3_Init 0 */

  /* USER CODE BEGIN USART3_Init 1 */

  /* USER CODE END USART3_Init 1 */
  huart3.Instance = USART3;
  huart3.Init.BaudRate = 115200;
  huart3.Init.WordLength = UART_WORDLENGTH_8B;
  huart3.Init.StopBits = UART_STOPBITS_1;
  huart3.Init.Parity = UART_PARITY_NONE;
  huart3.Init.Mode = UART_MODE_TX_RX;
  huart3.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart3.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart3) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART3_Init 2 */

  /* USER CODE END USART3_Init 2 */

}
/* USART6 init function */

void MX_USART6_UART_Init(void)
{

  /* USER CODE BEGIN USART6_Init 0 */

  /* USER CODE END USART6_Init 0 */

  /* USER CODE BEGIN USART6_Init 1 */

  /* USER CODE END USART6_Init 1 */
  huart6.Instance = USART6;
  huart6.Init.BaudRate = 115200;
  huart6.Init.WordLength = UART_WORDLENGTH_8B;
  huart6.Init.StopBits = UART_STOPBITS_1;
  huart6.Init.Parity = UART_PARITY_NONE;
  huart6.Init.Mode = UART_MODE_TX_RX;
  huart6.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart6.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart6) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART6_Init 2 */

  /* USER CODE END USART6_Init 2 */

}

void HAL_UART_MspInit(UART_HandleTypeDef* uartHandle)
{

  GPIO_InitTypeDef GPIO_InitStruct = {0};
  if(uartHandle->Instance==USART1)
  {
  /* USER CODE BEGIN USART1_MspInit 0 */

  /* USER CODE END USART1_MspInit 0 */
    /* USART1 clock enable */
    __HAL_RCC_USART1_CLK_ENABLE();

    __HAL_RCC_GPIOB_CLK_ENABLE();
    /**USART1 GPIO Configuration
    PB6     ------> USART1_TX
    PB7     ------> USART1_RX
    */
    GPIO_InitStruct.Pin = GPIO_PIN_6|GPIO_PIN_7;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF7_USART1;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    /* USART1 interrupt Init */
    HAL_NVIC_SetPriority(USART1_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(USART1_IRQn);
  /* USER CODE BEGIN USART1_MspInit 1 */

  /* USER CODE END USART1_MspInit 1 */
  }
  else if(uartHandle->Instance==USART2)
  {
  /* USER CODE BEGIN USART2_MspInit 0 */

  /* USER CODE END USART2_MspInit 0 */
    /* USART2 clock enable */
    __HAL_RCC_USART2_CLK_ENABLE();

    __HAL_RCC_GPIOA_CLK_ENABLE();
    /**USART2 GPIO Configuration
    PA2     ------> USART2_TX
    PA3     ------> USART2_RX
    */
    GPIO_InitStruct.Pin = GPIO_PIN_2|GPIO_PIN_3;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF7_USART2;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    /* USART2 interrupt Init */
    HAL_NVIC_SetPriority(USART2_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(USART2_IRQn);
  /* USER CODE BEGIN USART2_MspInit 1 */

  /* USER CODE END USART2_MspInit 1 */
  }
  else if(uartHandle->Instance==USART3)
  {
  /* USER CODE BEGIN USART3_MspInit 0 */

  /* USER CODE END USART3_MspInit 0 */
    /* USART3 clock enable */
    __HAL_RCC_USART3_CLK_ENABLE();

    __HAL_RCC_GPIOD_CLK_ENABLE();
    /**USART3 GPIO Configuration
    PD8     ------> USART3_TX
    PD9     ------> USART3_RX
    */
    GPIO_InitStruct.Pin = GPIO_PIN_8|GPIO_PIN_9;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF7_USART3;
    HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

    /* USART3 interrupt Init */
    HAL_NVIC_SetPriority(USART3_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(USART3_IRQn);
  /* USER CODE BEGIN USART3_MspInit 1 */

  /* USER CODE END USART3_MspInit 1 */
  }
  else if(uartHandle->Instance==USART6)
  {
  /* USER CODE BEGIN USART6_MspInit 0 */

  /* USER CODE END USART6_MspInit 0 */
    /* USART6 clock enable */
    __HAL_RCC_USART6_CLK_ENABLE();

    __HAL_RCC_GPIOC_CLK_ENABLE();
    /**USART6 GPIO Configuration
    PC6     ------> USART6_TX
    PC7     ------> USART6_RX
    */
    GPIO_InitStruct.Pin = GPIO_PIN_6|GPIO_PIN_7;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF8_USART6;
    HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

    /* USART6 interrupt Init */
    HAL_NVIC_SetPriority(USART6_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(USART6_IRQn);
  /* USER CODE BEGIN USART6_MspInit 1 */

  /* USER CODE END USART6_MspInit 1 */
  }
}

void HAL_UART_MspDeInit(UART_HandleTypeDef* uartHandle)
{

  if(uartHandle->Instance==USART1)
  {
  /* USER CODE BEGIN USART1_MspDeInit 0 */

  /* USER CODE END USART1_MspDeInit 0 */
    /* Peripheral clock disable */
    __HAL_RCC_USART1_CLK_DISABLE();

    /**USART1 GPIO Configuration
    PB6     ------> USART1_TX
    PB7     ------> USART1_RX
    */
    HAL_GPIO_DeInit(GPIOB, GPIO_PIN_6|GPIO_PIN_7);

    /* USART1 interrupt Deinit */
    HAL_NVIC_DisableIRQ(USART1_IRQn);
  /* USER CODE BEGIN USART1_MspDeInit 1 */

  /* USER CODE END USART1_MspDeInit 1 */
  }
  else if(uartHandle->Instance==USART2)
  {
  /* USER CODE BEGIN USART2_MspDeInit 0 */

  /* USER CODE END USART2_MspDeInit 0 */
    /* Peripheral clock disable */
    __HAL_RCC_USART2_CLK_DISABLE();

    /**USART2 GPIO Configuration
    PA2     ------> USART2_TX
    PA3     ------> USART2_RX
    */
    HAL_GPIO_DeInit(GPIOA, GPIO_PIN_2|GPIO_PIN_3);

    /* USART2 interrupt Deinit */
    HAL_NVIC_DisableIRQ(USART2_IRQn);
  /* USER CODE BEGIN USART2_MspDeInit 1 */

  /* USER CODE END USART2_MspDeInit 1 */
  }
  else if(uartHandle->Instance==USART3)
  {
  /* USER CODE BEGIN USART3_MspDeInit 0 */

  /* USER CODE END USART3_MspDeInit 0 */
    /* Peripheral clock disable */
    __HAL_RCC_USART3_CLK_DISABLE();

    /**USART3 GPIO Configuration
    PD8     ------> USART3_TX
    PD9     ------> USART3_RX
    */
    HAL_GPIO_DeInit(GPIOD, GPIO_PIN_8|GPIO_PIN_9);

    /* USART3 interrupt Deinit */
    HAL_NVIC_DisableIRQ(USART3_IRQn);
  /* USER CODE BEGIN USART3_MspDeInit 1 */

  /* USER CODE END USART3_MspDeInit 1 */
  }
  else if(uartHandle->Instance==USART6)
  {
  /* USER CODE BEGIN USART6_MspDeInit 0 */

  /* USER CODE END USART6_MspDeInit 0 */
    /* Peripheral clock disable */
    __HAL_RCC_USART6_CLK_DISABLE();

    /**USART6 GPIO Configuration
    PC6     ------> USART6_TX
    PC7     ------> USART6_RX
    */
    HAL_GPIO_DeInit(GPIOC, GPIO_PIN_6|GPIO_PIN_7);

    /* USART6 interrupt Deinit */
    HAL_NVIC_DisableIRQ(USART6_IRQn);
  /* USER CODE BEGIN USART6_MspDeInit 1 */

  /* USER CODE END USART6_MspDeInit 1 */
  }
}

/* USER CODE BEGIN 1 */

/*
uart1 tts语音模块
uart2 串口屏
uart3 STM32MP2
uart6 陀螺仪
*/
#define TEXT_FRAME_HEAD             0xFEU
#define VOICE_TEXT_FRAME_TYPE       0xEFU
#define SPELL_TEXT_FRAME_TYPE       0xFFU
#define VOICE_TEXT_FRAME_TAIL_1     0xEFU
#define VOICE_TEXT_FRAME_TAIL_2     0xFEU
#define SPELL_TEXT_FRAME_TAIL_1     0xFFU
#define SPELL_TEXT_FRAME_TAIL_2     0xEFU
#define TEXT_RX_MAX                 256U
#define GESTURE_RESULT_FRAME_HEAD   0xCCU
#define GESTURE_RESULT_FRAME_TAIL   0xDDU
#define GESTURE_RESULT_CLASS_COUNT  18U
#define GESTURE_CONFIDENCE_MAX      100U

typedef enum
{
    TEXT_RX_WAIT_HEAD = 0,
    TEXT_RX_WAIT_TYPE,
    TEXT_RX_PAYLOAD,          /* VOICE 帧: tail-scan UTF-8 */
    TEXT_RX_SPELL_ULEN,       /* 协议B v2: 读 UTF-8 段长度 */
    TEXT_RX_SPELL_UTF8,       /* 按 ulen 计数读 UTF-8 段 */
    TEXT_RX_SPELL_GLEN,       /* 读 GBK 段长度 */
    TEXT_RX_SPELL_GBK,        /* 按 glen 计数读 GBK 段 */
    TEXT_RX_SPELL_TAIL1,      /* 期望 0xFF */
    TEXT_RX_SPELL_TAIL2       /* 期望 0xEF, 收帧 */
} TextRxState;

typedef enum
{
    TEXT_FRAME_NONE = 0,
    TEXT_FRAME_VOICE,
    TEXT_FRAME_SPELL
} TextFrameType;

typedef enum
{
    GESTURE_RX_WAIT_HEAD = 0,
    GESTURE_RX_WAIT_INDEX,
    GESTURE_RX_WAIT_CONFIDENCE,
    GESTURE_RX_WAIT_TAIL
} GestureResultRxState;

uint8_t voice_text_rx_buf[TEXT_RX_MAX];
volatile uint8_t voice_text_rx_done = 0U;
uint8_t spell_rx_buf[TEXT_RX_MAX];
volatile uint8_t spell_rx_done = 0U;
uint8_t spell_gbk_rx_buf[TEXT_RX_MAX];   /* 拼音确认帧的 GBK 段, 供 USART1 TTS 播报 */
volatile uint16_t spell_gbk_len = 0U;

static uint8_t text_rx_work_buf[TEXT_RX_MAX];
static uint16_t text_rx_work_len;
static TextRxState text_rx_state = TEXT_RX_WAIT_HEAD;
static TextFrameType text_frame_type = TEXT_FRAME_NONE;
static uint8_t text_tail_wait;
static uint16_t spell_ulen;                 /* 协议B v2: UTF-8 段长度 */
static uint16_t spell_glen;                 /* 协议B v2: GBK 段长度 */
static uint16_t spell_gbk_cnt;              /* GBK 段已收字节计数 */
static GestureResultRxState gesture_rx_state = GESTURE_RX_WAIT_HEAD;
static uint8_t gesture_rx_index;
static uint8_t gesture_rx_confidence;
static volatile uint8_t gesture_rx_ready;
static uint8_t gesture_rx_result_index;
static uint8_t gesture_rx_result_confidence;
uint16_t rx_cnt;

uint8_t uart1_rx;
uint8_t uart2_rx;
uint8_t uart3_rx;
uint8_t uart6_rx;

uint8_t rx1_state;
uint8_t rx2_state;
uint8_t rx3_state;
uint8_t rx6_state;


uint8_t rx1_buffer[20];
uint8_t rx2_buffer[20]; 
uint8_t rx3_buffer[20];
uint8_t rx6_buffer[20];

uint8_t rx1_index;
uint8_t rx2_index;
uint8_t rx3_index;
uint8_t rx6_index;

extern volatile uint8_t gesture_enabled;
extern volatile uint8_t spell_enabled;
extern volatile uint8_t text_display_enabled;
uint8_t volatile uart1_flag;
uint8_t volatile uart2_flag;
uint8_t volatile uart3_flag;
uint8_t volatile uart6_flag;
volatile uint8_t uart2_last_command;
static uint8_t uart2_pending_command;

static void TextRx_Reset(uint8_t rec)
{
    text_rx_work_len = 0U;
    text_frame_type = TEXT_FRAME_NONE;
    text_tail_wait = 0U;
    spell_ulen = 0U;
    spell_glen = 0U;
    spell_gbk_cnt = 0U;
    text_rx_state = (rec == TEXT_FRAME_HEAD) ? TEXT_RX_WAIT_TYPE : TEXT_RX_WAIT_HEAD;
}

static void TextRx_Publish(void)
{
    if ((text_frame_type == TEXT_FRAME_SPELL) && (spell_rx_done == 0U))
    {
        /* 协议B v2: UTF-8 段 → 串口屏, GBK 段 → TTS 播报 */
        if (spell_ulen > 0U)
        {
            memcpy(spell_rx_buf, text_rx_work_buf, spell_ulen);
            spell_rx_buf[spell_ulen] = '\0';
        }
        spell_gbk_rx_buf[spell_glen] = '\0';   /* GBK 无 0x00, NUL 结尾安全 */
        spell_gbk_len = spell_glen;
        spell_rx_done = 1U;
        return;
    }

    if (text_rx_work_len == 0U)
    {
        return;
    }

    text_rx_work_buf[text_rx_work_len] = '\0';

    if ((text_frame_type == TEXT_FRAME_VOICE) && (voice_text_rx_done == 0U))
    {
        memcpy(voice_text_rx_buf, text_rx_work_buf, text_rx_work_len + 1U);
        voice_text_rx_done = 1U;
    }
}

static void TextRx_ParseByte(uint8_t rec)
{
    uint8_t tail_1;
    uint8_t tail_2;

    if (text_rx_state == TEXT_RX_WAIT_HEAD)
    {
        if (rec == TEXT_FRAME_HEAD)
        {
            text_rx_state = TEXT_RX_WAIT_TYPE;
        }
        return;
    }

    if (text_rx_state == TEXT_RX_WAIT_TYPE)
    {
        if (rec == VOICE_TEXT_FRAME_TYPE)
        {
            text_frame_type = TEXT_FRAME_VOICE;
            text_rx_work_len = 0U;
            text_tail_wait = 0U;
            text_rx_state = TEXT_RX_PAYLOAD;
        }
        else if (rec == SPELL_TEXT_FRAME_TYPE)
        {
            /* 协议B v2: 长度前缀双段帧 */
            text_frame_type = TEXT_FRAME_SPELL;
            text_rx_work_len = 0U;
            text_tail_wait = 0U;
            spell_ulen = 0U;
            spell_glen = 0U;
            spell_gbk_cnt = 0U;
            text_rx_state = TEXT_RX_SPELL_ULEN;
        }
        else
        {
            TextRx_Reset(rec);
        }
        return;
    }

    /* ---- VOICE 帧: tail-scan UTF-8, 原逻辑不变 ---- */
    if (text_frame_type == TEXT_FRAME_VOICE)
    {
        tail_1 = VOICE_TEXT_FRAME_TAIL_1;
        tail_2 = VOICE_TEXT_FRAME_TAIL_2;

        if (text_tail_wait != 0U)
        {
            if (rec == tail_2)
            {
                TextRx_Publish();
                TextRx_Reset(0U);
                return;
            }

            if (text_rx_work_len >= (TEXT_RX_MAX - 1U))
            {
                TextRx_Reset(rec);
                return;
            }
            text_rx_work_buf[text_rx_work_len++] = tail_1;
            text_tail_wait = 0U;
        }

        if (rec == TEXT_FRAME_HEAD)
        {
            TextRx_Reset(rec);
        }
        else if (rec == tail_1)
        {
            text_tail_wait = 1U;
        }
        else if (text_rx_work_len < (TEXT_RX_MAX - 1U))
        {
            text_rx_work_buf[text_rx_work_len++] = rec;
        }
        else
        {
            TextRx_Reset(rec);
        }
        return;
    }

    /* ---- SPELL 帧: 按 ulen/glen 计数解析, 不做 0xFE/0xFF 扫描 ---- */
    if (text_rx_state == TEXT_RX_SPELL_ULEN)
    {
        spell_ulen = rec;
        if (spell_ulen >= TEXT_RX_MAX)
        {
            TextRx_Reset(rec);
            return;
        }
        text_rx_work_len = 0U;
        text_rx_state = TEXT_RX_SPELL_UTF8;
        return;
    }

    if (text_rx_state == TEXT_RX_SPELL_UTF8)
    {
        text_rx_work_buf[text_rx_work_len++] = rec;
        if (text_rx_work_len >= spell_ulen)
        {
            text_rx_state = TEXT_RX_SPELL_GLEN;
        }
        return;
    }

    if (text_rx_state == TEXT_RX_SPELL_GLEN)
    {
        spell_glen = rec;
        if (spell_glen >= TEXT_RX_MAX)
        {
            TextRx_Reset(rec);
            return;
        }
        spell_gbk_cnt = 0U;
        text_rx_state = (spell_glen == 0U) ? TEXT_RX_SPELL_TAIL1 : TEXT_RX_SPELL_GBK;
        return;
    }

    if (text_rx_state == TEXT_RX_SPELL_GBK)
    {
        spell_gbk_rx_buf[spell_gbk_cnt++] = rec;
        if (spell_gbk_cnt >= spell_glen)
        {
            text_rx_state = TEXT_RX_SPELL_TAIL1;
        }
        return;
    }

    if (text_rx_state == TEXT_RX_SPELL_TAIL1)
    {
        if (rec == SPELL_TEXT_FRAME_TAIL_1)
        {
            text_rx_state = TEXT_RX_SPELL_TAIL2;
        }
        else
        {
            TextRx_Reset(rec);
        }
        return;
    }

    if (text_rx_state == TEXT_RX_SPELL_TAIL2)
    {
        if (rec == SPELL_TEXT_FRAME_TAIL_2)
        {
            TextRx_Publish();
        }
        TextRx_Reset(0U);
        return;
    }
}

/*
 * MP2 normal-gesture result frame:
 *   0xCC + gesture index (0..17) + confidence percent (0..100) + 0xDD
 * Return 1 when the byte belongs to a gesture-result frame. Other bytes are
 * left for the existing MP2 text-frame parser.
 */
static uint8_t GestureResultRx_ParseByte(uint8_t rec)
{
    if (gesture_rx_state == GESTURE_RX_WAIT_HEAD)
    {
        if (rec == GESTURE_RESULT_FRAME_HEAD)
        {
            gesture_rx_state = GESTURE_RX_WAIT_INDEX;
            return 1U;
        }
        return 0U;
    }

    if (gesture_rx_state == GESTURE_RX_WAIT_INDEX)
    {
        if (rec < GESTURE_RESULT_CLASS_COUNT)
        {
            gesture_rx_index = rec;
            gesture_rx_state = GESTURE_RX_WAIT_CONFIDENCE;
        }
        else if (rec != GESTURE_RESULT_FRAME_HEAD)
        {
            gesture_rx_state = GESTURE_RX_WAIT_HEAD;
        }
        return 1U;
    }

    if (gesture_rx_state == GESTURE_RX_WAIT_CONFIDENCE)
    {
        if (rec <= GESTURE_CONFIDENCE_MAX)
        {
            gesture_rx_confidence = rec;
            gesture_rx_state = GESTURE_RX_WAIT_TAIL;
        }
        else if (rec == GESTURE_RESULT_FRAME_HEAD)
        {
            gesture_rx_state = GESTURE_RX_WAIT_INDEX;
        }
        else
        {
            gesture_rx_state = GESTURE_RX_WAIT_HEAD;
        }
        return 1U;
    }

    if (rec == GESTURE_RESULT_FRAME_TAIL)
    {
        if (gesture_rx_ready == 0U)
        {
            gesture_rx_result_index = gesture_rx_index;
            gesture_rx_result_confidence = gesture_rx_confidence;
            gesture_rx_ready = 1U;
        }
        gesture_rx_state = GESTURE_RX_WAIT_HEAD;
    }
    else if (rec == GESTURE_RESULT_FRAME_HEAD)
    {
        gesture_rx_state = GESTURE_RX_WAIT_INDEX;
    }
    else
    {
        gesture_rx_state = GESTURE_RX_WAIT_HEAD;
    }
    return 1U;
}

void UART3_SpellRxClear(void)
{
    HAL_NVIC_DisableIRQ(USART3_IRQn);
    if (text_frame_type == TEXT_FRAME_SPELL)
    {
        TextRx_Reset(0U);
    }
    spell_rx_buf[0] = '\0';
    spell_rx_done = 0U;
    HAL_NVIC_EnableIRQ(USART3_IRQn);
}

void UART3_GestureRxClear(void)
{
    HAL_NVIC_DisableIRQ(USART3_IRQn);
    gesture_rx_state = GESTURE_RX_WAIT_HEAD;
    gesture_rx_ready = 0U;
    HAL_NVIC_EnableIRQ(USART3_IRQn);
}

uint8_t UART3_GestureResult_Get(uint8_t *gesture_index,
                               uint8_t *confidence_percent)
{
    uint8_t available = 0U;

    if ((gesture_index == NULL) || (confidence_percent == NULL))
    {
        return 0U;
    }

    HAL_NVIC_DisableIRQ(USART3_IRQn);
    if (gesture_rx_ready != 0U)
    {
        *gesture_index = gesture_rx_result_index;
        *confidence_percent = gesture_rx_result_confidence;
        gesture_rx_ready = 0U;
        available = 1U;
    }
    HAL_NVIC_EnableIRQ(USART3_IRQn);

    return available;
}

void uart_printf(UART_HandleTypeDef *huart, const char *format, ...)
{
    char buffer[256];   // 缓冲区大小可按需修改
    va_list args;
    int len;

    va_start(args, format);
    len = vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);

    if (len > 0)
    {
        if (len > sizeof(buffer))
        {
            len = sizeof(buffer);
        }
        HAL_UART_Transmit(huart, (uint8_t *)buffer, len, HAL_MAX_DELAY);
    }
}


uint8_t uart_send[3]={0xFF,0XEE,0xFE};
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
		if (huart->Instance == USART2)//串口屏
    {
			uint8_t rec=uart2_rx;
			if(rx2_state==0)
			{
				if(rec==0xAA)
				{
					rx2_state=1;
				}
			}
			else if(rx2_state==1)
			{
				if((rec>=0x01U)&&(rec<=0x06U))
				{
					uart2_pending_command=rec;
					rx2_state=2;
				}
				else
				{
					rx2_state=0;
				}
			}
			else if(rx2_state==2)
			{
				if(rec==0xBB)
				{
					if(uart2_pending_command==0x01U)
					{
						gesture_enabled=1U;
						spell_enabled=0U;
						HAL_UART_Transmit(&huart3,&uart_send[0],1,0xffff);
					}
					else if(uart2_pending_command==0x02U)
					{
						gesture_enabled=0U;
					}
					else if(uart2_pending_command==0x03U)//打开语音
					{
						HAL_UART_Transmit(&huart3,&uart_send[2],1,0xffff);
						gesture_enabled=0U;
						spell_enabled=0U;
						text_display_enabled=1U;
					}
					else if(uart2_pending_command==0x04U)//关闭语音
					{
						text_display_enabled=0U;
					}
					else if(uart2_pending_command==0x05U)//打开拼音模式
					{
						spell_enabled=1U;
						gesture_enabled=0U;
						HAL_UART_Transmit(&huart3,&uart_send[1],1,0xffff);
					}
					else if(uart2_pending_command==0x06U)//关闭拼音模式
					{
						spell_enabled=0U;
					}
					uart2_last_command=uart2_pending_command;
					uart2_flag=1;
				}
				rx2_state=0;
			}
			HAL_UART_Receive_IT(&huart2, &uart2_rx,1);
    }
		
		
		else if (huart->Instance == USART3)   // STM32MP2
		{
			if (GestureResultRx_ParseByte(uart3_rx) == 0U)
			{
				TextRx_ParseByte(uart3_rx);
			}
			rx_cnt++;
			HAL_UART_Receive_IT(&huart3, &uart3_rx, 1);
		}
		
		
		
		else if (huart->Instance == USART6)//陀螺仪
    {	
				JY62_ParseByte(uart6_rx);
				HAL_UART_Receive_IT(&huart6, &uart6_rx,1);
		}	
}

volatile uint32_t uart6_error_count;

void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == USART6)
    {
        uart6_error_count++;

        __HAL_UART_CLEAR_OREFLAG(huart);

        /* 重置JY62解析状态，并重新启动单字节接收 */
        JY62_Init(huart);
    }
}

/* USER CODE END 1 */


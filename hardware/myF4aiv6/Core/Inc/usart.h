/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    usart.h
  * @brief   This file contains all the function prototypes for
  *          the usart.c file
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
/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __USART_H__
#define __USART_H__

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* USER CODE BEGIN Includes */
#include "stdio.h"
#include "stdarg.h"
#include "string.h"
	
/* USER CODE END Includes */

extern UART_HandleTypeDef huart1;

extern UART_HandleTypeDef huart2;

extern UART_HandleTypeDef huart3;

extern UART_HandleTypeDef huart6;

/* USER CODE BEGIN Private defines */
void uart_printf(UART_HandleTypeDef *huart, const char *format, ...);
void UART3_SpellRxClear(void);
void UART3_GestureRxClear(void);
uint8_t UART3_GestureResult_Get(uint8_t *gesture_index,
                               uint8_t *confidence_percent);
/* USER CODE END Private defines */

void MX_USART1_UART_Init(void);
void MX_USART2_UART_Init(void);
void MX_USART3_UART_Init(void);
void MX_USART6_UART_Init(void);

/* USER CODE BEGIN Prototypes */
extern uint8_t uart1_rx;
extern uint8_t uart2_rx;
extern uint8_t uart3_rx;
extern uint8_t uart6_rx;

extern uint8_t rx1_state;
extern uint8_t rx2_state;
extern uint8_t rx3_state;
extern uint8_t rx6_state;

extern uint8_t rx1_buffer[20]; 
extern uint8_t rx2_buffer[20]; 
extern uint8_t rx3_buffer[20]; 
extern uint8_t rx6_buffer[20];

extern uint8_t rx1_index;
extern uint8_t rx2_index;
extern uint8_t rx3_index;
extern uint8_t rx6_index;

extern volatile uint8_t uart1_flag;
extern volatile uint8_t uart2_flag;
extern volatile uint8_t uart3_flag;
extern volatile uint8_t uart6_flag;
extern volatile uint8_t uart2_last_command;

extern uint8_t voice_text_rx_buf[];
extern volatile uint8_t voice_text_rx_done;
extern uint8_t spell_rx_buf[];
extern volatile uint8_t spell_rx_done;
extern uint8_t spell_gbk_rx_buf[];
extern volatile uint16_t spell_gbk_len;
/* USER CODE END Prototypes */

#ifdef __cplusplus
}
#endif

#endif /* __USART_H__ */


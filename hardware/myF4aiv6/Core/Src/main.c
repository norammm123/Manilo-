/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
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
#include "main.h"
#include "adc.h"
#include "dma.h"
#include "i2s.h"
#include "tim.h"
#include "usart.h"
#include "gpio.h"
#include "app_x-cube-ai.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "jy62.h"
#include "inmp441.h"
#include <stdio.h>
#include <stdint.h>
#include <string.h>
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */
volatile uint32_t mic_level = 0;
volatile uint32_t mic_peak = 0;
volatile uint32_t mic_energy = 0;
volatile uint32_t mic_zcr = 0;
volatile uint32_t mic_peak_avg_ratio = 0;
volatile int32_t mic_sample = 0;
volatile uint8_t adc_send_flag;
/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */
static void MotorAlarm_Start(uint8_t event);
static void MotorAlarm_Task(void);
static void SoundEvent_Handle(uint8_t event);
static void MP2Feature_Send(void);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
volatile uint16_t ad_value[5];
float adc_train_buf[200][11];
int32_t adc_value[11];
volatile uint32_t adc_cnt;
uint16_t adc_index;
volatile uint8_t adc_flag;

volatile uint8_t sound_inference_ready;
uint16_t voice_cnt;
uint16_t voice_valid_count;
uint16_t voice_step_cnt;
uint32_t voice_buffer[250][INMP441_VOICE_FEATURE_COUNT];
uint8_t gesture_result = 0xFFU;
uint8_t sound_event_result = SOUND_EVENT_UNKNOWN;
volatile uint8_t gesture_enabled = 1U;
volatile uint8_t spell_enabled = 0U;
volatile uint8_t text_display_enabled  = 0U;

#define MOTOR_ALARM_COOLDOWN_MS 2000U

static uint8_t motor_alarm_active;
static uint8_t motor_alarm_event = SOUND_EVENT_UNKNOWN;
static uint8_t motor_alarm_phase;
static uint8_t motor_last_alarm_event = SOUND_EVENT_UNKNOWN;
static uint32_t motor_alarm_deadline;
static uint32_t motor_last_alarm_tick;

#define SPELL_TX_BUF_SIZE 160U



/* 0xAA + 11 comma-separated feature values + uint16 sequence + 0xBB */
static void MP2Feature_Send(void)
{
    static uint16_t sequence;
    uint8_t packet[SPELL_TX_BUF_SIZE];
    int payload_len;
    uint16_t frame_sequence = sequence++;

    packet[0] = 0xAAU;
    payload_len = snprintf((char *)&packet[1], sizeof(packet) - 2U,
                           "%ld,%ld,%ld,%ld,%ld,%ld,%ld,%ld,%ld,%ld,%ld,%u",
                           (long)adc_value[0], (long)adc_value[1],
                           (long)adc_value[2], (long)adc_value[3],
                           (long)adc_value[4], (long)adc_value[5],
                           (long)adc_value[6], (long)adc_value[7],
                           (long)adc_value[8], (long)adc_value[9],
                           (long)adc_value[10],
                           (unsigned int)frame_sequence);

    if ((payload_len > 0) &&
        ((uint32_t)payload_len < (sizeof(packet) - 2U)))
    {
        packet[(uint32_t)payload_len + 1U] = 0xBBU;
        HAL_UART_Transmit(&huart3, packet,
                          (uint16_t)((uint32_t)payload_len + 2U),
                          HAL_MAX_DELAY);
    }
}

static void MotorAlarm_Start(uint8_t event)
{
    uint32_t now;

    if ((event != SOUND_EVENT_HELP) &&
        (event != SOUND_EVENT_KNOCK) &&
        (event != SOUND_EVENT_HORN))
    {
        return;
    }

    now = HAL_GetTick();
    if (motor_alarm_active != 0U)
    {
        return;
    }

    if ((motor_last_alarm_event == event) &&
        ((uint32_t)(now - motor_last_alarm_tick) < MOTOR_ALARM_COOLDOWN_MS))
    {
        return;
    }

    motor_alarm_active = 1U;
    motor_alarm_event = event;
    motor_alarm_phase = 0U;
    motor_last_alarm_event = event;
    motor_last_alarm_tick = now;
    HAL_GPIO_WritePin(MOTOR_GPIO_Port, MOTOR_Pin, GPIO_PIN_SET);

    if (event == SOUND_EVENT_HELP)
    {
        motor_alarm_deadline = now + 1000U;
    }
    else if (event == SOUND_EVENT_KNOCK)
    {
        motor_alarm_deadline = now + 200U;
    }
    else
    {
        motor_alarm_deadline = now + 150U;
    }
}

static void MotorAlarm_Task(void)
{
    uint32_t now;

    if (motor_alarm_active == 0U)
    {
        return;
    }

    now = HAL_GetTick();
    if ((int32_t)(now - motor_alarm_deadline) < 0)
    {
        return;
    }

    if (motor_alarm_event == SOUND_EVENT_HELP)
    {
        HAL_GPIO_WritePin(MOTOR_GPIO_Port, MOTOR_Pin, GPIO_PIN_RESET);
        motor_alarm_active = 0U;
    }
    else if (motor_alarm_event == SOUND_EVENT_KNOCK)
    {
        if (motor_alarm_phase == 0U)
        {
            HAL_GPIO_WritePin(MOTOR_GPIO_Port, MOTOR_Pin, GPIO_PIN_RESET);
            motor_alarm_phase = 1U;
            motor_alarm_deadline = now + 150U;
        }
        else if (motor_alarm_phase == 1U)
        {
            HAL_GPIO_WritePin(MOTOR_GPIO_Port, MOTOR_Pin, GPIO_PIN_SET);
            motor_alarm_phase = 2U;
            motor_alarm_deadline = now + 200U;
        }
        else
        {
            HAL_GPIO_WritePin(MOTOR_GPIO_Port, MOTOR_Pin, GPIO_PIN_RESET);
            motor_alarm_active = 0U;
        }
    }
    else
    {
        if ((motor_alarm_phase == 0U) || (motor_alarm_phase == 2U))
        {
            HAL_GPIO_WritePin(MOTOR_GPIO_Port, MOTOR_Pin, GPIO_PIN_RESET);
            motor_alarm_phase++;
            motor_alarm_deadline = now + 100U;
        }
        else if ((motor_alarm_phase == 1U) || (motor_alarm_phase == 3U))
        {
            HAL_GPIO_WritePin(MOTOR_GPIO_Port, MOTOR_Pin, GPIO_PIN_SET);
            motor_alarm_phase++;
            motor_alarm_deadline = now + 150U;
        }
        else
        {
            HAL_GPIO_WritePin(MOTOR_GPIO_Port, MOTOR_Pin, GPIO_PIN_RESET);
            motor_alarm_active = 0U;
        }
    }
}

static void SoundEvent_Handle(uint8_t event)
{
    if (event == SOUND_EVENT_QUIET)
    {
        uart_printf(&huart2, "t16.txt=\"%s\"\xff\xff\xff", "周围安静");
    }
    else if (event == SOUND_EVENT_KNOCK)
    {
        uart_printf(&huart2, "t16.txt=\"%s\"\xff\xff\xff", "有人敲门");
        MotorAlarm_Start(event);
    }
    else if (event == SOUND_EVENT_HORN)
    {
        uart_printf(&huart2, "t16.txt=\"%s\"\xff\xff\xff", "车辆鸣笛");
        MotorAlarm_Start(event);
    }
    else
    {
        uart_printf(&huart2, "t16.txt=\"%s\"\xff\xff\xff","正常");
    }
}
/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_DMA_Init();
  MX_I2S3_Init();
  MX_ADC1_Init();
  MX_ADC2_Init();
  MX_USART2_UART_Init();
  MX_USART3_UART_Init();
  MX_USART6_UART_Init();
  MX_TIM3_Init();
  MX_USART1_UART_Init();
  MX_TIM6_Init();
  MX_X_CUBE_AI_Init();
  /* USER CODE BEGIN 2 */
	
	
	/* IMU必须能够抢占TIM6 */
	HAL_NVIC_SetPriority(USART6_IRQn, 0, 0);
	/* ADC/I2S等 */
	HAL_NVIC_SetPriority(DMA2_Stream0_IRQn, 1, 0);
	HAL_NVIC_SetPriority(DMA1_Stream0_IRQn, 1, 0);
	/* MP2/HC-08发送定时器 */
	HAL_NVIC_SetPriority(TIM6_DAC_IRQn, 3, 0);
	Voice_AI_Init();
	Gesture_AI_ResetDecision();
	JY62_Init(&huart6);
	HAL_ADC_Start_DMA(&hadc1,(uint32_t*)ad_value,5);
	HAL_TIM_Base_Start(&htim3);
	HAL_TIM_Base_Start_IT(&htim6);
	HAL_UART_Receive_IT(&huart1,&uart1_rx,1);
	HAL_UART_Receive_IT(&huart2,&uart2_rx,1);
	HAL_UART_Receive_IT(&huart3,&uart3_rx,1);
	HAL_GPIO_WritePin(MOTOR_GPIO_Port,MOTOR_Pin,GPIO_PIN_RESET);
	INMP441_Init();
	INMP441_Start();
	
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
	while (1)
  {
		uint8_t mp2_gesture_index;
		uint8_t mp2_gesture_confidence;

		if(uart2_flag==1)
		{
			uart2_flag=0;
			if ((uart2_last_command == 0x01U) ||
			    (uart2_last_command == 0x02U) ||
			    (uart2_last_command == 0x05U) ||
			    (uart2_last_command == 0x06U))
			{
				Gesture_AI_ResetDecision();
			}
			if ((uart2_last_command == 0x01U) || (uart2_last_command == 0x05U))
			{
				if(adc_send_flag != 0U)
				{
					adc_send_flag = 0U;
					if(adc_flag == 0U)
					{
						HAL_ADC_Start_DMA(&hadc1,(uint32_t*)ad_value,5);
					}
				}
				UART3_SpellRxClear();
				UART3_GestureRxClear();

			}
		}
		if (UART3_GestureResult_Get(&mp2_gesture_index,
		                            &mp2_gesture_confidence) != 0U)
		{
			Gesture_MP2Result_Process(mp2_gesture_index,
			                          mp2_gesture_confidence);
		}
		
		if(adc_flag==1)
		{
			adc_flag=0;
			
			if(gesture_enabled)
			{
//				 MX_X_CUBE_AI_Process();
			}
			HAL_ADC_Start_DMA(&hadc1,(uint32_t*)ad_value,5);
			HAL_TIM_Base_Start(&htim3);
		}
		INMP441_Task();
		if (sound_inference_ready)
		{
			sound_inference_ready=0;
			SoundEvent_AI_Process();
			SoundEvent_Handle(sound_event_result);
		}
		MotorAlarm_Task();
		if (voice_text_rx_done)
		{
			if(text_display_enabled)
			{
				uart_printf(&huart2, "t12.txt=\"%s\"\xff\xff\xff", voice_text_rx_buf);
			}
			voice_text_rx_done = 0U;
		}
		if (spell_rx_done)
		{
			if(spell_enabled)
			{
				uart_printf(&huart2, "t11.txt=\"%s\"\xff\xff\xff", spell_rx_buf);
				if (spell_gbk_len > 0U)
				{
					uart_printf(&huart1, "%s\r\n", spell_gbk_rx_buf);
				}
			}
			spell_rx_done = 0U;
			spell_gbk_len = 0U;
		}
    /* USER CODE END WHILE */

//  MX_X_CUBE_AI_Process();
    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM = 8;
  RCC_OscInitStruct.PLL.PLLN = 168;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 4;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */
void HAL_I2S_RxHalfCpltCallback(I2S_HandleTypeDef *hi2s)
{
    INMP441_RxHalfCpltCallback(hi2s);
}

void HAL_I2S_RxCpltCallback(I2S_HandleTypeDef *hi2s)
{
    INMP441_RxCpltCallback(hi2s);
}

void HAL_I2S_ErrorCallback(I2S_HandleTypeDef *hi2s)
{
    INMP441_ErrorCallback(hi2s);
}

void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef* hadc)
{
    if (hadc->Instance == ADC1)//10ms进一次
    {
				uint8_t i;
				for(i=0;i<5;i++)
				{
					adc_train_buf[adc_index+adc_cnt][i]=(float)((ad_value[i] - 300.0f) * 5.0f);
					adc_value[i]=(((float)ad_value[i])-300.0f)*5.0f;
				}
				adc_train_buf[adc_index+adc_cnt][5]=jy62.ax*1000.0f;
				adc_train_buf[adc_index+adc_cnt][6]=jy62.ay*1000.0f;
				adc_train_buf[adc_index+adc_cnt][7]=jy62.az*1000.0f;
				adc_train_buf[adc_index+adc_cnt][8]=jy62.wx*100.0f;
				adc_train_buf[adc_index+adc_cnt][9]=jy62.wy*100.0f;
				adc_train_buf[adc_index+adc_cnt][10]=jy62.wz*100.0f;

				adc_value[5]=(int32_t)(jy62.ax*1000.0f);
				adc_value[6]=(int32_t)(jy62.ay*1000.0f);
				adc_value[7]=(int32_t)(jy62.az*1000.0f);
				adc_value[8]=(int32_t)(jy62.wx*100.0f);
				adc_value[9]=(int32_t)(jy62.wy*100.0f);
				adc_value[10]=(int32_t)(jy62.wz*100.0f);
				adc_cnt++;      // 每采完一组，计数加一
				
				HAL_ADC_Start_DMA(&hadc1,(uint32_t*)ad_value,5);

				if(adc_cnt==20)
				{
				adc_cnt=0; 
				adc_index=adc_index+20;
				if(adc_index==200)adc_index=0;
				HAL_TIM_Base_Stop(&htim3);
				HAL_ADC_Stop_DMA(&hadc1);
				__HAL_TIM_SET_COUNTER(&htim3, 0);
				adc_flag = 1;   // 等待主循环处理数据
				}
    }
}
/* USER CODE END 4 */

/**
  * @brief  Period elapsed callback in non blocking mode
  * @note   This function is called  when TIM13 interrupt took place, inside
  * HAL_TIM_IRQHandler(). It makes a direct call to HAL_IncTick() to increment
  * a global variable "uwTick" used as application time base.
  * @param  htim : TIM handle
  * @retval None
  */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
  /* USER CODE BEGIN Callback 0 */

  /* USER CODE END Callback 0 */
  if (htim->Instance == TIM13)
  {
    HAL_IncTick();
  }
	if (htim->Instance == TIM6)
	{
			if(spell_enabled||gesture_enabled)
			{
				MP2Feature_Send();
			}
	}
  /* USER CODE BEGIN Callback 1 */

  /* USER CODE END Callback 1 */
}

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */

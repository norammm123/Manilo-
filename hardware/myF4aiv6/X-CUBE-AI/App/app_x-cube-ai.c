
/**
  ******************************************************************************
  * @file    app_x-cube-ai.c
  * @author  X-CUBE-AI C code generator
  * @brief   AI program body
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

 /*
  * Description
  *   v1.0 - Minimum template to show how to use the Embedded Client API
  *          model. Only one input and one output is supported. All
  *          memory resources are allocated statically (AI_NETWORK_XX, defines
  *          are used).
  *          Re-target of the printf function is out-of-scope.
  *   v2.0 - add multiple IO and/or multiple heap support
  *
  *   For more information, see the embeded documentation:
  *
  *       [1] %X_CUBE_AI_DIR%/Documentation/index.html
  *
  *   X_CUBE_AI_DIR indicates the location where the X-CUBE-AI pack is installed
  *   typical : C:\Users\[user_name]\STM32Cube\Repository\STMicroelectronics\X-CUBE-AI\7.1.0
  */

#ifdef __cplusplus
 extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/

#if defined ( __ICCARM__ )
#elif defined ( __CC_ARM ) || ( __GNUC__ )
#endif

/* System headers */
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <inttypes.h>
#include <string.h>

#include "app_x-cube-ai.h"
#include "main.h"
#include "ai_datatypes_defines.h"
#include "gesture.h"
#include "gesture_data.h"
#include "voice.h"
#include "voice_data.h"

/* USER CODE BEGIN includes */
#include "usart.h"
#include "voice.h"
#include "inmp441.h"
#include <math.h>
#include "voice_data_params.h"
uint8_t best_idx;
float best_score;
uint8_t gesture_confidence_percent;
extern volatile uint8_t gesture_enabled;
/* USER CODE END includes */

/* IO buffers ----------------------------------------------------------------*/

#if !defined(AI_GESTURE_INPUTS_IN_ACTIVATIONS)
AI_ALIGNED(4) ai_i8 data_in_1[AI_GESTURE_IN_1_SIZE_BYTES];
ai_i8* data_ins[AI_GESTURE_IN_NUM] = {
data_in_1
};
#else
ai_i8* data_ins[AI_GESTURE_IN_NUM] = {
NULL
};
#endif

#if !defined(AI_GESTURE_OUTPUTS_IN_ACTIVATIONS)
AI_ALIGNED(4) ai_i8 data_out_1[AI_GESTURE_OUT_1_SIZE_BYTES];
ai_i8* data_outs[AI_GESTURE_OUT_NUM] = {
data_out_1
};
#else
ai_i8* data_outs[AI_GESTURE_OUT_NUM] = {
NULL
};
#endif

/* Activations buffers -------------------------------------------------------*/

AI_ALIGNED(32)
static uint8_t pool0[AI_GESTURE_DATA_ACTIVATION_1_SIZE];

ai_handle data_activations0[] = {pool0};
ai_handle data_activations1[] = {pool0};

/* AI objects ----------------------------------------------------------------*/

static ai_handle gesture = AI_HANDLE_NULL;

static ai_buffer* ai_input;
static ai_buffer* ai_output;

static void ai_log_err(const ai_error err, const char *fct)
{
  /* USER CODE BEGIN log */
  if (fct);
//    printf("TEMPLATE - Error (%s) - type=0x%02x code=0x%02x\r\n", fct,
//        err.type, err.code)
  else
//    printf("TEMPLATE - Error - type=0x%02x code=0x%02x\r\n", err.type, err.code);

  do {} while (1);
  /* USER CODE END log */
}

static int ai_boostrap(ai_handle *act_addr)
{
  ai_error err;

  /* Create and initialize an instance of the model */
  err = ai_gesture_create_and_init(&gesture, act_addr, NULL);
  if (err.type != AI_ERROR_NONE) {
    ai_log_err(err, "ai_gesture_create_and_init");
    return -1;
  }

  ai_input = ai_gesture_inputs_get(gesture, NULL);
  ai_output = ai_gesture_outputs_get(gesture, NULL);

#if defined(AI_GESTURE_INPUTS_IN_ACTIVATIONS)
  /*  In the case where "--allocate-inputs" option is used, memory buffer can be
   *  used from the activations buffer. This is not mandatory.
   */
  for (int idx=0; idx < AI_GESTURE_IN_NUM; idx++) {
	data_ins[idx] = ai_input[idx].data;
  }
#else
  for (int idx=0; idx < AI_GESTURE_IN_NUM; idx++) {
	  ai_input[idx].data = data_ins[idx];
  }
#endif

#if defined(AI_GESTURE_OUTPUTS_IN_ACTIVATIONS)
  /*  In the case where "--allocate-outputs" option is used, memory buffer can be
   *  used from the activations buffer. This is no mandatory.
   */
  for (int idx=0; idx < AI_GESTURE_OUT_NUM; idx++) {
	data_outs[idx] = ai_output[idx].data;
  }
#else
  for (int idx=0; idx < AI_GESTURE_OUT_NUM; idx++) {
	ai_output[idx].data = data_outs[idx];
  }
#endif

  return 0;
}

static int ai_run(void)
{
  ai_i32 batch;

  batch = ai_gesture_run(gesture, ai_input, ai_output);
  if (batch != 1) {
    ai_log_err(ai_gesture_get_error(gesture),
        "ai_gesture_run");
    return -1;
  }

  return 0;
}

/* USER CODE BEGIN 2 */

static ai_handle voice = AI_HANDLE_NULL;
static ai_buffer *voice_input;
static ai_buffer *voice_output;

AI_ALIGNED(32)
static uint8_t voice_pool[AI_VOICE_DATA_ACTIVATION_1_SIZE];

static ai_handle voice_activations[] = { voice_pool };

uint8_t voice_best_idx;
float voice_best_score;
extern uint8_t gesture_result;
extern uint8_t sound_event_result;
uint8_t sound_event_confidence_percent;

extern volatile uint8_t sound_inference_ready;
extern uint16_t voice_cnt;
extern uint32_t voice_buffer[AI_VOICE_IN_1_HEIGHT][INMP441_VOICE_FEATURE_COUNT];
static uint8_t sound_candidate = SOUND_EVENT_UNKNOWN;
static uint8_t sound_candidate_count;

void Voice_AI_Init(void)
{
    ai_error err;

    err = ai_voice_create_and_init(&voice, voice_activations, NULL);
    if (err.type != AI_ERROR_NONE)
    {
        while (1);
    }

    voice_input = ai_voice_inputs_get(voice, NULL);
    voice_output = ai_voice_outputs_get(voice, NULL);
    sound_candidate = SOUND_EVENT_UNKNOWN;
    sound_candidate_count = 0U;
    sound_event_result = SOUND_EVENT_UNKNOWN;
}

static uint8_t voice_class_from_output(float *out, float *best_score)
{
    uint8_t best_idx = 0;
    float best = out[0];
    float exp_sum = 0.0f;

    for (uint8_t i = 1; i < AI_VOICE_OUT_1_SIZE; i++)
    {
        if (out[i] > best)
        {
            best = out[i];
            best_idx = i;
        }
    }

    for (uint8_t i = 0; i < AI_VOICE_OUT_1_SIZE; i++)
    {
        exp_sum += expf(out[i] - best);
    }

    if (exp_sum > 0.0f)
    {
        *best_score = 1.0f / exp_sum;
    }
    else
    {
        *best_score = 0.0f;
    }

    return best_idx;
}

void SoundEvent_AI_Process(void)
{
    float *in = (float *)voice_input[0].data;
    float *out = (float *)voice_output[0].data;
    ai_i32 batch;
    uint8_t raw_result;

    for (uint16_t t = 0; t < AI_VOICE_IN_1_HEIGHT; t++)
    {
        uint16_t src = voice_cnt + t;

        if (src >= AI_VOICE_IN_1_HEIGHT)
        {
            src -= AI_VOICE_IN_1_HEIGHT;
        }

        for (uint8_t c = 0; c < AI_VOICE_IN_1_CHANNEL; c++)
        {
            in[(t * AI_VOICE_IN_1_CHANNEL) + c] = (float)voice_buffer[src][c];
        }
    }

    batch = ai_voice_run(voice, voice_input, voice_output);
    if (batch != 1)
    {
        while (1);
    }

    voice_best_idx = voice_class_from_output(out, &voice_best_score);
    sound_event_confidence_percent = (uint8_t)((voice_best_score * 100.0f) + 0.5f);
    if (voice_best_idx==3&&sound_event_confidence_percent >=80)
    {
        raw_result = voice_best_idx;
    }
		else if (voice_best_idx==2&&sound_event_confidence_percent >= 35)
    {
        raw_result = voice_best_idx;
    }
    else
    {
        raw_result = SOUND_EVENT_UNKNOWN;
    }

    if (raw_result == sound_candidate)
    {
        if (sound_candidate_count < SOUND_CONFIRM_COUNT)
        {
            sound_candidate_count++;
        }
    }
    else
    {
        sound_candidate = raw_result;
        sound_candidate_count = 1U;
    }

    if (sound_candidate_count >= SOUND_CONFIRM_COUNT)
    {
        sound_event_result = sound_candidate;
    }
}

extern float adc_train_buf[200][11];
extern volatile uint16_t adc_index;
static uint8_t gesture_candidate = 0xFFU;
static uint8_t gesture_candidate_count;
static uint8_t gesture_last_confirmed = 0xFFU;

const char gesture_name_gbk[AI_GESTURE_OUT_1_SIZE][16] =
{
    "\xB2\xBB",                         // 不
    "\xC4\xE3",                         // 你
    "\xBD\xD0",                         // 叫
    "\xC4\xC4\xB6\xF9",                 // 哪儿
    "\xD4\xDA",                         // 在
    "\xCC\xEC",                         // 天
    "\xBA\xC3",                         // 好
    "\xC2\xE8\xC2\xE8",                 // 妈妈
    "\xBA\xDC",                         // 很
    "\xCE\xD2",                         // 我
    "\xB4\xF2\xB5\xE7\xBB\xB0",         // 打电话
    "\xD4\xE7\xC9\xCF",                 // 早上
    "\xCA\xC7",                         // 是
    "\xD3\xD0",                         // 有
    "\xC0\xB4",                         // 来
    "\xD2\xAA",                         // 要
    "\xC8\xCF\xCA\xB6",                 // 认识
    "\xD0\xBB\xD0\xBB"                  // 谢谢
};

const char gesture_name_utf8[AI_GESTURE_OUT_1_SIZE][16] =
{
    "\xE4\xB8\x8D",                                     // 不
    "\xE4\xBD\xA0",                                     // 你
    "\xE5\x8F\xAB",                                     // 叫
    "\xE5\x93\xAA\xE5\x84\xBF",                     // 哪儿
    "\xE5\x9C\xA8",                                     // 在
    "\xE5\xA4\xA9",                                     // 天
    "\xE5\xA5\xBD",                                     // 好
    "\xE5\xA6\x88\xE5\xA6\x88",                     // 妈妈
    "\xE5\xBE\x88",                                     // 很
    "\xE6\x88\x91",                                     // 我
    "\xE6\x89\x93\xE7\x94\xB5\xE8\xAF\x9D",     // 打电话
    "\xE6\x97\xA9\xE4\xB8\x8A",                     // 早上
    "\xE6\x98\xAF",                                     // 是
    "\xE6\x9C\x89",                                     // 有
    "\xE6\x9D\xA5",                                     // 来
    "\xE8\xA6\x81",                                     // 要
    "\xE8\xAE\xA4\xE8\xAF\x86",                     // 认识
    "\xE8\xB0\xA2\xE8\xB0\xA2"                      // 谢谢
};


static uint8_t gesture_class_from_output(float *out, float *best_score)
{
    uint8_t best_idx = 0;
    float best = out[0];
    float exp_sum = 0.0f;

    for (uint8_t i = 1; i < AI_GESTURE_OUT_1_SIZE; i++)
    {
        if (out[i] > best)
        {
            best = out[i];
            best_idx = i;
        }
    }

    for (uint8_t i = 0; i < AI_GESTURE_OUT_1_SIZE; i++)
    {
        exp_sum += expf(out[i] - best);
    }

    if (exp_sum > 0.0f)
    {
        *best_score = 1.0f / exp_sum;
    }
    else
    {
        *best_score = 0.0f;
    }

    return best_idx;
}

void Gesture_AI_ResetDecision(void)
{
    gesture_candidate = 0xFFU;
    gesture_candidate_count = 0U;
    gesture_last_confirmed = 0xFFU;
    gesture_result = 0xFFU;
}

void Gesture_MP2Result_Process(uint8_t result_index,
                               uint8_t confidence_percent)
{
    if ((result_index >= AI_GESTURE_OUT_1_SIZE) ||
        (confidence_percent > 100U) ||
        (gesture_enabled == 0U))
    {
        return;
    }

    best_idx = result_index;
    gesture_confidence_percent = confidence_percent;
    gesture_result = best_idx;

    uart_printf(&huart1, "%s\r\n", gesture_name_gbk[best_idx]);
    uart_printf(&huart2, "t11.txt=\"%s\"\xff\xff\xff",
                gesture_name_utf8[best_idx]);
    uart_printf(&huart2, "n0.val=%d\xff\xff\xff",
                gesture_confidence_percent);
}


int acquire_and_process_data(ai_i8* data[])
{
  /* fill the inputs of the c-model
  for (int idx=0; idx < AI_GESTURE_IN_NUM; idx++ )
  {
      data[idx] = ....
  }

  */
  return 0;
}

int post_process(ai_i8* data[])
{
  /* process the predictions
  for (int idx=0; idx < AI_GESTURE_OUT_NUM; idx++ )
  {
      data[idx] = ....
  }

  */
  return 0;
}
/* USER CODE END 2 */

/* Entry points --------------------------------------------------------------*/

void MX_X_CUBE_AI_Init(void)
{
    /* USER CODE BEGIN 5 */
//  printf("\r\nTEMPLATE - initialization\r\n");

  ai_boostrap(data_activations0);
    /* USER CODE END 5 */
}

void MX_X_CUBE_AI_Process(void)
{
    /* USER CODE BEGIN 6 */
		float *in = (float *)ai_input[0].data;
    float *out = (float *)ai_output[0].data;

    for (uint16_t t = 0; t < AI_GESTURE_IN_1_CHANNEL; t++)
    {
        uint16_t src = adc_index + t;

        if (src >= AI_GESTURE_IN_1_CHANNEL)
        {
            src -= AI_GESTURE_IN_1_CHANNEL;
        }

        for (uint8_t c = 0; c < AI_GESTURE_IN_1_HEIGHT; c++)
        {
            in[(t * AI_GESTURE_IN_1_HEIGHT) + c] = adc_train_buf[src][c];
        }
    }

    if (ai_run() == 0)
    {
        
        best_idx = gesture_class_from_output(out, &best_score);
        gesture_confidence_percent = (uint8_t)((best_score * 100.0f) + 0.5f);

        if (gesture_confidence_percent < GESTURE_RELEASE_THRESHOLD)
        {
            gesture_candidate = 0xFFU;
            gesture_candidate_count = 0U;
            gesture_last_confirmed = 0xFFU;
        }
        else if (gesture_confidence_percent < GESTURE_CONFIDENCE_THRESHOLD)
        {
            gesture_candidate = 0xFFU;
            gesture_candidate_count = 0U;
        }
        else if (best_idx != gesture_last_confirmed)
        {
            if (best_idx == gesture_candidate)
            {
                if (gesture_candidate_count < GESTURE_CONFIRM_COUNT)
                {
                    gesture_candidate_count++;
                }
            }
            else
            {
                gesture_candidate = best_idx;
                gesture_candidate_count = 1U;
            }

            if ((gesture_candidate_count >= GESTURE_CONFIRM_COUNT) &&
                (gesture_enabled != 0U))
            {
                gesture_result = best_idx;
                gesture_last_confirmed = best_idx;
                gesture_candidate = 0xFFU;
                gesture_candidate_count = 0U;
                uart_printf(&huart1, "%s\r\n", gesture_name_gbk[best_idx]);
                uart_printf(&huart2, "t11.txt=\"%s\"\xff\xff\xff",
                            gesture_name_utf8[best_idx]);
								uart_printf(&huart2, "n0.val=%d\xff\xff\xff",
                            gesture_confidence_percent);
            }
        }
        else
        {
            gesture_candidate = 0xFFU;
            gesture_candidate_count = 0U;
        }
//        uart_printf(&huart3, "gesture=%u,score_x1000=%ld\r\n",
//                    (unsigned int)best_idx,
//                    (long)score_x1000);
    }
    /* USER CODE END 6 */
}
/* Multiple network support --------------------------------------------------*/

#include <string.h>
#include "ai_datatypes_defines.h"

static const ai_network_entry_t networks[AI_MNETWORK_NUMBER] = {
    {
        .name = (const char *)AI_GESTURE_MODEL_NAME,
        .config = AI_GESTURE_DATA_CONFIG,
        .ai_get_report = ai_gesture_get_report,
        .ai_create = ai_gesture_create,
        .ai_destroy = ai_gesture_destroy,
        .ai_get_error = ai_gesture_get_error,
        .ai_init = ai_gesture_init,
        .ai_run = ai_gesture_run,
        .ai_forward = ai_gesture_forward,
        .ai_data_params_get = ai_gesture_data_params_get,
        .activations = data_activations0
    },
    {
        .name = (const char *)AI_VOICE_MODEL_NAME,
        .config = AI_VOICE_DATA_CONFIG,
        .ai_get_report = ai_voice_get_report,
        .ai_create = ai_voice_create,
        .ai_destroy = ai_voice_destroy,
        .ai_get_error = ai_voice_get_error,
        .ai_init = ai_voice_init,
        .ai_run = ai_voice_run,
        .ai_forward = ai_voice_forward,
        .ai_data_params_get = ai_voice_data_params_get,
        .activations = data_activations1
    },
};

struct network_instance {
     const ai_network_entry_t *entry;
     ai_handle handle;
     ai_network_params params;
};

/* Number of instance is aligned on the number of network */
AI_STATIC struct network_instance gnetworks[AI_MNETWORK_NUMBER] = {0};

AI_DECLARE_STATIC
ai_bool ai_mnetwork_is_valid(const char* name,
        const ai_network_entry_t *entry)
{
    if (name && (strlen(entry->name) == strlen(name)) &&
            (strncmp(entry->name, name, strlen(entry->name)) == 0))
        return true;
    return false;
}

AI_DECLARE_STATIC
struct network_instance *ai_mnetwork_handle(struct network_instance *inst)
{
    for (int i=0; i<AI_MNETWORK_NUMBER; i++) {
        if ((inst) && (&gnetworks[i] == inst))
            return inst;
        else if ((!inst) && (gnetworks[i].entry == NULL))
            return &gnetworks[i];
    }
    return NULL;
}

AI_DECLARE_STATIC
void ai_mnetwork_release_handle(struct network_instance *inst)
{
    for (int i=0; i<AI_MNETWORK_NUMBER; i++) {
        if ((inst) && (&gnetworks[i] == inst)) {
            gnetworks[i].entry = NULL;
            return;
        }
    }
}

AI_API_ENTRY
const char* ai_mnetwork_find(const char *name, ai_int idx)
{
    const ai_network_entry_t *entry;

    for (int i=0; i<AI_MNETWORK_NUMBER; i++) {
        entry = &networks[i];
        if (ai_mnetwork_is_valid(name, entry))
            return entry->name;
        else {
            if (!idx--)
                return entry->name;
        }
    }
    return NULL;
}

AI_API_ENTRY
ai_error ai_mnetwork_create(const char *name, ai_handle* network,
        const ai_buffer* network_config)
{
    const ai_network_entry_t *entry;
    const ai_network_entry_t *found = NULL;
    ai_error err;
    struct network_instance *inst = ai_mnetwork_handle(NULL);

    if (!inst) {
        err.type = AI_ERROR_ALLOCATION_FAILED;
        err.code = AI_ERROR_CODE_NETWORK;
        return err;
    }

    for (int i=0; i<AI_MNETWORK_NUMBER; i++) {
        entry = &networks[i];
        if (ai_mnetwork_is_valid(name, entry)) {
            found = entry;
            break;
        }
    }

    if (!found) {
        err.type = AI_ERROR_INVALID_PARAM;
        err.code = AI_ERROR_CODE_NETWORK;
        return err;
    }

    if (network_config == NULL)
        err = found->ai_create(network, found->config);
    else
        err = found->ai_create(network, network_config);
    if ((err.code == AI_ERROR_CODE_NONE) && (err.type == AI_ERROR_NONE)) {
        inst->entry = found;
        inst->handle = *network;
        *network = (ai_handle*)inst;
    }

    return err;
}

AI_API_ENTRY
ai_handle ai_mnetwork_destroy(ai_handle network)
{
    struct network_instance *inn;
    inn =  ai_mnetwork_handle((struct network_instance *)network);
    if (inn) {
        ai_handle hdl = inn->entry->ai_destroy(inn->handle);
        if (hdl != inn->handle) {
            ai_mnetwork_release_handle(inn);
            network = AI_HANDLE_NULL;
        }
    }
    return network;
}

AI_API_ENTRY
ai_bool ai_mnetwork_get_report(ai_handle network, ai_network_report* report)
{
    struct network_instance *inn;
    inn =  ai_mnetwork_handle((struct network_instance *)network);
    if (inn)
        return inn->entry->ai_get_report(inn->handle, report);
    else
        return false;
}

AI_API_ENTRY
ai_error ai_mnetwork_get_error(ai_handle network)
{
    struct network_instance *inn;
    ai_error err;
    err.type = AI_ERROR_INVALID_PARAM;
    err.code = AI_ERROR_CODE_NETWORK;

    inn =  ai_mnetwork_handle((struct network_instance *)network);
    if (inn)
        return inn->entry->ai_get_error(inn->handle);
    else
        return err;
}

AI_API_ENTRY
ai_bool ai_mnetwork_init(ai_handle network)
{
    struct network_instance *inn;
    ai_network_params par;

    inn =  ai_mnetwork_handle((struct network_instance *)network);
    if (inn) {
        inn->entry->ai_data_params_get(&par);
        for (int idx=0; idx < par.map_activations.size; idx++)
          AI_BUFFER_ARRAY_ITEM_SET_ADDRESS(&par.map_activations, idx, inn->entry->activations[idx]);
        return inn->entry->ai_init(inn->handle, &par);
    }
    else
        return false;
}

AI_API_ENTRY
ai_i32 ai_mnetwork_run(ai_handle network, const ai_buffer* input,
        ai_buffer* output)
{
    struct network_instance* inn;
    inn =  ai_mnetwork_handle((struct network_instance *)network);
    if (inn)
        return inn->entry->ai_run(inn->handle, input, output);
    else
        return 0;
}

AI_API_ENTRY
ai_i32 ai_mnetwork_forward(ai_handle network, const ai_buffer* input)
{
    struct network_instance *inn;
    inn =  ai_mnetwork_handle((struct network_instance *)network);
    if (inn)
        return inn->entry->ai_forward(inn->handle, input);
    else
        return 0;
}

AI_API_ENTRY
 int ai_mnetwork_get_private_handle(ai_handle network,
         ai_handle *phandle,
         ai_network_params *pparams)
 {
     struct network_instance* inn;
     inn =  ai_mnetwork_handle((struct network_instance *)network);
     if (inn && phandle && pparams) {
         *phandle = inn->handle;
         *pparams = inn->params;
         return 0;
     }
     else
         return -1;
 }

#ifdef __cplusplus
}
#endif

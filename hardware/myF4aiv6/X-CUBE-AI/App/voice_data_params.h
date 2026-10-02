/**
  ******************************************************************************
  * @file    voice_data_params.h
  * @author  AST Embedded Analytics Research Platform
  * @date    2026-07-23T20:52:55+0800
  * @brief   AI Tool Automatic Code Generator for Embedded NN computing
  ******************************************************************************
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  ******************************************************************************
  */

#ifndef VOICE_DATA_PARAMS_H
#define VOICE_DATA_PARAMS_H

#include "ai_platform.h"

/*
#define AI_VOICE_DATA_WEIGHTS_PARAMS \
  (AI_HANDLE_PTR(&ai_voice_data_weights_params[1]))
*/

#define AI_VOICE_DATA_CONFIG               (NULL)


#define AI_VOICE_DATA_ACTIVATIONS_SIZES \
  { 9108, }
#define AI_VOICE_DATA_ACTIVATIONS_SIZE     (9108)
#define AI_VOICE_DATA_ACTIVATIONS_COUNT    (1)
#define AI_VOICE_DATA_ACTIVATION_1_SIZE    (9108)



#define AI_VOICE_DATA_WEIGHTS_SIZES \
  { 8480, }
#define AI_VOICE_DATA_WEIGHTS_SIZE         (8480)
#define AI_VOICE_DATA_WEIGHTS_COUNT        (1)
#define AI_VOICE_DATA_WEIGHT_1_SIZE        (8480)



#define AI_VOICE_DATA_ACTIVATIONS_TABLE_GET() \
  (&g_voice_activations_table[1])

extern ai_handle g_voice_activations_table[1 + 2];



#define AI_VOICE_DATA_WEIGHTS_TABLE_GET() \
  (&g_voice_weights_table[1])

extern ai_handle g_voice_weights_table[1 + 2];


#endif    /* VOICE_DATA_PARAMS_H */

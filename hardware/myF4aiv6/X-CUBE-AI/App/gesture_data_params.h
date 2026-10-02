/**
  ******************************************************************************
  * @file    gesture_data_params.h
  * @author  AST Embedded Analytics Research Platform
  * @date    2026-07-23T20:52:21+0800
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

#ifndef GESTURE_DATA_PARAMS_H
#define GESTURE_DATA_PARAMS_H

#include "ai_platform.h"

/*
#define AI_GESTURE_DATA_WEIGHTS_PARAMS \
  (AI_HANDLE_PTR(&ai_gesture_data_weights_params[1]))
*/

#define AI_GESTURE_DATA_CONFIG               (NULL)


#define AI_GESTURE_DATA_ACTIVATIONS_SIZES \
  { 53504, }
#define AI_GESTURE_DATA_ACTIVATIONS_SIZE     (53504)
#define AI_GESTURE_DATA_ACTIVATIONS_COUNT    (1)
#define AI_GESTURE_DATA_ACTIVATION_1_SIZE    (53504)



#define AI_GESTURE_DATA_WEIGHTS_SIZES \
  { 261536, }
#define AI_GESTURE_DATA_WEIGHTS_SIZE         (261536)
#define AI_GESTURE_DATA_WEIGHTS_COUNT        (1)
#define AI_GESTURE_DATA_WEIGHT_1_SIZE        (261536)



#define AI_GESTURE_DATA_ACTIVATIONS_TABLE_GET() \
  (&g_gesture_activations_table[1])

extern ai_handle g_gesture_activations_table[1 + 2];



#define AI_GESTURE_DATA_WEIGHTS_TABLE_GET() \
  (&g_gesture_weights_table[1])

extern ai_handle g_gesture_weights_table[1 + 2];


#endif    /* GESTURE_DATA_PARAMS_H */

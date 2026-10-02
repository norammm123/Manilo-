/**
  ******************************************************************************
  * @file    gesture.c
  * @author  AST Embedded Analytics Research Platform
  * @date    2026-07-23T20:52:21+0800
  * @brief   AI Tool Automatic Code Generator for Embedded NN computing
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  ******************************************************************************
  */


#include "gesture.h"
#include "gesture_data.h"

#include "ai_platform.h"
#include "ai_platform_interface.h"
#include "ai_math_helpers.h"

#include "core_common.h"
#include "core_convert.h"

#include "layers.h"



#undef AI_NET_OBJ_INSTANCE
#define AI_NET_OBJ_INSTANCE g_gesture
 
#undef AI_GESTURE_MODEL_SIGNATURE
#define AI_GESTURE_MODEL_SIGNATURE     "0xa3d1dc373f21bd33c30b005af28c8995"

#ifndef AI_TOOLS_REVISION_ID
#define AI_TOOLS_REVISION_ID     ""
#endif

#undef AI_TOOLS_DATE_TIME
#define AI_TOOLS_DATE_TIME   "2026-07-23T20:52:21+0800"

#undef AI_TOOLS_COMPILE_TIME
#define AI_TOOLS_COMPILE_TIME    __DATE__ " " __TIME__

#undef AI_GESTURE_N_BATCHES
#define AI_GESTURE_N_BATCHES         (1)

static ai_ptr g_gesture_activations_map[1] = AI_C_ARRAY_INIT;
static ai_ptr g_gesture_weights_map[1] = AI_C_ARRAY_INIT;



/**  Array declarations section  **********************************************/
/* Array#0 */
AI_ARRAY_OBJ_DECLARE(
  gesture_output_array, AI_ARRAY_FORMAT_FLOAT|AI_FMT_FLAG_IS_IO,
  NULL, NULL, 2200, AI_STATIC)

/* Array#1 */
AI_ARRAY_OBJ_DECLARE(
  gesture_Transpose_output_array, AI_ARRAY_FORMAT_FLOAT,
  NULL, NULL, 2200, AI_STATIC)

/* Array#2 */
AI_ARRAY_OBJ_DECLARE(
  _Sub_output_0_output_array, AI_ARRAY_FORMAT_FLOAT,
  NULL, NULL, 2200, AI_STATIC)

/* Array#3 */
AI_ARRAY_OBJ_DECLARE(
  _Div_output_0_output_array, AI_ARRAY_FORMAT_FLOAT,
  NULL, NULL, 2200, AI_STATIC)

/* Array#4 */
AI_ARRAY_OBJ_DECLARE(
  _Transpose_output_0_output_array, AI_ARRAY_FORMAT_FLOAT,
  NULL, NULL, 2200, AI_STATIC)

/* Array#5 */
AI_ARRAY_OBJ_DECLARE(
  _model_stem_stem_0_Conv_output_0_output_array, AI_ARRAY_FORMAT_FLOAT,
  NULL, NULL, 6400, AI_STATIC)

/* Array#6 */
AI_ARRAY_OBJ_DECLARE(
  _model_stem_stem_2_Relu_output_0_output_array, AI_ARRAY_FORMAT_FLOAT,
  NULL, NULL, 6400, AI_STATIC)

/* Array#7 */
AI_ARRAY_OBJ_DECLARE(
  _model_block1_conv1_Conv_output_0_output_array, AI_ARRAY_FORMAT_FLOAT,
  NULL, NULL, 6400, AI_STATIC)

/* Array#8 */
AI_ARRAY_OBJ_DECLARE(
  _model_block1_Relu_output_0_output_array, AI_ARRAY_FORMAT_FLOAT,
  NULL, NULL, 6400, AI_STATIC)

/* Array#9 */
AI_ARRAY_OBJ_DECLARE(
  _model_block1_conv2_Conv_output_0_output_array, AI_ARRAY_FORMAT_FLOAT,
  NULL, NULL, 6400, AI_STATIC)

/* Array#10 */
AI_ARRAY_OBJ_DECLARE(
  _model_block1_Add_output_0_output_array, AI_ARRAY_FORMAT_FLOAT,
  NULL, NULL, 6400, AI_STATIC)

/* Array#11 */
AI_ARRAY_OBJ_DECLARE(
  _model_block1_Relu_1_output_0_output_array, AI_ARRAY_FORMAT_FLOAT,
  NULL, NULL, 6400, AI_STATIC)

/* Array#12 */
AI_ARRAY_OBJ_DECLARE(
  _model_down1_down1_0_Conv_output_0_output_array, AI_ARRAY_FORMAT_FLOAT,
  NULL, NULL, 6400, AI_STATIC)

/* Array#13 */
AI_ARRAY_OBJ_DECLARE(
  _model_down1_down1_2_Relu_output_0_output_array, AI_ARRAY_FORMAT_FLOAT,
  NULL, NULL, 6400, AI_STATIC)

/* Array#14 */
AI_ARRAY_OBJ_DECLARE(
  _model_block2_conv1_Conv_output_0_output_array, AI_ARRAY_FORMAT_FLOAT,
  NULL, NULL, 6400, AI_STATIC)

/* Array#15 */
AI_ARRAY_OBJ_DECLARE(
  _model_block2_Relu_output_0_output_array, AI_ARRAY_FORMAT_FLOAT,
  NULL, NULL, 6400, AI_STATIC)

/* Array#16 */
AI_ARRAY_OBJ_DECLARE(
  _model_block2_conv2_Conv_output_0_output_array, AI_ARRAY_FORMAT_FLOAT,
  NULL, NULL, 6400, AI_STATIC)

/* Array#17 */
AI_ARRAY_OBJ_DECLARE(
  _model_block2_Add_output_0_output_array, AI_ARRAY_FORMAT_FLOAT,
  NULL, NULL, 6400, AI_STATIC)

/* Array#18 */
AI_ARRAY_OBJ_DECLARE(
  _model_block2_Relu_1_output_0_output_array, AI_ARRAY_FORMAT_FLOAT,
  NULL, NULL, 6400, AI_STATIC)

/* Array#19 */
AI_ARRAY_OBJ_DECLARE(
  _model_pool_GlobalAveragePool_output_0_output_array, AI_ARRAY_FORMAT_FLOAT,
  NULL, NULL, 64, AI_STATIC)

/* Array#20 */
AI_ARRAY_OBJ_DECLARE(
  logits_output_array, AI_ARRAY_FORMAT_FLOAT|AI_FMT_FLAG_IS_IO,
  NULL, NULL, 18, AI_STATIC)

/* Array#21 */
AI_ARRAY_OBJ_DECLARE(
  std_array, AI_ARRAY_FORMAT_FLOAT,
  NULL, NULL, 11, AI_STATIC)

/* Array#22 */
AI_ARRAY_OBJ_DECLARE(
  mean_array, AI_ARRAY_FORMAT_FLOAT,
  NULL, NULL, 11, AI_STATIC)

/* Array#23 */
AI_ARRAY_OBJ_DECLARE(
  _model_stem_stem_0_Conv_output_0_weights_array, AI_ARRAY_FORMAT_FLOAT,
  NULL, NULL, 2464, AI_STATIC)

/* Array#24 */
AI_ARRAY_OBJ_DECLARE(
  _model_stem_stem_0_Conv_output_0_bias_array, AI_ARRAY_FORMAT_FLOAT,
  NULL, NULL, 32, AI_STATIC)

/* Array#25 */
AI_ARRAY_OBJ_DECLARE(
  _model_block1_conv1_Conv_output_0_weights_array, AI_ARRAY_FORMAT_FLOAT,
  NULL, NULL, 5120, AI_STATIC)

/* Array#26 */
AI_ARRAY_OBJ_DECLARE(
  _model_block1_conv1_Conv_output_0_bias_array, AI_ARRAY_FORMAT_FLOAT,
  NULL, NULL, 32, AI_STATIC)

/* Array#27 */
AI_ARRAY_OBJ_DECLARE(
  _model_block1_conv2_Conv_output_0_weights_array, AI_ARRAY_FORMAT_FLOAT,
  NULL, NULL, 5120, AI_STATIC)

/* Array#28 */
AI_ARRAY_OBJ_DECLARE(
  _model_block1_conv2_Conv_output_0_bias_array, AI_ARRAY_FORMAT_FLOAT,
  NULL, NULL, 32, AI_STATIC)

/* Array#29 */
AI_ARRAY_OBJ_DECLARE(
  _model_down1_down1_0_Conv_output_0_weights_array, AI_ARRAY_FORMAT_FLOAT,
  NULL, NULL, 10240, AI_STATIC)

/* Array#30 */
AI_ARRAY_OBJ_DECLARE(
  _model_down1_down1_0_Conv_output_0_bias_array, AI_ARRAY_FORMAT_FLOAT,
  NULL, NULL, 64, AI_STATIC)

/* Array#31 */
AI_ARRAY_OBJ_DECLARE(
  _model_block2_conv1_Conv_output_0_weights_array, AI_ARRAY_FORMAT_FLOAT,
  NULL, NULL, 20480, AI_STATIC)

/* Array#32 */
AI_ARRAY_OBJ_DECLARE(
  _model_block2_conv1_Conv_output_0_bias_array, AI_ARRAY_FORMAT_FLOAT,
  NULL, NULL, 64, AI_STATIC)

/* Array#33 */
AI_ARRAY_OBJ_DECLARE(
  _model_block2_conv2_Conv_output_0_weights_array, AI_ARRAY_FORMAT_FLOAT,
  NULL, NULL, 20480, AI_STATIC)

/* Array#34 */
AI_ARRAY_OBJ_DECLARE(
  _model_block2_conv2_Conv_output_0_bias_array, AI_ARRAY_FORMAT_FLOAT,
  NULL, NULL, 64, AI_STATIC)

/* Array#35 */
AI_ARRAY_OBJ_DECLARE(
  logits_weights_array, AI_ARRAY_FORMAT_FLOAT,
  NULL, NULL, 1152, AI_STATIC)

/* Array#36 */
AI_ARRAY_OBJ_DECLARE(
  logits_bias_array, AI_ARRAY_FORMAT_FLOAT,
  NULL, NULL, 18, AI_STATIC)

/* Array#37 */
AI_ARRAY_OBJ_DECLARE(
  _model_stem_stem_0_Conv_output_0_scratch0_array, AI_ARRAY_FORMAT_FLOAT,
  NULL, NULL, 77, AI_STATIC)

/* Array#38 */
AI_ARRAY_OBJ_DECLARE(
  _model_block1_conv1_Conv_output_0_scratch0_array, AI_ARRAY_FORMAT_FLOAT,
  NULL, NULL, 160, AI_STATIC)

/* Array#39 */
AI_ARRAY_OBJ_DECLARE(
  _model_block1_conv2_Conv_output_0_scratch0_array, AI_ARRAY_FORMAT_FLOAT,
  NULL, NULL, 160, AI_STATIC)

/* Array#40 */
AI_ARRAY_OBJ_DECLARE(
  _model_down1_down1_0_Conv_output_0_scratch0_array, AI_ARRAY_FORMAT_FLOAT,
  NULL, NULL, 160, AI_STATIC)

/* Array#41 */
AI_ARRAY_OBJ_DECLARE(
  _model_block2_conv1_Conv_output_0_scratch0_array, AI_ARRAY_FORMAT_FLOAT,
  NULL, NULL, 320, AI_STATIC)

/* Array#42 */
AI_ARRAY_OBJ_DECLARE(
  _model_block2_conv2_Conv_output_0_scratch0_array, AI_ARRAY_FORMAT_FLOAT,
  NULL, NULL, 320, AI_STATIC)

/**  Tensor declarations section  *********************************************/
/* Tensor #0 */
AI_TENSOR_OBJ_DECLARE(
  _Div_output_0_output, AI_STATIC,
  0, 0x0,
  AI_SHAPE_INIT(4, 1, 200, 1, 11), AI_STRIDE_INIT(4, 4, 4, 800, 800),
  1, &_Div_output_0_output_array, NULL)

/* Tensor #1 */
AI_TENSOR_OBJ_DECLARE(
  _Sub_output_0_output, AI_STATIC,
  1, 0x0,
  AI_SHAPE_INIT(4, 1, 200, 1, 11), AI_STRIDE_INIT(4, 4, 4, 800, 800),
  1, &_Sub_output_0_output_array, NULL)

/* Tensor #2 */
AI_TENSOR_OBJ_DECLARE(
  _Transpose_output_0_output, AI_STATIC,
  2, 0x0,
  AI_SHAPE_INIT(4, 1, 11, 1, 200), AI_STRIDE_INIT(4, 4, 4, 44, 44),
  1, &_Transpose_output_0_output_array, NULL)

/* Tensor #3 */
AI_TENSOR_OBJ_DECLARE(
  _model_block1_Add_output_0_output, AI_STATIC,
  3, 0x0,
  AI_SHAPE_INIT(4, 1, 32, 1, 200), AI_STRIDE_INIT(4, 4, 4, 128, 128),
  1, &_model_block1_Add_output_0_output_array, NULL)

/* Tensor #4 */
AI_TENSOR_OBJ_DECLARE(
  _model_block1_Relu_1_output_0_output, AI_STATIC,
  4, 0x0,
  AI_SHAPE_INIT(4, 1, 32, 1, 200), AI_STRIDE_INIT(4, 4, 4, 128, 128),
  1, &_model_block1_Relu_1_output_0_output_array, NULL)

/* Tensor #5 */
AI_TENSOR_OBJ_DECLARE(
  _model_block1_Relu_output_0_output, AI_STATIC,
  5, 0x0,
  AI_SHAPE_INIT(4, 1, 32, 1, 200), AI_STRIDE_INIT(4, 4, 4, 128, 128),
  1, &_model_block1_Relu_output_0_output_array, NULL)

/* Tensor #6 */
AI_TENSOR_OBJ_DECLARE(
  _model_block1_conv1_Conv_output_0_bias, AI_STATIC,
  6, 0x0,
  AI_SHAPE_INIT(4, 1, 32, 1, 1), AI_STRIDE_INIT(4, 4, 4, 128, 128),
  1, &_model_block1_conv1_Conv_output_0_bias_array, NULL)

/* Tensor #7 */
AI_TENSOR_OBJ_DECLARE(
  _model_block1_conv1_Conv_output_0_output, AI_STATIC,
  7, 0x0,
  AI_SHAPE_INIT(4, 1, 32, 1, 200), AI_STRIDE_INIT(4, 4, 4, 128, 128),
  1, &_model_block1_conv1_Conv_output_0_output_array, NULL)

/* Tensor #8 */
AI_TENSOR_OBJ_DECLARE(
  _model_block1_conv1_Conv_output_0_scratch0, AI_STATIC,
  8, 0x0,
  AI_SHAPE_INIT(4, 1, 32, 1, 5), AI_STRIDE_INIT(4, 4, 4, 128, 128),
  1, &_model_block1_conv1_Conv_output_0_scratch0_array, NULL)

/* Tensor #9 */
AI_TENSOR_OBJ_DECLARE(
  _model_block1_conv1_Conv_output_0_weights, AI_STATIC,
  9, 0x0,
  AI_SHAPE_INIT(4, 32, 1, 5, 32), AI_STRIDE_INIT(4, 4, 128, 4096, 4096),
  1, &_model_block1_conv1_Conv_output_0_weights_array, NULL)

/* Tensor #10 */
AI_TENSOR_OBJ_DECLARE(
  _model_block1_conv2_Conv_output_0_bias, AI_STATIC,
  10, 0x0,
  AI_SHAPE_INIT(4, 1, 32, 1, 1), AI_STRIDE_INIT(4, 4, 4, 128, 128),
  1, &_model_block1_conv2_Conv_output_0_bias_array, NULL)

/* Tensor #11 */
AI_TENSOR_OBJ_DECLARE(
  _model_block1_conv2_Conv_output_0_output, AI_STATIC,
  11, 0x0,
  AI_SHAPE_INIT(4, 1, 32, 1, 200), AI_STRIDE_INIT(4, 4, 4, 128, 128),
  1, &_model_block1_conv2_Conv_output_0_output_array, NULL)

/* Tensor #12 */
AI_TENSOR_OBJ_DECLARE(
  _model_block1_conv2_Conv_output_0_scratch0, AI_STATIC,
  12, 0x0,
  AI_SHAPE_INIT(4, 1, 32, 1, 5), AI_STRIDE_INIT(4, 4, 4, 128, 128),
  1, &_model_block1_conv2_Conv_output_0_scratch0_array, NULL)

/* Tensor #13 */
AI_TENSOR_OBJ_DECLARE(
  _model_block1_conv2_Conv_output_0_weights, AI_STATIC,
  13, 0x0,
  AI_SHAPE_INIT(4, 32, 1, 5, 32), AI_STRIDE_INIT(4, 4, 128, 4096, 4096),
  1, &_model_block1_conv2_Conv_output_0_weights_array, NULL)

/* Tensor #14 */
AI_TENSOR_OBJ_DECLARE(
  _model_block2_Add_output_0_output, AI_STATIC,
  14, 0x0,
  AI_SHAPE_INIT(4, 1, 64, 1, 100), AI_STRIDE_INIT(4, 4, 4, 256, 256),
  1, &_model_block2_Add_output_0_output_array, NULL)

/* Tensor #15 */
AI_TENSOR_OBJ_DECLARE(
  _model_block2_Relu_1_output_0_output, AI_STATIC,
  15, 0x0,
  AI_SHAPE_INIT(4, 1, 64, 1, 100), AI_STRIDE_INIT(4, 4, 4, 256, 256),
  1, &_model_block2_Relu_1_output_0_output_array, NULL)

/* Tensor #16 */
AI_TENSOR_OBJ_DECLARE(
  _model_block2_Relu_output_0_output, AI_STATIC,
  16, 0x0,
  AI_SHAPE_INIT(4, 1, 64, 1, 100), AI_STRIDE_INIT(4, 4, 4, 256, 256),
  1, &_model_block2_Relu_output_0_output_array, NULL)

/* Tensor #17 */
AI_TENSOR_OBJ_DECLARE(
  _model_block2_conv1_Conv_output_0_bias, AI_STATIC,
  17, 0x0,
  AI_SHAPE_INIT(4, 1, 64, 1, 1), AI_STRIDE_INIT(4, 4, 4, 256, 256),
  1, &_model_block2_conv1_Conv_output_0_bias_array, NULL)

/* Tensor #18 */
AI_TENSOR_OBJ_DECLARE(
  _model_block2_conv1_Conv_output_0_output, AI_STATIC,
  18, 0x0,
  AI_SHAPE_INIT(4, 1, 64, 1, 100), AI_STRIDE_INIT(4, 4, 4, 256, 256),
  1, &_model_block2_conv1_Conv_output_0_output_array, NULL)

/* Tensor #19 */
AI_TENSOR_OBJ_DECLARE(
  _model_block2_conv1_Conv_output_0_scratch0, AI_STATIC,
  19, 0x0,
  AI_SHAPE_INIT(4, 1, 64, 1, 5), AI_STRIDE_INIT(4, 4, 4, 256, 256),
  1, &_model_block2_conv1_Conv_output_0_scratch0_array, NULL)

/* Tensor #20 */
AI_TENSOR_OBJ_DECLARE(
  _model_block2_conv1_Conv_output_0_weights, AI_STATIC,
  20, 0x0,
  AI_SHAPE_INIT(4, 64, 1, 5, 64), AI_STRIDE_INIT(4, 4, 256, 16384, 16384),
  1, &_model_block2_conv1_Conv_output_0_weights_array, NULL)

/* Tensor #21 */
AI_TENSOR_OBJ_DECLARE(
  _model_block2_conv2_Conv_output_0_bias, AI_STATIC,
  21, 0x0,
  AI_SHAPE_INIT(4, 1, 64, 1, 1), AI_STRIDE_INIT(4, 4, 4, 256, 256),
  1, &_model_block2_conv2_Conv_output_0_bias_array, NULL)

/* Tensor #22 */
AI_TENSOR_OBJ_DECLARE(
  _model_block2_conv2_Conv_output_0_output, AI_STATIC,
  22, 0x0,
  AI_SHAPE_INIT(4, 1, 64, 1, 100), AI_STRIDE_INIT(4, 4, 4, 256, 256),
  1, &_model_block2_conv2_Conv_output_0_output_array, NULL)

/* Tensor #23 */
AI_TENSOR_OBJ_DECLARE(
  _model_block2_conv2_Conv_output_0_scratch0, AI_STATIC,
  23, 0x0,
  AI_SHAPE_INIT(4, 1, 64, 1, 5), AI_STRIDE_INIT(4, 4, 4, 256, 256),
  1, &_model_block2_conv2_Conv_output_0_scratch0_array, NULL)

/* Tensor #24 */
AI_TENSOR_OBJ_DECLARE(
  _model_block2_conv2_Conv_output_0_weights, AI_STATIC,
  24, 0x0,
  AI_SHAPE_INIT(4, 64, 1, 5, 64), AI_STRIDE_INIT(4, 4, 256, 16384, 16384),
  1, &_model_block2_conv2_Conv_output_0_weights_array, NULL)

/* Tensor #25 */
AI_TENSOR_OBJ_DECLARE(
  _model_down1_down1_0_Conv_output_0_bias, AI_STATIC,
  25, 0x0,
  AI_SHAPE_INIT(4, 1, 64, 1, 1), AI_STRIDE_INIT(4, 4, 4, 256, 256),
  1, &_model_down1_down1_0_Conv_output_0_bias_array, NULL)

/* Tensor #26 */
AI_TENSOR_OBJ_DECLARE(
  _model_down1_down1_0_Conv_output_0_output, AI_STATIC,
  26, 0x0,
  AI_SHAPE_INIT(4, 1, 64, 1, 100), AI_STRIDE_INIT(4, 4, 4, 256, 256),
  1, &_model_down1_down1_0_Conv_output_0_output_array, NULL)

/* Tensor #27 */
AI_TENSOR_OBJ_DECLARE(
  _model_down1_down1_0_Conv_output_0_scratch0, AI_STATIC,
  27, 0x0,
  AI_SHAPE_INIT(4, 1, 32, 1, 5), AI_STRIDE_INIT(4, 4, 4, 128, 128),
  1, &_model_down1_down1_0_Conv_output_0_scratch0_array, NULL)

/* Tensor #28 */
AI_TENSOR_OBJ_DECLARE(
  _model_down1_down1_0_Conv_output_0_weights, AI_STATIC,
  28, 0x0,
  AI_SHAPE_INIT(4, 32, 1, 5, 64), AI_STRIDE_INIT(4, 4, 128, 8192, 8192),
  1, &_model_down1_down1_0_Conv_output_0_weights_array, NULL)

/* Tensor #29 */
AI_TENSOR_OBJ_DECLARE(
  _model_down1_down1_2_Relu_output_0_output, AI_STATIC,
  29, 0x0,
  AI_SHAPE_INIT(4, 1, 64, 1, 100), AI_STRIDE_INIT(4, 4, 4, 256, 256),
  1, &_model_down1_down1_2_Relu_output_0_output_array, NULL)

/* Tensor #30 */
AI_TENSOR_OBJ_DECLARE(
  _model_pool_GlobalAveragePool_output_0_output, AI_STATIC,
  30, 0x0,
  AI_SHAPE_INIT(4, 1, 64, 1, 1), AI_STRIDE_INIT(4, 4, 4, 256, 256),
  1, &_model_pool_GlobalAveragePool_output_0_output_array, NULL)

/* Tensor #31 */
AI_TENSOR_OBJ_DECLARE(
  _model_stem_stem_0_Conv_output_0_bias, AI_STATIC,
  31, 0x0,
  AI_SHAPE_INIT(4, 1, 32, 1, 1), AI_STRIDE_INIT(4, 4, 4, 128, 128),
  1, &_model_stem_stem_0_Conv_output_0_bias_array, NULL)

/* Tensor #32 */
AI_TENSOR_OBJ_DECLARE(
  _model_stem_stem_0_Conv_output_0_output, AI_STATIC,
  32, 0x0,
  AI_SHAPE_INIT(4, 1, 32, 1, 200), AI_STRIDE_INIT(4, 4, 4, 128, 128),
  1, &_model_stem_stem_0_Conv_output_0_output_array, NULL)

/* Tensor #33 */
AI_TENSOR_OBJ_DECLARE(
  _model_stem_stem_0_Conv_output_0_scratch0, AI_STATIC,
  33, 0x0,
  AI_SHAPE_INIT(4, 1, 11, 1, 7), AI_STRIDE_INIT(4, 4, 4, 44, 44),
  1, &_model_stem_stem_0_Conv_output_0_scratch0_array, NULL)

/* Tensor #34 */
AI_TENSOR_OBJ_DECLARE(
  _model_stem_stem_0_Conv_output_0_weights, AI_STATIC,
  34, 0x0,
  AI_SHAPE_INIT(4, 11, 1, 7, 32), AI_STRIDE_INIT(4, 4, 44, 1408, 1408),
  1, &_model_stem_stem_0_Conv_output_0_weights_array, NULL)

/* Tensor #35 */
AI_TENSOR_OBJ_DECLARE(
  _model_stem_stem_2_Relu_output_0_output, AI_STATIC,
  35, 0x0,
  AI_SHAPE_INIT(4, 1, 32, 1, 200), AI_STRIDE_INIT(4, 4, 4, 128, 128),
  1, &_model_stem_stem_2_Relu_output_0_output_array, NULL)

/* Tensor #36 */
AI_TENSOR_OBJ_DECLARE(
  gesture_Transpose_output, AI_STATIC,
  36, 0x0,
  AI_SHAPE_INIT(4, 1, 200, 1, 11), AI_STRIDE_INIT(4, 4, 4, 800, 800),
  1, &gesture_Transpose_output_array, NULL)

/* Tensor #37 */
AI_TENSOR_OBJ_DECLARE(
  gesture_output, AI_STATIC,
  37, 0x0,
  AI_SHAPE_INIT(4, 1, 11, 1, 200), AI_STRIDE_INIT(4, 4, 4, 44, 44),
  1, &gesture_output_array, NULL)

/* Tensor #38 */
AI_TENSOR_OBJ_DECLARE(
  logits_bias, AI_STATIC,
  38, 0x0,
  AI_SHAPE_INIT(4, 1, 18, 1, 1), AI_STRIDE_INIT(4, 4, 4, 72, 72),
  1, &logits_bias_array, NULL)

/* Tensor #39 */
AI_TENSOR_OBJ_DECLARE(
  logits_output, AI_STATIC,
  39, 0x0,
  AI_SHAPE_INIT(4, 1, 18, 1, 1), AI_STRIDE_INIT(4, 4, 4, 72, 72),
  1, &logits_output_array, NULL)

/* Tensor #40 */
AI_TENSOR_OBJ_DECLARE(
  logits_weights, AI_STATIC,
  40, 0x0,
  AI_SHAPE_INIT(4, 64, 18, 1, 1), AI_STRIDE_INIT(4, 4, 256, 4608, 4608),
  1, &logits_weights_array, NULL)

/* Tensor #41 */
AI_TENSOR_OBJ_DECLARE(
  mean, AI_STATIC,
  41, 0x0,
  AI_SHAPE_INIT(4, 1, 1, 1, 11), AI_STRIDE_INIT(4, 4, 4, 4, 4),
  1, &mean_array, NULL)

/* Tensor #42 */
AI_TENSOR_OBJ_DECLARE(
  std, AI_STATIC,
  42, 0x0,
  AI_SHAPE_INIT(4, 1, 1, 1, 11), AI_STRIDE_INIT(4, 4, 4, 4, 4),
  1, &std_array, NULL)



/**  Layer declarations section  **********************************************/


AI_TENSOR_CHAIN_OBJ_DECLARE(
  logits_chain, AI_STATIC_CONST, 4,
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 1, &_model_pool_GlobalAveragePool_output_0_output),
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 1, &logits_output),
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 2, &logits_weights, &logits_bias),
  AI_TENSOR_LIST_OBJ_EMPTY
)

AI_LAYER_OBJ_DECLARE(
  logits_layer, 20,
  DENSE_TYPE, 0x0, NULL,
  dense, forward_dense,
  &logits_chain,
  NULL, &logits_layer, AI_STATIC, 
)

AI_TENSOR_CHAIN_OBJ_DECLARE(
  _model_pool_GlobalAveragePool_output_0_chain, AI_STATIC_CONST, 4,
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 1, &_model_block2_Relu_1_output_0_output),
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 1, &_model_pool_GlobalAveragePool_output_0_output),
  AI_TENSOR_LIST_OBJ_EMPTY,
  AI_TENSOR_LIST_OBJ_EMPTY
)

AI_LAYER_OBJ_DECLARE(
  _model_pool_GlobalAveragePool_output_0_layer, 18,
  POOL_TYPE, 0x0, NULL,
  pool, forward_ap,
  &_model_pool_GlobalAveragePool_output_0_chain,
  NULL, &logits_layer, AI_STATIC, 
  .pool_size = AI_SHAPE_2D_INIT(1, 100), 
  .pool_stride = AI_SHAPE_2D_INIT(1, 100), 
  .pool_pad = AI_SHAPE_INIT(4, 0, 0, 0, 0), 
)

AI_TENSOR_CHAIN_OBJ_DECLARE(
  _model_block2_Relu_1_output_0_chain, AI_STATIC_CONST, 4,
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 1, &_model_block2_Add_output_0_output),
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 1, &_model_block2_Relu_1_output_0_output),
  AI_TENSOR_LIST_OBJ_EMPTY,
  AI_TENSOR_LIST_OBJ_EMPTY
)

AI_LAYER_OBJ_DECLARE(
  _model_block2_Relu_1_output_0_layer, 17,
  NL_TYPE, 0x0, NULL,
  nl, forward_relu,
  &_model_block2_Relu_1_output_0_chain,
  NULL, &_model_pool_GlobalAveragePool_output_0_layer, AI_STATIC, 
  .nl_params = NULL, 
)

AI_TENSOR_CHAIN_OBJ_DECLARE(
  _model_block2_Add_output_0_chain, AI_STATIC_CONST, 4,
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 2, &_model_block2_conv2_Conv_output_0_output, &_model_down1_down1_2_Relu_output_0_output),
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 1, &_model_block2_Add_output_0_output),
  AI_TENSOR_LIST_OBJ_EMPTY,
  AI_TENSOR_LIST_OBJ_EMPTY
)

AI_LAYER_OBJ_DECLARE(
  _model_block2_Add_output_0_layer, 16,
  ELTWISE_TYPE, 0x0, NULL,
  eltwise, forward_eltwise,
  &_model_block2_Add_output_0_chain,
  NULL, &_model_block2_Relu_1_output_0_layer, AI_STATIC, 
  .operation = ai_sum_f32, 
  .buffer_operation = ai_sum_buffer_f32, 
)

AI_TENSOR_CHAIN_OBJ_DECLARE(
  _model_block2_conv2_Conv_output_0_chain, AI_STATIC_CONST, 4,
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 1, &_model_block2_Relu_output_0_output),
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 1, &_model_block2_conv2_Conv_output_0_output),
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 3, &_model_block2_conv2_Conv_output_0_weights, &_model_block2_conv2_Conv_output_0_bias, NULL),
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 2, &_model_block2_conv2_Conv_output_0_scratch0, NULL)
)

AI_LAYER_OBJ_DECLARE(
  _model_block2_conv2_Conv_output_0_layer, 15,
  CONV2D_TYPE, 0x0, NULL,
  conv2d, forward_conv2d_if32of32wf32,
  &_model_block2_conv2_Conv_output_0_chain,
  NULL, &_model_block2_Add_output_0_layer, AI_STATIC, 
  .groups = 1, 
  .filter_stride = AI_SHAPE_2D_INIT(1, 1), 
  .dilation = AI_SHAPE_2D_INIT(1, 1), 
  .filter_pad = AI_SHAPE_INIT(4, 2, 0, 2, 0), 
  .in_ch_format = AI_LAYER_FORMAT_CHANNEL_LAST_SAME, 
  .out_ch_format = AI_LAYER_FORMAT_CHANNEL_LAST_VALID, 
)

AI_TENSOR_CHAIN_OBJ_DECLARE(
  _model_block2_Relu_output_0_chain, AI_STATIC_CONST, 4,
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 1, &_model_block2_conv1_Conv_output_0_output),
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 1, &_model_block2_Relu_output_0_output),
  AI_TENSOR_LIST_OBJ_EMPTY,
  AI_TENSOR_LIST_OBJ_EMPTY
)

AI_LAYER_OBJ_DECLARE(
  _model_block2_Relu_output_0_layer, 14,
  NL_TYPE, 0x0, NULL,
  nl, forward_relu,
  &_model_block2_Relu_output_0_chain,
  NULL, &_model_block2_conv2_Conv_output_0_layer, AI_STATIC, 
  .nl_params = NULL, 
)

AI_TENSOR_CHAIN_OBJ_DECLARE(
  _model_block2_conv1_Conv_output_0_chain, AI_STATIC_CONST, 4,
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 1, &_model_down1_down1_2_Relu_output_0_output),
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 1, &_model_block2_conv1_Conv_output_0_output),
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 3, &_model_block2_conv1_Conv_output_0_weights, &_model_block2_conv1_Conv_output_0_bias, NULL),
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 2, &_model_block2_conv1_Conv_output_0_scratch0, NULL)
)

AI_LAYER_OBJ_DECLARE(
  _model_block2_conv1_Conv_output_0_layer, 13,
  CONV2D_TYPE, 0x0, NULL,
  conv2d, forward_conv2d_if32of32wf32,
  &_model_block2_conv1_Conv_output_0_chain,
  NULL, &_model_block2_Relu_output_0_layer, AI_STATIC, 
  .groups = 1, 
  .filter_stride = AI_SHAPE_2D_INIT(1, 1), 
  .dilation = AI_SHAPE_2D_INIT(1, 1), 
  .filter_pad = AI_SHAPE_INIT(4, 2, 0, 2, 0), 
  .in_ch_format = AI_LAYER_FORMAT_CHANNEL_LAST_SAME, 
  .out_ch_format = AI_LAYER_FORMAT_CHANNEL_LAST_VALID, 
)

AI_TENSOR_CHAIN_OBJ_DECLARE(
  _model_down1_down1_2_Relu_output_0_chain, AI_STATIC_CONST, 4,
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 1, &_model_down1_down1_0_Conv_output_0_output),
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 1, &_model_down1_down1_2_Relu_output_0_output),
  AI_TENSOR_LIST_OBJ_EMPTY,
  AI_TENSOR_LIST_OBJ_EMPTY
)

AI_LAYER_OBJ_DECLARE(
  _model_down1_down1_2_Relu_output_0_layer, 12,
  NL_TYPE, 0x0, NULL,
  nl, forward_relu,
  &_model_down1_down1_2_Relu_output_0_chain,
  NULL, &_model_block2_conv1_Conv_output_0_layer, AI_STATIC, 
  .nl_params = NULL, 
)

AI_TENSOR_CHAIN_OBJ_DECLARE(
  _model_down1_down1_0_Conv_output_0_chain, AI_STATIC_CONST, 4,
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 1, &_model_block1_Relu_1_output_0_output),
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 1, &_model_down1_down1_0_Conv_output_0_output),
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 3, &_model_down1_down1_0_Conv_output_0_weights, &_model_down1_down1_0_Conv_output_0_bias, NULL),
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 2, &_model_down1_down1_0_Conv_output_0_scratch0, NULL)
)

AI_LAYER_OBJ_DECLARE(
  _model_down1_down1_0_Conv_output_0_layer, 11,
  CONV2D_TYPE, 0x0, NULL,
  conv2d, forward_conv2d_if32of32wf32,
  &_model_down1_down1_0_Conv_output_0_chain,
  NULL, &_model_down1_down1_2_Relu_output_0_layer, AI_STATIC, 
  .groups = 1, 
  .filter_stride = AI_SHAPE_2D_INIT(1, 2), 
  .dilation = AI_SHAPE_2D_INIT(1, 1), 
  .filter_pad = AI_SHAPE_INIT(4, 2, 0, 2, 0), 
  .in_ch_format = AI_LAYER_FORMAT_CHANNEL_LAST_SAME, 
  .out_ch_format = AI_LAYER_FORMAT_CHANNEL_LAST_VALID, 
)

AI_TENSOR_CHAIN_OBJ_DECLARE(
  _model_block1_Relu_1_output_0_chain, AI_STATIC_CONST, 4,
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 1, &_model_block1_Add_output_0_output),
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 1, &_model_block1_Relu_1_output_0_output),
  AI_TENSOR_LIST_OBJ_EMPTY,
  AI_TENSOR_LIST_OBJ_EMPTY
)

AI_LAYER_OBJ_DECLARE(
  _model_block1_Relu_1_output_0_layer, 10,
  NL_TYPE, 0x0, NULL,
  nl, forward_relu,
  &_model_block1_Relu_1_output_0_chain,
  NULL, &_model_down1_down1_0_Conv_output_0_layer, AI_STATIC, 
  .nl_params = NULL, 
)

AI_TENSOR_CHAIN_OBJ_DECLARE(
  _model_block1_Add_output_0_chain, AI_STATIC_CONST, 4,
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 2, &_model_block1_conv2_Conv_output_0_output, &_model_stem_stem_2_Relu_output_0_output),
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 1, &_model_block1_Add_output_0_output),
  AI_TENSOR_LIST_OBJ_EMPTY,
  AI_TENSOR_LIST_OBJ_EMPTY
)

AI_LAYER_OBJ_DECLARE(
  _model_block1_Add_output_0_layer, 9,
  ELTWISE_TYPE, 0x0, NULL,
  eltwise, forward_eltwise,
  &_model_block1_Add_output_0_chain,
  NULL, &_model_block1_Relu_1_output_0_layer, AI_STATIC, 
  .operation = ai_sum_f32, 
  .buffer_operation = ai_sum_buffer_f32, 
)

AI_TENSOR_CHAIN_OBJ_DECLARE(
  _model_block1_conv2_Conv_output_0_chain, AI_STATIC_CONST, 4,
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 1, &_model_block1_Relu_output_0_output),
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 1, &_model_block1_conv2_Conv_output_0_output),
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 3, &_model_block1_conv2_Conv_output_0_weights, &_model_block1_conv2_Conv_output_0_bias, NULL),
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 2, &_model_block1_conv2_Conv_output_0_scratch0, NULL)
)

AI_LAYER_OBJ_DECLARE(
  _model_block1_conv2_Conv_output_0_layer, 8,
  CONV2D_TYPE, 0x0, NULL,
  conv2d, forward_conv2d_if32of32wf32,
  &_model_block1_conv2_Conv_output_0_chain,
  NULL, &_model_block1_Add_output_0_layer, AI_STATIC, 
  .groups = 1, 
  .filter_stride = AI_SHAPE_2D_INIT(1, 1), 
  .dilation = AI_SHAPE_2D_INIT(1, 1), 
  .filter_pad = AI_SHAPE_INIT(4, 2, 0, 2, 0), 
  .in_ch_format = AI_LAYER_FORMAT_CHANNEL_LAST_SAME, 
  .out_ch_format = AI_LAYER_FORMAT_CHANNEL_LAST_VALID, 
)

AI_TENSOR_CHAIN_OBJ_DECLARE(
  _model_block1_Relu_output_0_chain, AI_STATIC_CONST, 4,
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 1, &_model_block1_conv1_Conv_output_0_output),
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 1, &_model_block1_Relu_output_0_output),
  AI_TENSOR_LIST_OBJ_EMPTY,
  AI_TENSOR_LIST_OBJ_EMPTY
)

AI_LAYER_OBJ_DECLARE(
  _model_block1_Relu_output_0_layer, 7,
  NL_TYPE, 0x0, NULL,
  nl, forward_relu,
  &_model_block1_Relu_output_0_chain,
  NULL, &_model_block1_conv2_Conv_output_0_layer, AI_STATIC, 
  .nl_params = NULL, 
)

AI_TENSOR_CHAIN_OBJ_DECLARE(
  _model_block1_conv1_Conv_output_0_chain, AI_STATIC_CONST, 4,
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 1, &_model_stem_stem_2_Relu_output_0_output),
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 1, &_model_block1_conv1_Conv_output_0_output),
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 3, &_model_block1_conv1_Conv_output_0_weights, &_model_block1_conv1_Conv_output_0_bias, NULL),
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 2, &_model_block1_conv1_Conv_output_0_scratch0, NULL)
)

AI_LAYER_OBJ_DECLARE(
  _model_block1_conv1_Conv_output_0_layer, 6,
  CONV2D_TYPE, 0x0, NULL,
  conv2d, forward_conv2d_if32of32wf32,
  &_model_block1_conv1_Conv_output_0_chain,
  NULL, &_model_block1_Relu_output_0_layer, AI_STATIC, 
  .groups = 1, 
  .filter_stride = AI_SHAPE_2D_INIT(1, 1), 
  .dilation = AI_SHAPE_2D_INIT(1, 1), 
  .filter_pad = AI_SHAPE_INIT(4, 2, 0, 2, 0), 
  .in_ch_format = AI_LAYER_FORMAT_CHANNEL_LAST_SAME, 
  .out_ch_format = AI_LAYER_FORMAT_CHANNEL_LAST_VALID, 
)

AI_TENSOR_CHAIN_OBJ_DECLARE(
  _model_stem_stem_2_Relu_output_0_chain, AI_STATIC_CONST, 4,
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 1, &_model_stem_stem_0_Conv_output_0_output),
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 1, &_model_stem_stem_2_Relu_output_0_output),
  AI_TENSOR_LIST_OBJ_EMPTY,
  AI_TENSOR_LIST_OBJ_EMPTY
)

AI_LAYER_OBJ_DECLARE(
  _model_stem_stem_2_Relu_output_0_layer, 5,
  NL_TYPE, 0x0, NULL,
  nl, forward_relu,
  &_model_stem_stem_2_Relu_output_0_chain,
  NULL, &_model_block1_conv1_Conv_output_0_layer, AI_STATIC, 
  .nl_params = NULL, 
)

AI_TENSOR_CHAIN_OBJ_DECLARE(
  _model_stem_stem_0_Conv_output_0_chain, AI_STATIC_CONST, 4,
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 1, &_Transpose_output_0_output),
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 1, &_model_stem_stem_0_Conv_output_0_output),
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 3, &_model_stem_stem_0_Conv_output_0_weights, &_model_stem_stem_0_Conv_output_0_bias, NULL),
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 2, &_model_stem_stem_0_Conv_output_0_scratch0, NULL)
)

AI_LAYER_OBJ_DECLARE(
  _model_stem_stem_0_Conv_output_0_layer, 4,
  CONV2D_TYPE, 0x0, NULL,
  conv2d, forward_conv2d_if32of32wf32,
  &_model_stem_stem_0_Conv_output_0_chain,
  NULL, &_model_stem_stem_2_Relu_output_0_layer, AI_STATIC, 
  .groups = 1, 
  .filter_stride = AI_SHAPE_2D_INIT(1, 1), 
  .dilation = AI_SHAPE_2D_INIT(1, 1), 
  .filter_pad = AI_SHAPE_INIT(4, 3, 0, 3, 0), 
  .in_ch_format = AI_LAYER_FORMAT_CHANNEL_LAST_SAME, 
  .out_ch_format = AI_LAYER_FORMAT_CHANNEL_LAST_VALID, 
)

AI_TENSOR_CHAIN_OBJ_DECLARE(
  _Transpose_output_0_chain, AI_STATIC_CONST, 4,
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 1, &_Div_output_0_output),
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 1, &_Transpose_output_0_output),
  AI_TENSOR_LIST_OBJ_EMPTY,
  AI_TENSOR_LIST_OBJ_EMPTY
)

AI_LAYER_OBJ_DECLARE(
  _Transpose_output_0_layer, 3,
  TRANSPOSE_TYPE, 0x0, NULL,
  transpose, forward_transpose,
  &_Transpose_output_0_chain,
  NULL, &_model_stem_stem_0_Conv_output_0_layer, AI_STATIC, 
  .out_mapping = AI_SHAPE_INIT(6, AI_SHAPE_IN_CHANNEL, AI_SHAPE_HEIGHT, AI_SHAPE_WIDTH, AI_SHAPE_CHANNEL, AI_SHAPE_DEPTH, AI_SHAPE_EXTENSION), 
)

AI_TENSOR_CHAIN_OBJ_DECLARE(
  _Div_output_0_chain, AI_STATIC_CONST, 4,
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 2, &_Sub_output_0_output, &std),
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 1, &_Div_output_0_output),
  AI_TENSOR_LIST_OBJ_EMPTY,
  AI_TENSOR_LIST_OBJ_EMPTY
)

AI_LAYER_OBJ_DECLARE(
  _Div_output_0_layer, 2,
  ELTWISE_TYPE, 0x0, NULL,
  eltwise, forward_eltwise,
  &_Div_output_0_chain,
  NULL, &_Transpose_output_0_layer, AI_STATIC, 
  .operation = ai_div_f32, 
  .buffer_operation = ai_div_buffer_f32, 
)

AI_TENSOR_CHAIN_OBJ_DECLARE(
  _Sub_output_0_chain, AI_STATIC_CONST, 4,
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 2, &gesture_Transpose_output, &mean),
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 1, &_Sub_output_0_output),
  AI_TENSOR_LIST_OBJ_EMPTY,
  AI_TENSOR_LIST_OBJ_EMPTY
)

AI_LAYER_OBJ_DECLARE(
  _Sub_output_0_layer, 1,
  ELTWISE_TYPE, 0x0, NULL,
  eltwise, forward_eltwise,
  &_Sub_output_0_chain,
  NULL, &_Div_output_0_layer, AI_STATIC, 
  .operation = ai_sub_f32, 
  .buffer_operation = ai_sub_buffer_f32, 
)

AI_TENSOR_CHAIN_OBJ_DECLARE(
  gesture_Transpose_chain, AI_STATIC_CONST, 4,
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 1, &gesture_output),
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 1, &gesture_Transpose_output),
  AI_TENSOR_LIST_OBJ_EMPTY,
  AI_TENSOR_LIST_OBJ_EMPTY
)

AI_LAYER_OBJ_DECLARE(
  gesture_Transpose_layer, 2,
  TRANSPOSE_TYPE, 0x0, NULL,
  transpose, forward_transpose,
  &gesture_Transpose_chain,
  NULL, &_Sub_output_0_layer, AI_STATIC, 
  .out_mapping = AI_SHAPE_INIT(6, AI_SHAPE_IN_CHANNEL, AI_SHAPE_HEIGHT, AI_SHAPE_WIDTH, AI_SHAPE_CHANNEL, AI_SHAPE_DEPTH, AI_SHAPE_EXTENSION), 
)


#if (AI_TOOLS_API_VERSION < AI_TOOLS_API_VERSION_1_5)

AI_NETWORK_OBJ_DECLARE(
  AI_NET_OBJ_INSTANCE, AI_STATIC,
  AI_BUFFER_INIT(AI_FLAG_NONE,  AI_BUFFER_FORMAT_U8,
    AI_BUFFER_SHAPE_INIT(AI_SHAPE_BCWH, 4, 1, 261536, 1, 1),
    261536, NULL, NULL),
  AI_BUFFER_INIT(AI_FLAG_NONE,  AI_BUFFER_FORMAT_U8,
    AI_BUFFER_SHAPE_INIT(AI_SHAPE_BCWH, 4, 1, 53504, 1, 1),
    53504, NULL, NULL),
  AI_TENSOR_LIST_IO_OBJ_INIT(AI_FLAG_NONE, AI_GESTURE_IN_NUM, &gesture_output),
  AI_TENSOR_LIST_IO_OBJ_INIT(AI_FLAG_NONE, AI_GESTURE_OUT_NUM, &logits_output),
  &gesture_Transpose_layer, 0x2164f9d4, NULL)

#else

AI_NETWORK_OBJ_DECLARE(
  AI_NET_OBJ_INSTANCE, AI_STATIC,
  AI_BUFFER_ARRAY_OBJ_INIT_STATIC(
  	AI_FLAG_NONE, 1,
    AI_BUFFER_INIT(AI_FLAG_NONE,  AI_BUFFER_FORMAT_U8,
      AI_BUFFER_SHAPE_INIT(AI_SHAPE_BCWH, 4, 1, 261536, 1, 1),
      261536, NULL, NULL)
  ),
  AI_BUFFER_ARRAY_OBJ_INIT_STATIC(
  	AI_FLAG_NONE, 1,
    AI_BUFFER_INIT(AI_FLAG_NONE,  AI_BUFFER_FORMAT_U8,
      AI_BUFFER_SHAPE_INIT(AI_SHAPE_BCWH, 4, 1, 53504, 1, 1),
      53504, NULL, NULL)
  ),
  AI_TENSOR_LIST_IO_OBJ_INIT(AI_FLAG_NONE, AI_GESTURE_IN_NUM, &gesture_output),
  AI_TENSOR_LIST_IO_OBJ_INIT(AI_FLAG_NONE, AI_GESTURE_OUT_NUM, &logits_output),
  &gesture_Transpose_layer, 0x2164f9d4, NULL)

#endif	/*(AI_TOOLS_API_VERSION < AI_TOOLS_API_VERSION_1_5)*/



/******************************************************************************/
AI_DECLARE_STATIC
ai_bool gesture_configure_activations(
  ai_network* net_ctx, const ai_network_params* params)
{
  AI_ASSERT(net_ctx)

  if (ai_platform_get_activations_map(g_gesture_activations_map, 1, params)) {
    /* Updating activations (byte) offsets */
    
    gesture_output_array.data = AI_PTR(g_gesture_activations_map[0] + 19104);
    gesture_output_array.data_start = AI_PTR(g_gesture_activations_map[0] + 19104);
    gesture_Transpose_output_array.data = AI_PTR(g_gesture_activations_map[0] + 10304);
    gesture_Transpose_output_array.data_start = AI_PTR(g_gesture_activations_map[0] + 10304);
    _Sub_output_0_output_array.data = AI_PTR(g_gesture_activations_map[0] + 10304);
    _Sub_output_0_output_array.data_start = AI_PTR(g_gesture_activations_map[0] + 10304);
    _Div_output_0_output_array.data = AI_PTR(g_gesture_activations_map[0] + 19104);
    _Div_output_0_output_array.data_start = AI_PTR(g_gesture_activations_map[0] + 19104);
    _Transpose_output_0_output_array.data = AI_PTR(g_gesture_activations_map[0] + 27904);
    _Transpose_output_0_output_array.data_start = AI_PTR(g_gesture_activations_map[0] + 27904);
    _model_stem_stem_0_Conv_output_0_scratch0_array.data = AI_PTR(g_gesture_activations_map[0] + 36704);
    _model_stem_stem_0_Conv_output_0_scratch0_array.data_start = AI_PTR(g_gesture_activations_map[0] + 36704);
    _model_stem_stem_0_Conv_output_0_output_array.data = AI_PTR(g_gesture_activations_map[0] + 2304);
    _model_stem_stem_0_Conv_output_0_output_array.data_start = AI_PTR(g_gesture_activations_map[0] + 2304);
    _model_stem_stem_2_Relu_output_0_output_array.data = AI_PTR(g_gesture_activations_map[0] + 27904);
    _model_stem_stem_2_Relu_output_0_output_array.data_start = AI_PTR(g_gesture_activations_map[0] + 27904);
    _model_block1_conv1_Conv_output_0_scratch0_array.data = AI_PTR(g_gesture_activations_map[0] + 27264);
    _model_block1_conv1_Conv_output_0_scratch0_array.data_start = AI_PTR(g_gesture_activations_map[0] + 27264);
    _model_block1_conv1_Conv_output_0_output_array.data = AI_PTR(g_gesture_activations_map[0] + 1664);
    _model_block1_conv1_Conv_output_0_output_array.data_start = AI_PTR(g_gesture_activations_map[0] + 1664);
    _model_block1_Relu_output_0_output_array.data = AI_PTR(g_gesture_activations_map[0] + 1664);
    _model_block1_Relu_output_0_output_array.data_start = AI_PTR(g_gesture_activations_map[0] + 1664);
    _model_block1_conv2_Conv_output_0_scratch0_array.data = AI_PTR(g_gesture_activations_map[0] + 27264);
    _model_block1_conv2_Conv_output_0_scratch0_array.data_start = AI_PTR(g_gesture_activations_map[0] + 27264);
    _model_block1_conv2_Conv_output_0_output_array.data = AI_PTR(g_gesture_activations_map[0] + 1152);
    _model_block1_conv2_Conv_output_0_output_array.data_start = AI_PTR(g_gesture_activations_map[0] + 1152);
    _model_block1_Add_output_0_output_array.data = AI_PTR(g_gesture_activations_map[0] + 1152);
    _model_block1_Add_output_0_output_array.data_start = AI_PTR(g_gesture_activations_map[0] + 1152);
    _model_block1_Relu_1_output_0_output_array.data = AI_PTR(g_gesture_activations_map[0] + 1152);
    _model_block1_Relu_1_output_0_output_array.data_start = AI_PTR(g_gesture_activations_map[0] + 1152);
    _model_down1_down1_0_Conv_output_0_scratch0_array.data = AI_PTR(g_gesture_activations_map[0] + 26752);
    _model_down1_down1_0_Conv_output_0_scratch0_array.data_start = AI_PTR(g_gesture_activations_map[0] + 26752);
    _model_down1_down1_0_Conv_output_0_output_array.data = AI_PTR(g_gesture_activations_map[0] + 27904);
    _model_down1_down1_0_Conv_output_0_output_array.data_start = AI_PTR(g_gesture_activations_map[0] + 27904);
    _model_down1_down1_2_Relu_output_0_output_array.data = AI_PTR(g_gesture_activations_map[0] + 27904);
    _model_down1_down1_2_Relu_output_0_output_array.data_start = AI_PTR(g_gesture_activations_map[0] + 27904);
    _model_block2_conv1_Conv_output_0_scratch0_array.data = AI_PTR(g_gesture_activations_map[0] + 26624);
    _model_block2_conv1_Conv_output_0_scratch0_array.data_start = AI_PTR(g_gesture_activations_map[0] + 26624);
    _model_block2_conv1_Conv_output_0_output_array.data = AI_PTR(g_gesture_activations_map[0] + 1024);
    _model_block2_conv1_Conv_output_0_output_array.data_start = AI_PTR(g_gesture_activations_map[0] + 1024);
    _model_block2_Relu_output_0_output_array.data = AI_PTR(g_gesture_activations_map[0] + 1024);
    _model_block2_Relu_output_0_output_array.data_start = AI_PTR(g_gesture_activations_map[0] + 1024);
    _model_block2_conv2_Conv_output_0_scratch0_array.data = AI_PTR(g_gesture_activations_map[0] + 26624);
    _model_block2_conv2_Conv_output_0_scratch0_array.data_start = AI_PTR(g_gesture_activations_map[0] + 26624);
    _model_block2_conv2_Conv_output_0_output_array.data = AI_PTR(g_gesture_activations_map[0] + 0);
    _model_block2_conv2_Conv_output_0_output_array.data_start = AI_PTR(g_gesture_activations_map[0] + 0);
    _model_block2_Add_output_0_output_array.data = AI_PTR(g_gesture_activations_map[0] + 0);
    _model_block2_Add_output_0_output_array.data_start = AI_PTR(g_gesture_activations_map[0] + 0);
    _model_block2_Relu_1_output_0_output_array.data = AI_PTR(g_gesture_activations_map[0] + 25600);
    _model_block2_Relu_1_output_0_output_array.data_start = AI_PTR(g_gesture_activations_map[0] + 25600);
    _model_pool_GlobalAveragePool_output_0_output_array.data = AI_PTR(g_gesture_activations_map[0] + 0);
    _model_pool_GlobalAveragePool_output_0_output_array.data_start = AI_PTR(g_gesture_activations_map[0] + 0);
    logits_output_array.data = AI_PTR(g_gesture_activations_map[0] + 256);
    logits_output_array.data_start = AI_PTR(g_gesture_activations_map[0] + 256);
    return true;
  }
  AI_ERROR_TRAP(net_ctx, INIT_FAILED, NETWORK_ACTIVATIONS);
  return false;
}




/******************************************************************************/
AI_DECLARE_STATIC
ai_bool gesture_configure_weights(
  ai_network* net_ctx, const ai_network_params* params)
{
  AI_ASSERT(net_ctx)

  if (ai_platform_get_weights_map(g_gesture_weights_map, 1, params)) {
    /* Updating weights (byte) offsets */
    
    std_array.format |= AI_FMT_FLAG_CONST;
    std_array.data = AI_PTR(g_gesture_weights_map[0] + 0);
    std_array.data_start = AI_PTR(g_gesture_weights_map[0] + 0);
    mean_array.format |= AI_FMT_FLAG_CONST;
    mean_array.data = AI_PTR(g_gesture_weights_map[0] + 44);
    mean_array.data_start = AI_PTR(g_gesture_weights_map[0] + 44);
    _model_stem_stem_0_Conv_output_0_weights_array.format |= AI_FMT_FLAG_CONST;
    _model_stem_stem_0_Conv_output_0_weights_array.data = AI_PTR(g_gesture_weights_map[0] + 88);
    _model_stem_stem_0_Conv_output_0_weights_array.data_start = AI_PTR(g_gesture_weights_map[0] + 88);
    _model_stem_stem_0_Conv_output_0_bias_array.format |= AI_FMT_FLAG_CONST;
    _model_stem_stem_0_Conv_output_0_bias_array.data = AI_PTR(g_gesture_weights_map[0] + 9944);
    _model_stem_stem_0_Conv_output_0_bias_array.data_start = AI_PTR(g_gesture_weights_map[0] + 9944);
    _model_block1_conv1_Conv_output_0_weights_array.format |= AI_FMT_FLAG_CONST;
    _model_block1_conv1_Conv_output_0_weights_array.data = AI_PTR(g_gesture_weights_map[0] + 10072);
    _model_block1_conv1_Conv_output_0_weights_array.data_start = AI_PTR(g_gesture_weights_map[0] + 10072);
    _model_block1_conv1_Conv_output_0_bias_array.format |= AI_FMT_FLAG_CONST;
    _model_block1_conv1_Conv_output_0_bias_array.data = AI_PTR(g_gesture_weights_map[0] + 30552);
    _model_block1_conv1_Conv_output_0_bias_array.data_start = AI_PTR(g_gesture_weights_map[0] + 30552);
    _model_block1_conv2_Conv_output_0_weights_array.format |= AI_FMT_FLAG_CONST;
    _model_block1_conv2_Conv_output_0_weights_array.data = AI_PTR(g_gesture_weights_map[0] + 30680);
    _model_block1_conv2_Conv_output_0_weights_array.data_start = AI_PTR(g_gesture_weights_map[0] + 30680);
    _model_block1_conv2_Conv_output_0_bias_array.format |= AI_FMT_FLAG_CONST;
    _model_block1_conv2_Conv_output_0_bias_array.data = AI_PTR(g_gesture_weights_map[0] + 51160);
    _model_block1_conv2_Conv_output_0_bias_array.data_start = AI_PTR(g_gesture_weights_map[0] + 51160);
    _model_down1_down1_0_Conv_output_0_weights_array.format |= AI_FMT_FLAG_CONST;
    _model_down1_down1_0_Conv_output_0_weights_array.data = AI_PTR(g_gesture_weights_map[0] + 51288);
    _model_down1_down1_0_Conv_output_0_weights_array.data_start = AI_PTR(g_gesture_weights_map[0] + 51288);
    _model_down1_down1_0_Conv_output_0_bias_array.format |= AI_FMT_FLAG_CONST;
    _model_down1_down1_0_Conv_output_0_bias_array.data = AI_PTR(g_gesture_weights_map[0] + 92248);
    _model_down1_down1_0_Conv_output_0_bias_array.data_start = AI_PTR(g_gesture_weights_map[0] + 92248);
    _model_block2_conv1_Conv_output_0_weights_array.format |= AI_FMT_FLAG_CONST;
    _model_block2_conv1_Conv_output_0_weights_array.data = AI_PTR(g_gesture_weights_map[0] + 92504);
    _model_block2_conv1_Conv_output_0_weights_array.data_start = AI_PTR(g_gesture_weights_map[0] + 92504);
    _model_block2_conv1_Conv_output_0_bias_array.format |= AI_FMT_FLAG_CONST;
    _model_block2_conv1_Conv_output_0_bias_array.data = AI_PTR(g_gesture_weights_map[0] + 174424);
    _model_block2_conv1_Conv_output_0_bias_array.data_start = AI_PTR(g_gesture_weights_map[0] + 174424);
    _model_block2_conv2_Conv_output_0_weights_array.format |= AI_FMT_FLAG_CONST;
    _model_block2_conv2_Conv_output_0_weights_array.data = AI_PTR(g_gesture_weights_map[0] + 174680);
    _model_block2_conv2_Conv_output_0_weights_array.data_start = AI_PTR(g_gesture_weights_map[0] + 174680);
    _model_block2_conv2_Conv_output_0_bias_array.format |= AI_FMT_FLAG_CONST;
    _model_block2_conv2_Conv_output_0_bias_array.data = AI_PTR(g_gesture_weights_map[0] + 256600);
    _model_block2_conv2_Conv_output_0_bias_array.data_start = AI_PTR(g_gesture_weights_map[0] + 256600);
    logits_weights_array.format |= AI_FMT_FLAG_CONST;
    logits_weights_array.data = AI_PTR(g_gesture_weights_map[0] + 256856);
    logits_weights_array.data_start = AI_PTR(g_gesture_weights_map[0] + 256856);
    logits_bias_array.format |= AI_FMT_FLAG_CONST;
    logits_bias_array.data = AI_PTR(g_gesture_weights_map[0] + 261464);
    logits_bias_array.data_start = AI_PTR(g_gesture_weights_map[0] + 261464);
    return true;
  }
  AI_ERROR_TRAP(net_ctx, INIT_FAILED, NETWORK_WEIGHTS);
  return false;
}


/**  PUBLIC APIs SECTION  *****************************************************/



AI_DEPRECATED
AI_API_ENTRY
ai_bool ai_gesture_get_info(
  ai_handle network, ai_network_report* report)
{
  ai_network* net_ctx = AI_NETWORK_ACQUIRE_CTX(network);

  if (report && net_ctx)
  {
    ai_network_report r = {
      .model_name        = AI_GESTURE_MODEL_NAME,
      .model_signature   = AI_GESTURE_MODEL_SIGNATURE,
      .model_datetime    = AI_TOOLS_DATE_TIME,
      
      .compile_datetime  = AI_TOOLS_COMPILE_TIME,
      
      .runtime_revision  = ai_platform_runtime_get_revision(),
      .runtime_version   = ai_platform_runtime_get_version(),

      .tool_revision     = AI_TOOLS_REVISION_ID,
      .tool_version      = {AI_TOOLS_VERSION_MAJOR, AI_TOOLS_VERSION_MINOR,
                            AI_TOOLS_VERSION_MICRO, 0x0},
      .tool_api_version  = AI_STRUCT_INIT,

      .api_version            = ai_platform_api_get_version(),
      .interface_api_version  = ai_platform_interface_api_get_version(),
      
      .n_macc            = 7735258,
      .n_inputs          = 0,
      .inputs            = NULL,
      .n_outputs         = 0,
      .outputs           = NULL,
      .params            = AI_STRUCT_INIT,
      .activations       = AI_STRUCT_INIT,
      .n_nodes           = 0,
      .signature         = 0x2164f9d4,
    };

    if (!ai_platform_api_get_network_report(network, &r)) return false;

    *report = r;
    return true;
  }
  return false;
}



AI_API_ENTRY
ai_bool ai_gesture_get_report(
  ai_handle network, ai_network_report* report)
{
  ai_network* net_ctx = AI_NETWORK_ACQUIRE_CTX(network);

  if (report && net_ctx)
  {
    ai_network_report r = {
      .model_name        = AI_GESTURE_MODEL_NAME,
      .model_signature   = AI_GESTURE_MODEL_SIGNATURE,
      .model_datetime    = AI_TOOLS_DATE_TIME,
      
      .compile_datetime  = AI_TOOLS_COMPILE_TIME,
      
      .runtime_revision  = ai_platform_runtime_get_revision(),
      .runtime_version   = ai_platform_runtime_get_version(),

      .tool_revision     = AI_TOOLS_REVISION_ID,
      .tool_version      = {AI_TOOLS_VERSION_MAJOR, AI_TOOLS_VERSION_MINOR,
                            AI_TOOLS_VERSION_MICRO, 0x0},
      .tool_api_version  = AI_STRUCT_INIT,

      .api_version            = ai_platform_api_get_version(),
      .interface_api_version  = ai_platform_interface_api_get_version(),
      
      .n_macc            = 7735258,
      .n_inputs          = 0,
      .inputs            = NULL,
      .n_outputs         = 0,
      .outputs           = NULL,
      .map_signature     = AI_MAGIC_SIGNATURE,
      .map_weights       = AI_STRUCT_INIT,
      .map_activations   = AI_STRUCT_INIT,
      .n_nodes           = 0,
      .signature         = 0x2164f9d4,
    };

    if (!ai_platform_api_get_network_report(network, &r)) return false;

    *report = r;
    return true;
  }
  return false;
}


AI_API_ENTRY
ai_error ai_gesture_get_error(ai_handle network)
{
  return ai_platform_network_get_error(network);
}


AI_API_ENTRY
ai_error ai_gesture_create(
  ai_handle* network, const ai_buffer* network_config)
{
  return ai_platform_network_create(
    network, network_config, 
    AI_CONTEXT_OBJ(&AI_NET_OBJ_INSTANCE),
    AI_TOOLS_API_VERSION_MAJOR, AI_TOOLS_API_VERSION_MINOR, AI_TOOLS_API_VERSION_MICRO);
}


AI_API_ENTRY
ai_error ai_gesture_create_and_init(
  ai_handle* network, const ai_handle activations[], const ai_handle weights[])
{
  ai_error err;
  ai_network_params params;

  err = ai_gesture_create(network, AI_GESTURE_DATA_CONFIG);
  if (err.type != AI_ERROR_NONE) {
    return err;
  }
  
  if (ai_gesture_data_params_get(&params) != true) {
    err = ai_gesture_get_error(*network);
    return err;
  }
#if defined(AI_GESTURE_DATA_ACTIVATIONS_COUNT)
  /* set the addresses of the activations buffers */
  for (ai_u16 idx=0; activations && idx<params.map_activations.size; idx++) {
    AI_BUFFER_ARRAY_ITEM_SET_ADDRESS(&params.map_activations, idx, activations[idx]);
  }
#endif
#if defined(AI_GESTURE_DATA_WEIGHTS_COUNT)
  /* set the addresses of the weight buffers */
  for (ai_u16 idx=0; weights && idx<params.map_weights.size; idx++) {
    AI_BUFFER_ARRAY_ITEM_SET_ADDRESS(&params.map_weights, idx, weights[idx]);
  }
#endif
  if (ai_gesture_init(*network, &params) != true) {
    err = ai_gesture_get_error(*network);
  }
  return err;
}


AI_API_ENTRY
ai_buffer* ai_gesture_inputs_get(ai_handle network, ai_u16 *n_buffer)
{
  if (network == AI_HANDLE_NULL) {
    network = (ai_handle)&AI_NET_OBJ_INSTANCE;
    AI_NETWORK_OBJ(network)->magic = AI_MAGIC_CONTEXT_TOKEN;
  }
  return ai_platform_inputs_get(network, n_buffer);
}


AI_API_ENTRY
ai_buffer* ai_gesture_outputs_get(ai_handle network, ai_u16 *n_buffer)
{
  if (network == AI_HANDLE_NULL) {
    network = (ai_handle)&AI_NET_OBJ_INSTANCE;
    AI_NETWORK_OBJ(network)->magic = AI_MAGIC_CONTEXT_TOKEN;
  }
  return ai_platform_outputs_get(network, n_buffer);
}


AI_API_ENTRY
ai_handle ai_gesture_destroy(ai_handle network)
{
  return ai_platform_network_destroy(network);
}


AI_API_ENTRY
ai_bool ai_gesture_init(
  ai_handle network, const ai_network_params* params)
{
  ai_network* net_ctx = AI_NETWORK_OBJ(ai_platform_network_init(network, params));
  ai_bool ok = true;

  if (!net_ctx) return false;
  ok &= gesture_configure_weights(net_ctx, params);
  ok &= gesture_configure_activations(net_ctx, params);

  ok &= ai_platform_network_post_init(network);

  return ok;
}


AI_API_ENTRY
ai_i32 ai_gesture_run(
  ai_handle network, const ai_buffer* input, ai_buffer* output)
{
  return ai_platform_network_process(network, input, output);
}


AI_API_ENTRY
ai_i32 ai_gesture_forward(ai_handle network, const ai_buffer* input)
{
  return ai_platform_network_process(network, input, NULL);
}



#undef AI_GESTURE_MODEL_SIGNATURE
#undef AI_NET_OBJ_INSTANCE
#undef AI_TOOLS_DATE_TIME
#undef AI_TOOLS_COMPILE_TIME


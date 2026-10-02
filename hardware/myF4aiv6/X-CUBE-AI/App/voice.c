/**
  ******************************************************************************
  * @file    voice.c
  * @author  AST Embedded Analytics Research Platform
  * @date    2026-07-23T20:52:55+0800
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


#include "voice.h"
#include "voice_data.h"

#include "ai_platform.h"
#include "ai_platform_interface.h"
#include "ai_math_helpers.h"

#include "core_common.h"
#include "core_convert.h"

#include "layers.h"



#undef AI_NET_OBJ_INSTANCE
#define AI_NET_OBJ_INSTANCE g_voice
 
#undef AI_VOICE_MODEL_SIGNATURE
#define AI_VOICE_MODEL_SIGNATURE     "0xbcf5d8813a7f0b7d482b68c90316baaf"

#ifndef AI_TOOLS_REVISION_ID
#define AI_TOOLS_REVISION_ID     ""
#endif

#undef AI_TOOLS_DATE_TIME
#define AI_TOOLS_DATE_TIME   "2026-07-23T20:52:55+0800"

#undef AI_TOOLS_COMPILE_TIME
#define AI_TOOLS_COMPILE_TIME    __DATE__ " " __TIME__

#undef AI_VOICE_N_BATCHES
#define AI_VOICE_N_BATCHES         (1)

static ai_ptr g_voice_activations_map[1] = AI_C_ARRAY_INIT;
static ai_ptr g_voice_weights_map[1] = AI_C_ARRAY_INIT;



/**  Array declarations section  **********************************************/
/* Array#0 */
AI_ARRAY_OBJ_DECLARE(
  voice_output_array, AI_ARRAY_FORMAT_FLOAT|AI_FMT_FLAG_IS_IO,
  NULL, NULL, 2250, AI_STATIC)

/* Array#1 */
AI_ARRAY_OBJ_DECLARE(
  sub_output_array, AI_ARRAY_FORMAT_FLOAT,
  NULL, NULL, 2250, AI_STATIC)

/* Array#2 */
AI_ARRAY_OBJ_DECLARE(
  div_output_array, AI_ARRAY_FORMAT_FLOAT,
  NULL, NULL, 2250, AI_STATIC)

/* Array#3 */
AI_ARRAY_OBJ_DECLARE(
  getitem_2_output_array, AI_ARRAY_FORMAT_FLOAT,
  NULL, NULL, 9, AI_STATIC)

/* Array#4 */
AI_ARRAY_OBJ_DECLARE(
  getitem_output_array, AI_ARRAY_FORMAT_FLOAT,
  NULL, NULL, 9, AI_STATIC)

/* Array#5 */
AI_ARRAY_OBJ_DECLARE(
  mean_1_output_array, AI_ARRAY_FORMAT_FLOAT,
  NULL, NULL, 9, AI_STATIC)

/* Array#6 */
AI_ARRAY_OBJ_DECLARE(
  mean_1_Mul_output_array, AI_ARRAY_FORMAT_FLOAT,
  NULL, NULL, 9, AI_STATIC)

/* Array#7 */
AI_ARRAY_OBJ_DECLARE(
  cat_output_array, AI_ARRAY_FORMAT_FLOAT,
  NULL, NULL, 27, AI_STATIC)

/* Array#8 */
AI_ARRAY_OBJ_DECLARE(
  linear_output_array, AI_ARRAY_FORMAT_FLOAT,
  NULL, NULL, 32, AI_STATIC)

/* Array#9 */
AI_ARRAY_OBJ_DECLARE(
  relu_output_array, AI_ARRAY_FORMAT_FLOAT,
  NULL, NULL, 32, AI_STATIC)

/* Array#10 */
AI_ARRAY_OBJ_DECLARE(
  linear_1_output_array, AI_ARRAY_FORMAT_FLOAT,
  NULL, NULL, 32, AI_STATIC)

/* Array#11 */
AI_ARRAY_OBJ_DECLARE(
  relu_1_output_array, AI_ARRAY_FORMAT_FLOAT,
  NULL, NULL, 32, AI_STATIC)

/* Array#12 */
AI_ARRAY_OBJ_DECLARE(
  logits_output_array, AI_ARRAY_FORMAT_FLOAT|AI_FMT_FLAG_IS_IO,
  NULL, NULL, 4, AI_STATIC)

/* Array#13 */
AI_ARRAY_OBJ_DECLARE(
  std_array, AI_ARRAY_FORMAT_FLOAT,
  NULL, NULL, 9, AI_STATIC)

/* Array#14 */
AI_ARRAY_OBJ_DECLARE(
  mean_array, AI_ARRAY_FORMAT_FLOAT,
  NULL, NULL, 9, AI_STATIC)

/* Array#15 */
AI_ARRAY_OBJ_DECLARE(
  mean_1_Mul_scale_array, AI_ARRAY_FORMAT_FLOAT,
  NULL, NULL, 9, AI_STATIC)

/* Array#16 */
AI_ARRAY_OBJ_DECLARE(
  mean_1_Mul_bias_array, AI_ARRAY_FORMAT_FLOAT,
  NULL, NULL, 9, AI_STATIC)

/* Array#17 */
AI_ARRAY_OBJ_DECLARE(
  linear_weights_array, AI_ARRAY_FORMAT_FLOAT,
  NULL, NULL, 864, AI_STATIC)

/* Array#18 */
AI_ARRAY_OBJ_DECLARE(
  linear_bias_array, AI_ARRAY_FORMAT_FLOAT,
  NULL, NULL, 32, AI_STATIC)

/* Array#19 */
AI_ARRAY_OBJ_DECLARE(
  linear_1_weights_array, AI_ARRAY_FORMAT_FLOAT,
  NULL, NULL, 1024, AI_STATIC)

/* Array#20 */
AI_ARRAY_OBJ_DECLARE(
  linear_1_bias_array, AI_ARRAY_FORMAT_FLOAT,
  NULL, NULL, 32, AI_STATIC)

/* Array#21 */
AI_ARRAY_OBJ_DECLARE(
  logits_weights_array, AI_ARRAY_FORMAT_FLOAT,
  NULL, NULL, 128, AI_STATIC)

/* Array#22 */
AI_ARRAY_OBJ_DECLARE(
  logits_bias_array, AI_ARRAY_FORMAT_FLOAT,
  NULL, NULL, 4, AI_STATIC)

/**  Tensor declarations section  *********************************************/
/* Tensor #0 */
AI_TENSOR_OBJ_DECLARE(
  cat_output, AI_STATIC,
  0, 0x0,
  AI_SHAPE_INIT(4, 1, 27, 1, 1), AI_STRIDE_INIT(4, 4, 4, 108, 108),
  1, &cat_output_array, NULL)

/* Tensor #1 */
AI_TENSOR_OBJ_DECLARE(
  div_output, AI_STATIC,
  1, 0x0,
  AI_SHAPE_INIT(4, 1, 9, 1, 250), AI_STRIDE_INIT(4, 4, 4, 36, 36),
  1, &div_output_array, NULL)

/* Tensor #2 */
AI_TENSOR_OBJ_DECLARE(
  getitem_2_output, AI_STATIC,
  2, 0x0,
  AI_SHAPE_INIT(4, 1, 9, 1, 1), AI_STRIDE_INIT(4, 4, 4, 36, 36),
  1, &getitem_2_output_array, NULL)

/* Tensor #3 */
AI_TENSOR_OBJ_DECLARE(
  getitem_output, AI_STATIC,
  3, 0x0,
  AI_SHAPE_INIT(4, 1, 9, 1, 1), AI_STRIDE_INIT(4, 4, 4, 36, 36),
  1, &getitem_output_array, NULL)

/* Tensor #4 */
AI_TENSOR_OBJ_DECLARE(
  linear_1_bias, AI_STATIC,
  4, 0x0,
  AI_SHAPE_INIT(4, 1, 32, 1, 1), AI_STRIDE_INIT(4, 4, 4, 128, 128),
  1, &linear_1_bias_array, NULL)

/* Tensor #5 */
AI_TENSOR_OBJ_DECLARE(
  linear_1_output, AI_STATIC,
  5, 0x0,
  AI_SHAPE_INIT(4, 1, 32, 1, 1), AI_STRIDE_INIT(4, 4, 4, 128, 128),
  1, &linear_1_output_array, NULL)

/* Tensor #6 */
AI_TENSOR_OBJ_DECLARE(
  linear_1_weights, AI_STATIC,
  6, 0x0,
  AI_SHAPE_INIT(4, 32, 32, 1, 1), AI_STRIDE_INIT(4, 4, 128, 4096, 4096),
  1, &linear_1_weights_array, NULL)

/* Tensor #7 */
AI_TENSOR_OBJ_DECLARE(
  linear_bias, AI_STATIC,
  7, 0x0,
  AI_SHAPE_INIT(4, 1, 32, 1, 1), AI_STRIDE_INIT(4, 4, 4, 128, 128),
  1, &linear_bias_array, NULL)

/* Tensor #8 */
AI_TENSOR_OBJ_DECLARE(
  linear_output, AI_STATIC,
  8, 0x0,
  AI_SHAPE_INIT(4, 1, 32, 1, 1), AI_STRIDE_INIT(4, 4, 4, 128, 128),
  1, &linear_output_array, NULL)

/* Tensor #9 */
AI_TENSOR_OBJ_DECLARE(
  linear_weights, AI_STATIC,
  9, 0x0,
  AI_SHAPE_INIT(4, 27, 32, 1, 1), AI_STRIDE_INIT(4, 4, 108, 3456, 3456),
  1, &linear_weights_array, NULL)

/* Tensor #10 */
AI_TENSOR_OBJ_DECLARE(
  logits_bias, AI_STATIC,
  10, 0x0,
  AI_SHAPE_INIT(4, 1, 4, 1, 1), AI_STRIDE_INIT(4, 4, 4, 16, 16),
  1, &logits_bias_array, NULL)

/* Tensor #11 */
AI_TENSOR_OBJ_DECLARE(
  logits_output, AI_STATIC,
  11, 0x0,
  AI_SHAPE_INIT(4, 1, 4, 1, 1), AI_STRIDE_INIT(4, 4, 4, 16, 16),
  1, &logits_output_array, NULL)

/* Tensor #12 */
AI_TENSOR_OBJ_DECLARE(
  logits_weights, AI_STATIC,
  12, 0x0,
  AI_SHAPE_INIT(4, 32, 4, 1, 1), AI_STRIDE_INIT(4, 4, 128, 512, 512),
  1, &logits_weights_array, NULL)

/* Tensor #13 */
AI_TENSOR_OBJ_DECLARE(
  mean, AI_STATIC,
  13, 0x0,
  AI_SHAPE_INIT(4, 1, 9, 1, 1), AI_STRIDE_INIT(4, 4, 4, 36, 36),
  1, &mean_array, NULL)

/* Tensor #14 */
AI_TENSOR_OBJ_DECLARE(
  mean_1_Mul_bias, AI_STATIC,
  14, 0x0,
  AI_SHAPE_INIT(4, 1, 9, 1, 1), AI_STRIDE_INIT(4, 4, 4, 36, 36),
  1, &mean_1_Mul_bias_array, NULL)

/* Tensor #15 */
AI_TENSOR_OBJ_DECLARE(
  mean_1_Mul_output, AI_STATIC,
  15, 0x0,
  AI_SHAPE_INIT(4, 1, 9, 1, 1), AI_STRIDE_INIT(4, 4, 4, 36, 36),
  1, &mean_1_Mul_output_array, NULL)

/* Tensor #16 */
AI_TENSOR_OBJ_DECLARE(
  mean_1_Mul_scale, AI_STATIC,
  16, 0x0,
  AI_SHAPE_INIT(4, 1, 9, 1, 1), AI_STRIDE_INIT(4, 4, 4, 36, 36),
  1, &mean_1_Mul_scale_array, NULL)

/* Tensor #17 */
AI_TENSOR_OBJ_DECLARE(
  mean_1_output, AI_STATIC,
  17, 0x0,
  AI_SHAPE_INIT(4, 1, 9, 1, 1), AI_STRIDE_INIT(4, 4, 4, 36, 36),
  1, &mean_1_output_array, NULL)

/* Tensor #18 */
AI_TENSOR_OBJ_DECLARE(
  relu_1_output, AI_STATIC,
  18, 0x0,
  AI_SHAPE_INIT(4, 1, 32, 1, 1), AI_STRIDE_INIT(4, 4, 4, 128, 128),
  1, &relu_1_output_array, NULL)

/* Tensor #19 */
AI_TENSOR_OBJ_DECLARE(
  relu_output, AI_STATIC,
  19, 0x0,
  AI_SHAPE_INIT(4, 1, 32, 1, 1), AI_STRIDE_INIT(4, 4, 4, 128, 128),
  1, &relu_output_array, NULL)

/* Tensor #20 */
AI_TENSOR_OBJ_DECLARE(
  std, AI_STATIC,
  20, 0x0,
  AI_SHAPE_INIT(4, 1, 9, 1, 1), AI_STRIDE_INIT(4, 4, 4, 36, 36),
  1, &std_array, NULL)

/* Tensor #21 */
AI_TENSOR_OBJ_DECLARE(
  sub_output, AI_STATIC,
  21, 0x0,
  AI_SHAPE_INIT(4, 1, 9, 1, 250), AI_STRIDE_INIT(4, 4, 4, 36, 36),
  1, &sub_output_array, NULL)

/* Tensor #22 */
AI_TENSOR_OBJ_DECLARE(
  voice_output, AI_STATIC,
  22, 0x0,
  AI_SHAPE_INIT(4, 1, 9, 1, 250), AI_STRIDE_INIT(4, 4, 4, 36, 36),
  1, &voice_output_array, NULL)



/**  Layer declarations section  **********************************************/


AI_TENSOR_CHAIN_OBJ_DECLARE(
  logits_chain, AI_STATIC_CONST, 4,
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 1, &relu_1_output),
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 1, &logits_output),
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 2, &logits_weights, &logits_bias),
  AI_TENSOR_LIST_OBJ_EMPTY
)

AI_LAYER_OBJ_DECLARE(
  logits_layer, 11,
  DENSE_TYPE, 0x0, NULL,
  dense, forward_dense,
  &logits_chain,
  NULL, &logits_layer, AI_STATIC, 
)

AI_TENSOR_CHAIN_OBJ_DECLARE(
  relu_1_chain, AI_STATIC_CONST, 4,
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 1, &linear_1_output),
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 1, &relu_1_output),
  AI_TENSOR_LIST_OBJ_EMPTY,
  AI_TENSOR_LIST_OBJ_EMPTY
)

AI_LAYER_OBJ_DECLARE(
  relu_1_layer, 10,
  NL_TYPE, 0x0, NULL,
  nl, forward_relu,
  &relu_1_chain,
  NULL, &logits_layer, AI_STATIC, 
  .nl_params = NULL, 
)

AI_TENSOR_CHAIN_OBJ_DECLARE(
  linear_1_chain, AI_STATIC_CONST, 4,
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 1, &relu_output),
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 1, &linear_1_output),
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 2, &linear_1_weights, &linear_1_bias),
  AI_TENSOR_LIST_OBJ_EMPTY
)

AI_LAYER_OBJ_DECLARE(
  linear_1_layer, 9,
  DENSE_TYPE, 0x0, NULL,
  dense, forward_dense,
  &linear_1_chain,
  NULL, &relu_1_layer, AI_STATIC, 
)

AI_TENSOR_CHAIN_OBJ_DECLARE(
  relu_chain, AI_STATIC_CONST, 4,
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 1, &linear_output),
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 1, &relu_output),
  AI_TENSOR_LIST_OBJ_EMPTY,
  AI_TENSOR_LIST_OBJ_EMPTY
)

AI_LAYER_OBJ_DECLARE(
  relu_layer, 8,
  NL_TYPE, 0x0, NULL,
  nl, forward_relu,
  &relu_chain,
  NULL, &linear_1_layer, AI_STATIC, 
  .nl_params = NULL, 
)

AI_TENSOR_CHAIN_OBJ_DECLARE(
  linear_chain, AI_STATIC_CONST, 4,
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 1, &cat_output),
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 1, &linear_output),
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 2, &linear_weights, &linear_bias),
  AI_TENSOR_LIST_OBJ_EMPTY
)

AI_LAYER_OBJ_DECLARE(
  linear_layer, 7,
  DENSE_TYPE, 0x0, NULL,
  dense, forward_dense,
  &linear_chain,
  NULL, &relu_layer, AI_STATIC, 
)

AI_TENSOR_CHAIN_OBJ_DECLARE(
  cat_chain, AI_STATIC_CONST, 4,
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 3, &mean_1_Mul_output, &getitem_output, &getitem_2_output),
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 1, &cat_output),
  AI_TENSOR_LIST_OBJ_EMPTY,
  AI_TENSOR_LIST_OBJ_EMPTY
)

AI_LAYER_OBJ_DECLARE(
  cat_layer, 6,
  CONCAT_TYPE, 0x0, NULL,
  concat, forward_concat,
  &cat_chain,
  NULL, &linear_layer, AI_STATIC, 
  .axis = AI_SHAPE_CHANNEL, 
)

AI_TENSOR_CHAIN_OBJ_DECLARE(
  mean_1_Mul_chain, AI_STATIC_CONST, 4,
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 1, &mean_1_output),
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 1, &mean_1_Mul_output),
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 2, &mean_1_Mul_scale, &mean_1_Mul_bias),
  AI_TENSOR_LIST_OBJ_EMPTY
)

AI_LAYER_OBJ_DECLARE(
  mean_1_Mul_layer, 3,
  BN_TYPE, 0x0, NULL,
  bn, forward_bn,
  &mean_1_Mul_chain,
  NULL, &cat_layer, AI_STATIC, 
)


AI_STATIC_CONST ai_float mean_1_neutral_value_data[] = { 0.0f };
AI_ARRAY_OBJ_DECLARE(
    mean_1_neutral_value, AI_ARRAY_FORMAT_FLOAT,
    mean_1_neutral_value_data, mean_1_neutral_value_data, 1, AI_STATIC_CONST)
AI_TENSOR_CHAIN_OBJ_DECLARE(
  mean_1_chain, AI_STATIC_CONST, 4,
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 1, &div_output),
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 1, &mean_1_output),
  AI_TENSOR_LIST_OBJ_EMPTY,
  AI_TENSOR_LIST_OBJ_EMPTY
)

AI_LAYER_OBJ_DECLARE(
  mean_1_layer, 3,
  REDUCE_TYPE, 0x0, NULL,
  reduce, forward_reduce,
  &mean_1_chain,
  NULL, &mean_1_Mul_layer, AI_STATIC, 
  .operation = ai_sum, 
  .neutral_value = &mean_1_neutral_value, 
)


AI_STATIC_CONST ai_float getitem_neutral_value_data[] = { -AI_FLT_MAX };
AI_ARRAY_OBJ_DECLARE(
    getitem_neutral_value, AI_ARRAY_FORMAT_FLOAT,
    getitem_neutral_value_data, getitem_neutral_value_data, 1, AI_STATIC_CONST)
AI_TENSOR_CHAIN_OBJ_DECLARE(
  getitem_chain, AI_STATIC_CONST, 4,
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 1, &div_output),
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 1, &getitem_output),
  AI_TENSOR_LIST_OBJ_EMPTY,
  AI_TENSOR_LIST_OBJ_EMPTY
)

AI_LAYER_OBJ_DECLARE(
  getitem_layer, 4,
  REDUCE_TYPE, 0x0, NULL,
  reduce, forward_reduce,
  &getitem_chain,
  NULL, &mean_1_layer, AI_STATIC, 
  .operation = ai_max, 
  .neutral_value = &getitem_neutral_value, 
)


AI_STATIC_CONST ai_float getitem_2_neutral_value_data[] = { AI_FLT_MAX };
AI_ARRAY_OBJ_DECLARE(
    getitem_2_neutral_value, AI_ARRAY_FORMAT_FLOAT,
    getitem_2_neutral_value_data, getitem_2_neutral_value_data, 1, AI_STATIC_CONST)
AI_TENSOR_CHAIN_OBJ_DECLARE(
  getitem_2_chain, AI_STATIC_CONST, 4,
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 1, &div_output),
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 1, &getitem_2_output),
  AI_TENSOR_LIST_OBJ_EMPTY,
  AI_TENSOR_LIST_OBJ_EMPTY
)

AI_LAYER_OBJ_DECLARE(
  getitem_2_layer, 5,
  REDUCE_TYPE, 0x0, NULL,
  reduce, forward_reduce,
  &getitem_2_chain,
  NULL, &getitem_layer, AI_STATIC, 
  .operation = ai_min, 
  .neutral_value = &getitem_2_neutral_value, 
)

AI_TENSOR_CHAIN_OBJ_DECLARE(
  div_chain, AI_STATIC_CONST, 4,
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 2, &sub_output, &std),
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 1, &div_output),
  AI_TENSOR_LIST_OBJ_EMPTY,
  AI_TENSOR_LIST_OBJ_EMPTY
)

AI_LAYER_OBJ_DECLARE(
  div_layer, 2,
  ELTWISE_TYPE, 0x0, NULL,
  eltwise, forward_eltwise,
  &div_chain,
  NULL, &getitem_2_layer, AI_STATIC, 
  .operation = ai_div_f32, 
  .buffer_operation = ai_div_buffer_f32, 
)

AI_TENSOR_CHAIN_OBJ_DECLARE(
  sub_chain, AI_STATIC_CONST, 4,
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 2, &voice_output, &mean),
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 1, &sub_output),
  AI_TENSOR_LIST_OBJ_EMPTY,
  AI_TENSOR_LIST_OBJ_EMPTY
)

AI_LAYER_OBJ_DECLARE(
  sub_layer, 1,
  ELTWISE_TYPE, 0x0, NULL,
  eltwise, forward_eltwise,
  &sub_chain,
  NULL, &div_layer, AI_STATIC, 
  .operation = ai_sub_f32, 
  .buffer_operation = ai_sub_buffer_f32, 
)


#if (AI_TOOLS_API_VERSION < AI_TOOLS_API_VERSION_1_5)

AI_NETWORK_OBJ_DECLARE(
  AI_NET_OBJ_INSTANCE, AI_STATIC,
  AI_BUFFER_INIT(AI_FLAG_NONE,  AI_BUFFER_FORMAT_U8,
    AI_BUFFER_SHAPE_INIT(AI_SHAPE_BCWH, 4, 1, 8480, 1, 1),
    8480, NULL, NULL),
  AI_BUFFER_INIT(AI_FLAG_NONE,  AI_BUFFER_FORMAT_U8,
    AI_BUFFER_SHAPE_INIT(AI_SHAPE_BCWH, 4, 1, 9108, 1, 1),
    9108, NULL, NULL),
  AI_TENSOR_LIST_IO_OBJ_INIT(AI_FLAG_NONE, AI_VOICE_IN_NUM, &voice_output),
  AI_TENSOR_LIST_IO_OBJ_INIT(AI_FLAG_NONE, AI_VOICE_OUT_NUM, &logits_output),
  &sub_layer, 0x5ef2a264, NULL)

#else

AI_NETWORK_OBJ_DECLARE(
  AI_NET_OBJ_INSTANCE, AI_STATIC,
  AI_BUFFER_ARRAY_OBJ_INIT_STATIC(
  	AI_FLAG_NONE, 1,
    AI_BUFFER_INIT(AI_FLAG_NONE,  AI_BUFFER_FORMAT_U8,
      AI_BUFFER_SHAPE_INIT(AI_SHAPE_BCWH, 4, 1, 8480, 1, 1),
      8480, NULL, NULL)
  ),
  AI_BUFFER_ARRAY_OBJ_INIT_STATIC(
  	AI_FLAG_NONE, 1,
    AI_BUFFER_INIT(AI_FLAG_NONE,  AI_BUFFER_FORMAT_U8,
      AI_BUFFER_SHAPE_INIT(AI_SHAPE_BCWH, 4, 1, 9108, 1, 1),
      9108, NULL, NULL)
  ),
  AI_TENSOR_LIST_IO_OBJ_INIT(AI_FLAG_NONE, AI_VOICE_IN_NUM, &voice_output),
  AI_TENSOR_LIST_IO_OBJ_INIT(AI_FLAG_NONE, AI_VOICE_OUT_NUM, &logits_output),
  &sub_layer, 0x5ef2a264, NULL)

#endif	/*(AI_TOOLS_API_VERSION < AI_TOOLS_API_VERSION_1_5)*/



/******************************************************************************/
AI_DECLARE_STATIC
ai_bool voice_configure_activations(
  ai_network* net_ctx, const ai_network_params* params)
{
  AI_ASSERT(net_ctx)

  if (ai_platform_get_activations_map(g_voice_activations_map, 1, params)) {
    /* Updating activations (byte) offsets */
    
    voice_output_array.data = AI_PTR(g_voice_activations_map[0] + 0);
    voice_output_array.data_start = AI_PTR(g_voice_activations_map[0] + 0);
    sub_output_array.data = AI_PTR(g_voice_activations_map[0] + 0);
    sub_output_array.data_start = AI_PTR(g_voice_activations_map[0] + 0);
    div_output_array.data = AI_PTR(g_voice_activations_map[0] + 0);
    div_output_array.data_start = AI_PTR(g_voice_activations_map[0] + 0);
    getitem_2_output_array.data = AI_PTR(g_voice_activations_map[0] + 9000);
    getitem_2_output_array.data_start = AI_PTR(g_voice_activations_map[0] + 9000);
    getitem_output_array.data = AI_PTR(g_voice_activations_map[0] + 9036);
    getitem_output_array.data_start = AI_PTR(g_voice_activations_map[0] + 9036);
    mean_1_output_array.data = AI_PTR(g_voice_activations_map[0] + 9072);
    mean_1_output_array.data_start = AI_PTR(g_voice_activations_map[0] + 9072);
    mean_1_Mul_output_array.data = AI_PTR(g_voice_activations_map[0] + 0);
    mean_1_Mul_output_array.data_start = AI_PTR(g_voice_activations_map[0] + 0);
    cat_output_array.data = AI_PTR(g_voice_activations_map[0] + 36);
    cat_output_array.data_start = AI_PTR(g_voice_activations_map[0] + 36);
    linear_output_array.data = AI_PTR(g_voice_activations_map[0] + 144);
    linear_output_array.data_start = AI_PTR(g_voice_activations_map[0] + 144);
    relu_output_array.data = AI_PTR(g_voice_activations_map[0] + 0);
    relu_output_array.data_start = AI_PTR(g_voice_activations_map[0] + 0);
    linear_1_output_array.data = AI_PTR(g_voice_activations_map[0] + 128);
    linear_1_output_array.data_start = AI_PTR(g_voice_activations_map[0] + 128);
    relu_1_output_array.data = AI_PTR(g_voice_activations_map[0] + 0);
    relu_1_output_array.data_start = AI_PTR(g_voice_activations_map[0] + 0);
    logits_output_array.data = AI_PTR(g_voice_activations_map[0] + 128);
    logits_output_array.data_start = AI_PTR(g_voice_activations_map[0] + 128);
    return true;
  }
  AI_ERROR_TRAP(net_ctx, INIT_FAILED, NETWORK_ACTIVATIONS);
  return false;
}




/******************************************************************************/
AI_DECLARE_STATIC
ai_bool voice_configure_weights(
  ai_network* net_ctx, const ai_network_params* params)
{
  AI_ASSERT(net_ctx)

  if (ai_platform_get_weights_map(g_voice_weights_map, 1, params)) {
    /* Updating weights (byte) offsets */
    
    std_array.format |= AI_FMT_FLAG_CONST;
    std_array.data = AI_PTR(g_voice_weights_map[0] + 0);
    std_array.data_start = AI_PTR(g_voice_weights_map[0] + 0);
    mean_array.format |= AI_FMT_FLAG_CONST;
    mean_array.data = AI_PTR(g_voice_weights_map[0] + 36);
    mean_array.data_start = AI_PTR(g_voice_weights_map[0] + 36);
    mean_1_Mul_scale_array.format |= AI_FMT_FLAG_CONST;
    mean_1_Mul_scale_array.data = AI_PTR(g_voice_weights_map[0] + 72);
    mean_1_Mul_scale_array.data_start = AI_PTR(g_voice_weights_map[0] + 72);
    mean_1_Mul_bias_array.format |= AI_FMT_FLAG_CONST;
    mean_1_Mul_bias_array.data = AI_PTR(g_voice_weights_map[0] + 108);
    mean_1_Mul_bias_array.data_start = AI_PTR(g_voice_weights_map[0] + 108);
    linear_weights_array.format |= AI_FMT_FLAG_CONST;
    linear_weights_array.data = AI_PTR(g_voice_weights_map[0] + 144);
    linear_weights_array.data_start = AI_PTR(g_voice_weights_map[0] + 144);
    linear_bias_array.format |= AI_FMT_FLAG_CONST;
    linear_bias_array.data = AI_PTR(g_voice_weights_map[0] + 3600);
    linear_bias_array.data_start = AI_PTR(g_voice_weights_map[0] + 3600);
    linear_1_weights_array.format |= AI_FMT_FLAG_CONST;
    linear_1_weights_array.data = AI_PTR(g_voice_weights_map[0] + 3728);
    linear_1_weights_array.data_start = AI_PTR(g_voice_weights_map[0] + 3728);
    linear_1_bias_array.format |= AI_FMT_FLAG_CONST;
    linear_1_bias_array.data = AI_PTR(g_voice_weights_map[0] + 7824);
    linear_1_bias_array.data_start = AI_PTR(g_voice_weights_map[0] + 7824);
    logits_weights_array.format |= AI_FMT_FLAG_CONST;
    logits_weights_array.data = AI_PTR(g_voice_weights_map[0] + 7952);
    logits_weights_array.data_start = AI_PTR(g_voice_weights_map[0] + 7952);
    logits_bias_array.format |= AI_FMT_FLAG_CONST;
    logits_bias_array.data = AI_PTR(g_voice_weights_map[0] + 8464);
    logits_bias_array.data_start = AI_PTR(g_voice_weights_map[0] + 8464);
    return true;
  }
  AI_ERROR_TRAP(net_ctx, INIT_FAILED, NETWORK_WEIGHTS);
  return false;
}


/**  PUBLIC APIs SECTION  *****************************************************/



AI_DEPRECATED
AI_API_ENTRY
ai_bool ai_voice_get_info(
  ai_handle network, ai_network_report* report)
{
  ai_network* net_ctx = AI_NETWORK_ACQUIRE_CTX(network);

  if (report && net_ctx)
  {
    ai_network_report r = {
      .model_name        = AI_VOICE_MODEL_NAME,
      .model_signature   = AI_VOICE_MODEL_SIGNATURE,
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
      
      .n_macc            = 22416,
      .n_inputs          = 0,
      .inputs            = NULL,
      .n_outputs         = 0,
      .outputs           = NULL,
      .params            = AI_STRUCT_INIT,
      .activations       = AI_STRUCT_INIT,
      .n_nodes           = 0,
      .signature         = 0x5ef2a264,
    };

    if (!ai_platform_api_get_network_report(network, &r)) return false;

    *report = r;
    return true;
  }
  return false;
}



AI_API_ENTRY
ai_bool ai_voice_get_report(
  ai_handle network, ai_network_report* report)
{
  ai_network* net_ctx = AI_NETWORK_ACQUIRE_CTX(network);

  if (report && net_ctx)
  {
    ai_network_report r = {
      .model_name        = AI_VOICE_MODEL_NAME,
      .model_signature   = AI_VOICE_MODEL_SIGNATURE,
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
      
      .n_macc            = 22416,
      .n_inputs          = 0,
      .inputs            = NULL,
      .n_outputs         = 0,
      .outputs           = NULL,
      .map_signature     = AI_MAGIC_SIGNATURE,
      .map_weights       = AI_STRUCT_INIT,
      .map_activations   = AI_STRUCT_INIT,
      .n_nodes           = 0,
      .signature         = 0x5ef2a264,
    };

    if (!ai_platform_api_get_network_report(network, &r)) return false;

    *report = r;
    return true;
  }
  return false;
}


AI_API_ENTRY
ai_error ai_voice_get_error(ai_handle network)
{
  return ai_platform_network_get_error(network);
}


AI_API_ENTRY
ai_error ai_voice_create(
  ai_handle* network, const ai_buffer* network_config)
{
  return ai_platform_network_create(
    network, network_config, 
    AI_CONTEXT_OBJ(&AI_NET_OBJ_INSTANCE),
    AI_TOOLS_API_VERSION_MAJOR, AI_TOOLS_API_VERSION_MINOR, AI_TOOLS_API_VERSION_MICRO);
}


AI_API_ENTRY
ai_error ai_voice_create_and_init(
  ai_handle* network, const ai_handle activations[], const ai_handle weights[])
{
  ai_error err;
  ai_network_params params;

  err = ai_voice_create(network, AI_VOICE_DATA_CONFIG);
  if (err.type != AI_ERROR_NONE) {
    return err;
  }
  
  if (ai_voice_data_params_get(&params) != true) {
    err = ai_voice_get_error(*network);
    return err;
  }
#if defined(AI_VOICE_DATA_ACTIVATIONS_COUNT)
  /* set the addresses of the activations buffers */
  for (ai_u16 idx=0; activations && idx<params.map_activations.size; idx++) {
    AI_BUFFER_ARRAY_ITEM_SET_ADDRESS(&params.map_activations, idx, activations[idx]);
  }
#endif
#if defined(AI_VOICE_DATA_WEIGHTS_COUNT)
  /* set the addresses of the weight buffers */
  for (ai_u16 idx=0; weights && idx<params.map_weights.size; idx++) {
    AI_BUFFER_ARRAY_ITEM_SET_ADDRESS(&params.map_weights, idx, weights[idx]);
  }
#endif
  if (ai_voice_init(*network, &params) != true) {
    err = ai_voice_get_error(*network);
  }
  return err;
}


AI_API_ENTRY
ai_buffer* ai_voice_inputs_get(ai_handle network, ai_u16 *n_buffer)
{
  if (network == AI_HANDLE_NULL) {
    network = (ai_handle)&AI_NET_OBJ_INSTANCE;
    AI_NETWORK_OBJ(network)->magic = AI_MAGIC_CONTEXT_TOKEN;
  }
  return ai_platform_inputs_get(network, n_buffer);
}


AI_API_ENTRY
ai_buffer* ai_voice_outputs_get(ai_handle network, ai_u16 *n_buffer)
{
  if (network == AI_HANDLE_NULL) {
    network = (ai_handle)&AI_NET_OBJ_INSTANCE;
    AI_NETWORK_OBJ(network)->magic = AI_MAGIC_CONTEXT_TOKEN;
  }
  return ai_platform_outputs_get(network, n_buffer);
}


AI_API_ENTRY
ai_handle ai_voice_destroy(ai_handle network)
{
  return ai_platform_network_destroy(network);
}


AI_API_ENTRY
ai_bool ai_voice_init(
  ai_handle network, const ai_network_params* params)
{
  ai_network* net_ctx = AI_NETWORK_OBJ(ai_platform_network_init(network, params));
  ai_bool ok = true;

  if (!net_ctx) return false;
  ok &= voice_configure_weights(net_ctx, params);
  ok &= voice_configure_activations(net_ctx, params);

  ok &= ai_platform_network_post_init(network);

  return ok;
}


AI_API_ENTRY
ai_i32 ai_voice_run(
  ai_handle network, const ai_buffer* input, ai_buffer* output)
{
  return ai_platform_network_process(network, input, output);
}


AI_API_ENTRY
ai_i32 ai_voice_forward(ai_handle network, const ai_buffer* input)
{
  return ai_platform_network_process(network, input, NULL);
}



#undef AI_VOICE_MODEL_SIGNATURE
#undef AI_NET_OBJ_INSTANCE
#undef AI_TOOLS_DATE_TIME
#undef AI_TOOLS_COMPILE_TIME


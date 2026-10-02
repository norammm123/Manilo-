#include "inmp441.h"

extern I2S_HandleTypeDef hi2s3;
#include "usart.h"
#define ARM_MATH_CM4
#include "arm_math.h"
/*
 * I2S 接收缓冲区
 *
 * CubeMX 配置：
 * I2S3
 * 24 Bits Data on 32 Bits Frame
 * DMA Circular
 *
 * HAL_I2S_Receive_DMA 使用 uint16_t 接收。
 *
 * 一个完整立体声帧通常对应 4 个 uint16_t：
 * buf[i + 0]：左声道高 16 位
 * buf[i + 1]：左声道低 16 位
 * buf[i + 2]：右声道高 16 位
 * buf[i + 3]：右声道低 16 位
 */
static uint16_t inmp441_rx_buf[INMP441_RX_BUF_SIZE];

INMP441_TypeDef inmp441;

#define INMP441_RAW_LIMIT          1000000U
#define INMP441_LEVEL_LIMIT        200000U
#define INMP441_SPIKE_MARGIN       50000U
#define INMP441_ENERGY_SAMPLE_SHIFT 4U
#define INMP441_FFT_SIZE            128U
#define INMP441_FFT_BIN_COUNT       (INMP441_FFT_SIZE / 2U)
#define INMP441_SAMPLE_RATE_HZ      16000U
#define INMP441_FFT_INPUT_SCALE     32768.0f
#define INMP441_PI                  3.14159265358979f

extern volatile uint8_t sound_inference_ready;
extern uint16_t voice_cnt;
extern uint16_t voice_valid_count;
extern uint16_t voice_step_cnt;
extern uint32_t voice_buffer[250][INMP441_VOICE_FEATURE_COUNT];

#define INMP441_VOICE_BLOCK_COUNT  250U
#define INMP441_VOICE_INFER_STEP   25U
#define INMP441_VOICE_WARMUP_BLOCKS 2U
#define INMP441_DMA_FRAME_COUNT    (INMP441_RX_BUF_SIZE / 2U)

static uint8_t inmp441_voice_warmup;
static uint8_t inmp441_fft_ready;
static arm_rfft_fast_instance_f32 inmp441_rfft;
static float32_t inmp441_fft_input[INMP441_FFT_SIZE];
static float32_t inmp441_fft_output[INMP441_FFT_SIZE];
static float32_t inmp441_fft_window[INMP441_FFT_SIZE];

static void INMP441_ResetVoiceCapture(void);
static void INMP441_SaveVoiceFeature(void);

static void INMP441_ResetVoiceCapture(void)
{
    voice_cnt = 0;
    voice_valid_count = 0;
    voice_step_cnt = 0;
    sound_inference_ready = 0;
    inmp441_voice_warmup = INMP441_VOICE_WARMUP_BLOCKS;
}
/**
 * @brief  将 I2S 接收到的两个 uint16_t 合成为 24bit 有符号音频数据
 * @param  high16 高 16 位
 * @param  low16  低 16 位
 * @retval 24bit 有符号采样值
 */
static int32_t INMP441_ConvertSample(uint16_t high16, uint16_t low16)
{
    uint32_t raw24;

    raw24 = (((uint32_t)high16 << 16) | low16) >> 8;

    if (raw24 & 0x00800000U)
    {
        raw24 |= 0xFF000000U;
    }

    return (int32_t)raw24;
}

/**
 * @brief  int32 绝对值
 */
static uint32_t INMP441_Abs32(int32_t x)
{
    if (x >= 0)
    {
        return (uint32_t)x;
    }
    else
    {
        return (uint32_t)(-x);
    }
}

static uint8_t INMP441_IsValidSample(int32_t sample)
{
    return (INMP441_Abs32(sample) <= INMP441_RAW_LIMIT);
}

static int8_t INMP441_Sign32(int32_t x)
{
    if (x > 0)
    {
        return 1;
    }

    if (x < 0)
    {
        return -1;
    }

    return 0;
}

static uint32_t INMP441_SaturateU32(uint64_t value)
{
    if (value > 0xFFFFFFFFULL)
    {
        return 0xFFFFFFFFU;
    }

    return (uint32_t)value;
}

static uint32_t INMP441_FloatToU32(float32_t value)
{
    if (value <= 0.0f)
    {
        return 0U;
    }

    if (value >= 4294967295.0f)
    {
        return 0xFFFFFFFFU;
    }

    return (uint32_t)(value + 0.5f);
}

static void INMP441_ClearFftFeatures(void)
{
    inmp441.fft_low_energy = 0;
    inmp441.fft_mid_energy = 0;
    inmp441.fft_high_energy = 0;
    inmp441.spectral_centroid = 0;
    inmp441.dominant_freq = 0;
}

static void INMP441_InitFft(void)
{
    uint32_t i;

    inmp441_fft_ready = 0;

    if (arm_rfft_fast_init_f32(&inmp441_rfft, INMP441_FFT_SIZE) == ARM_MATH_SUCCESS)
    {
        for (i = 0; i < INMP441_FFT_SIZE; i++)
        {
            inmp441_fft_window[i] = 0.54f -
                                    (0.46f * arm_cos_f32((2.0f * INMP441_PI * (float32_t)i) /
                                                         (float32_t)(INMP441_FFT_SIZE - 1U)));
        }
        inmp441_fft_ready = 1;
    }
}

static void INMP441_CalculateFftFeatures(uint32_t sample_count)
{
    uint32_t bin;
    uint32_t bin_hz;
    uint32_t dominant_bin = 0;
    float32_t re;
    float32_t im;
    float32_t power;
    float32_t low_sum = 0.0f;
    float32_t mid_sum = 0.0f;
    float32_t high_sum = 0.0f;
    float32_t total_power = 0.0f;
    float32_t centroid_sum = 0.0f;
    float32_t dominant_power = 0.0f;

    if ((inmp441_fft_ready == 0U) || (sample_count < INMP441_FFT_SIZE))
    {
        INMP441_ClearFftFeatures();
        return;
    }

    arm_rfft_fast_f32(&inmp441_rfft, inmp441_fft_input, inmp441_fft_output, 0);

    for (bin = 1; bin < INMP441_FFT_BIN_COUNT; bin++)
    {
        re = inmp441_fft_output[bin * 2U];
        im = inmp441_fft_output[(bin * 2U) + 1U];
        power = (re * re) + (im * im);
        bin_hz = (bin * INMP441_SAMPLE_RATE_HZ) / INMP441_FFT_SIZE;

        total_power += power;
        centroid_sum += power * (float32_t)bin_hz;

        if (bin_hz <= 500U)
        {
            low_sum += power;
        }
        else if (bin_hz <= 2000U)
        {
            mid_sum += power;
        }
        else if (bin_hz <= 4000U)
        {
            high_sum += power;
        }

        if (power > dominant_power)
        {
            dominant_power = power;
            dominant_bin = bin;
        }
    }

    inmp441.fft_low_energy = INMP441_FloatToU32(low_sum);
    inmp441.fft_mid_energy = INMP441_FloatToU32(mid_sum);
    inmp441.fft_high_energy = INMP441_FloatToU32(high_sum);
    inmp441.dominant_freq = (dominant_bin * INMP441_SAMPLE_RATE_HZ) / INMP441_FFT_SIZE;

    if (total_power > 0.0f)
    {
        inmp441.spectral_centroid = INMP441_FloatToU32(centroid_sum / total_power);
    }
    else
    {
        inmp441.spectral_centroid = 0;
    }
}

static void INMP441_ClearFrameFeatures(void)
{
    inmp441.energy = 0;
    inmp441.zcr = 0;
    inmp441.peak_avg_ratio = 0;
    INMP441_ClearFftFeatures();
}

static uint32_t INMP441_ClampGlitch(uint32_t previous, uint32_t current)
{
    if (current > INMP441_LEVEL_LIMIT)
    {
        inmp441.glitch_count++;
        if (previous == 0U)
        {
            return INMP441_LEVEL_LIMIT;
        }
        return previous;
    }

    if ((previous > 0U) && (current > ((previous * 8U) + INMP441_SPIKE_MARGIN)))
    {
        inmp441.glitch_count++;
        return previous;
    }

    return current;
}

static uint32_t INMP441_HoldZeroDrop(uint32_t previous, uint32_t current)
{
    if ((current == 0U) && (previous > 0U))
    {
        inmp441.glitch_count++;
        return previous - (previous >> 3);
    }

    return current;
}

/**
 * @brief  处理一段 DMA 缓冲区
 * @param  buf 缓冲区起始地址
 * @param  len 缓冲区长度，单位 uint16_t
 */
static void INMP441_ProcessBuffer(uint16_t *buf, uint32_t len)
{
    uint32_t i;
    int64_t left_dc_sum = 0;
    int64_t right_dc_sum = 0;
    uint64_t left_sum = 0;
    uint64_t right_sum = 0;
    uint32_t left_peak = 0;
    uint32_t right_peak = 0;
    uint32_t left_count = 0;
    uint32_t right_count = 0;
    int32_t left_dc = 0;
    int32_t right_dc = 0;

    int32_t left_sample;
    int32_t right_sample;
    int32_t left_ac;
    int32_t right_ac;
    uint32_t left_abs;
    uint32_t right_abs;
    uint32_t selected_level;
    uint32_t selected_peak;
    int32_t selected_sample;
    uint8_t selected_channel;
    int32_t selected_dc;
    int32_t ac_sample;
    uint32_t feature_count = 0;
    uint64_t energy_sum = 0;
    uint32_t zcr_count = 0;
    int8_t last_sign = 0;
    int8_t current_sign;
    uint32_t energy_sample;
    uint32_t fft_sample_count = 0;

    for (i = 0; i + 3 < len; i += 4)
    {
        left_sample = INMP441_ConvertSample(buf[i], buf[i + 1]);
        right_sample = INMP441_ConvertSample(buf[i + 2], buf[i + 3]);

        inmp441.last_left_sample = left_sample;
        inmp441.last_right_sample = right_sample;

        if (INMP441_IsValidSample(left_sample))
        {
            left_dc_sum += left_sample;
            left_count++;
        }
        else
        {
            inmp441.invalid_count++;
        }

        if (INMP441_IsValidSample(right_sample))
        {
            right_dc_sum += right_sample;
            right_count++;
        }
        else
        {
            inmp441.invalid_count++;
        }
    }

    if (left_count > 0U)
    {
        left_dc = (int32_t)(left_dc_sum / left_count);
    }

    if (right_count > 0U)
    {
        right_dc = (int32_t)(right_dc_sum / right_count);
    }

    left_count = 0;
    right_count = 0;

    for (i = 0; i + 3 < len; i += 4)
    {
        left_sample = INMP441_ConvertSample(buf[i], buf[i + 1]);
        right_sample = INMP441_ConvertSample(buf[i + 2], buf[i + 3]);

        if (INMP441_IsValidSample(left_sample))
        {
            left_ac = left_sample - left_dc;
            left_abs = INMP441_Abs32(left_ac);
            left_sum += left_abs;
            left_count++;

            if (left_abs > left_peak)
            {
                left_peak = left_abs;
            }
        }

        if (INMP441_IsValidSample(right_sample))
        {
            right_ac = right_sample - right_dc;
            right_abs = INMP441_Abs32(right_ac);
            right_sum += right_abs;
            right_count++;

            if (right_abs > right_peak)
            {
                right_peak = right_abs;
            }
        }
    }

    if ((left_count > 0U) || (right_count > 0U))
    {
        inmp441.left_level = left_count > 0 ? (uint32_t)(left_sum / left_count) : 0;
        inmp441.right_level = right_count > 0 ? (uint32_t)(right_sum / right_count) : 0;
        inmp441.left_peak = left_peak;
        inmp441.right_peak = right_peak;
        inmp441.left_dc = left_dc;
        inmp441.right_dc = right_dc;
        inmp441.left_valid_count = left_count;
        inmp441.right_valid_count = right_count;

#if (INMP441_USE_CHANNEL == INMP441_CHANNEL_LEFT)
        selected_channel = INMP441_CHANNEL_LEFT;
#elif (INMP441_USE_CHANNEL == INMP441_CHANNEL_RIGHT)
        selected_channel = INMP441_CHANNEL_RIGHT;
#else
        if ((right_peak > 0U) && (left_peak == 0U))
        {
            selected_channel = INMP441_CHANNEL_RIGHT;
        }
        else if ((left_peak > 0U) && (right_peak == 0U))
        {
            selected_channel = INMP441_CHANNEL_LEFT;
        }
        else if ((right_count > left_count) ||
                 ((right_count == left_count) && (right_peak > ((left_peak * 2U) + 100U))))
        {
            selected_channel = INMP441_CHANNEL_RIGHT;
        }
        else
        {
            selected_channel = INMP441_CHANNEL_LEFT;
        }
#endif

        if (selected_channel == INMP441_CHANNEL_RIGHT)
        {
            selected_level = inmp441.right_level;
            selected_peak = inmp441.right_peak;
            selected_sample = inmp441.last_right_sample;
            selected_dc = inmp441.right_dc;
        }
        else
        {
            selected_level = inmp441.left_level;
            selected_peak = inmp441.left_peak;
            selected_sample = inmp441.last_left_sample;
            selected_dc = inmp441.left_dc;
        }

        for (i = 0; i + 3 < len; i += 4)
        {
            if (selected_channel == INMP441_CHANNEL_RIGHT)
            {
                selected_sample = INMP441_ConvertSample(buf[i + 2], buf[i + 3]);
            }
            else
            {
                selected_sample = INMP441_ConvertSample(buf[i], buf[i + 1]);
            }

            if (INMP441_IsValidSample(selected_sample))
            {
                ac_sample = selected_sample - selected_dc;
                energy_sample = INMP441_Abs32(ac_sample) >> INMP441_ENERGY_SAMPLE_SHIFT;
                energy_sum += (uint64_t)energy_sample * (uint64_t)energy_sample;

                current_sign = INMP441_Sign32(ac_sample);
                if (current_sign != 0)
                {
                    if ((last_sign != 0) && (current_sign != last_sign))
                    {
                        zcr_count++;
                    }
                    last_sign = current_sign;
                }

                if (fft_sample_count < INMP441_FFT_SIZE)
                {
                    inmp441_fft_input[fft_sample_count] =
                        (((float32_t)ac_sample) / INMP441_FFT_INPUT_SCALE) *
                        inmp441_fft_window[fft_sample_count];
                    fft_sample_count++;
                }
                feature_count++;
            }
        }

        while (fft_sample_count < INMP441_FFT_SIZE)
        {
            inmp441_fft_input[fft_sample_count] = 0.0f;
            fft_sample_count++;
        }

        inmp441.active_channel = selected_channel;
        inmp441.level = INMP441_ClampGlitch(inmp441.level, selected_level);
        inmp441.peak = INMP441_HoldZeroDrop(inmp441.peak,
                                            INMP441_ClampGlitch(inmp441.peak, selected_peak));
        if (feature_count > 0U)
        {
            inmp441.energy = INMP441_SaturateU32(energy_sum / feature_count);
            inmp441.zcr = (feature_count > 1U) ? ((zcr_count * 1000U) / (feature_count - 1U)) : 0U;
            inmp441.peak_avg_ratio = (selected_level > 0U) ?
                                     INMP441_SaturateU32(((uint64_t)selected_peak * 1000ULL) / selected_level) :
                                     0U;
            INMP441_CalculateFftFeatures(feature_count);
        }
        else
        {
            INMP441_ClearFrameFeatures();
        }
        inmp441.last_sample = selected_sample;
    }
    else
    {
        INMP441_ClearFrameFeatures();
    }
}

/**
 * @brief 初始化 INMP441 软件状态
 */
void INMP441_Init(void)
{
    inmp441.half_ready = 0;
    inmp441.full_ready = 0;
    inmp441.is_running = 0;
    INMP441_ResetVoiceCapture();
    INMP441_InitFft();

    inmp441.last_sample = 0;
    inmp441.last_left_sample = 0;
    inmp441.last_right_sample = 0;
    inmp441.level = 0;
    inmp441.peak = 0;
    INMP441_ClearFrameFeatures();
    inmp441.left_level = 0;
    inmp441.right_level = 0;
    inmp441.left_peak = 0;
    inmp441.right_peak = 0;
    inmp441.left_dc = 0;
    inmp441.right_dc = 0;
    inmp441.left_valid_count = 0;
    inmp441.right_valid_count = 0;
    inmp441.half_count = 0;
    inmp441.full_count = 0;
    inmp441.error_count = 0;
    inmp441.last_error = 0;
    inmp441.invalid_count = 0;
    inmp441.glitch_count = 0;
    inmp441.active_channel = INMP441_USE_CHANNEL;
}

/**
 * @brief 启动 I2S DMA 接收
 */
HAL_StatusTypeDef INMP441_Start(void)
{
    HAL_StatusTypeDef ret;

    INMP441_ResetVoiceCapture();
    inmp441.half_ready = 0;
    inmp441.full_ready = 0;
    inmp441.level = 0;
    inmp441.peak = 0;
    inmp441.last_sample = 0;
    inmp441.last_left_sample = 0;
    inmp441.last_right_sample = 0;
    inmp441.left_level = 0;
    inmp441.right_level = 0;
    inmp441.left_peak = 0;
    inmp441.right_peak = 0;
    INMP441_ClearFrameFeatures();
    inmp441.left_dc = 0;
    inmp441.right_dc = 0;
    inmp441.left_valid_count = 0;
    inmp441.right_valid_count = 0;
    inmp441.half_count = 0;
    inmp441.full_count = 0;
    inmp441.error_count = 0;
    inmp441.last_error = 0;
    inmp441.invalid_count = 0;
    inmp441.glitch_count = 0;
    inmp441.active_channel = INMP441_USE_CHANNEL;

    ret = HAL_I2S_Receive_DMA(&hi2s3,
                              inmp441_rx_buf,
                              INMP441_DMA_FRAME_COUNT);

    if (ret == HAL_OK)
    {
        inmp441.is_running = 1;
    }

    return ret;
}

/**
 * @brief 停止 I2S DMA 接收
 */
HAL_StatusTypeDef INMP441_Stop(void)
{
    HAL_StatusTypeDef ret;

    ret = HAL_I2S_DMAStop(&hi2s3);

    if (ret == HAL_OK)
    {
        inmp441.is_running = 0;
    }

    return ret;
}

static void INMP441_SaveVoiceFeature(void)
{
    if (sound_inference_ready)
    {
        return;
    }

    if (inmp441_voice_warmup > 0U)
    {
        inmp441_voice_warmup--;
        return;
    }

    voice_buffer[voice_cnt][0] = inmp441.peak;
    voice_buffer[voice_cnt][1] = inmp441.energy;
    voice_buffer[voice_cnt][2] = inmp441.zcr;
    voice_buffer[voice_cnt][3] = inmp441.peak_avg_ratio;
    voice_buffer[voice_cnt][4] = inmp441.fft_low_energy;
    voice_buffer[voice_cnt][5] = inmp441.fft_mid_energy;
    voice_buffer[voice_cnt][6] = inmp441.fft_high_energy;
    voice_buffer[voice_cnt][7] = inmp441.spectral_centroid;
    voice_buffer[voice_cnt][8] = inmp441.dominant_freq;
    voice_cnt++;

    if (voice_cnt >= INMP441_VOICE_BLOCK_COUNT)
    {
        voice_cnt = 0;
    }

    if (voice_valid_count < INMP441_VOICE_BLOCK_COUNT)
    {
        voice_valid_count++;
    }

    if (voice_valid_count >= INMP441_VOICE_BLOCK_COUNT)
    {
        voice_step_cnt++;
        if (voice_step_cnt >= INMP441_VOICE_INFER_STEP)
        {
            voice_step_cnt = 0;
            sound_inference_ready = 1;
        }
    }
}
/**
 * @brief 麦克风任务函数，需要在 while(1) 中不断调用
 */
void INMP441_Task(void)
{
    if (sound_inference_ready)
    {
        return;
    }

    if (inmp441.half_ready)
    {
        inmp441.half_ready = 0;
        INMP441_ProcessBuffer(&inmp441_rx_buf[0],
                              INMP441_RX_BUF_SIZE / 2);
        INMP441_SaveVoiceFeature();
    }

    if (inmp441.full_ready)
    {
        if (sound_inference_ready)
        {
            return;
        }

        inmp441.full_ready = 0;
        INMP441_ProcessBuffer(&inmp441_rx_buf[INMP441_RX_BUF_SIZE / 2],
                              INMP441_RX_BUF_SIZE / 2);
        INMP441_SaveVoiceFeature();
    }
}

uint32_t INMP441_GetLevel(void)
{
    return inmp441.level;
}

uint32_t INMP441_GetPeak(void)
{
    return inmp441.peak;
}

uint32_t INMP441_GetEnergy(void)
{
    return inmp441.energy;
}

uint32_t INMP441_GetZcr(void)
{
    return inmp441.zcr;
}

uint32_t INMP441_GetPeakAvgRatio(void)
{
    return inmp441.peak_avg_ratio;
}

int32_t INMP441_GetLastSample(void)
{
    return inmp441.last_sample;
}

/**
 * @brief I2S DMA 半满回调转接函数
 */
void INMP441_RxHalfCpltCallback(I2S_HandleTypeDef *hi2s)
{
    if (hi2s->Instance == SPI3)
    {
        inmp441.half_count++;
        inmp441.half_ready = 1;
    }
}

/**
 * @brief I2S DMA 全满回调转接函数
 */
void INMP441_RxCpltCallback(I2S_HandleTypeDef *hi2s)
{
    if (hi2s->Instance == SPI3)
    {
        inmp441.full_count++;
        inmp441.full_ready = 1;
    }
}

void INMP441_ErrorCallback(I2S_HandleTypeDef *hi2s)
{
    if (hi2s->Instance == SPI3)
    {
        inmp441.error_count++;
        inmp441.last_error = hi2s->ErrorCode;
    }
}

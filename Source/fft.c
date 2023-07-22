#include "fft.h"

#include <stdlib.h>
#include <string.h>

#include <arm_math.h>
#include <arm_math_types.h>

#include "adc.h"
#include "window.h"

#include "vofa.h"

/* 选择窗函数 */
// #define FFT_WINDOW_FUNCTION arm_blackman_harris_92db_f32
// #define FFT_WINDOW_FUNCTION arm_hanning_f32

static arm_rfft_fast_instance_f32 fast_rfft_instance;

float32_t FFT_AMP[FFT_OUTPUT_SIZE];
float32_t FFT_PHASE[FFT_OUTPUT_SIZE];
// 相位计算写在外部
float32_t FFT_OUTPUT_RAW[ADC_SAMPLE_SIZE];

void fft_init(void) {
  // 初始化窗函数
  // 初始化FFT
  arm_rfft_fast_init_f32(&fast_rfft_instance, ADC_SAMPLE_SIZE);
}

void fft_with_window() {
  // 加窗
  for (uint16_t i = 0; i < ADC_SAMPLE_SIZE; i++) {
    SAMPLE_DATA[i] *= BLACKMAN_HARRIS_WINDOW[i];
  }
  // 进行FFT
  arm_rfft_fast_f32(&fast_rfft_instance, (float32_t *)SAMPLE_DATA, (float32_t *)FFT_OUTPUT_RAW, 0);

  // 归一化（防止溢出）
  for (uint16_t i = 0; i < ADC_SAMPLE_SIZE; i++) {
    FFT_OUTPUT_RAW[i] /= ADC_SAMPLE_SIZE / 5.0f;
  }

  // 计算幅值
  FFT_AMP[0]               = fabs(FFT_OUTPUT_RAW[0]);
  FFT_AMP[FFT_OUTPUT_SIZE - 1] = fabs(FFT_OUTPUT_RAW[1]);
  arm_cmplx_mag_f32(FFT_OUTPUT_RAW + 2, FFT_AMP + 1, FFT_OUTPUT_SIZE - 2);
}

#include "fft.h"

#include <stdlib.h>
#include <string.h>

#include <arm_math_f16.h>
#include <arm_math_types_f16.h>
#include <dsp/window_functions.h>

#include "adc.h"

/* 选择窗函数 */
#define FFT_WINDOW_FUNCTION arm_blackman_harris_92db_f32
// #define FFT_WINDOW_FUNCTION arm_hanning_f32

static float32_t window[ADC_SAMPLE_SIZE];

static arm_rfft_fast_instance_f32 fast_rfft_instance;

float32_t FFT_OUTPUT[ADC_SAMPLE_SIZE / 2 + 1];
float32_t FFT_PHASE[ADC_SAMPLE_SIZE / 2 + 1];

void fft_init(void) {
  // 初始化窗函数
  FFT_WINDOW_FUNCTION(window, ADC_SAMPLE_SIZE);
  // 初始化FFT
  arm_rfft_fast_init_f32(&fast_rfft_instance, ADC_SAMPLE_SIZE);
}

void fft_with_window() {
  // 加窗
  for (uint16_t i = 0; i < ADC_SAMPLE_SIZE; i++) {
    SAMPLE_DATA[i] *= window[i];
  }

  // 进行FFT
  float32_t fft_output_raw[ADC_SAMPLE_SIZE];
  arm_rfft_fast_f32(&fast_rfft_instance, (float32_t *)SAMPLE_DATA, (float32_t *)fft_output_raw, 0);

  // 归一化（防止溢出）
  for (uint16_t i = 0; i < ADC_SAMPLE_SIZE; i++) {
    fft_output_raw[i] /= ADC_SAMPLE_SIZE / 5.0f;
  }

  // 计算幅值
  FFT_OUTPUT[0]                   = fft_output_raw[0];
  FFT_OUTPUT[ADC_SAMPLE_SIZE / 2] = fft_output_raw[1];
  arm_cmplx_mag_f32(fft_output_raw + 2, FFT_OUTPUT + 1, ADC_SAMPLE_SIZE / 2 - 1);

  // 计算相位
  FFT_PHASE[0]                   = 0;
  FFT_PHASE[ADC_SAMPLE_SIZE / 2] = 0;
  for (uint16_t i = 1; i < ADC_SAMPLE_SIZE / 2; i++) {
    if (FFT_OUTPUT[i] > 8e-2) {
      arm_atan2_f32(fft_output_raw[2 * i + 1], fft_output_raw[2 * i], FFT_PHASE + i);
    } else {
      FFT_PHASE[i] = 0; // 去除噪声
    }
  }
}

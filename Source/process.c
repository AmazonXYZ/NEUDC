#include "process.h"
#include <driverlib/driverlib.h>

#include <arm_math_f16.h>
#include <arm_math_types.h>
#include <arm_math_types_f16.h>

#include "adc.h"
#include "fft.h"

#include "vofa.h"

extern volatile uint8_t STATE_CODE;

#define INDEX_RADIUS 8

static float32_t sample_freq_k = 1000;
static float32_t fundamental_freq;
static float32_t freq_resolution = (1000 * 1e3 / 4096);

float32_t AMPLITUDE[5];
float32_t PHASE[5];
float32_t THD;

void update(uint16_t freq) {
  adc_run(freq);
  while (STATE_CODE == 0x01) {
    __no_operation();
  }
  adc_stop();

  // 规范化并转浮点数
  adc_sacle();

  // FFT处理
  fft_with_window();

  // 更新频谱分辨率
  freq_resolution = (sample_freq_k * 1e3 / 4096);
}

void preprocess() {

  update(1000);

  // 估算基频
  float32_t _;
  uint32_t  fw_freq_index;
  arm_max_f32(FFT_OUTPUT + 1, ADC_SAMPLE_SIZE / 2, &_, &fw_freq_index);
  fundamental_freq = fw_freq_index * freq_resolution;
}

void process(uint8_t time) {
  // 首先计算需要采样的谐波频率
  float32_t freq = fundamental_freq * (time + 1);

  // 确定最佳采样率（和分辨率）
  uint16_t sample_freq_selected_k;
  if (freq > 125e3) {
    sample_freq_selected_k = 1000;
    sample_freq_k          = 1000;

  } else if (freq > 50e3) {
    sample_freq_selected_k = 500;
    sample_freq_k          = 500;

  } else if (freq > 25e3) {
    sample_freq_selected_k = 250;
    sample_freq_k          = 250;

  } else if (freq > 12.5e3) {
    sample_freq_selected_k = 125;
    sample_freq_k          = 125;

  } else if (freq > 5e3) {
    sample_freq_selected_k = 62;
    sample_freq_k          = 62.5;

  } else {
    sample_freq_selected_k = 25;
    sample_freq_k          = 25;
  }

  // 重新采样
  update(sample_freq_selected_k);

  // 计算大致分布位置
  uint32_t index = round(freq / freq_resolution);
  // 暴力区间搜索
  float32_t *interval_start = FFT_OUTPUT + index - INDEX_RADIUS;

  // 直接取最大值
  arm_max_f32(interval_start, INDEX_RADIUS * 2 + 1, AMPLITUDE + time, &index); // 幅值
  PHASE[time] = FFT_PHASE[index];                                              // 相位

  // 如果是基频，就更新基频
  if (time == 0) {
    fundamental_freq = index * freq_resolution;
  }

  // float32_t  trigger_amplitude;
  // arm_mean_f32(interval_start, INDEX_RADIUS * 2, &trigger_amplitude);

  // //
  // 求出有效加权和。最大值有更高的权重(此处无需额外处理，因为幅度没有作标准变换，已经存在权重分配)
  // float32_t amplitude_sum = 0;
  // float32_t weighted_sum  = 0;
  // for (uint8_t i = 0; i <= 2 * INDEX_RADIUS; i++) {
  //   float32_t amplitude = *(interval_start + i);

  //   if (amplitude > trigger_amplitude) {
  //     weighted_sum += i * amplitude;
  //   }
  // }
}
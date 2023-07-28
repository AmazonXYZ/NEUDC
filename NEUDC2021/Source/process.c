#include "process.h"
#include <driverlib/driverlib.h>

#include <arm_math.h>
#include <arm_math_types.h>

#include "adc.h"
#include "bluetooth_serial.h"
#include "fft.h"

extern volatile uint8_t STATE_CODE;

#define INDEX_RADIUS       8
#define FILTER_WINDOW_SIZE 8

static float32_t fundamental_freq;
static float32_t freq_resolution;

bool SAMPLE_FLAG = true;

float32_t hw_amp[5];
float32_t hw_phase[5];

static float32_t amp_window[5][FILTER_WINDOW_SIZE] = {{0.5}};
static uint8_t   amp_window_offset                 = 0;

void update(float32_t freq) {
  SAMPLE_FLAG = true;

  float32_t sample_freq;
  adc_run(freq, &sample_freq);

  while (SAMPLE_FLAG) {
    __no_operation();
  }
  adc_stop();

  // 规范化并转浮点数
  adc_sacle();

  // FFT处理
  fft_with_window();

  // 更新频谱分辨率
  freq_resolution = (sample_freq / 4096);
}

void preprocess() {

  update(500e3);

  // 估算基频
  float32_t _;
  uint32_t  fw_freq_index;
  arm_max_f32(FFT_AMP + 1, FFT_OUTPUT_SIZE - 1, &_, &fw_freq_index);
  fw_freq_index++; // 因为最大幅度的查找没有包括在0偏移位置的直流分量，此处对索引补偿1

  fundamental_freq = fw_freq_index * freq_resolution;

  // 防止基频过小
  if (fundamental_freq < 1e3) {
    fundamental_freq = 1e3;
  }

  // 由于相位数据无法利用统计方法处理减小误差，这里直接算出基频相位
  // 使用最大五次谐波的频率做参考采样
  update(fundamental_freq * 5);

  // 计算相位
  FFT_PHASE[0]                   = 0;
  FFT_PHASE[FFT_OUTPUT_SIZE - 1] = 0;
  for (uint16_t i = 1; i < FFT_OUTPUT_SIZE - 1; i++) {
    if (FFT_AMP[i] > 8e-2) {
      arm_atan2_f32(FFT_OUTPUT_RAW[2 * i + 1], FFT_OUTPUT_RAW[2 * i], FFT_PHASE + i);
    } else {
      FFT_PHASE[i] = 0; // 去除噪声
    }
  }

  for (uint8_t time = 0; time < 5; time++) {
    uint32_t index = round(fundamental_freq * (time + 1) / freq_resolution);
    hw_phase[time] = FFT_PHASE[index]; // 相位

    // 顺便窗口初始化
    for (uint8_t i = 0; i < FILTER_WINDOW_SIZE; i++) {
      amp_window[time][i] = FFT_AMP[index];
    }
  }

  // 发送相位数据
  bluetooth_printf(
      "<%f,%f,%f,%f,%f:p>", hw_phase[0], hw_phase[1], hw_phase[2], hw_phase[3], hw_phase[4]
  );
}

void process(uint8_t time) {
  // 首先计算需要采样的谐波频率
  float32_t target_freq = fundamental_freq * (time + 1);

  // 重新采样
  update(target_freq);

  // 计算大致分布位置
  uint32_t index = round(target_freq / freq_resolution);

  // 直接取区间最大值
  uint32_t index_offset;

  // 边界处理
  uint8_t    search_size  = 2 * INDEX_RADIUS + 1;
  float32_t *search_start = FFT_AMP - INDEX_RADIUS + index;

  if (INDEX_RADIUS + index >= FFT_OUTPUT_SIZE) {
    search_size = INDEX_RADIUS + FFT_OUTPUT_SIZE - index;
  } else if (INDEX_RADIUS - index >= 0) {
    search_size  = INDEX_RADIUS + index - 1;
    search_start = FFT_AMP + 1;
  }

  arm_max_f32(search_start, search_size, hw_amp + time, &index_offset); // 幅值
  index += index_offset - INDEX_RADIUS;

  // 滑动窗口滤波
  amp_window[time][amp_window_offset++] = FFT_AMP[index];
  if (amp_window_offset == FILTER_WINDOW_SIZE) {
    amp_window_offset = 0;
  }

  // 计算平均值
  arm_mean_f32(amp_window[time], FILTER_WINDOW_SIZE, hw_amp + time); // 幅值

#ifdef DEBUG
#endif
}
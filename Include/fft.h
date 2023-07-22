#pragma once

#include <arm_math_types.h>

extern float32_t FFT_AMP[];
extern float32_t FFT_PHASE[];
extern float32_t FFT_OUTPUT_RAW[];

void fft_init(void);
void fft_with_window();

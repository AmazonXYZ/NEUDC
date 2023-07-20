#pragma once

#include <arm_math_types_f16.h>

extern float16_t FFT_OUTPUT[];
extern float16_t FFT_PHASE[];

void fft_init(void);
void fft_with_window();

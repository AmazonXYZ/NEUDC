#pragma once

#include <stdint.h>
#include <stdbool.h>

#include <arm_math_types.h>

#define ADC_SAMPLE_SIZE 4096
#define FFT_OUTPUT_SIZE     (ADC_SAMPLE_SIZE / 2 + 1)

extern volatile float32_t SAMPLE_DATA[];

extern bool SAMPLE_FLAG;

void adc_init(void);
void adc_stop(void);
void adc_sacle(void);

void adc_run(float32_t target_freq, float32_t *sample_freq_addr);
#pragma once

#include <stdint.h>

#include <arm_math_types_f16.h>

#define ADC_SAMPLE_SIZE 4096

extern volatile float16_t SAMPLE_DATA[];

void adc_init(void);
void adc_run(uint16_t freq);
void adc_stop(void);
void adc_sacle(void);

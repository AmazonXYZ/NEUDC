#pragma once

#include <stdint.h>

#include <arm_math_types.h>

extern float32_t hw_amp[];
extern float32_t hw_phase[];
extern float32_t THD;

void update(float32_t freq);
void preprocess(void);
void process(uint8_t time);
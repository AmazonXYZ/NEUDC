#pragma once

#include <stdint.h>

#include <arm_math_types_f16.h>

extern float16_t AMPLITUDE[];
extern float16_t PHASE[];
extern float16_t THD;

void update(uint16_t freq);
void preprocess(void);
void process(uint8_t time);
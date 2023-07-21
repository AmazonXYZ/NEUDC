#pragma once

#include <stdint.h>

#include <arm_math_types.h>

extern float32_t AMPLITUDE[];
extern float32_t PHASE[];
extern float32_t THD;

void update(uint16_t freq);
void preprocess(void);
void process(uint8_t time);
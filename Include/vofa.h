#pragma once

#include <stdbool.h>
#include <stdint.h>

void vofa_justfloat_single(void *array, uint16_t length, bool if16);
void vofa_firewater_duo(void *array1, void *array2, uint16_t length, bool if16);
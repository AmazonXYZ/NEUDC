#pragma once

#include <stdint.h>

extern volatile uint8_t STATE_CODE;

void serial_init(void);
void serial_send(const uint8_t *data, uint16_t length);
void serial_printf(char *format, ...);

#pragma once

#include <stdint.h>

#include <arm_math_types_f16.h>

void lcd_init(void);
void lcd_send(const uint8_t *data, uint16_t length);
void lcd_printf(char *format, ...);
void lcd_refresh(void);
void lcd_graph(void);
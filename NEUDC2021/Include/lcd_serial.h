#pragma once

#include <stdint.h>

void lcd_init(void);
void lcd_printf(char *format, ...);
void lcd_refresh(void);
void lcd_graph(void);
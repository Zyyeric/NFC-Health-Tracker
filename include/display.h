// TFT LCD with Touchscreen Breakout Display

#pragma once
#include <stdint.h>
#include <stddef.h>

void spi_init(void);
void display_init(void);
// void draw_T(uint16_t x, uint16_t y);
void spi_write_command(uint8_t cmd);
void spi_write_data(uint8_t *data, size_t length);
void fill_screen(uint16_t color);
void write_text(char c);
// TFT LCD with Touchscreen Breakout Display

#pragma once
#include <stdint.h>
#include <stddef.h>

void spi_init(void);

void display_init(void);

void spi_write_command(uint8_t cmd);

void spi_write_data(uint8_t *data, size_t length);

void fill_screen(uint16_t color);

void write_text(char c, uint16_t color, uint16_t background, uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2);

void write_temp(int temp, int frac);

void write_bpm(int bpm);

void write_initializing(void);

void clear_initializing(void);
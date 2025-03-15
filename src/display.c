#include "display.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include "nrf_delay.h"
#include "nrfx_spim.h"

#include "microbit_v2.h"

static const nrfx_spim_t spi = NRFX_SPIM_INSTANCE(2);

static const uint8_t letter_T[8] = {
  0xFF,  
  0x18,     
  0x18,     
  0x18,     
  0x18,     
  0x18,     
  0x18,     
  0x00               
};

void spi_init(void) {
  nrfx_spim_config_t config = {
    .sck_pin      = EDGE_P13,  // SCK for SPI2
    .mosi_pin     = EDGE_P15,  // MOSI for SPI2
    .miso_pin     = NRFX_SPIM_PIN_NOT_USED,  // ILI9341 does NOT need MISO
    .ss_pin       = EDGE_P16,  // Chip Select (CS)
    .orc          = 0xFF,      // Over-run character
    .frequency    = NRF_SPIM_FREQ_1M,  // 4MHz SPI Clock for stability
    .mode         = NRF_SPIM_MODE_0,  // CPOL=0, CPHA=0
    .bit_order    = NRF_SPIM_BIT_ORDER_MSB_FIRST,
  };

  nrfx_err_t err = nrfx_spim_init(&spi, &config, NULL, NULL);
  if (err == NRFX_SUCCESS) {
    printf("SPI Initialized Successfully!\n");
  } else if (err == NRFX_ERROR_FORBIDDEN) {
      printf("SPI Init Failed: FORBIDDEN (Error Code: 8)\n");
  } else if (err == NRFX_ERROR_BUSY) {
      printf("SPI Init Failed: BUSY\n");
  } else if (err == NRFX_ERROR_INVALID_STATE) {
      printf("SPI Init Failed: INVALID STATE\n");
  } else {
      printf("SPI Init Failed: Unknown Error %ld\n", err);
  }
}

void gpio_init(void) {
  nrf_gpio_cfg_output(EDGE_P12);  // D/C Pin
  nrf_gpio_cfg_output(EDGE_P8);   // RESET Pin
  nrf_gpio_cfg_output(EDGE_P16);  // CS Pin

  nrf_gpio_pin_set(EDGE_P16);  // CS HIGH (inactive)
  nrf_gpio_pin_set(EDGE_P12);  // D/C HIGH (data mode)
}

void display_init(void) {
  gpio_init();

  nrf_gpio_pin_clear(EDGE_P8);  // RESET LOW
  nrf_delay_ms(50);
  nrf_gpio_pin_set(EDGE_P8);  // RESET HIGH
  nrf_delay_ms(100);

  // Send Initialization Commands
  spi_write_command(0x11);  // Sleep OUT
  nrf_delay_ms(120);
  spi_write_command(0x36);  // Memory Access Control
  uint8_t madctl = 0x48;    // Set RGB Mode
  spi_write_data(&madctl, 1);
  spi_write_command(0x3A);  // Pixel Format
  uint8_t pixfmt = 0x55;    // 16-bit RGB565
  spi_write_data(&pixfmt, 1);
  spi_write_command(0x29);  // Display ON

  printf("Display Initialized!\n");
}

void spi_write_command(uint8_t cmd) {
  uint8_t tx_data = cmd;
  
  nrf_gpio_pin_clear(EDGE_P16);  // Select LCD (CS LOW)
  nrf_gpio_pin_clear(EDGE_P12);  // Command Mode (D/C LOW)

  nrfx_spim_xfer_desc_t xfer = NRFX_SPIM_XFER_TX(&tx_data, 1);
  nrfx_err_t err = nrfx_spim_xfer(&spi, &xfer, 0);
  if (err != NRFX_SUCCESS) {
      printf("SPI Command Failed: %lu\n", err);
  }

  nrf_gpio_pin_set(EDGE_P16);  // Deselect LCD (CS HIGH)
}

void spi_write_data(uint8_t *data, size_t length) {
  nrf_gpio_pin_clear(EDGE_P16);  // Select LCD (CS LOW)
  nrf_gpio_pin_set(EDGE_P12);    // Data Mode (D/C HIGH)

  nrfx_spim_xfer_desc_t xfer = NRFX_SPIM_XFER_TX(&data, 1);
  nrfx_err_t err = nrfx_spim_xfer(&spi, &xfer, 0);  
  if (err != NRFX_SUCCESS) {
      printf("SPI Data Failed: %lu\n", err);
  }

  nrf_gpio_pin_set(EDGE_P16);  // Deselect LCD (CS HIGH)
}

void set_cursor(uint16_t x, uint16_t y) {
  uint8_t data[4];

  // Set Column Address (X)
  spi_write_command(0x2A);
  data[0] = x >> 8; data[1] = x & 0xFF;
  data[2] = (x + 7) >> 8; data[3] = (x + 7) & 0xFF; // 8px width
  spi_write_data(data, 4);

  // Set Row Address (Y)
  spi_write_command(0x2B);
  data[0] = y >> 8; data[1] = y & 0xFF;
  data[2] = (y + 7) >> 8; data[3] = (y + 7) & 0xFF; // 8px height
  spi_write_data(data, 4);

  // Prepare for pixel data
  spi_write_command(0x2C);
}

void draw_T(uint16_t x, uint16_t y) {
  set_cursor(x, y);  // Set Position

  uint8_t data[2];  // 16-bit Color
  for (uint8_t row = 0; row < 8; row++) {
      for (uint8_t col = 0; col < 8; col++) {
          // Check if pixel is ON (1) or OFF (0)
          if (letter_T[row] & (1 << (7 - col))) {
              data[0] = 0x00; data[1] = 0x00;  // BLACK (T)
          } else {
              data[0] = 0xFF; data[1] = 0xFF;  // WHITE (Background)
          }
          spi_write_data(data, 2); // Send pixel
      }
  }
}


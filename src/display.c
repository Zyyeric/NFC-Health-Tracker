#include "display.h"
#include "display_font.h"
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
    .frequency    = NRF_SPIM_FREQ_8M,  // 4MHz SPI Clock for stability
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
  nrf_gpio_pin_set(EDGE_P8);   // RESET HIGH
}

void display_init(void) {
  gpio_init();

  nrf_gpio_pin_clear(EDGE_P8);  // RESET LOW
  nrf_delay_ms(100);
  nrf_gpio_pin_set(EDGE_P8);    // RESET HIGH
  nrf_delay_ms(100);

  // Official Adafruit-style initialization
  spi_write_command(0xEF);  uint8_t ef[] = {0x03, 0x80, 0x02};  spi_write_data(ef, 3);
  spi_write_command(0xCF);  uint8_t cf[] = {0x00, 0xC1, 0x30};  spi_write_data(cf, 3);
  spi_write_command(0xED);  uint8_t ed[] = {0x64, 0x03, 0x12, 0x81}; spi_write_data(ed, 4);
  spi_write_command(0xE8);  uint8_t e8[] = {0x85, 0x00, 0x78}; spi_write_data(e8, 3);
  spi_write_command(0xCB);  uint8_t cb[] = {0x39, 0x2C, 0x00, 0x34, 0x02}; spi_write_data(cb, 5);
  spi_write_command(0xF7);  uint8_t f7[] = {0x20}; spi_write_data(f7, 1);
  spi_write_command(0xEA);  uint8_t ea[] = {0x00, 0x00}; spi_write_data(ea, 2);
  
  spi_write_command(0xC0);  uint8_t c0[] = {0x23}; spi_write_data(c0, 1);  // Power Control 1
  spi_write_command(0xC1);  uint8_t c1[] = {0x10}; spi_write_data(c1, 1);  // Power Control 2
  spi_write_command(0xC5);  uint8_t c5[] = {0x3E, 0x28}; spi_write_data(c5, 2);  // VCOM Control 1
  spi_write_command(0xC7);  uint8_t c7[] = {0x86}; spi_write_data(c7, 1);  // VCOM Control 2

  spi_write_command(0x36);  uint8_t madctl[] = {0x00}; spi_write_data(madctl, 1); // Memory Access Control
  spi_write_command(0x3A);  uint8_t pixfmt[] = {0x55}; spi_write_data(pixfmt, 1); // Pixel Format = 16-bit RGB565

  spi_write_command(0xB1);  uint8_t frmctr1[] = {0x00, 0x18}; spi_write_data(frmctr1, 2);
  spi_write_command(0xB6);  uint8_t dfunctr[] = {0x08, 0x82, 0x27}; spi_write_data(dfunctr, 3);
  
  spi_write_command(0xF2);  uint8_t f2[] = {0x00}; spi_write_data(f2, 1);  // Disable Gamma correction
  spi_write_command(0x26);  uint8_t gamma[] = {0x01}; spi_write_data(gamma, 1);  // Gamma curve

  spi_write_command(0xE0);  uint8_t gmctrp1[] = {0x0F, 0x31, 0x2B, 0x0C, 0x0E, 0x08, 0x4E, 0xF1, 0x37, 0x07, 0x10, 0x03, 0x0E, 0x09, 0x00}; spi_write_data(gmctrp1, 15);
  spi_write_command(0xE1);  uint8_t gmctrn1[] = {0x00, 0x0E, 0x14, 0x03, 0x11, 0x07, 0x31, 0xC1, 0x48, 0x08, 0x0F, 0x0C, 0x31, 0x36, 0x0F}; spi_write_data(gmctrn1, 15);

  spi_write_command(0x11);  // Exit Sleep Mode
  nrf_delay_ms(120);
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

  nrfx_spim_xfer_desc_t xfer = NRFX_SPIM_XFER_TX(data, length);
  nrfx_err_t err = nrfx_spim_xfer(&spi, &xfer, 0);  
  if (err != NRFX_SUCCESS) {
      printf("SPI Data Failed: %lu\n", err);
  }

  nrf_gpio_pin_set(EDGE_P16);  // Deselect LCD (CS HIGH)
}

void setAddrWindow(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2) {
  uint8_t data[4];

  spi_write_command(0x2A); // Column Address Set
  data[0] = x1 >> 8; data[1] = x1 & 0xFF;
  data[2] = x2 >> 8; data[3] = x2 & 0xFF;
  spi_write_data(data, 4);

  spi_write_command(0x2B); // Page Address Set
  data[0] = y1 >> 8; data[1] = y1 & 0xFF;
  data[2] = y2 >> 8; data[3] = y2 & 0xFF;
  spi_write_data(data, 4);

  spi_write_command(0x2C); // Memory Write
}

void fill_screen(uint16_t color) {
  setAddrWindow(0, 0, 239, 319); // Full screen (assuming 240x320 display)
  
  uint8_t data[2] = {color >> 8, color & 0xFF};

  printf("Filling screen with color: 0x%04X\n", color);

  for (uint32_t i = 0; i < 240 * 320; i++) { // Total pixels
      spi_write_data(data, 2);  // Send color
  }

  printf("Done filling screen\n");
}

void write_text(char c) {
  int ascii = c;
  int index = ascii - 32;

  printf("ASCII: %d\n", ascii);
  printf("Index: %d\n", index);

  setAddrWindow(0, 0, 23, 38); // Full screen (assuming 240x320 display)
  
  uint8_t data[2] = {0xFF, 0xFF};

  uint8_t background[2] = {0x00, 0x00};

  // for(int i = 0; i < 13 * 8; i++){
  //   if(i < 8){
  //     spi_write_data(data, 2);
  //   }
  //   else{
  //     spi_write_data(background, 2);
  //   }
  // }

  // for(int i = 0; i < 26; i++){
  //   for(int j = 0; j < 16; j++){
  //     spi_write_data(background, 2);
  //   }
  // }

  for(int i = 0; i < 39; i++){
    uint8_t curr = display_font[index][i/3];
    // printf("%d\n", curr);
    uint8_t mask = 0x80;
    for(int j = 0; j < 8; j++){
      if((curr & mask) == mask){
        spi_write_data(data, 2);
        spi_write_data(data, 2);
        spi_write_data(data, 2);
        // printf("Writing white\n");
      }
      else{
        spi_write_data(background, 2);
        spi_write_data(background, 2);
        spi_write_data(background, 2);
      }

      // curr = curr << 1;
      // printf("%d\n", curr);
      mask = mask >> 1;
    }
  }

  printf("Done writing char");
}
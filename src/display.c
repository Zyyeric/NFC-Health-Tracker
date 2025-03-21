#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "nrf_delay.h"
#include "nrfx_spim.h"
#include "microbit_v2.h"
#include "display.h"
#include "display_font.h"

// Create SPIM instance
static const nrfx_spim_t spi = NRFX_SPIM_INSTANCE(2);

// Initialize the SPIM
void spi_init(void) {
  // Edit the configuration 
  nrfx_spim_config_t config = {
    .sck_pin      = EDGE_P13,  
    .mosi_pin     = EDGE_P15,  
    .miso_pin     = NRFX_SPIM_PIN_NOT_USED,  
    .ss_pin       = EDGE_P16, 
    .orc          = 0xFF,      
    .frequency    = NRF_SPIM_FREQ_8M,  
    .mode         = NRF_SPIM_MODE_0,  
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

// Initialize GPIO pins and set initial state
void gpio_init(void) {
  // Set pins as output
  nrf_gpio_cfg_output(EDGE_P12);  // D/C Pin
  nrf_gpio_cfg_output(EDGE_P8);   // RESET Pin
  nrf_gpio_cfg_output(EDGE_P16);  // CS Pin
  // Set pins high initially
  nrf_gpio_pin_set(EDGE_P12);
  nrf_gpio_pin_set(EDGE_P8);  
  nrf_gpio_pin_set(EDGE_P16);  
}

// Initialize the display
void display_init(void) {
  gpio_init();

  // Reset everything
  nrf_gpio_pin_clear(EDGE_P8);  
  nrf_delay_ms(100);
  nrf_gpio_pin_set(EDGE_P8);   
  nrf_delay_ms(100);

  // Official Adafruit-style initialization
  spi_write_command(0xEF);  uint8_t ef[] = {0x03, 0x80, 0x02};  spi_write_data(ef, 3);
  spi_write_command(0xCF);  uint8_t cf[] = {0x00, 0xC1, 0x30};  spi_write_data(cf, 3);
  spi_write_command(0xED);  uint8_t ed[] = {0x64, 0x03, 0x12, 0x81}; spi_write_data(ed, 4);
  spi_write_command(0xE8);  uint8_t e8[] = {0x85, 0x00, 0x78}; spi_write_data(e8, 3);
  spi_write_command(0xCB);  uint8_t cb[] = {0x39, 0x2C, 0x00, 0x34, 0x02}; spi_write_data(cb, 5);
  spi_write_command(0xF7);  uint8_t f7[] = {0x20}; spi_write_data(f7, 1);
  spi_write_command(0xEA);  uint8_t ea[] = {0x00, 0x00}; spi_write_data(ea, 2);
  
  // Initialize power controls
  spi_write_command(0xC0);  uint8_t c0[] = {0x23}; spi_write_data(c0, 1);  
  spi_write_command(0xC1);  uint8_t c1[] = {0x10}; spi_write_data(c1, 1);  

  // Initialize VCOM controls
  spi_write_command(0xC5);  uint8_t c5[] = {0x3E, 0x28}; spi_write_data(c5, 2);  
  spi_write_command(0xC7);  uint8_t c7[] = {0x86}; spi_write_data(c7, 1);  
  spi_write_command(0x36);  uint8_t madctl[] = {0x00}; spi_write_data(madctl, 1); // Memory Access Control
  spi_write_command(0x3A);  uint8_t pixfmt[] = {0x55}; spi_write_data(pixfmt, 1); // Pixel Format
  spi_write_command(0xB1);  uint8_t frmctr1[] = {0x00, 0x18}; spi_write_data(frmctr1, 2);
  spi_write_command(0xB6);  uint8_t dfunctr[] = {0x08, 0x82, 0x27}; spi_write_data(dfunctr, 3);
  spi_write_command(0xF2);  uint8_t f2[] = {0x00}; spi_write_data(f2, 1);  // Disable Gamma correction
  spi_write_command(0x26);  uint8_t gamma[] = {0x01}; spi_write_data(gamma, 1);  // Gamma curves
  spi_write_command(0xE0);  uint8_t gmctrp1[] = {0x0F, 0x31, 0x2B, 0x0C, 0x0E, 0x08, 0x4E, 0xF1, 0x37, 0x07, 0x10, 0x03, 0x0E, 0x09, 0x00}; spi_write_data(gmctrp1, 15);
  spi_write_command(0xE1);  uint8_t gmctrn1[] = {0x00, 0x0E, 0x14, 0x03, 0x11, 0x07, 0x31, 0xC1, 0x48, 0x08, 0x0F, 0x0C, 0x31, 0x36, 0x0F}; spi_write_data(gmctrn1, 15);

  // Exit sleep mode and turn on the display
  spi_write_command(0x11);  
  nrf_delay_ms(120);
  spi_write_command(0x29);  

  printf("Display Initialized!\n");
}

// Helper to write commands to the display
void spi_write_command(uint8_t cmd) {
  uint8_t tx_data = cmd;
  
  nrf_gpio_pin_clear(EDGE_P16);  // Pull the chip select low
  nrf_gpio_pin_clear(EDGE_P12);  // Set D/C low because we are sending a command

  // Send the TX transfer
  nrfx_spim_xfer_desc_t xfer = NRFX_SPIM_XFER_TX(&tx_data, 1);
  nrfx_err_t err = nrfx_spim_xfer(&spi, &xfer, 0);
  if (err != NRFX_SUCCESS) {
      printf("SPI Command Failed: %lu\n", err);
  }

  // Pull chip select back high
  nrf_gpio_pin_set(EDGE_P16); 
}

// Helper to write data to the display
void spi_write_data(uint8_t *data, size_t length) {
  nrf_gpio_pin_clear(EDGE_P16);  // Pull the chip select low
  nrf_gpio_pin_set(EDGE_P12);    // Set D/C high because we are sending data

  // Send the TX transfer
  nrfx_spim_xfer_desc_t xfer = NRFX_SPIM_XFER_TX(data, length);
  nrfx_err_t err = nrfx_spim_xfer(&spi, &xfer, 0);  
  if (err != NRFX_SUCCESS) {
      printf("SPI Data Failed: %lu\n", err);
  }

  // Pull chip select back high
  nrf_gpio_pin_set(EDGE_P16); 
}

// Set the display window for the next writes
void setAddrWindow(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2) {
  uint8_t data[4];

  // Set the Column Address
  spi_write_command(0x2A); 
  data[0] = x1 >> 8; data[1] = x1 & 0xFF;
  data[2] = x2 >> 8; data[3] = x2 & 0xFF;
  spi_write_data(data, 4);

  // Set the Page Address
  spi_write_command(0x2B);
  data[0] = y1 >> 8; data[1] = y1 & 0xFF;
  data[2] = y2 >> 8; data[3] = y2 & 0xFF;
  spi_write_data(data, 4);

  // Start the write command
  spi_write_command(0x2C);
}

// Fill the screen with one color
void fill_screen(uint16_t color) {
  // Set the window to the full screen
  setAddrWindow(0, 0, 239, 319);
  // Extract the color
  uint8_t data[2] = {color >> 8, color & 0xFF};
  // Fill each pixel with the given color
  for (uint32_t i = 0; i < 240 * 320; i++) {
      spi_write_data(data, 2);  
  }

  printf("Done filling screen\n");
}

// Write a character on the display at the given coordinates and colors
void write_text(char c, uint16_t color, uint16_t background_color, uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2) {
  // Get the ASCII and bit-map index of the char
  int ascii = c;
  int index = ascii - 32;
  // Set the address window
  setAddrWindow(x1, y1, x2, y2);
  // Extract the text and background color
  uint8_t data[2] = {color >> 8, color & 0xFF};
  uint8_t background[2] = {background_color >> 8, background_color & 0xFF};

  // Set the mask and loop through the desired pixels
  uint8_t mask = 0x80;
  for(int i = 0; i < 24; i++){
    for(int j = 0; j < 13; j++){
      // Get the current row
      uint8_t curr = display_font[index][12 - j];
      if((curr & mask) == mask){  
        // 3 times since 3x original
        spi_write_data(data, 2);
        spi_write_data(data, 2);
        spi_write_data(data, 2);
      }
      else{
        // 3 times since 3x original
        spi_write_data(background, 2);
        spi_write_data(background, 2);
        spi_write_data(background, 2);
      }
    }

    // Adjust mask
    if((i + 1) % 3 == 0){
      mask = mask >> 1;
    }
  }
}

// Write the temperature to the display
void write_temp(int temp, int frac){
  // Set original coordinates and colors
  int y1 = 150;
  int y2 = 173;
  uint16_t white = 0xFFFF;
  uint16_t black = 0x0000;
  int original_temp = temp;
  int original_frac = frac;
  // Write the digits to the display
  while(temp != 0){
    int curr = temp % 10;
    int ascii = curr + 48;
    write_text(ascii, white, black, 48, y1, 86, y2);
    y1 -= 25;
    y2 -= 25;
    temp /= 10;
  }

  float fractional = (float) frac * 0.0625;
  int fractional_int = (int) (fractional * 100);
  write_text(46, white, black, 48, 175, 86, 198);
  
  // Write the fractional digits to the display
  y1 = 225;
  y2 = 248;
  while(fractional_int != 0){
    int curr = fractional_int % 10;
    int ascii = curr + 48;
    write_text(ascii, white, black, 48, y1, 86, y2);
    y1 -= 25;
    y2 -= 25;
    fractional_int /= 10;
  }

  write_text(32, white, black, 48, 250, 86, 273);
  write_text(67, white, black, 48, 275, 86, 298);

  float real_temp = original_temp + original_frac * 0.0625;
  // Display the correct diagnosis for the current temp
  if(real_temp < 28)
  {
    write_text('H', 0x00F8, 0x0000, 144, 0, 182, 23);
    write_text('Y', 0x00F8, 0x0000, 144, 25, 182, 48);
    write_text('P', 0x00F8, 0x0000, 144, 50, 182, 73);
    write_text('O', 0x00F8, 0x0000, 144, 75, 182, 98);
    write_text('T', 0x00F8, 0x0000, 144, 100, 182, 123);
    write_text('H', 0x00F8, 0x0000, 144, 125, 182, 148);
    write_text('E', 0x00F8, 0x0000, 144, 150, 182, 173);
    write_text('R', 0x00F8, 0x0000, 144, 175, 182, 198);
    write_text('M', 0x00F8, 0x0000, 144, 200, 182, 223);
    write_text('I', 0x00F8, 0x0000, 144, 225, 182, 248);
    write_text('A', 0x00F8, 0x0000, 144, 250, 182, 273); 
  }
  else if(real_temp > 34)
  {
    write_text('F', 0x00F8, 0x0000, 144, 0, 182, 23);
    write_text('E', 0x00F8, 0x0000, 144, 25, 182, 48);
    write_text('V', 0x00F8, 0x0000, 144, 50, 182, 73);
    write_text('E', 0x00F8, 0x0000, 144, 75, 182, 98);
    write_text('R', 0x00F8, 0x0000, 144, 100, 182, 123);
    write_text(' ', 0x00F8, 0x0000, 144, 125, 182, 148);
    write_text(' ', 0x00F8, 0x0000, 144, 150, 182, 173);
    write_text(' ', 0x00F8, 0x0000, 144, 175, 182, 198);
    write_text(' ', 0x00F8, 0x0000, 144, 200, 182, 223);
    write_text(' ', 0x00F8, 0x0000, 144, 225, 182, 248);
    write_text(' ', 0x00F8, 0x0000, 144, 250, 182, 273); 
  }
  else
  {
    write_text('N', 0x07E0, 0x0000, 144, 0, 182, 23);
    write_text('O', 0x07E0, 0x0000, 144, 25, 182, 48);
    write_text('R', 0x07E0, 0x0000, 144, 50, 182, 73);
    write_text('M', 0x07E0, 0x0000, 144, 75, 182, 98);
    write_text('A', 0x07E0, 0x0000, 144, 100, 182, 123);
    write_text('L', 0x07E0, 0x0000, 144, 125, 182, 148);
    write_text(' ', 0x07E0, 0x0000, 144, 150, 182, 173);
    write_text('T', 0x07E0, 0x0000, 144, 175, 182, 198);
    write_text('E', 0x07E0, 0x0000, 144, 200, 182, 223);
    write_text('M', 0x07E0, 0x0000, 144, 225, 182, 248);
    write_text('P', 0x07E0, 0x0000, 144, 250, 182, 273); 
  }
  
}

// Write the initializatoin screen
void write_initializing(void) {
  uint16_t white = 0xFFFF;
  uint16_t black = 0x0000;
  write_text('I', white, black, 0, 0, 38, 23);
  write_text('N', white, black, 0, 25, 38, 48);
  write_text('I', white, black, 0, 50, 38, 73);
  write_text('T', white, black, 0, 75, 38, 98);
  write_text('.', white, black, 0, 100, 38, 123);
  write_text('.', white, black, 0, 125, 38, 148);
}

// Write the BPM to the display
void write_bpm(int bpm) {
  static int prev_digit_count = 0;  // Track previous number of digits
  char buffer[10];  // Buffer to store number as a string
  sprintf(buffer, "%d", bpm);  // Convert number to string
  int new_digit_count = strlen(buffer);  // Get the current digit count

  printf("BPM as string: %s\n", buffer); // Debugging

  // Display settings
  uint16_t white = 0xFFFF;
  uint16_t black = 0x0000;
  uint16_t background = 0x0000; // Assuming black is the background
  int y1 = 100;
  int y2 = 123;

  // Write the new number
  for (int i = 0; i < new_digit_count; i++) {
      write_text(buffer[i], white, black, 0, y1, 38, y2);
      y1 += 25;
      y2 += 25;
  }

  // Clear any extra digits from the previous number
  for (int i = new_digit_count; i < prev_digit_count; i++) {
      write_text(' ', background, background, 0, y1, 38, y2); // Overwrite with a space
      y1 += 25;
      y2 += 25;
  }

  // Update previous digit count
  prev_digit_count = new_digit_count;
}
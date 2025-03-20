#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <math.h>

#include "nrf_delay.h"
#include "nrf_twi_mngr.h"
#include "app_timer.h"
#include "microbit_v2.h"
#include "max30102.h"
#include "pulsesensor.h"
#include "pulsesensor_util.h"
#include "display.h"
#include "nrfx_spim.h"

#include <stdio.h>
#include <math.h>
#include <string.h>

// Global I2C manager instance
NRF_TWI_MNGR_DEF(twi_mngr_instance, 1, 0);

int main(void) {
  printf("Board started!\n");

  // Initialize the SPI
  spi_init();
  display_init();

  // Different Colors
  uint16_t white = 0xFFFF;
  uint16_t black = 0x0000;

  // Fill the screen black
  fill_screen(black);
  // Write initializing to the screen
  write_initializing();

  // Initialize I2C and configure peripheral and driver
  nrf_drv_twi_config_t i2c_config = NRF_DRV_TWI_DEFAULT_CONFIG;
  i2c_config.scl = EDGE_P19;  
  i2c_config.sda = EDGE_P20;  
  i2c_config.frequency = NRF_TWIM_FREQ_100K;
  i2c_config.interrupt_priority = 0;
  nrf_twi_mngr_init(&twi_mngr_instance, &i2c_config);
  printf("I2C initialized!\n");

  // // Initialize the MAX30102 sensor
  max30102_init(&twi_mngr_instance);
  printf("Temp Sensor initialized!\n");
  
  // Initialize the ADC
  adc_init();
  printf("Pulse Sensor initialized!\n");

  // Initalize Timer Module 
  ret_code_t err_code = app_timer_init();
  APP_ERROR_CHECK(err_code);
  printf("Timer initialized!\n");

  // Start pulse sensor sampling (2 ms interval)
  start_sample_timer();

  while (1) {
  }
  
  return 0;
}
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <math.h>

#include "nrf_delay.h"
#include "nrf_twi_mngr.h"
#include "app_timer.h"
#include "microbit_v2.h"
#include "max30102.h"

// Global I2C manager instance
NRF_TWI_MNGR_DEF(twi_mngr_instance, 1, 0);

int main(void) {
  printf("Board started!\n");
  
  // Initialize I2C and configure peripheral and driver
  nrf_drv_twi_config_t i2c_config = NRF_DRV_TWI_DEFAULT_CONFIG;
  i2c_config.scl = EDGE_P19;  
  i2c_config.sda = EDGE_P20;  
  i2c_config.frequency = NRF_TWIM_FREQ_100K;
  i2c_config.interrupt_priority = 0;
  nrf_twi_mngr_init(&twi_mngr_instance, &i2c_config);
  
  // Initialize the MAX30102 sensor
  max30102_init(&twi_mngr_instance);
  
  while (1) {
    // uint8_t sample_count = max30102_get_sample_count();
    // printf("Sample count before reading: %d\n", sample_count);
    
    // if (sample_count > 0) {
    //   for (uint8_t i = 0; i < sample_count; i++) {
    // max30102_measurement_t sample = max30102_read_sample();
    // printf("Sample %d: RED = %lu, IR = %lu\n", 1, sample.red, sample.ir);
    max30102_read_temp();
    //   }
    // } else {
    //   printf("No new samples available.\n");
    // }
    nrf_delay_ms(100);
  }
  
  return 0;
}
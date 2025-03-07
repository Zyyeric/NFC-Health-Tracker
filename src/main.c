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

  FILE *file = fopen("output.txt", "w");  // Open file for writing (overwrite mode)
  if (file == NULL) {
      perror("Failed to open file");
      return 1;
  }

  fprintf(file, "Hello, world!\n");  // Write formatted text
  fputs("This is another line.\n", file);

  fclose(file);  // Close the file
  
  while (1) {
    max30102_measurement_t sample = max30102_read_sample();
    printf("%lu\n", sample.red);
    // nrf_delay_ms(50);
  }
  
  return 0;
}
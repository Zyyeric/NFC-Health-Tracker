#include "display.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include "nrf_delay.h"
#include "nrfx_spim.h"

#include "microbit_v2.h"


static const nrfx_spim_t spi = NRFX_SPIM_INSTANCE(2);

void spi_init(void) {
  // nrfx_spi_config_t config = NRFX_SPI_DEFAULT_CONFIG;
  // config.sck_pin = 13;
  // config.mosi_pin = 15;
  // config.miso_pin = 14;
  // config.ss_pin = 16;

  // nrfx_err_t res = nrfx_spi_init(&spi, &config, NULL, NULL);
  // if(res != NRFX_SUCCESS){
  //   printf("SPI initialization failed");
  // }
}
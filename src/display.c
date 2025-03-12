#include "display.h"
#include "hal/nrf_spi.h"
#include "nrfx.h"
#include "nrfx_spi.h"
#include "sdk_config.h"

static const nrfx_spi_t spi = NRFX_SPI_INSTANCE(0);

void spi_init(void) {
  nrfx_spi_config_t config = NRFX_SPI_DEFAULT_CONFIG;
  config.sck_pin = 13;
  config.mosi_pin = 15;
  config.miso_pin = 14;
  config.ss_pin = 16;

  nrfx_err_t res = nrfx_spi_init(&spi, &config, NULL, NULL);
  // if(res != NRFX_SUCCESS){
  //   printf("SPI initialization failed");
  // }
}
#include "pulsesensor.h"
#include <stdint.h>

// Pulse Sensor Output
#define PULSE_INPUT NRF_SAADC_INPUT_AIN1

// Channel Configurations
#define ADC_PULSE 0

// Ignore
void saadc_event_callback(nrfx_saadc_evt_t const* _unused) {
}

void adc_init(void) {
  // Initialize the SAADC
  nrfx_saadc_config_t saadc_config = {
    .resolution = NRF_SAADC_RESOLUTION_12BIT,
    .oversample = NRF_SAADC_OVERSAMPLE_DISABLED,
    .interrupt_priority = 4,
    .low_power_mode = false,
  };

  ret_code_t error_code = nrfx_saadc_init(&saadc_config, saadc_event_callback);
  APP_ERROR_CHECK(error_code);

  // Initialize pulse sensor channel
  nrf_saadc_channel_config_t pulse_channel_config = NRFX_SAADC_DEFAULT_CHANNEL_CONFIG_SE(PULSE_INPUT);
  error_code = nrfx_saadc_channel_init(ADC_PULSE, &pulse_channel_config);
  APP_ERROR_CHECK(error_code);
}

float adc_sample_blocking(void) {
  // read ADC counts (0-4095)
  int16_t adc_counts = 0;
  ret_code_t error_code = nrfx_saadc_sample_convert(ADC_PULSE, &adc_counts);
  APP_ERROR_CHECK(error_code);

  // 12-bit ADC with range from 0 to 3.6 Volts ()
  // float volt = ((float) adc_counts/4095) * 3.6;
  
  // return adc measurement (I think we are just playing with adc at this point???)
  return adc_counts;
}

uint32_t convert_adc_to_bpm(const float *voltage_sample) {
  return 0;
}
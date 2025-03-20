// Pulse Sensor Measurement Functions

#include "nrfx_saadc.h"
#pragma once

void adc_init(void);

float adc_sample_blocking(void); 

void saadc_event_callback(nrfx_saadc_evt_t const* _unused);
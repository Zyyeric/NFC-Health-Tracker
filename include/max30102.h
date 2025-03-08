// MAX30102 Heart Rate and Blood Oxygen Sensor

#pragma once

#include "nrf_twi_mngr.h"

// MAX30102 chip address 
static const uint8_t MAX30102_ADDRESS = 0x57;

// Register addresses
typedef enum {
  INTERRUPT_STATUS_1 = 0x00,
  INTERRUPT_STATUS_2 = 0x01,
  INTERRUPT_ENABLE_1 = 0x02,
  INTERRUPT_ENABLE_2 = 0x03,
  FIFO_WR_PTR        = 0x04,
  OVERFLOW_COUNTER   = 0x05,
  FIFO_RD_PTR        = 0x06,
  FIFO_DATA          = 0x07,
  FIFO_CONFIG        = 0x08,
  MODE_CONFIG        = 0x09,
  SPO2_CONFIG        = 0x0A,
  LED1_RED_PULSE_AMP = 0x0C,  
  LED2_IR_PULSE_AMP  = 0x0D,  
  TEMP_INT           = 0x1F,
  TEMP_FRAC          = 0x20,
  TEMP_CONFIG        = 0x21, 
  PART_ID            = 0xFF,
} max30102_reg_t;

// Measurement data type
typedef struct {
  uint32_t red;
  uint32_t ir;
} max30102_measurement_t;

// Function prototypes
void max30102_init(const nrf_twi_mngr_t* i2c);

void i2c_reg_read(uint8_t i2c_addr, uint8_t reg_addr, uint8_t len, uint8_t* rx_buf);

void i2c_reg_write(uint8_t i2c_addr, uint8_t reg_addr, uint8_t data);

uint8_t max30102_get_sample_count(void);

max30102_measurement_t max30102_read_sample(void);

void max30102_read_temp(void);


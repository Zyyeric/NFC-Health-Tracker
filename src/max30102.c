// MAX30102 driver for Microbit_v2

#include <stdbool.h>
#include <stdint.h>
#include <math.h>

#include "max30102.h"
#include "nrf_delay.h"

// Pointer to an initialized I2C instance to use for transactions
static const nrf_twi_mngr_t* i2c_manager = NULL;

// Helper function to perform an arbitrary length I2C read of a given register
//
// i2c_addr - address of the device to read from
// reg_addr - address of the register within the device to read
// len - length of read
// rx_buf - where the read data should be stored
//
// returns 8-bit read value
void i2c_reg_read(uint8_t i2c_addr, uint8_t reg_addr, uint8_t len, uint8_t* rx_buf) {
  nrf_twi_mngr_transfer_t const read_transfer[] = {
    NRF_TWI_MNGR_WRITE(i2c_addr, &reg_addr, 1, NRF_TWI_MNGR_NO_STOP),
    NRF_TWI_MNGR_READ(i2c_addr, rx_buf , len, 0),
  };

  ret_code_t result = nrf_twi_mngr_perform(i2c_manager, NULL, read_transfer, 2, NULL);

  if (result != NRF_SUCCESS) {
    // Likely error codes:
    //  NRF_ERROR_INTERNAL            (0x0003) - something is wrong with the driver itself
    //  NRF_ERROR_INVALID_ADDR        (0x0010) - buffer passed was in Flash instead of RAM
    //  NRF_ERROR_BUSY                (0x0011) - driver was busy with another transfer still
    //  NRF_ERROR_DRV_TWI_ERR_OVERRUN (0x8200) - data was overwritten during the transaction
    //  NRF_ERROR_DRV_TWI_ERR_ANACK   (0x8201) - i2c device did not acknowledge its address
    //  NRF_ERROR_DRV_TWI_ERR_DNACK   (0x8202) - i2c device did not acknowledge a data byte
    printf("I2C transaction failed! Error: %lX\n", result);
  }
}

// Helper function to perform a 1-byte I2C write of a given register
//
// i2c_addr - address of the device to write to
// reg_addr - address of the register within the device to write
// data - the data desired to be written
void i2c_reg_write(uint8_t i2c_addr, uint8_t reg_addr, uint8_t data) {
  uint8_t tx_buf[2] = {reg_addr, data};
  nrf_twi_mngr_transfer_t const write_transfer[] = {
    NRF_TWI_MNGR_WRITE(i2c_addr,tx_buf, 2, 0),
  };

  ret_code_t result = nrf_twi_mngr_perform(i2c_manager, NULL, write_transfer, 1, NULL);
  
  if (result != NRF_SUCCESS) {
    //  Likely error codes:
    //  NRF_ERROR_INTERNAL            (0x0003) - something is wrong with the driver itself
    //  NRF_ERROR_INVALID_ADDR        (0x0010) - buffer passed was in Flash instead of RAM
    //  NRF_ERROR_BUSY                (0x0011) - driver was busy with another transfer still
    //  NRF_ERROR_DRV_TWI_ERR_OVERRUN (0x8200) - data was overwritten during the transaction
    //  NRF_ERROR_DRV_TWI_ERR_ANACK   (0x8201) - i2c device did not acknowledge its address
    //  NRF_ERROR_DRV_TWI_ERR_DNACK   (0x8202) - i2c device did not acknowledge a data byte
    printf("I2C transaction failed! Error: %lX\n", result);
  }
}

// Initialize the MAX30102 sensor.
void max30102_init(const nrf_twi_mngr_t* i2c) {
  i2c_manager = i2c;

  uint8_t data;
  i2c_reg_read(MAX30102_ADDRESS, PART_ID, 1, &data);
  printf("WHO_AM_I_A: 0x%X\n", data);
  // printf("WHO_AM_I_A: 0x%X\n", i2c_reg_read(0x48, 0x02));

  // Reset the FIFO to default values
  i2c_reg_write(MAX30102_ADDRESS, FIFO_WR_PTR, 0x00);
  i2c_reg_write(MAX30102_ADDRESS, OVERFLOW_COUNTER, 0x00);
  i2c_reg_write(MAX30102_ADDRESS, FIFO_RD_PTR, 0x00);
  
  // Configure FIFO settings 
  i2c_reg_write(MAX30102_ADDRESS, FIFO_CONFIG, 0x0F);
  
  // Configure Mode settings
  i2c_reg_write(MAX30102_ADDRESS, MODE_CONFIG, 0x03);
  
  // Configure SpO₂ settings
  i2c_reg_write(MAX30102_ADDRESS, SPO2_CONFIG, 0x27);
  
  // Set LED pulse amplitudes
  i2c_reg_write(MAX30102_ADDRESS, LED1_RED_PULSE_AMP, 0x24); 
  i2c_reg_write(MAX30102_ADDRESS, LED2_IR_PULSE_AMP, 0x24); 
  
  printf("MAX30102 initialization complete.\n");
}

// Returns the number of samples available in the FIFO
uint8_t max30102_get_sample_count(void) {
  uint8_t wr_ptr;
  i2c_reg_read(MAX30102_ADDRESS, FIFO_WR_PTR, 1, &wr_ptr);
  printf("Write pointer: %d\n", wr_ptr);
  uint8_t rd_ptr;
  i2c_reg_read(MAX30102_ADDRESS, FIFO_RD_PTR, 1, &rd_ptr);
  printf("Read pointer: %d\n", rd_ptr);
  uint8_t sample_count = (wr_ptr >= rd_ptr) ? (wr_ptr - rd_ptr) : (wr_ptr + 32 - rd_ptr);
  return sample_count;
}

// Read one sample from the FIFO
max30102_measurement_t max30102_read_sample(void) {
  uint8_t data[6];
  i2c_reg_read(MAX30102_ADDRESS, FIFO_DATA, 6, data);
  
  // Since data is left-justified, need to shift right at the end
  max30102_measurement_t sample;
  sample.red = (((uint32_t)data[0] << 16) | ((uint32_t)data[1] << 8) | data[2]) >> 6;
  sample.ir  = (((uint32_t)data[3] << 16) | ((uint32_t)data[4] << 8) | data[5]) >> 6;
  
  return sample;
}


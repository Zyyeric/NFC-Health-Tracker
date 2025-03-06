// LSM303AGR driver for Microbit_v2
//
// Initializes sensor and communicates over I2C
// Capable of reading temperature, acceleration, and magnetic field strength

#include <stdbool.h>
#include <stdint.h>
#include <math.h>

#include "lsm303agr.h"
#include "nrf_delay.h"

// Pointer to an initialized I2C instance to use for transactions
static const nrf_twi_mngr_t* i2c_manager = NULL;

// Helper function to perform a 1-byte I2C read of a given register
//
// i2c_addr - address of the device to read from
// reg_addr - address of the register within the device to read
//
// returns 8-bit read value
static uint8_t i2c_reg_read(uint8_t i2c_addr, uint8_t reg_addr) {
  uint8_t rx_buf = 0;
  nrf_twi_mngr_transfer_t const read_transfer[] = {
    //TODO: implement me
    NRF_TWI_MNGR_WRITE(i2c_addr, &reg_addr, 1, NRF_TWI_MNGR_NO_STOP),
    NRF_TWI_MNGR_READ(i2c_addr, &rx_buf , 1, 0),
  };

  printf("Before perform\n");
  ret_code_t result = nrf_twi_mngr_perform(i2c_manager, NULL, read_transfer, 2, NULL);
  printf("After perform\n");
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
  else{
    printf("I2C Transaction success! \n");
  }

  return rx_buf;
}

// Helper function to perform a 1-byte I2C write of a given register
//
// i2c_addr - address of the device to write to
// reg_addr - address of the register within the device to write
static void i2c_reg_write(uint8_t i2c_addr, uint8_t reg_addr, uint8_t data) {
  //TODO: implement me
  //Note: there should only be a single two-byte transfer to be performed
  uint8_t tx_buf[2] = {reg_addr, data};
  nrf_twi_mngr_transfer_t const write_transfer[] = {
    NRF_TWI_MNGR_WRITE(i2c_addr,tx_buf, 2, 0),
  };

  ret_code_t result = nrf_twi_mngr_perform(i2c_manager, NULL, write_transfer, 1, NULL);
  
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

// Initialize and configure the LSM303AGR accelerometer/magnetometer
//
// i2c - pointer to already initialized and enabled twim instance
void lsm303agr_init(const nrf_twi_mngr_t* i2c) {
  i2c_manager = i2c;

  printf("WHO_AM_I_A: 0x%X\n", i2c_reg_read(0x57, 0xFF));
  // printf("WHO_AM_I_A: 0x%X\n", i2c_reg_read(0x48, 0x02));

  // ---Initialize Accelerometer---

  // Reboot acclerometer
  // i2c_reg_write(LSM303AGR_ACC_ADDRESS, CTRL_REG5_A, 0x80);
  nrf_delay_ms(100); // needs delay to wait for reboot

  // Enable Block Data Update
  // Only updates sensor data when both halves of the data has been read
  // i2c_reg_write(LSM303AGR_ACC_ADDRESS, CTRL_REG4_A, 0x80);

  // Configure accelerometer at 100Hz, normal mode (10-bit)
  // Enable x, y and z axes
  // i2c_reg_write(LSM303AGR_ACC_ADDRESS, CTRL_REG1_A, 0x57);

  // Read WHO AM I register
  // Always returns the same value if working
  //TODO: read the Accelerometer WHO AM I register and check the result
  // printf("WHO_AM_I_A: 0x%X\n", i2c_reg_read(LSM303AGR_ACC_ADDRESS, WHO_AM_I_A));
  // printf("WHO_AM_I_M: 0x%X\n", i2c_reg_read(LSM303AGR_MAG_ADDRESS, WHO_AM_I_M));

  // ---Initialize Magnetometer---

  // Reboot magnetometer
  // i2c_reg_write(LSM303AGR_MAG_ADDRESS, CFG_REG_A_M, 0x40);
  nrf_delay_ms(100); // needs delay to wait for reboot

  // Enable Block Data Update
  // Only updates sensor data when both halves of the data has been read
  // i2c_reg_write(LSM303AGR_MAG_ADDRESS, CFG_REG_C_M, 0x10);

  // Configure magnetometer at 100Hz, continuous mode
  // i2c_reg_write(LSM303AGR_MAG_ADDRESS, CFG_REG_A_M, 0x0C);

  // Read WHO AM I register
  //TODO: read the Magnetometer WHO AM I register and check the result

  // ---Initialize Temperature---

  // Enable temperature sensor
  // i2c_reg_write(LSM303AGR_ACC_ADDRESS, TEMP_CFG_REG_A, 0xC0);
}

// Read the internal temperature sensor
//
// Return measurement as floating point value in degrees C
float lsm303agr_read_temperature(void) {
  //TODO: implement me
  int8_t lower = i2c_reg_read(LSM303AGR_ACC_ADDRESS, OUT_TEMP_L_A);
  int8_t higher = i2c_reg_read(LSM303AGR_ACC_ADDRESS, OUT_TEMP_H_A);

  printf("Lower: %d\n", lower);
  printf("Higher: %d\n", higher);

  int16_t temp = lower + (higher << 8);
  float converted = (float)temp;
  float result = converted/256.0 + 25.0;
  printf("TEMP: %f\n", result);
  return result;
}

void temp_callback(void* _unused) {
  lsm303agr_read_temperature();
}

lsm303agr_measurement_t lsm303agr_read_accelerometer(void) {
  //TODO: implement me
  int8_t acc_x_l = i2c_reg_read(LSM303AGR_ACC_ADDRESS, OUT_X_L_A);
  int8_t acc_x_h = i2c_reg_read(LSM303AGR_ACC_ADDRESS, OUT_X_H_A);

  int16_t acc_x = acc_x_l + (acc_x_h << 8);
  acc_x = acc_x >> 6;
  float dec_x = (float) acc_x;
  dec_x = (dec_x * 3.9)/1000;

  int8_t acc_y_l = i2c_reg_read(LSM303AGR_ACC_ADDRESS, OUT_Y_L_A);
  int8_t acc_y_h = i2c_reg_read(LSM303AGR_ACC_ADDRESS, OUT_Y_H_A);

  int16_t acc_y = acc_y_l + (acc_y_h << 8);
  acc_y = acc_y >> 6;
  float dec_y = (float) acc_y;
  dec_y = (dec_y * 3.9)/1000;

  int8_t acc_z_l =  i2c_reg_read(LSM303AGR_ACC_ADDRESS, OUT_Z_L_A);
  int8_t acc_z_h = i2c_reg_read(LSM303AGR_ACC_ADDRESS, OUT_Z_H_A);

  int16_t acc_z = acc_z_l + (acc_z_h << 8);
  acc_z = acc_z >> 6;
  float dec_z = (float) acc_z;
  dec_z = (dec_z * 3.9)/1000;

  lsm303agr_measurement_t measurement = {dec_x, dec_y, dec_z};
  return measurement;
}

lsm303agr_measurement_t lsm303agr_read_magnetometer(void) {

  int8_t mag_x_l = i2c_reg_read(LSM303AGR_MAG_ADDRESS, OUTX_L_REG_M);
  int8_t mag_x_h = i2c_reg_read(LSM303AGR_MAG_ADDRESS, OUTX_H_REG_M);
  int16_t mag_x = mag_x_l + (mag_x_h << 8);
  float dec_x = (float) mag_x;
  dec_x = (dec_x * 1.5)/10;

  int8_t mag_y_l = i2c_reg_read(LSM303AGR_MAG_ADDRESS, OUTY_L_REG_M);
  int8_t mag_y_h = i2c_reg_read(LSM303AGR_MAG_ADDRESS, OUTY_H_REG_M);
  int16_t mag_y = mag_y_l + (mag_y_h << 8);
  float dec_y = (float) mag_y;
  dec_y = (dec_y * 1.5)/10;

  int8_t mag_z_l = i2c_reg_read(LSM303AGR_MAG_ADDRESS, OUTZ_L_REG_M);
  int8_t mag_z_h = i2c_reg_read(LSM303AGR_MAG_ADDRESS, OUTZ_H_REG_M);
  int16_t mag_z = mag_z_l + (mag_z_h << 8);
  float dec_z = (float) mag_z; 
  dec_z = (dec_z * 1.5)/10;

  lsm303agr_measurement_t measurement = {dec_x, dec_y, dec_z};

  return measurement;
}

void print_measurement(void* unused){
  lsm303agr_measurement_t mag = lsm303agr_read_magnetometer();
  printf("Mag X: %f\n", mag.x_axis);
  printf("Mag Y: %f\n", mag.y_axis);
  printf("Mag Z: %f\n", mag.z_axis);
  lsm303agr_measurement_t acc = lsm303agr_read_accelerometer();
  printf("Acc X: %f\n", acc.x_axis);
  printf("Acc Y: %f\n", acc.y_axis);
  printf("Acc Z: %f\n", acc.z_axis);
}

void tilt_angle(void* unused){
  lsm303agr_measurement_t acc = lsm303agr_read_accelerometer();
  float x = acc.x_axis;
  float y = acc.y_axis;
  float z = acc.z_axis;
  
  float theta = atan(x/sqrt(y*y + z*z)) * 57.2958;
  float psi = atan(y/sqrt(x*x + z*z)) * 57.2958;
  float phi = atan(sqrt(x*x + y*y)/z) * 57.2958;

  printf("Theta: %f\n", theta);
  printf("Psi: %f\n", psi);
  printf("Phi: %f\n", phi);
}
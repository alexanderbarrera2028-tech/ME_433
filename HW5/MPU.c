#include "MPU.h"
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include <stdio.h>
#include <stdint.h>


#define ADDR 0x68

// config registers
#define CONFIG 0x1A
#define GYRO_CONFIG 0x1B
#define ACCEL_CONFIG 0x1C
#define PWR_MGMT_1 0x6B
#define PWR_MGMT_2 0x6C
// sensor data registers:
#define ACCEL_XOUT_H 0x3B
#define ACCEL_XOUT_L 0x3C
#define ACCEL_YOUT_H 0x3D
#define ACCEL_YOUT_L 0x3E
#define ACCEL_ZOUT_H 0x3F
#define ACCEL_ZOUT_L 0x40
#define TEMP_OUT_H   0x41
#define TEMP_OUT_L   0x42
#define GYRO_XOUT_H  0x43
#define GYRO_XOUT_L  0x44
#define GYRO_YOUT_H  0x45
#define GYRO_YOUT_L  0x46
#define GYRO_ZOUT_H  0x47
#define GYRO_ZOUT_L  0x48
#define WHO_AM_I     0x75


//Setup I2C
void setReg(uint8_t reg ,uint8_t value){
    uint8_t buf[2];
    buf[0] = reg;
    buf[1] = value;
    i2c_write_blocking(I2C_PORT, ADDR, buf, 2, false);
}

uint8_t readReg(uint8_t reg){
    uint8_t buf;
    i2c_write_blocking(I2C_PORT, ADDR, &reg, 1, true);  // true to keep host control of bus
    i2c_read_blocking(I2C_PORT, ADDR, &buf, 1, false);  // false - finished with bus
    return buf;
}

void MPUinit() {
    // initialize I2C at 400kHz
    i2c_init(I2C_PORT, 400 * 1000);
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA);
    gpio_pull_up(I2C_SCL);

    // check WHO_AM_I; if incorrect, turn on LED 
    uint8_t check = readReg(WHO_AM_I);
    if (check != 0x68 && check != 0x98) {
        gpio_put(PICO_DEFAULT_LED_PIN, 1);
        while (1);
    }
    // wake the chip up
    setReg(PWR_MGMT_1, 0x00);
    // set sensitivity
    setReg(ACCEL_CONFIG, 0x00);
    // set dps
    setReg(GYRO_CONFIG, 0b00011000);
}



void MPUread(imuData* datap) {
    uint8_t reg = 0x3B;
    uint8_t buf[14];
    i2c_write_blocking(I2C_PORT, ADDR, &reg, 1, true);  // true to keep host control of bus
    i2c_read_blocking(I2C_PORT, ADDR, buf, 14, false);  // false - finished with bus

    signed short accel_x = (buf[0] << 8) | buf[1];
    signed short accel_y = (buf[2] << 8) | buf[3];
    signed short accel_z = (buf[4] << 8) | buf[5];
    signed short temp = (buf[6] << 8) | buf[7];
    signed short gyro_x = (buf[8] << 8) | buf[9];
    signed short gyro_y = (buf[10] << 8) | buf[11];
    signed short gyro_z = (buf[12] << 8) | buf[13];

    datap->accel_x = accel_x * 0.000061; // convert to g
    datap->accel_y = accel_y * 0.000061; // convert to g
    datap->accel_z = accel_z * 0.000061; // convert to g
    datap->temp = temp/340.00 + 36.53; // convert to deg C
    datap->gyro_x = gyro_x * 0.007630; // convert to deg/s
    datap->gyro_y = gyro_y * 0.007630; // convert to deg/s
    datap->gyro_z = gyro_z * 0.007630; // convert to deg/s
}


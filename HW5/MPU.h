#ifndef MPU_H
#define MPU_H
#include <stdint.h>

#define I2C_PORT i2c0
#define I2C_SDA 16
#define I2C_SCL 17

typedef struct {
    double accel_x; // unit (g)
    double accel_y; // unit (g)
    double accel_z; // unit (g)
    double temp; // unit (deg c)
    double gyro_x; // unit (deg/s)
    double gyro_y; // unit (deg/s)
    double gyro_z; // unit (deg/s)
} imuData;

uint8_t MPUcheck();
void MPUinit();
void MPUread();

#endif
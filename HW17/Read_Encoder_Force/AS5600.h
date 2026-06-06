#ifndef AS5600_h
#define AS5600_h

#include <stdint.h>
#include <stdbool.h>

#ifdef PICO_BOARD
#include "hardware/i2c.h"
typedef i2c_inst_t AS5600_I2C_t;
#endif

#ifdef STM32C0XX_NUCLEO_CONF_H
//#include "stm32c0xx_hal_conf.h"
typedef I2C_HandleTypeDef AS5600_I2C_t;
#endif

#define AS5600_DEFAULT_ADDRESS  0x36

struct AS5600{
    AS5600_I2C_t *i2c;
    uint16_t offset;
};

struct AS5600 AS5600_create(AS5600_I2C_t *address);

bool AS5600_isConnected(struct AS5600 *enc);

uint16_t AS5600_rawAngle(struct AS5600 *encoder);
float AS5600_readAngle(struct AS5600 *encoder);

void AS5600_setOffset(struct AS5600 *encoder, float degrees);

bool AS5600_magnetDetected(struct AS5600 *encoder);
bool AS5600_magnetTooStrong(struct AS5600 *encoder);
bool AS5600_magnetTooWeak(struct AS5600 *encoder);

float encoder_normalized(float angle);

#endif

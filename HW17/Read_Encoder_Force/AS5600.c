#include "AS5600.h"
#include "hardware/i2c.h"
#include <stdint.h>
#include <stdlib.h>
#include "math.h"

// ------------------- Registers ------------------- //

//  OUTPUT REGISTERS
#define AS5600_RAW_ANGLE 0x0C   //  + 0x0D
#define AS5600_ANGLE     0x0E   //  + 0x0F

//  STATUS REGISTERS
#define AS5600_STATUS    0x0B

// --------------- Conversion Numbers --------------- //
static const float   AS5600_RAW_TO_DEGREES   =   360.0 / 4096;
static const float   AS5600_DEGREES_TO_RAW    =  4096 / 360.0;

//
static int as5600_write(
    struct AS5600 *enc,
    uint8_t *data,
    size_t len,
    bool nostop);

static int as5600_read(
    struct AS5600 *enc,
    uint8_t *data,
    size_t len);

// ----------------- Struct Creation ----------------- //
struct AS5600 AS5600_create(AS5600_I2C_t *i2c){
    struct AS5600 encoder;
    encoder.i2c = i2c;
    encoder.offset = 0;
    return encoder;
}

// ------------ Helper Function Prototypes ------------ //

/// Reads 8 bits from a register
uint8_t readReg(struct AS5600* encoder, uint8_t reg);
/// Reads 16 bits from a register
uint16_t readReg2(struct AS5600* encoder, uint8_t reg);

// -------------------- Functions -------------------- //

/// Returns True if AS5600 is detected
bool AS5600_isConnected(struct AS5600 *encoder){
    uint8_t reg = 0x0B;

#ifdef PICO_BOARD

    return i2c_write_blocking(
        encoder->i2c,
        AS5600_DEFAULT_ADDRESS,
        &reg,
        1,
        true) == 1;

#elif defined(PLATFORM_STM32)

    return HAL_I2C_IsDeviceReady(
        encoder->i2c,
        AS5600_DEFAULT_ADDRESS << 1,
        3,
        HAL_MAX_DELAY) == HAL_OK;

    #endif
}

/// Reports raw angle (0-4095)
uint16_t AS5600_rawAngle(struct AS5600* encoder){
    int16_t value = readReg2(encoder, AS5600_RAW_ANGLE);

    if (encoder->offset > 0) value += encoder->offset;
    value &= 0x0FFF;

    return value;
}

/// Reports angle (0-360 degrees)
float AS5600_readAngle(struct AS5600* encoder){
    uint16_t value = readReg2(encoder, AS5600_ANGLE);

    if (encoder->offset > 0) value += encoder->offset;
    value &= 0x0FFF;

    return value*AS5600_RAW_TO_DEGREES;
}

/// Sets offset angle (degrees)
void AS5600_setOffset(struct AS5600* encoder, float degrees){
    bool neg = (degrees < 0);
    if (neg) degrees = -degrees;

    uint16_t offset = round(degrees * AS5600_DEGREES_TO_RAW);
    offset &= 0x0FFF;
    if (neg) offset = (4096 - offset) & 0x0FFF;
    encoder->offset = offset;
}

/// Returns True if magnet is detected
bool AS5600_magnetDetected(struct AS5600* encoder){
    return (readReg(encoder, AS5600_STATUS) & 0x20) > 1;
}

/// Returns True if magnet is too close
bool AS5600_magnetTooStrong(struct AS5600* encoder){
    return (readReg(encoder, AS5600_STATUS) & 0x8) > 1;
}

/// Returns True if magnet is too far
bool AS5600_magnetTooWeak(struct AS5600* encoder){
    return (readReg(encoder, AS5600_STATUS) & 0x10) > 1;
}

// ------------------- Helper Functions ------------------- //

uint8_t readReg(struct AS5600* encoder, uint8_t reg)
{
    uint8_t data;

#ifdef PICO_BOARD

    i2c_write_blocking(
        encoder->i2c,
        AS5600_DEFAULT_ADDRESS,
        &reg,
        1,
        true);

    i2c_read_blocking(
        encoder->i2c,
        AS5600_DEFAULT_ADDRESS,
        &data,
        1,
        false);

#elif defined(PLATFORM_STM32)

    HAL_I2C_Mem_Read(
        encoder->i2c,
        AS5600_DEFAULT_ADDRESS << 1,
        reg,
        I2C_MEMADD_SIZE_8BIT,
        &data,
        1,
        HAL_MAX_DELAY);

#endif

    return data;
}
uint16_t readReg2(struct AS5600* encoder, uint8_t reg)
{
    uint8_t buf[2];

#ifdef PICO_BOARD

    i2c_write_blocking(
        encoder->i2c,
        AS5600_DEFAULT_ADDRESS,
        &reg,
        1,
        true);

    i2c_read_blocking(
        encoder->i2c,
        AS5600_DEFAULT_ADDRESS,
        buf,
        2,
        false);

#elif defined(PLATFORM_STM32)

    HAL_I2C_Mem_Read(
        encoder->i2c,
        AS5600_DEFAULT_ADDRESS << 1,
        reg,
        I2C_MEMADD_SIZE_8BIT,
        buf,
        2,
        HAL_MAX_DELAY);

#endif

    return ((uint16_t)buf[0] << 8) | buf[1];
}

float encoder_normalized(float angle)
{
    const float center = 187.0f;
    const float half_range = 93.0f; // 280 - 187

    float value = (angle - center) / half_range;

    if (value > 1.0f)
        value = 1.0f;

    if (value < -1.0f)
        value = -1.0f;

    return value;
}


#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/spi.h"
#include <math.h>

#define PIN_CS 13
#define SCK_PIN 14
#define TX_PIN 15
#define SAMPLES 4000

static inline void cs_select() {
    asm volatile("nop \n nop \n nop");
    gpio_put(PIN_CS, 0);
    asm volatile("nop \n nop \n nop");
}

static inline void cs_deselect() {
    asm volatile("nop \n nop \n nop");
    gpio_put(PIN_CS, 1);
    asm volatile("nop \n nop \n nop");
}

void writeDAC(int channel, uint16_t myV) {
    uint8_t buf[2];
    buf[0] = 0b01110000;
    buf[0] = buf[0] | (channel << 7);
    buf[0] = buf[0] | (uint8_t)((myV >> 6) & 0b00001111);
    buf[1] = (uint8_t)((myV & 0b00111111) << 2);

    cs_select();
    spi_write_blocking(spi1, buf, 2);
    cs_deselect();
}

int main() {
    stdio_init_all();
    spi_init(spi1, 1000 * 1000);
    gpio_set_function(SCK_PIN, GPIO_FUNC_SPI);
    gpio_set_function(TX_PIN, GPIO_FUNC_SPI);

    gpio_init(PIN_CS);
    gpio_set_dir(PIN_CS, GPIO_OUT);
    gpio_put(PIN_CS, 1);

    uint16_t sinTable[SAMPLES];
    uint16_t triTable[SAMPLES];

    // sine 
    for (int i = 0; i < SAMPLES; i++) {
        sinTable[i] = (uint16_t)(((sinf((2.0f * M_PI * i) / SAMPLES) * 0.5f) + 0.5f) * 1023.0f + 0.5f);
    }

    // triangle 
    for (int i = 0; i < SAMPLES / 2; i++) {
        triTable[i] = (uint16_t)(1023.9999 * (i / (SAMPLES / 2.0 - 1)));
        triTable[SAMPLES - 1 - i] = (uint16_t)(1023.9999 * (i / (SAMPLES / 2.0 - 1)));
    }

    int t1 = 0;
    int t2 = 0;

    float freq = 1.0f; // 1Hz triangle, 2Hz sine (2 cycles in sine table)
    uint32_t dt_us = (uint32_t)(1e6f / freq / SAMPLES);

    while (true) {
        writeDAC(0, sinTable[t1]);
        writeDAC(1, triTable[t2]);
        sleep_us(dt_us);
        t1 = (t1 + 2) % SAMPLES;
        t2 = (t2 + 1) % SAMPLES;    
    }
}
    





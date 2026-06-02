#include "HX711_helpers.h"
#define DOUT_PIN 17
#define SCK_PIN 16
#define clock_time_us 50 //delay time in microseconds

void hx711_init(void) {
    gpio_init(DOUT_PIN);
    gpio_set_dir(DOUT_PIN, GPIO_IN);
    gpio_pull_up(DOUT_PIN); //enable pull-up resistor

    gpio_init(SCK_PIN);
    gpio_set_dir(SCK_PIN, GPIO_OUT);
    gpio_put(SCK_PIN, 0); //set SCK low
}

int hx711_read_raw(void) {
    //wait for data to be ready (Dout goes low)
    while (gpio_get(DOUT_PIN)) {
        tight_loop_contents();
    }

    unsigned int raw = 0;
    for (int i = 0; i < 24; i++) {
        gpio_put(SCK_PIN, 1); //set SCK low
        sleep_us(clock_time_us); //small delay
        raw = (raw<<1)| (gpio_get(DOUT_PIN) ? 1 : 0); //shift left by 1
        gpio_put(SCK_PIN, 0); //set SCK high
        sleep_us(clock_time_us); //small delay
    }
    
    //25th pulse to set gain to 128
    gpio_put(SCK_PIN, 1);
    sleep_us(clock_time_us); //small delay
    gpio_put(SCK_PIN, 0);
    sleep_us(clock_time_us); //small delay

    //convert from 24-bit two's complement to signed int
    if (raw & 0x800000) { //if the sign bit is set
        raw |= 0xFF000000; //set upper bits to 1 for negative value
    }
    return (int)raw;
}

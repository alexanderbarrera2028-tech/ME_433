#ifndef HX711_HELPERS_H
#define HX711_HELPERS_H

#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include <stdio.h>


void hx711_init(void);
int hx711_read_raw(void);

#endif // HX711_HELPERS_H
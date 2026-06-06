#ifndef HX711_HELPER3_H
#define HX711_HELPER3_H

#include "pico/stdlib.h"

// Initialization
void hx711_init(void);

// Raw and filtered readings
int hx711_read_raw(void);
int hx711_read_filtered(void);

// Range calibration
void hx711_set_range(void);

// Returns value between 0.0 and 1.0
float hx711_read_normalized(void);

#endif
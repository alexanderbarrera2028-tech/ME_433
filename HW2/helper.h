#ifndef HELPER_H
#define HELPER_H

#define PWM_PIN 16 // define the GPIO pin number for the PWM signal
#define wrap_value 60000 // define the wrap value for the PWM signal, which determines the
#define DIV 50

#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/adc.h"
#include "hardware/pwm.h"

bool timer_callback(struct repeating_timer *t);
void init_pwm();
void setServo(int angle);

#endif 
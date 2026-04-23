#include "helper.h"


void setServo(int angle) { // function to set the PWM duty cycle based on the desired angle
    pwm_set_gpio_level(PWM_PIN, (int)((.015+(angle/180.0)*.105)*wrap_value)); // set the PWM duty cycle for the specified pin
}

void init_pwm() { // function to initialize the PWM settings
    gpio_set_function(PWM_PIN, GPIO_FUNC_PWM); // set the PWM pin to be controlled by the PWM hardware
    uint slice_num = pwm_gpio_to_slice_num(PWM_PIN); // get the PWM slice number for the specified pin
    float div = DIV;
    pwm_set_clkdiv(slice_num, DIV);
    pwm_set_wrap(slice_num, wrap_value); // set the wrap value for the PWM slice
    pwm_set_enabled(slice_num, true); // enable the PWM slice
    pwm_set_gpio_level(PWM_PIN, 0); // start with a duty cycle of 0%
}
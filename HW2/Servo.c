#include <stdio.h>
#include "pico/stdlib.h"
#include "helper.h"

int main() {
    stdio_init_all();
    init_pwm();

    adc_init(); // initialize the ADC hardware
    adc_gpio_init(26); // initialize GPIO 26 for ADC input
    adc_select_input(0); // select ADC input 0 (GPIO 26)

    while (1) {
        int i = 0;
        for (i=10; i<170; i++) {
            setServo(i); // set the PWM duty cycle to the current value of i
            sleep_ms(10); // delay for a short period to allow the servo to move
        }
        for (i=170; i>10; i--) {
            setServo(i); // set the PWM duty cycle to the current value of i
            sleep_ms(10); // delay for a short period to allow the servo to move
        }
    }
    return 0;
}

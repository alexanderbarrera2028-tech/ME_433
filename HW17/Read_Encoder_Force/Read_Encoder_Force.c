#include <pico/time.h>
#include <stdint.h>
#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "hardware/adc.h"
#include "AS5600.h"
#include "HX711_Helper3.h"

#define I2C_USED i2c1
#define Data_pin 14
#define Clock_Pin 15

int main()
{
    stdio_init_all();

    i2c_init(I2C_USED, 400*1000);
    gpio_set_function(Data_pin, GPIO_FUNC_I2C);
    gpio_set_function(Clock_Pin, GPIO_FUNC_I2C);
    gpio_pull_up(Data_pin);
    gpio_pull_up(Clock_Pin);

    hx711_init();

    while (!stdio_usb_connected())
    {
        sleep_ms(100);
    }

    struct AS5600 encoder = AS5600_create(I2C_USED);
    if(AS5600_isConnected(&encoder)){
        printf("connected\n");
    }
    else return 0;
    
    AS5600_setOffset(&encoder, 10.0);
    hx711_init();

    // Calibration process
    printf("Apply full range of forces for 5 seconds...\n");
    hx711_set_range();
    
    //Read HX711 and AS5600 in a loop, print the values to the console
    while (true) {
        //Test HX711 Calibration
        float force = hx711_read_normalized();

        // AS5600
        if(!AS5600_magnetDetected(&encoder)){
            printf("Magnet Not Detected!\n");
        }
        else if(AS5600_magnetTooStrong(&encoder)){
            printf("Magnet too close!\n");
        }
        else if(AS5600_magnetTooWeak(&encoder)){
            printf("Magnet too far!\n");
        }
        else{
            printf("(%.3f,%.3f)\n", force, encoder_normalized(AS5600_readAngle(&encoder)));
        }
        sleep_ms(20);
    }
}
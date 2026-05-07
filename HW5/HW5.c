#include <stdio.h>
#include "pico/stdlib.h"
#include "MPU.h"
#include "ssd1306.h"
#include "hardware/i2c.h"
#include "font.h"
#include <math.h>

int main()
{
    stdio_init_all();
    gpio_init(PICO_DEFAULT_LED_PIN);
    gpio_set_dir(PICO_DEFAULT_LED_PIN, GPIO_OUT);   
    sleep_ms(100);

    while(!stdio_usb_connected()) {
        tight_loop_contents();
    }

    sleep_ms(1000);
    printf("USB Connected\n");

    MPUinit();
    printf("MPU Initialized\n");

    sleep_ms(10);
    ssd1306_setup();
    printf("SSD1306 Initialized\n");

    imuData data;

    while (true) {
        ssd1306_clear();
        MPUread(&data);
        printf("Accel X: %f g, Accel Y: %f g\n", data.accel_x, data.accel_y);

        // Compute the angle of tilt in radians
        float angle = atan2f(data.accel_y, data.accel_x);

        // Compute the magnitude of the tilt vector
        float magnitude = sqrtf(data.accel_x * data.accel_x + data.accel_y * data.accel_y);

        // Scale magnitude to pixel length, clamp to max 20px
        int len = (int)(magnitude * 20);
        if (len > 20) len = 20;

        // Draw a line from center outward along the tilt angle
        for (int i = 0; i <= len; i++) {
            int px = 64 + (int)(-cosf(angle) * i);
            int py = 15 + (int)(sinf(angle) * i);
            ssd1306_drawPixel(px, py, 1);
        }

        ssd1306_update();
        sleep_ms(1000/100); // update at 100Hz
    }
}
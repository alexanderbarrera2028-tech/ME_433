#include <stdio.h>
#include "pico/stdlib.h"
#include "HX711_helpers.h"


int main()
{

    stdio_init_all();

    while (!stdio_usb_connected()) {
        tight_loop_contents();
    }
    sleep_ms(100); // pause

    printf("HX711 test\n");
    sleep_ms(100); // pause
    hx711_init();
    printf("HX711 initialized\n");
    sleep_ms(100); // pause

    int i = 0;
    uint64_t last_t = 0;


    while (true) {
        int v[1000];
        int raw[1000];
        int num = 0;
        uint64_t t[1000];
        scanf("%d", &num); //wait for print from python client
        sleep_ms(100); // pause
        int avg = 0;
        for (int i = 0; i < num; i++) {
            int value = hx711_read_raw();
            raw[i] = value;
            if (i == 0) {
                avg = value;
            } else {
                avg = value * 0.1 + avg * 0.9; //simple low-pass filter
            }
            v[i] = avg;
            t[i] = to_ms_since_boot(get_absolute_time());
        }
        for (int i = 0; i < num; i++) {
            printf("%d %llu %d %d\n", i, t[i], v[i], raw[i]);
        }
    }
}

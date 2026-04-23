#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "hardware/adc.h"
#include "ssd1306.h"
#include "font.h"

#define I2C_PORT i2c1
#define I2C_SDA 16
#define I2C_SCL 17

void draw_letter(int x, int y, char c){
    if (c < 32 || c > 127) return; //error check for non-printable characters
        
    for (int i = 0; i<=4;i++){
        for (int j =  0; j < 8; j++){
            if ((ASCII[c-32][i]>>j)&1) 
                ssd1306_drawPixel(i+x,y+j,1);
            else 
                ssd1306_drawPixel(i+x,y+j,0);
        }
    }   
}

void draw_message(int x, int y, char *str){
    int counter = 0;
    while(str[counter]!='\0'){
        draw_letter(x+counter*5,y,str[counter]);
        counter++;
    }
}

int main()
{
    stdio_init_all();

    i2c_init(I2C_PORT, 400*1000);
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA);
    gpio_pull_up(I2C_SCL);
    adc_init();
    adc_gpio_init(26);
    adc_select_input(0);

    // setup
    sleep_ms(10);
    ssd1306_setup();

    /*Test MEssage
    char message[] = "Hello World!";


    while (1) {
        draw_message(0,0,message);
        ssd1306_update();
    }
        */

    //FPS
char filler[] = "--------------------";
uint64_t t1 = to_us_since_boot(get_absolute_time());
float frames = 0;
char message2[50] = "FPS = __";   // default before first second

while (true) {
    float result = (adc_read() / (float)4095) * 3.3;
    char message[50];
    sprintf(message, "Voltage = %.2f", result);
    draw_message(0, 0, message);
    //draw_message(0, 12, filler);

    uint64_t t2 = to_us_since_boot(get_absolute_time());
    frames++;

    if (t2 - t1 >= 1000000) {
        float elapsed_time = (t2 - t1) / 1000000.0;
        float fps = frames / elapsed_time;
        t1 = t2;
        frames = 0;
        sprintf(message2, "FPS = %.2f", fps);  // update the string
    }

    draw_message(0, 25, message2);  // draw every frame from stored string
    ssd1306_update();
}
}
#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"

#define I2C_PORT i2c1
#define I2C_SDA 14
#define I2C_SCL 15

#define PIN 17

#define ADDR 0x20

#define IODIR 0x00
#define GPIO  0x09
#define OLAT  0x0A


void setPin(unsigned char addr, unsigned char reg, unsigned char value) {
    unsigned char buf[2];
    buf[0] = reg;
    buf[1] = value;
    i2c_write_blocking(I2C_PORT, addr, buf, 2, false);
}

unsigned char readPin(unsigned char addr, unsigned char reg) {
    unsigned char value;
    i2c_write_blocking(I2C_PORT, addr, &reg, 1, true);// tell chip which register we want
    i2c_read_blocking(I2C_PORT, addr, &value, 1, false);// read the register
    return value;
}


int main()
{
    stdio_init_all();

    while(!stdio_usb_connected()) {
        tight_loop_contents();
    }
    sleep_ms(1000); // wait a moment for the USB connection to stabilize
    printf("USB connected, starting I2C expander test...\n");

    gpio_init(PIN);
    gpio_set_dir(PIN, GPIO_OUT);

    
    i2c_init(I2C_PORT, 400*1000);
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA);  // didn't have 1k resistors
    gpio_pull_up(I2C_SCL);  // didn't have 1k resistors

    sleep_ms(100); // let bus settle before first transaction


    //debug test
    printf("Starting scan...\n");
    for (int addr = 0x08; addr < 0x78; addr++) {
        uint8_t dummy;
        int ret = i2c_read_blocking(I2C_PORT, addr, &dummy, 1, false);
        if (ret >= 0) {
            printf("0x%02X: %d\n", addr, ret);
        }
    }

    printf("Scan done\n");

    
    // set pin directions: GP7 = output, others = input
    setPin(ADDR, IODIR, 0x7F);
    
    

    while (true) {
        
        unsigned char gpio = readPin(ADDR, GPIO); //read button

        unsigned char val = readPin(ADDR, OLAT); //read current output latch

        //if button is pressed, turn GP7 off, leave it on otherwise
        if ((gpio & (1 << 0)) == 0) {
            // button pressed → turn GP7 off
            val &= ~(1 << 7);
        } else {
            // button not pressed → turn GP7 on
            val |= (1 << 7);
        }

        // write updated value back
        setPin(ADDR, OLAT, val);

        //heartbeat
        gpio_put(PIN, 1);
        sleep_ms(500);
        gpio_put(PIN, 0);
        sleep_ms(500);
    }

}


#include "HX711_Helper3.h"

#include <stdint.h>
#include <limits.h>

#define DOUT_PIN 17
#define SCK_PIN 16
#define clock_time_us 50

// EMA filter variables
static float filtered_value = 0.0f;
static bool filter_initialized = false;

// Learned force range
static int min_force = INT_MAX;
static int max_force = INT_MIN;

void hx711_init(void)
{
    gpio_init(DOUT_PIN);
    gpio_set_dir(DOUT_PIN, GPIO_IN);
    gpio_pull_up(DOUT_PIN);

    gpio_init(SCK_PIN);
    gpio_set_dir(SCK_PIN, GPIO_OUT);
    gpio_put(SCK_PIN, 0);
}

int hx711_read_raw(void)
{
    while (gpio_get(DOUT_PIN))
    {
        tight_loop_contents();
    }

    unsigned int raw = 0;

    for (int i = 0; i < 24; i++)
    {
        gpio_put(SCK_PIN, 1);
        sleep_us(clock_time_us);

        raw = (raw << 1) |
              (gpio_get(DOUT_PIN) ? 1 : 0);

        gpio_put(SCK_PIN, 0);
        sleep_us(clock_time_us);
    }

    // 25th pulse sets gain to 128
    gpio_put(SCK_PIN, 1);
    sleep_us(clock_time_us);

    gpio_put(SCK_PIN, 0);
    sleep_us(clock_time_us);

    // Sign extension
    if (raw & 0x800000)
    {
        raw |= 0xFF000000;
    }

    return (int)raw;
}

int hx711_read_filtered(void)
{
    int raw = hx711_read_raw();

    if (!filter_initialized)
    {
        filtered_value = (float)raw;
        filter_initialized = true;
    }

    // Exponential Moving Average
    filtered_value =
        0.90f * filtered_value +
        0.10f * raw;

    return (int)filtered_value;
}

void hx711_set_range(void)
{
    min_force = INT_MAX;
    max_force = INT_MIN;

    filter_initialized = false;

    uint32_t start_time =
        to_ms_since_boot(get_absolute_time());

    while ((to_ms_since_boot(get_absolute_time()) -
            start_time) < 5000)
    {
        int value = hx711_read_filtered();

        if (value < min_force)
        {
            min_force = value;
        }

        if (value > max_force)
        {
            max_force = value;
        }

        sleep_ms(10);
    }
}

float hx711_read_normalized(void)
{
    int value = hx711_read_filtered();

    if (max_force <= min_force)
    {
        return 0.5f;
    }

    if (value < min_force)
    {
        value = min_force;
    }

    if (value > max_force)
    {
        value = max_force;
    }

    return (float)(value - min_force) /
           (float)(max_force - min_force);
}
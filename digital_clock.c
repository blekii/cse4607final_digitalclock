#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>

#ifndef DESKTOP_DEMO
#include "pico/stdlib.h"
#include "hardware/i2c.h"

#define I2C_PORT i2c0
#define I2C_SDA 4
#define I2C_SCL 5
#define BUTTON_TIME 14
#define BUTTON_TEMP 15
#endif

// Demo data until the RTC, NTP, weather API, and OLED are connected.
static uint32_t demo_seconds = 13 * 3600 + 25 * 60;
static const int temperature_c10 = 225; // 22.5 C, in tenths
static bool use_12_hour = false;
static bool use_fahrenheit = false;

static void show_clock(void) {
    uint32_t today = demo_seconds % (24 * 3600);
    int hours = today / 3600;
    int minutes = (today / 60) % 60;
    int seconds = today % 60;

    char time_text[20];
    if (use_12_hour) {
        int hour_12 = hours % 12;
        if (hour_12 == 0) hour_12 = 12;
        snprintf(time_text, sizeof(time_text), "%02d:%02d:%02d %s",
                 hour_12, minutes, seconds, hours < 12 ? "AM" : "PM");
    } else {
        snprintf(time_text, sizeof(time_text), "%02d:%02d:%02d",
                 hours, minutes, seconds);
    }

    int temperature10 = use_fahrenheit
        ? (temperature_c10 * 9 / 5) + 320
        : temperature_c10;

    printf("\n=== DIGITAL CLOCK DEMO ===\n");
    printf("Time: %s\n", time_text);
    printf("Date: Nov 23, 2026 (simulated)\n");
    printf("Temp: %d.%d %c\n", temperature10 / 10,
           temperature10 % 10, use_fahrenheit ? 'F' : 'C');
    printf("Weather: Cloudy (simulated)\n");
}

#ifdef DESKTOP_DEMO
int main(void) {
    char command[32];
    printf("Laptop demo: n=next second, m=next minute, t=12/24h, u=C/F, q=quit\n");
    show_clock();

    while (true) {
        printf("\nCommand: ");
        if (!fgets(command, sizeof(command), stdin)) break;
        switch (command[0]) {
            case 'n': demo_seconds++; break;
            case 'm': demo_seconds += 60; break;
            case 't': use_12_hour = !use_12_hour; break;
            case 'u': use_fahrenheit = !use_fahrenheit; break;
            case 'q': return 0;
            default: printf("Unknown command.\n"); continue;
        }
        show_clock();
    }
    return 0;
}
#else
int main(void) {
    stdio_init_all();

    // Prepare the I2C bus for the future OLED and RTC modules.
    i2c_init(I2C_PORT, 100 * 1000);
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA);
    gpio_pull_up(I2C_SCL);

    // Buttons connect each GPIO to GND when pressed.
    gpio_init(BUTTON_TIME);
    gpio_set_dir(BUTTON_TIME, GPIO_IN);
    gpio_pull_up(BUTTON_TIME);
    gpio_init(BUTTON_TEMP);
    gpio_set_dir(BUTTON_TEMP, GPIO_IN);
    gpio_pull_up(BUTTON_TEMP);

    bool previous_time_pressed = false;
    bool previous_temp_pressed = false;
    uint32_t last_time_press_ms = 0;
    uint32_t last_temp_press_ms = 0;
    uint32_t last_second = to_ms_since_boot(get_absolute_time()) / 1000;
    bool needs_redraw = true;

    while (true) {
        uint32_t now_ms = to_ms_since_boot(get_absolute_time());
        uint32_t current_second = now_ms / 1000;
        if (current_second != last_second) {
            demo_seconds += current_second - last_second;
            last_second = current_second;
            needs_redraw = true;
        }

        bool time_pressed = !gpio_get(BUTTON_TIME);
        bool temp_pressed = !gpio_get(BUTTON_TEMP);

        if (time_pressed && !previous_time_pressed &&
            now_ms - last_time_press_ms >= 200) {
            use_12_hour = !use_12_hour;
            last_time_press_ms = now_ms;
            needs_redraw = true;
        }
        if (temp_pressed && !previous_temp_pressed &&
            now_ms - last_temp_press_ms >= 200) {
            use_fahrenheit = !use_fahrenheit;
            last_temp_press_ms = now_ms;
            needs_redraw = true;
        }

        previous_time_pressed = time_pressed;
        previous_temp_pressed = temp_pressed;

        if (needs_redraw) {
            show_clock(); // Serial output for now; OLED driver will come next.
            needs_redraw = false;
        }
        sleep_ms(20);
    }
}
#endif

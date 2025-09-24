#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/adc.h"
#include "hardware/i2c.h"
#include "tusb.h"

#define I2C_SDA 4
#define I2C_SCL 5
#define ADC_PIN 26  // GP26 = ADC0

#define PIR_PIN 16  // GP16

static void setup_pir(void) {
    gpio_init(PIR_PIN);
    gpio_set_dir(PIR_PIN, GPIO_IN);
    gpio_pull_down(PIR_PIN); // keep default low when idle
}


static void i2c_scan(void) {
    printf("I2C scan (SDA=%d SCL=%d):\n", I2C_SDA, I2C_SCL);
    for (uint addr = 0x08; addr <= 0x77; addr++) {
        uint8_t b=0;
        int r = i2c_write_timeout_us(i2c0, addr, &b, 1, false, 2000);
        if (r >= 0) printf("  device @ 0x%02x\n", addr);
        sleep_ms(5);
    }
}

int main(void) {
    stdio_init_all();
    for (int i=0; i<100 && !tud_cdc_connected(); i++) sleep_ms(10);

    const uint LED = PICO_DEFAULT_LED_PIN;
    gpio_init(LED);
    gpio_set_dir(LED, GPIO_OUT);

    // I2C + ADC init...
    i2c_init(i2c0, 400*1000);
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA);
    gpio_pull_up(I2C_SCL);

    adc_init();
    adc_gpio_init(ADC_PIN);
    adc_select_input(0);

    printf("sensor_mvp started\n");

    // NOW the main loop
    while (true) {
        // your PIR / LED / prints
        sleep_ms(50);
    }
}


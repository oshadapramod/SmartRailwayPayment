#include "led.h"

void led_init(void)
{
    esp_rom_gpio_pad_select_gpio(LED_PIN);
    gpio_set_direction(LED_PIN, GPIO_MODE_OUTPUT);
}

void led_on(void)
{
    gpio_set_level(LED_PIN, 1); // Turn on the LED
}

void led_off(void)
{
    gpio_set_level(LED_PIN, 0); // Turn off the LED
}

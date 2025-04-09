#include "buzzer.h"

void buzzer_init(void) {
    gpio_pad_select_gpio(BUZZER_PIN);
    gpio_set_direction(BUZZER_PIN, GPIO_MODE_OUTPUT);
}

void buzzer_short_beep(void) {
    gpio_set_level(BUZZER_PIN, 1);  // Turn on the buzzer
    vTaskDelay(100 / portTICK_PERIOD_MS);  // Beep for 100ms
    gpio_set_level(BUZZER_PIN, 0);  // Turn off the buzzer
}

void buzzer_long_beep(void) {
    gpio_set_level(BUZZER_PIN, 1);  // Turn on the buzzer
    vTaskDelay(1000 / portTICK_PERIOD_MS);  // Beep for 1 second
    gpio_set_level(BUZZER_PIN, 0);  // Turn off the buzzer
}

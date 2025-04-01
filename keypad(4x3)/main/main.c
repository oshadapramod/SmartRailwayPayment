#include <stdio.h>
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

// Define keypad row and column pins
#define ROW_1  25
#define ROW_2  26
#define ROW_3  27
#define ROW_4  14
#define COL_1  32
#define COL_2  33
#define COL_3  34

// Define LED pins corresponding to keypad buttons
int led_pins[4][3] = {
    {4, 5, 18},    // Row 1 LEDs
    {19, 21, 22},  // Row 2 LEDs
    {23, 12, 13},  // Row 3 LEDs
    {15, 2, 0}     // Row 4 LEDs
};

// Initialize keypad rows and columns
void keypad_init() {
    int row_pins[] = {ROW_1, ROW_2, ROW_3, ROW_4};
    int col_pins[] = {COL_1, COL_2, COL_3};

    // Set row pins as outputs
    for (int i = 0; i < 4; i++) {
        gpio_set_direction(row_pins[i], GPIO_MODE_OUTPUT);
        gpio_set_level(row_pins[i], 1);
    }

    // Set column pins as inputs with pull-up resistors
    for (int i = 0; i < 3; i++) {
        gpio_set_direction(col_pins[i], GPIO_MODE_INPUT);
        gpio_pullup_en(col_pins[i]);
    }

    // Set LED pins as outputs
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 3; j++) {
            gpio_set_direction(led_pins[i][j], GPIO_MODE_OUTPUT);
            gpio_set_level(led_pins[i][j], 0); // Turn off all LEDs initially
        }
    }
}

// Scan keypad and turn on corresponding LED
void scan_keypad() {
    int row_pins[] = {ROW_1, ROW_2, ROW_3, ROW_4};
    int col_pins[] = {COL_1, COL_2, COL_3};

    while (1) {
        for (int row = 0; row < 4; row++) {
            // Set the current row low
            gpio_set_level(row_pins[row], 0);

            for (int col = 0; col < 3; col++) {
                if (gpio_get_level(col_pins[col]) == 0) { // Button pressed
                    gpio_set_level(led_pins[row][col], 1); // Turn on corresponding LED
                    vTaskDelay(pdMS_TO_TICKS(300)); // Debounce delay
                } else {
                    gpio_set_level(led_pins[row][col], 0); // Turn off LED when button is released
                }
            }

            // Reset the row to high
            gpio_set_level(row_pins[row], 1);
        }
        vTaskDelay(pdMS_TO_TICKS(10)); // Small delay for stable scanning
    }
}

void app_main() {
    keypad_init();
    xTaskCreate(scan_keypad, "keypad_scan_task", 2048, NULL, 5, NULL);
}

#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "driver/i2c.h"

// I2C LCD Definitions
#define I2C_MASTER_SCL_IO 22
#define I2C_MASTER_SDA_IO 21
#define I2C_MASTER_NUM I2C_NUM_0
#define I2C_MASTER_FREQ_HZ 100000

// LED Definitions
#define YELLOW_LED 26
#define GREEN_LED 25

// Keypad Definitions
#define ROWS 4
#define COLS 3

int rowPins[ROWS] = {23, 19, 18, 5};
int colPins[COLS] = {4, 2, 15};

char keys[ROWS][COLS] = {
    {'1', '2', '3'},
    {'4', '5', '6'},
    {'7', '8', '9'},
    {'*', '0', '#'}
};

char input[3] = {0}; // Stores entered number
int inputIndex = 0;

// Function to initialize I2C LCD
void i2c_init() {
    i2c_config_t config = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = I2C_MASTER_SDA_IO,
        .scl_io_num = I2C_MASTER_SCL_IO,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = I2C_MASTER_FREQ_HZ
    };
    i2c_param_config(I2C_MASTER_NUM, &config);
    i2c_driver_install(I2C_MASTER_NUM, config.mode, 0, 0, 0);
}

// Function to initialize Keypad
void keypad_init() {
    for (int i = 0; i < ROWS; i++) {
        gpio_set_direction(rowPins[i], GPIO_MODE_OUTPUT);
    }
    for (int i = 0; i < COLS; i++) {
        gpio_set_direction(colPins[i], GPIO_MODE_INPUT);
    }
}

// Function to initialize LEDs
void led_init() {
    gpio_set_direction(YELLOW_LED, GPIO_MODE_OUTPUT);
    gpio_set_direction(GREEN_LED, GPIO_MODE_OUTPUT);

    gpio_set_level(YELLOW_LED, 1); // Keep yellow LED ON
    gpio_set_level(GREEN_LED, 0);  // Keep green LED OFF
}

// Function to scan keypad
char scan_keypad() {
    for (int i = 0; i < ROWS; i++) {
        gpio_set_level(rowPins[i], 1);
        for (int j = 0; j < COLS; j++) {
            if (gpio_get_level(colPins[j]) == 1) {
                gpio_set_level(rowPins[i], 0);
                return keys[i][j];
            }
        }
        gpio_set_level(rowPins[i], 0);
    }
    return '\0'; // No key pressed
}

// Function to handle input
void process_input() {
    if (inputIndex == 2) {
        input[inputIndex] = '\0';  // Null terminate string
        int destination = atoi(input);

        if (destination >= 1 && destination <= 17) {
            printf("Destination: %d\n", destination);
            
            // Turn on Green LED for 2 seconds
            gpio_set_level(GREEN_LED, 1);
            vTaskDelay(pdMS_TO_TICKS(2000));
            gpio_set_level(GREEN_LED, 0);
        } else {
            printf("Invalid Destination\n");
        }
        inputIndex = 0;  // Reset input
    }
}

// Task to handle keypad input
void keypad_task(void *arg) {
    while (1) {
        char key = scan_keypad();
        if (key != '\0') {
            if (key == '#') { // Confirm entry
                process_input();
            } else if (key == '*') { // Backspace
                if (inputIndex > 0) {
                    inputIndex--;
                }
            } else if (inputIndex < 2) { // Only accept 2-digit input
                input[inputIndex++] = key;
            }
            vTaskDelay(pdMS_TO_TICKS(200));
        }
    }
}

void app_main() {
    i2c_init();
    keypad_init();
    led_init();

    xTaskCreate(keypad_task, "keypad_task", 2048, NULL, 5, NULL);
}

#include "keypad.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

static const char *TAG = "KEYPAD";

// Define keypad layout
static const char keymap[ROW_NUM][COL_NUM] = {
    {'1', '2', '3'},
    {'4', '5', '6'},
    {'7', '8', '9'},
    {'*', '0', '#'}
};

// GPIO pins for rows and columns
static const gpio_num_t row_pins[ROW_NUM] = {ROW1_PIN, ROW2_PIN, ROW3_PIN, ROW4_PIN};
static const gpio_num_t col_pins[COL_NUM] = {COL1_PIN, COL2_PIN, COL3_PIN};

void keypad_init(void) {
    gpio_config_t io_conf = {};
    
    // Configure row pins as outputs
    io_conf.intr_type = GPIO_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.pin_bit_mask = 0;
    for (int i = 0; i < ROW_NUM; i++) {
        io_conf.pin_bit_mask |= (1ULL << row_pins[i]);
    }
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
    gpio_config(&io_conf);
    
    // Configure column pins as inputs with pull-up
    io_conf.intr_type = GPIO_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pin_bit_mask = 0;
    for (int i = 0; i < COL_NUM; i++) {
        io_conf.pin_bit_mask |= (1ULL << col_pins[i]);
    }
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io_conf.pull_up_en = GPIO_PULLUP_ENABLE;
    gpio_config(&io_conf);
    
    // Set all row pins high initially
    for (int i = 0; i < ROW_NUM; i++) {
        gpio_set_level(row_pins[i], 1);
    }
    
    ESP_LOGI(TAG, "Keypad initialized");
}

char keypad_scan(void) {
    char key = 0;
    
    // Scan rows
    for (int r = 0; r < ROW_NUM; r++) {
        // Set current row to low
        gpio_set_level(row_pins[r], 0);
        
        // Small delay for stability
        vTaskDelay(5 / portTICK_PERIOD_MS);
        
        // Check columns
        for (int c = 0; c < COL_NUM; c++) {
            if (gpio_get_level(col_pins[c]) == 0) {
                // Key pressed - store value from keymap
                key = keymap[r][c];
                
                // Wait for key release (debounce)
                while (gpio_get_level(col_pins[c]) == 0) {
                    vTaskDelay(10 / portTICK_PERIOD_MS);
                }
                
                // Additional debounce delay
                vTaskDelay(50 / portTICK_PERIOD_MS);
            }
        }
        
        // Set current row back to high
        gpio_set_level(row_pins[r], 1);
        
        // If a key was pressed, return it immediately
        if (key != 0) {
            return key;
        }
    }
    
    return 0; // No key pressed
}
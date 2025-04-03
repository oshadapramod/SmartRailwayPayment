#include <stdio.h>
#include <string.h>
#include "esp_log.h"
#include "driver/i2c.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "i2c-lcd.h"
#include "keypad.h"
#include "rfid.h"  // Add the RFID header

static const char *TAG = "train-ticket-system";

#define I2C_MASTER_SCL_IO GPIO_NUM_22      /*!< GPIO number used for I2C master clock */
#define I2C_MASTER_SDA_IO GPIO_NUM_21      /*!< GPIO number used for I2C master data */
#define I2C_MASTER_NUM 0                   /*!< I2C master i2c port number */
#define I2C_MASTER_FREQ_HZ 400000          /*!< I2C master clock frequency */
#define I2C_MASTER_TX_BUF_DISABLE 0        /*!< I2C master doesn't need buffer */
#define I2C_MASTER_RX_BUF_DISABLE 0        /*!< I2C master doesn't need buffer */
#define I2C_MASTER_TIMEOUT_MS 1000

#define MAX_INPUT_LENGTH 3  // Max 2 digits + null terminator
#define MAX_NAME_LENGTH 20  // For temporary string storage
#define MAX_UID_LENGTH 10   // Max UID buffer size for RFID

// Define destinations with their IDs
typedef struct {
    int id;
    char name[20];
} Destination;

// Define classes with their IDs
typedef struct {
    int id;
    char name[15];
} TrainClass;

// Available destinations
const Destination destinations[] = {
    {1, "Polgahawela"},
    {2, "Alawwa"},
    {3, "Ambepussa"},
    {4, "Botale"},
    {5, "Wellawatte"},
    {6, "Mirigama"},
    {7, "Ganegoda"},
    {8, "Veyangoda"},
    {9, "Heendeniya"},
    {10, "Gampaha"},
    {11, "Ganemulla"},
    {12, "Ragama"},
    {13, "Enderamulla"},
    {14, "Kelaniya"},
    {15, "Dematagoda"},
    {16, "Maradana"},
    {17, "Colombo Fort"}
};

// Available classes
const TrainClass train_classes[] = {
    {1, "First class"},
    {2, "Second class"},
    {3, "Third class"}
};

// Number of destinations and classes
#define NUM_DESTINATIONS (sizeof(destinations) / sizeof(destinations[0]))
#define NUM_CLASSES (sizeof(train_classes) / sizeof(train_classes[0]))

// System states
typedef enum {
    STATE_WELCOME,
    STATE_WAIT_FOR_RFID,    // New state for RFID scanning
    STATE_SELECT_DESTINATION,
    STATE_SHOW_DESTINATION,
    STATE_SELECT_CLASS,
    STATE_SHOW_CLASS,
    STATE_CONFIRMED
} SystemState;

/**
 * @brief i2c master initialization
 */
static esp_err_t i2c_master_init(void) {
    int i2c_master_port = I2C_MASTER_NUM;
    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = I2C_MASTER_SDA_IO,
        .scl_io_num = I2C_MASTER_SCL_IO,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = I2C_MASTER_FREQ_HZ,
    };
    i2c_param_config(i2c_master_port, &conf);
    return i2c_driver_install(i2c_master_port, conf.mode, I2C_MASTER_RX_BUF_DISABLE, I2C_MASTER_TX_BUF_DISABLE, 0);
}

// Find destination by ID
const char* get_destination_name(int id) {
    for (int i = 0; i < NUM_DESTINATIONS; i++) {
        if (destinations[i].id == id) {
            return destinations[i].name;
        }
    }
    return "Unknown";
}

// Find class by ID
const char* get_class_name(int id) {
    for (int i = 0; i < NUM_CLASSES; i++) {
        if (train_classes[i].id == id) {
            return train_classes[i].name;
        }
    }
    return "Unknown";
}

void ticket_system_task(void *pvParameter) {
    char input_buffer[MAX_INPUT_LENGTH] = {0};
    int input_pos = 0;
    int selected_destination = 0;
    int selected_class = 0;
    SystemState current_state = STATE_WELCOME;
    
    // Buffer for copying const strings to non-const strings for lcd_send_string
    char display_buffer[MAX_NAME_LENGTH] = {0};
    
    // RFID card UID buffer
    uint8_t card_uid[MAX_UID_LENGTH];
    uint8_t uid_size;
    
    while (1) {
        // State machine for the ticket system
        switch (current_state) {
            case STATE_WELCOME:
                lcd_clear();
                lcd_put_cur(0, 0);
                lcd_send_string("Welcome!");
                vTaskDelay(1000 / portTICK_PERIOD_MS);
                current_state = STATE_WAIT_FOR_RFID;
                break;
                
            case STATE_WAIT_FOR_RFID:
                lcd_clear();
                lcd_put_cur(0, 0);
                lcd_send_string("Scan RFID Card");
                
                // Check for RFID card presence
                if (rfid_card_present()) {
                    ESP_LOGI(TAG, "RFID card detected");
                    
                    // Try to read card UID
                    if (rfid_read_card_uid(card_uid, &uid_size)) {
                        ESP_LOGI(TAG, "Card UID read successfully");
                        
                        // Display card detected message
                        lcd_clear();
                        lcd_put_cur(0, 0);
                        lcd_send_string("Card detected!");
                        
                        // Display UID (first 4 bytes) for debug
                        lcd_put_cur(1, 0);
                        sprintf(display_buffer, "UID:%02X%02X%02X%02X", 
                                card_uid[0], card_uid[1], 
                                card_uid[2], card_uid[3]);
                        lcd_send_string(display_buffer);
                        
                        // Wait a moment before moving to next state
                        vTaskDelay(1500 / portTICK_PERIOD_MS);
                        current_state = STATE_SELECT_DESTINATION;
                    } else {
                        ESP_LOGE(TAG, "Failed to read card UID");
                        
                        // Display error
                        lcd_clear();
                        lcd_put_cur(0, 0);
                        lcd_send_string("Card read error!");
                        lcd_put_cur(1, 0);
                        lcd_send_string("Try again");
                        vTaskDelay(1500 / portTICK_PERIOD_MS);
                    }
                }
                
                // Add a small delay to prevent CPU hogging
                vTaskDelay(200 / portTICK_PERIOD_MS);
                break;
                
            case STATE_SELECT_DESTINATION:
                lcd_clear();
                lcd_put_cur(0, 0);
                lcd_send_string("Select Dest num:");
                lcd_put_cur(1, 0);
                input_pos = 0;
                memset(input_buffer, 0, MAX_INPUT_LENGTH);
                
                // Keep scanning until user presses #
                while (1) {
                    char key = keypad_scan();
                    
                    if (key != 0) {
                        ESP_LOGI(TAG, "Key pressed: %c", key);
                        
                        if (key == '#') {
                            // User confirmed input
                            if (input_pos > 0) {
                                selected_destination = atoi(input_buffer);
                                if (selected_destination >= 1 && selected_destination <= NUM_DESTINATIONS) {
                                    current_state = STATE_SHOW_DESTINATION;
                                    break;
                                } else {
                                    // Invalid destination number
                                    lcd_clear();
                                    lcd_put_cur(0, 0);
                                    lcd_send_string("Invalid number!");
                                    vTaskDelay(1500 / portTICK_PERIOD_MS);
                                    
                                    // Return to destination selection
                                    lcd_clear();
                                    lcd_put_cur(0, 0);
                                    lcd_send_string("Select Dest num:");
                                    lcd_put_cur(1, 0);
                                    input_pos = 0;
                                    memset(input_buffer, 0, MAX_INPUT_LENGTH);
                                }
                            }
                        } 
                        else if (key == '*') {
                            // Backspace functionality
                            if (input_pos > 0) {
                                input_pos--;
                                input_buffer[input_pos] = '\0';
                                
                                // Update display
                                lcd_put_cur(1, input_pos);
                                lcd_send_data(' ');
                                lcd_put_cur(1, input_pos);
                            }
                        }
                        else if (key >= '0' && key <= '9' && input_pos < MAX_INPUT_LENGTH - 1) {
                            // Add digit to buffer
                            input_buffer[input_pos] = key;
                            input_pos++;
                            input_buffer[input_pos] = '\0'; // Ensure null termination
                            
                            // Display the character
                            lcd_put_cur(1, input_pos - 1);
                            lcd_send_data(key);
                        }
                    }
                    
                    vTaskDelay(50 / portTICK_PERIOD_MS);
                }
                break;
                
            case STATE_SHOW_DESTINATION:
                lcd_clear();
                lcd_put_cur(0, 0);
                lcd_send_string("Selected Dest is");
                lcd_put_cur(1, 0);
                
                // Copy const string to non-const buffer
                strncpy(display_buffer, get_destination_name(selected_destination), MAX_NAME_LENGTH - 1);
                display_buffer[MAX_NAME_LENGTH - 1] = '\0'; // Ensure null termination
                lcd_send_string(display_buffer);
                
                vTaskDelay(2000 / portTICK_PERIOD_MS);
                current_state = STATE_SELECT_CLASS;
                break;
                
            case STATE_SELECT_CLASS:
                lcd_clear();
                lcd_put_cur(0, 0);
                lcd_send_string("Select Class:");
                lcd_put_cur(1, 0);
                input_pos = 0;
                memset(input_buffer, 0, MAX_INPUT_LENGTH);
                
                // Keep scanning until user presses #
                while (1) {
                    char key = keypad_scan();
                    
                    if (key != 0) {
                        ESP_LOGI(TAG, "Key pressed: %c", key);
                        
                        if (key == '#') {
                            // User confirmed input
                            if (input_pos > 0) {
                                selected_class = atoi(input_buffer);
                                if (selected_class >= 1 && selected_class <= NUM_CLASSES) {
                                    current_state = STATE_SHOW_CLASS;
                                    break;
                                } else {
                                    // Invalid class number
                                    lcd_clear();
                                    lcd_put_cur(0, 0);
                                    lcd_send_string("Invalid class!");
                                    vTaskDelay(1500 / portTICK_PERIOD_MS);
                                    
                                    // Return to class selection
                                    lcd_clear();
                                    lcd_put_cur(0, 0);
                                    lcd_send_string("Select Class:");
                                    lcd_put_cur(1, 0);
                                    input_pos = 0;
                                    memset(input_buffer, 0, MAX_INPUT_LENGTH);
                                }
                            }
                        } 
                        else if (key == '*') {
                            // Backspace functionality
                            if (input_pos > 0) {
                                input_pos--;
                                input_buffer[input_pos] = '\0';
                                
                                // Update display
                                lcd_put_cur(1, input_pos);
                                lcd_send_data(' ');
                                lcd_put_cur(1, input_pos);
                            }
                        }
                        else if (key >= '0' && key <= '9' && input_pos < MAX_INPUT_LENGTH - 1) {
                            // Add digit to buffer
                            input_buffer[input_pos] = key;
                            input_pos++;
                            input_buffer[input_pos] = '\0'; // Ensure null termination
                            
                            // Display the character
                            lcd_put_cur(1, input_pos - 1);
                            lcd_send_data(key);
                        }
                    }
                    
                    vTaskDelay(50 / portTICK_PERIOD_MS);
                }
                break;
                
            case STATE_SHOW_CLASS:
                lcd_clear();
                lcd_put_cur(0, 0);
                lcd_send_string("Selected class is");
                lcd_put_cur(1, 0);
                
                // Copy const string to non-const buffer
                strncpy(display_buffer, get_class_name(selected_class), MAX_NAME_LENGTH - 1);
                display_buffer[MAX_NAME_LENGTH - 1] = '\0'; // Ensure null termination
                lcd_send_string(display_buffer);
                
                vTaskDelay(2000 / portTICK_PERIOD_MS);
                current_state = STATE_CONFIRMED;
                break;
                
            case STATE_CONFIRMED:
                lcd_clear();
                lcd_put_cur(0, 0);
                lcd_send_string("Confirmed");
                vTaskDelay(2000 / portTICK_PERIOD_MS);
                current_state = STATE_WELCOME;
                break;
                
            default:
                current_state = STATE_WELCOME;
                break;
        }
        
        vTaskDelay(50 / portTICK_PERIOD_MS);
    }
}

void app_main(void) {
    // Initialize I2C
    ESP_ERROR_CHECK(i2c_master_init());
    ESP_LOGI(TAG, "I2C initialized successfully");
    
    // Initialize LCD
    lcd_init();
    lcd_clear();
    
    // Initialize keypad
    keypad_init();
    
    // Initialize RFID module
    rfid_init();
    ESP_LOGI(TAG, "RFID module initialized");
    
    // Create task for ticket system
    xTaskCreate(ticket_system_task, "ticket_system_task", 4096, NULL, 5, NULL);
    
    ESP_LOGI(TAG, "Train ticket system initialized");
}
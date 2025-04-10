#include <stdio.h>
#include <string.h>
#include "esp_log.h"
#include "driver/i2c.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "i2c-lcd.h"
#include "keypad.h"
#include "led.h"
#include "buzzer.h"
#include "rfid.h"
#include "stations.h"
#include "wifi_setup.h"
#include "firebase.h"
#include "esp_sntp.h"

static const char *TAG = "train-ticket-system";

#define I2C_MASTER_SCL_IO GPIO_NUM_22 /*!< GPIO number used for I2C master clock */
#define I2C_MASTER_SDA_IO GPIO_NUM_21 /*!< GPIO number used for I2C master data */
#define I2C_MASTER_NUM 0              /*!< I2C master i2c port number */
#define I2C_MASTER_FREQ_HZ 400000     /*!< I2C master clock frequency */
#define I2C_MASTER_TX_BUF_DISABLE 0   /*!< I2C master doesn't need buffer */
#define I2C_MASTER_RX_BUF_DISABLE 0   /*!< I2C master doesn't need buffer */
#define I2C_MASTER_TIMEOUT_MS 1000

#define MAX_INPUT_LENGTH 3 // Max 2 digits + null terminator
#define MAX_NAME_LENGTH 40 // For temporary string storage
#define MAX_UID_LENGTH 15  // Max UID buffer size for RFID

// Current station ID (where this device is installed)
#define CURRENT_STATION_ID 1 // Change this based on where the system is deployed

// Number of destinations and classes - now retrieved from stations module
#define NUM_DESTINATIONS get_number_of_destinations()
#define NUM_CLASSES get_number_of_train_classes()

// System states
typedef enum
{
    STATE_WELCOME,
    STATE_WAIT_FOR_RFID,
    STATE_VERIFY_USER,
    STATE_CHECK_ACTIVE_JOURNEY,
    STATE_START_JOURNEY,
    STATE_END_JOURNEY,
    STATE_SELECT_DESTINATION,
    STATE_SHOW_DESTINATION,
    STATE_SELECT_CLASS,
    STATE_SHOW_CLASS,
    STATE_CONFIRM_JOURNEY,
    STATE_ERROR,
    STATE_TRANSACTION_SUCCESSFUL
} SystemState;

// Current active journey
static journey_session_t current_journey;
static bool has_active_journey = false;

/**
 * @brief i2c master initialization
 */
static esp_err_t i2c_master_init(void)
{
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

// Time sync notification callback
void time_sync_notification_cb(struct timeval *tv)
{
    ESP_LOGI(TAG, "Time synchronized with NTP server");
}

// Initialize time with SNTP
static void initialize_sntp(void)
{
    ESP_LOGI(TAG, "Initializing SNTP");
    esp_sntp_setoperatingmode(SNTP_OPMODE_POLL);
    esp_sntp_setservername(0, "pool.ntp.org");
    sntp_set_time_sync_notification_cb(time_sync_notification_cb);
    esp_sntp_init();

    // Wait for time to be set
    time_t now = 0;
    struct tm timeinfo = {0};
    int retry = 0;
    const int retry_count = 15;

    while (timeinfo.tm_year < (2020 - 1900) && ++retry < retry_count)
    {
        ESP_LOGI(TAG, "Waiting for system time to be set... (%d/%d)", retry, retry_count);
        vTaskDelay(2000 / portTICK_PERIOD_MS);
        time(&now);
        localtime_r(&now, &timeinfo);
    }
}

void handle_keypad_press(void)
{
    // Short beep for keypad press
    gpio_set_level(BUZZER_PIN, 1);       // Turn on the buzzer
    vTaskDelay(50 / portTICK_PERIOD_MS); // Very short beep
    gpio_set_level(BUZZER_PIN, 0);       // Turn off the buzzer
}

void beep_success(void)
{
    // Double short beep for success (beep beeeep)
    gpio_set_level(BUZZER_PIN, 1);        // Turn on the buzzer
    vTaskDelay(100 / portTICK_PERIOD_MS); // Short beep
    gpio_set_level(BUZZER_PIN, 0);        // Turn off the buzzer
    vTaskDelay(100 / portTICK_PERIOD_MS); // Short pause
    gpio_set_level(BUZZER_PIN, 1);        // Turn on the buzzer
    vTaskDelay(300 / portTICK_PERIOD_MS); // Longer beep
    gpio_set_level(BUZZER_PIN, 0);        // Turn off the buzzer
}

void beep_error(void)
{
    // Long beep for errors or invalid operations (beeeeeeeep)
    gpio_set_level(BUZZER_PIN, 1);        // Turn on the buzzer
    vTaskDelay(800 / portTICK_PERIOD_MS); // Long beep
    gpio_set_level(BUZZER_PIN, 0);        // Turn off the buzzer
}

// Now modify the ticket_system_task to include these functions at the appropriate points

void ticket_system_task(void *pvParameter)
{
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

    // User data
    user_t current_user;

    // Wait for WiFi connection before proceeding
    while (!wifi_is_connected())
    {
        lcd_clear();
        lcd_put_cur(0, 0);
        lcd_send_string("Connecting WiFi...");
        vTaskDelay(500 / portTICK_PERIOD_MS);
    }

    // Init Firebase after WiFi is connected
    firebase_init();

    while (1)
    {
        // State machine for the ticket system
        switch (current_state)
        {
        case STATE_WELCOME:
            lcd_clear();
            lcd_put_cur(1, 0);
            lcd_send_string("Scan Your Card");
            lcd_continuous_scroll("Welcome to RailGo! ", 0, 900, 0);
            current_state = STATE_WAIT_FOR_RFID;
            break;

        case STATE_WAIT_FOR_RFID:
            // Continuously check for RFID card presence
            if (rfid_card_present())
            {
                lcd_stop_scroll();

                // Beep to acknowledge card detection
                buzzer_short_beep();

                ESP_LOGI(TAG, "RFID card detected");

                // Try to read card UID
                if (rfid_read_card_uid(card_uid, &uid_size))
                {
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

                    vTaskDelay(1000 / portTICK_PERIOD_MS);
                    current_state = STATE_VERIFY_USER;
                }
                else
                {
                    ESP_LOGE(TAG, "Failed to read card UID");

                    // Beep to indicate read error
                    beep_error();

                    // Display error
                    lcd_clear();
                    lcd_put_cur(0, 0);
                    lcd_send_string("Card read error!");
                    lcd_put_cur(1, 0);
                    lcd_send_string("Try again!");
                    vTaskDelay(1500 / portTICK_PERIOD_MS);
                    current_state = STATE_WELCOME;
                }
            }

            // Add a small delay to prevent CPU hogging
            vTaskDelay(200 / portTICK_PERIOD_MS);
            break;

        case STATE_VERIFY_USER:
            lcd_clear();
            lcd_put_cur(0, 0);
            lcd_send_string("Verifying card...");

            if (firebase_verify_rfid(card_uid, uid_size, &current_user))
            {
                ESP_LOGI(TAG, "RFID verified for user: %s", current_user.name);

                // Beep for successful verification
                beep_success();

                lcd_clear();
                lcd_put_cur(0, 0);
                lcd_send_string("Welcome,");
                lcd_put_cur(1, 0);
                lcd_send_string(current_user.name);

                vTaskDelay(1500 / portTICK_PERIOD_MS);
                current_state = STATE_CHECK_ACTIVE_JOURNEY;
            }
            else
            {
                ESP_LOGE(TAG, "RFID verification failed");

                // Beep for verification error
                beep_error();

                lcd_clear();
                lcd_put_cur(0, 0);
                lcd_send_string("Invalid card!");
                lcd_put_cur(1, 0);
                lcd_send_string("Contact support");

                vTaskDelay(2000 / portTICK_PERIOD_MS);
                current_state = STATE_WELCOME;
            }
            break;

        case STATE_CHECK_ACTIVE_JOURNEY:
            lcd_clear();
            lcd_put_cur(0, 0);
            lcd_send_string("Checking journey");
            lcd_put_cur(1, 0);
            lcd_send_string("status...");

            // Check if user has an active journey
            if (firebase_check_active_journey(card_uid, uid_size, &current_journey))
            {
                // User has an active journey
                has_active_journey = true;
                ESP_LOGI(TAG, "Active journey found for user");

                // Beep to acknowledge active journey
                buzzer_short_beep();

                lcd_clear();
                lcd_put_cur(0, 0);
                lcd_send_string("Journey in");
                lcd_put_cur(1, 0);
                lcd_send_string("progress...");

                vTaskDelay(1500 / portTICK_PERIOD_MS);
                current_state = STATE_END_JOURNEY;
            }
            else
            {
                // No active journey, start a new one
                has_active_journey = false;
                ESP_LOGI(TAG, "No active journey found, starting new journey");

                // Beep to acknowledge no active journey
                buzzer_short_beep();

                lcd_clear();
                lcd_put_cur(0, 0);
                lcd_send_string("Starting new");
                lcd_put_cur(1, 0);
                lcd_send_string("journey");

                vTaskDelay(1500 / portTICK_PERIOD_MS);
                current_state = STATE_SELECT_DESTINATION;
            }
            break;

        case STATE_SELECT_DESTINATION:
            lcd_clear();
            lcd_put_cur(0, 0);
            lcd_send_string("Select dest (1-17)");
            lcd_put_cur(1, 0);
            input_pos = 0;
            memset(input_buffer, 0, MAX_INPUT_LENGTH);

            // Keep scanning until user presses #
            while (1)
            {
                char key = keypad_scan();

                if (key != 0)
                {
                    ESP_LOGI(TAG, "Key pressed: %c", key);

                    // Beep for keypad press
                    handle_keypad_press();

                    if (key == '#')
                    {
                        // User confirmed input
                        if (input_pos > 0)
                        {
                            selected_destination = atoi(input_buffer);
                            if (selected_destination >= 1 && selected_destination <= NUM_DESTINATIONS)
                            {
                                current_state = STATE_SHOW_DESTINATION;
                                break;
                            }
                            else
                            {
                                // Invalid destination number
                                beep_error();

                                lcd_clear();
                                lcd_put_cur(0, 0);
                                lcd_send_string("Invalid number!");
                                vTaskDelay(1500 / portTICK_PERIOD_MS);

                                // Return to destination selection
                                lcd_clear();
                                lcd_put_cur(0, 0);
                                lcd_send_string("Select dest (1-17)");
                                lcd_put_cur(1, 0);
                                input_pos = 0;
                                memset(input_buffer, 0, MAX_INPUT_LENGTH);
                            }
                        }
                    }
                    else if (key == '*')
                    {
                        // Backspace functionality
                        if (input_pos > 0)
                        {
                            input_pos--;
                            input_buffer[input_pos] = '\0';

                            // Update display
                            lcd_put_cur(1, input_pos);
                            lcd_send_data(' ');
                            lcd_put_cur(1, input_pos);
                        }
                    }
                    else if (key >= '0' && key <= '9' && input_pos < MAX_INPUT_LENGTH - 1)
                    {
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
            lcd_send_string("Destination:");
            lcd_put_cur(1, 0);

            // Copy const string to non-const buffer - using function from stations.h
            strncpy(display_buffer, get_destination_name(selected_destination), MAX_NAME_LENGTH - 1);
            display_buffer[MAX_NAME_LENGTH - 1] = '\0'; // Ensure null termination
            lcd_send_string(display_buffer);

            vTaskDelay(2000 / portTICK_PERIOD_MS);
            current_state = STATE_SELECT_CLASS;
            break;

        case STATE_SELECT_CLASS:
            lcd_clear();
            lcd_put_cur(0, 0);
            lcd_send_string("Select Class(1-3):");
            lcd_put_cur(1, 0);
            input_pos = 0;
            memset(input_buffer, 0, MAX_INPUT_LENGTH);

            // Keep scanning until user presses #
            while (1)
            {
                char key = keypad_scan();

                if (key != 0)
                {
                    ESP_LOGI(TAG, "Key pressed: %c", key);

                    // Beep for keypad press
                    handle_keypad_press();

                    if (key == '#')
                    {
                        // User confirmed input
                        if (input_pos > 0)
                        {
                            selected_class = atoi(input_buffer);
                            if (selected_class >= 1 && selected_class <= NUM_CLASSES)
                            {
                                current_state = STATE_SHOW_CLASS;
                                break;
                            }
                            else
                            {
                                // Invalid class number
                                beep_error();

                                lcd_clear();
                                lcd_put_cur(0, 0);
                                lcd_send_string("Invalid class!");
                                vTaskDelay(1500 / portTICK_PERIOD_MS);

                                // Return to class selection
                                lcd_clear();
                                lcd_put_cur(0, 0);
                                lcd_send_string("Select Class(1-3):");
                                lcd_put_cur(1, 0);
                                input_pos = 0;
                                memset(input_buffer, 0, MAX_INPUT_LENGTH);
                            }
                        }
                    }
                    else if (key == '*')
                    {
                        // Backspace functionality
                        if (input_pos > 0)
                        {
                            input_pos--;
                            input_buffer[input_pos] = '\0';

                            // Update display
                            lcd_put_cur(1, input_pos);
                            lcd_send_data(' ');
                            lcd_put_cur(1, input_pos);
                        }
                    }
                    else if (key >= '0' && key <= '9' && input_pos < MAX_INPUT_LENGTH - 1)
                    {
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
            lcd_send_string("Selected class:");
            lcd_put_cur(1, 0);

            // Copy const string to non-const buffer - using function from stations.h
            strncpy(display_buffer, get_class_name(selected_class), MAX_NAME_LENGTH - 1);
            display_buffer[MAX_NAME_LENGTH - 1] = '\0'; // Ensure null termination
            lcd_send_string(display_buffer);

            vTaskDelay(2000 / portTICK_PERIOD_MS);
            current_state = STATE_CONFIRM_JOURNEY;
            break;

        case STATE_CONFIRM_JOURNEY:
            lcd_clear();
            lcd_put_cur(0, 0);
            lcd_send_string("Confirm? 1:Y 2:N");
            lcd_put_cur(1, 0);

            while (1)
            {
                char key = keypad_scan();

                if (key != 0)
                {
                    // Beep for keypad press
                    handle_keypad_press();

                    if (key == '1')
                    {
                        // Confirmed, start journey
                        current_state = STATE_START_JOURNEY;
                        break;
                    }
                    else if (key == '2')
                    {
                        // Cancelled, go back to welcome
                        lcd_clear();
                        lcd_put_cur(0, 0);
                        lcd_send_string("Cancelled");
                        vTaskDelay(1500 / portTICK_PERIOD_MS);
                        current_state = STATE_WELCOME;
                        break;
                    }
                }

                vTaskDelay(50 / portTICK_PERIOD_MS);
            }
            break;

        case STATE_START_JOURNEY:
            lcd_clear();
            lcd_put_cur(0, 0);
            lcd_send_string("Starting journey");
            lcd_put_cur(1, 0);
            lcd_send_string("Please wait...");

            // Initialize current journey data
            memset(&current_journey, 0, sizeof(journey_session_t));
            memcpy(current_journey.rfid_uid, card_uid, uid_size);
            current_journey.uid_size = uid_size;
            current_journey.origin_station = CURRENT_STATION_ID;
            current_journey.selected_class = selected_class;
            current_journey.selected_destination = selected_destination;
            current_journey.current_state = JOURNEY_STATE_ACTIVE;

            // Save to Firebase
            if (firebase_start_journey(&current_journey))
            {
                ESP_LOGI(TAG, "Journey started successfully with ticket ID: %s", current_journey.ticket_id);

                // Turn on LED for 2 seconds when journey starts
                led_on();

                // Sound the buzzer for journey start
                buzzer_long_beep();

                lcd_clear();
                lcd_put_cur(0, 0);
                lcd_send_string("Journey started!");
                lcd_put_cur(1, 0);
                // Display ticket ID partially
                snprintf(display_buffer, sizeof(display_buffer), "ID: %.15s", current_journey.ticket_id);
                lcd_send_string(display_buffer);

                vTaskDelay(2000 / portTICK_PERIOD_MS);
                led_off(); // Turn off LED after 2 seconds

                current_state = STATE_TRANSACTION_SUCCESSFUL;
            }
            else
            {
                ESP_LOGE(TAG, "Failed to start journey");

                // Beep to indicate error
                beep_error();

                lcd_clear();
                lcd_put_cur(0, 0);
                lcd_send_string("Error saving");
                lcd_put_cur(1, 0);
                lcd_send_string("journey data");

                vTaskDelay(2000 / portTICK_PERIOD_MS);
                current_state = STATE_ERROR;
            }
            break;

        case STATE_END_JOURNEY:
            lcd_clear();
            lcd_put_cur(0, 0);
            lcd_send_string("Ending journey");
            lcd_put_cur(1, 0);
            lcd_send_string("Please wait...");

            // Update journey data for ending
            current_journey.actual_destination = CURRENT_STATION_ID;
            current_journey.end_timestamp = get_current_timestamp();

            // Check for potential fraud (different destination than selected)
            current_journey.is_fraud_suspected = (current_journey.actual_destination != current_journey.selected_destination);

            // Set journey state to inactive
            current_journey.current_state = JOURNEY_STATE_INACTIVE;

            // Save to Firebase
            if (firebase_end_journey(&current_journey))
            {
                ESP_LOGI(TAG, "Journey ended successfully");

                // Sound the buzzer for journey end
                buzzer_long_beep();

                lcd_clear();
                lcd_put_cur(0, 0);
                lcd_send_string("Journey ended!");

                // Show if destination matches or not
                lcd_put_cur(1, 0);
                if (current_journey.is_fraud_suspected)
                {
                    lcd_send_string("Dest mismatch!");
                    // Sound error if fraud suspected
                    beep_error();
                }
                else
                {
                    lcd_send_string("Thank you!");
                }

                vTaskDelay(2500 / portTICK_PERIOD_MS);
                current_state = STATE_TRANSACTION_SUCCESSFUL;
            }
            else
            {
                ESP_LOGE(TAG, "Failed to end journey");

                // Beep to indicate error
                beep_error();

                lcd_clear();
                lcd_put_cur(0, 0);
                lcd_send_string("Error ending");
                lcd_put_cur(1, 0);
                lcd_send_string("journey");

                vTaskDelay(2000 / portTICK_PERIOD_MS);
                current_state = STATE_ERROR;
            }
            break;

        case STATE_ERROR:
            lcd_clear();
            lcd_put_cur(0, 0);
            lcd_send_string("System Error");
            lcd_put_cur(1, 0);
            lcd_send_string("Try again later");

            // Beep to indicate error state
            beep_error();

            vTaskDelay(2000 / portTICK_PERIOD_MS);
            current_state = STATE_WELCOME;
            break;

        case STATE_TRANSACTION_SUCCESSFUL:
            lcd_clear();
            lcd_put_cur(0, 0);
            lcd_send_string("Transaction");
            lcd_put_cur(1, 0);
            lcd_send_string("Successful!");

            // Beep for successful transaction
            beep_success();

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

void handle_rfid_detection(bool is_valid)
{
    if (is_valid)
    {
        buzzer_short_beep();                  // Short beep for valid card
        led_on();                             // Turn on the LED for a moment
        vTaskDelay(500 / portTICK_PERIOD_MS); // Wait for a moment
        led_off();                            // Turn off LED
    }
    else
    {
        buzzer_long_beep(); // Long beep for invalid card
    }
}

void app_main(void)
{
    // Initialize I2C
    ESP_ERROR_CHECK(i2c_master_init());
    ESP_LOGI(TAG, "I2C initialized successfully");

    // Initialize LCD and show boot message
    lcd_init();
    lcd_clear();
    lcd_put_cur(0, 0);
    lcd_send_string("Initializing...");

    // Initialize keypad
    keypad_init();

    // Initialize RFID module
    rfid_init();
    ESP_LOGI(TAG, "RFID module initialized");

    // Initialize WiFi
    lcd_put_cur(1, 0);
    lcd_send_string("WiFi connecting");
    ESP_ERROR_CHECK(wifi_init_sta());

    // Initialize time synchronization
    initialize_sntp();

    // Initialize buzzer and LED
    buzzer_init(); // Initialize buzzer
    led_init();    // Initialize LED

    // Create task for ticket system
    xTaskCreate(ticket_system_task, "ticket_system_task", 8192, NULL, 5, NULL);

    ESP_LOGI(TAG, "Welcome to RailGo");
}
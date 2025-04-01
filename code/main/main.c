#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/i2c.h"
#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "esp_log.h"
#include "sdkconfig.h"

// I2C LCD Configuration
#define I2C_MASTER_SCL_IO           22      // GPIO for SCL
#define I2C_MASTER_SDA_IO           21      // GPIO for SDA
#define I2C_MASTER_NUM              I2C_NUM_0
#define I2C_MASTER_FREQ_HZ          100000  // I2C master clock frequency
#define I2C_MASTER_TX_BUF_DISABLE   0       // I2C master doesn't need buffer
#define I2C_MASTER_RX_BUF_DISABLE   0       // I2C master doesn't need buffer

#define LCD_ADDR                    0x27    // I2C address of the LCD
#define LCD_COLS                    16
#define LCD_ROWS                    2

// 4x3 Keypad Configuration
#define ROW1_PIN                    32
#define ROW2_PIN                    33
#define ROW3_PIN                    25
#define ROW4_PIN                    26
#define COL1_PIN                    27
#define COL2_PIN                    14
#define COL3_PIN                    17

// LED Configuration
#define LED1_PIN                    4       // Status LED
#define LED2_PIN                    16      // Confirmation LED

// RFID-RC522 Configuration (SPI)
#define RFID_CS_PIN                 5       // SDA (SS)
#define RFID_SCK_PIN                18      // SCK
#define RFID_MOSI_PIN               23      // MOSI
#define RFID_MISO_PIN               19      // MISO
#define RFID_RST_PIN                22      // RST (shared with I2C SCL)
#define SPI_HOST_NUM                HSPI_HOST

// LCD Commands
#define LCD_CLEAR                   0x01
#define LCD_HOME                    0x02
#define LCD_ENTRY_MODE              0x04
#define LCD_DISPLAY_CONTROL         0x08
#define LCD_CURSOR_SHIFT            0x10
#define LCD_FUNCTION_SET            0x20
#define LCD_SET_CGRAM_ADDR          0x40
#define LCD_SET_DDRAM_ADDR          0x80

// Entry Mode Flags
#define LCD_ENTRY_RIGHT             0x00
#define LCD_ENTRY_LEFT              0x02
#define LCD_ENTRY_SHIFT_INCREMENT   0x01
#define LCD_ENTRY_SHIFT_DECREMENT   0x00

// Display Control Flags
#define LCD_DISPLAY_ON              0x04
#define LCD_DISPLAY_OFF             0x00
#define LCD_CURSOR_ON               0x02
#define LCD_CURSOR_OFF              0x00
#define LCD_BLINK_ON                0x01
#define LCD_BLINK_OFF               0x00

// Function Set Flags
#define LCD_8BIT_MODE               0x10
#define LCD_4BIT_MODE               0x00
#define LCD_2_LINE                  0x08
#define LCD_1_LINE                  0x00
#define LCD_5x10_DOTS               0x04
#define LCD_5x8_DOTS                0x00

// I2C LCD Control Bits
#define LCD_BACKLIGHT               0x08
#define LCD_ENABLE                  0x04
#define LCD_RW                      0x02
#define LCD_RS                      0x01

// RFID Commands and Constants
#define MFRC522_IDLE                0x00
#define MFRC522_TRANSCEIVE          0x0C
#define MFRC522_AUTHENT             0x0E
#define MFRC522_SOFTRESET           0x0F

static const char *TAG = "smart_railway";

// Keypad mapping
static const char keypad[4][3] = {
    {'1', '2', '3'},
    {'4', '5', '6'},
    {'7', '8', '9'},
    {'*', '0', '#'}
};

// GPIO pins for rows and columns
static const uint32_t row_pins[4] = {ROW1_PIN, ROW2_PIN, ROW3_PIN, ROW4_PIN};
static const uint32_t col_pins[3] = {COL1_PIN, COL2_PIN, COL3_PIN};

// Destination mapping
typedef struct {
    int id;
    const char* name;
} Destination;

static const Destination destinations[] = {
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

// Class mapping
typedef struct {
    int id;
    const char* name;
} Class;

static const Class classes[] = {
    {1, "First class"},
    {2, "Second class"},
    {3, "Third class"}
};

// Declare SPI handle for RFID
spi_device_handle_t spi_rfid;

// Initialize I2C
static esp_err_t i2c_master_init(void)
{
    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = I2C_MASTER_SDA_IO,
        .scl_io_num = I2C_MASTER_SCL_IO,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = I2C_MASTER_FREQ_HZ,
    };
    
    esp_err_t err = i2c_param_config(I2C_MASTER_NUM, &conf);
    if (err != ESP_OK) {
        return err;
    }
    
    return i2c_driver_install(I2C_MASTER_NUM, conf.mode, 
                              I2C_MASTER_RX_BUF_DISABLE, 
                              I2C_MASTER_TX_BUF_DISABLE, 0);
}

// Send a byte to the LCD with a pulse on the enable pin
static esp_err_t lcd_write_byte(uint8_t data, uint8_t mode)
{
    uint8_t high_nibble = (data & 0xF0) | mode | LCD_BACKLIGHT;
    uint8_t low_nibble = ((data << 4) & 0xF0) | mode | LCD_BACKLIGHT;
    
    // Send high nibble with enable pulse
    esp_err_t err = i2c_master_write_to_device(I2C_MASTER_NUM, LCD_ADDR, &high_nibble, 1, pdMS_TO_TICKS(10));
    if (err != ESP_OK) return err;
    
    high_nibble |= LCD_ENABLE;
    err = i2c_master_write_to_device(I2C_MASTER_NUM, LCD_ADDR, &high_nibble, 1, pdMS_TO_TICKS(10));
    if (err != ESP_OK) return err;
    
    vTaskDelay(pdMS_TO_TICKS(1));
    
    high_nibble &= ~LCD_ENABLE;
    err = i2c_master_write_to_device(I2C_MASTER_NUM, LCD_ADDR, &high_nibble, 1, pdMS_TO_TICKS(10));
    if (err != ESP_OK) return err;
    
    // Send low nibble with enable pulse
    err = i2c_master_write_to_device(I2C_MASTER_NUM, LCD_ADDR, &low_nibble, 1, pdMS_TO_TICKS(10));
    if (err != ESP_OK) return err;
    
    low_nibble |= LCD_ENABLE;
    err = i2c_master_write_to_device(I2C_MASTER_NUM, LCD_ADDR, &low_nibble, 1, pdMS_TO_TICKS(10));
    if (err != ESP_OK) return err;
    
    vTaskDelay(pdMS_TO_TICKS(1));
    
    low_nibble &= ~LCD_ENABLE;
    return i2c_master_write_to_device(I2C_MASTER_NUM, LCD_ADDR, &low_nibble, 1, pdMS_TO_TICKS(10));
}

// Send a command to the LCD
static esp_err_t lcd_command(uint8_t cmd)
{
    return lcd_write_byte(cmd, 0);  // RS = 0 for command
}

// Send data to the LCD
static esp_err_t lcd_data(uint8_t data)
{
    return lcd_write_byte(data, LCD_RS);  // RS = 1 for data
}

// Initialize the LCD
static esp_err_t lcd_init(void)
{
    esp_err_t err;
    
    // Wait for LCD to power up
    vTaskDelay(pdMS_TO_TICKS(50));
    
    // Initialize in 4-bit mode according to datasheet procedure
    uint8_t init_seq[] = {0x30, 0x30, 0x30, 0x20};  // Special 4-bit initialization sequence
    
    for (int i = 0; i < 3; i++) {
        // Send first 3 initialization commands without processing as regular commands
        uint8_t data = init_seq[i] | LCD_BACKLIGHT;
        err = i2c_master_write_to_device(I2C_MASTER_NUM, LCD_ADDR, &data, 1, pdMS_TO_TICKS(10));
        if (err != ESP_OK) return err;
        
        data |= LCD_ENABLE;
        err = i2c_master_write_to_device(I2C_MASTER_NUM, LCD_ADDR, &data, 1, pdMS_TO_TICKS(10));
        if (err != ESP_OK) return err;
        
        vTaskDelay(pdMS_TO_TICKS(1));
        
        data &= ~LCD_ENABLE;
        err = i2c_master_write_to_device(I2C_MASTER_NUM, LCD_ADDR, &data, 1, pdMS_TO_TICKS(10));
        if (err != ESP_OK) return err;
        
        vTaskDelay(pdMS_TO_TICKS(5));
    }
    
    // Now set to 4-bit mode
    uint8_t data = init_seq[3] | LCD_BACKLIGHT;
    err = i2c_master_write_to_device(I2C_MASTER_NUM, LCD_ADDR, &data, 1, pdMS_TO_TICKS(10));
    if (err != ESP_OK) return err;
    
    data |= LCD_ENABLE;
    err = i2c_master_write_to_device(I2C_MASTER_NUM, LCD_ADDR, &data, 1, pdMS_TO_TICKS(10));
    if (err != ESP_OK) return err;
    
    vTaskDelay(pdMS_TO_TICKS(1));
    
    data &= ~LCD_ENABLE;
    err = i2c_master_write_to_device(I2C_MASTER_NUM, LCD_ADDR, &data, 1, pdMS_TO_TICKS(10));
    if (err != ESP_OK) return err;
    
    // Function set: 4-bit mode, 2 lines, 5x8 dots
    err = lcd_command(LCD_FUNCTION_SET | LCD_4BIT_MODE | LCD_2_LINE | LCD_5x8_DOTS);
    if (err != ESP_OK) return err;
    
    // Display control: display on, cursor off, blink off
    err = lcd_command(LCD_DISPLAY_CONTROL | LCD_DISPLAY_ON | LCD_CURSOR_OFF | LCD_BLINK_OFF);
    if (err != ESP_OK) return err;
    
    // Entry mode set: increment cursor, no display shift
    err = lcd_command(LCD_ENTRY_MODE | LCD_ENTRY_LEFT | LCD_ENTRY_SHIFT_DECREMENT);
    if (err != ESP_OK) return err;
    
    // Clear display to remove any garbage data
    err = lcd_command(LCD_CLEAR);
    if (err != ESP_OK) return err;
    
    // Wait for the clear command to complete
    vTaskDelay(pdMS_TO_TICKS(2));
    
    return ESP_OK;
}

// Set cursor position
static esp_err_t lcd_set_cursor(uint8_t col, uint8_t row)
{
    static const uint8_t row_offsets[] = {0x00, 0x40, 0x14, 0x54};
    
    if (row >= LCD_ROWS) {
        row = LCD_ROWS - 1;
    }
    
    return lcd_command(LCD_SET_DDRAM_ADDR | (col + row_offsets[row]));
}

// Print a string to the LCD
static esp_err_t lcd_print(const char *str)
{
    esp_err_t err;
    while (*str) {
        err = lcd_data(*str++);
        if (err != ESP_OK) return err;
    }
    return ESP_OK;
}

// Clear the LCD display
static esp_err_t lcd_clear(void)
{
    esp_err_t err = lcd_command(LCD_CLEAR);
    vTaskDelay(pdMS_TO_TICKS(2));  // Clear command needs extra time
    return err;
}

// Initialize keypad GPIO pins
static void keypad_init(void)
{
    // Configure column pins as outputs
    for (int i = 0; i < 3; i++) {
        gpio_config_t io_conf = {
            .pin_bit_mask = (1ULL << col_pins[i]),
            .mode = GPIO_MODE_OUTPUT,
            .pull_up_en = GPIO_PULLUP_DISABLE,
            .pull_down_en = GPIO_PULLDOWN_DISABLE,
            .intr_type = GPIO_INTR_DISABLE
        };
        gpio_config(&io_conf);
        gpio_set_level(col_pins[i], 1);  // Set high initially
    }
    
    // Configure row pins as inputs with pull-down resistors
    for (int i = 0; i < 4; i++) {
        gpio_config_t io_conf = {
            .pin_bit_mask = (1ULL << row_pins[i]),
            .mode = GPIO_MODE_INPUT,
            .pull_up_en = GPIO_PULLUP_DISABLE,
            .pull_down_en = GPIO_PULLDOWN_ENABLE,
            .intr_type = GPIO_INTR_DISABLE
        };
        gpio_config(&io_conf);
    }
}

// Initialize LED GPIO pins
static void led_init(void)
{
    // Configure LED pins as outputs
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << LED1_PIN) | (1ULL << LED2_PIN),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&io_conf);
    
    // Initial state: off
    gpio_set_level(LED1_PIN, 0);
    gpio_set_level(LED2_PIN, 0);
}

// Initialize RFID module
static esp_err_t rfid_init(void)
{
    esp_err_t ret;
    
    // Configure SPI bus
    spi_bus_config_t bus_cfg = {
        .mosi_io_num = RFID_MOSI_PIN,
        .miso_io_num = RFID_MISO_PIN,
        .sclk_io_num = RFID_SCK_PIN,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 0  // Use default
    };
    
    ret = spi_bus_initialize(SPI_HOST_NUM, &bus_cfg, SPI_DMA_CH_AUTO);
    if (ret != ESP_OK) return ret;
    
    // Configure device
    spi_device_interface_config_t dev_cfg = {
        .clock_speed_hz = 1000000,  // 1 MHz
        .mode = 0,                  // SPI mode 0
        .spics_io_num = RFID_CS_PIN,
        .queue_size = 1,
        .flags = 0,
        .pre_cb = NULL,
        .post_cb = NULL
    };
    
    ret = spi_bus_add_device(SPI_HOST_NUM, &dev_cfg, &spi_rfid);
    if (ret != ESP_OK) return ret;
    
    // Configure RST pin
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << RFID_RST_PIN),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&io_conf);
    
    // Reset RFID module
    gpio_set_level(RFID_RST_PIN, 0);
    vTaskDelay(pdMS_TO_TICKS(50));
    gpio_set_level(RFID_RST_PIN, 1);
    vTaskDelay(pdMS_TO_TICKS(50));
    
    // Note: For a complete implementation, we would need to initialize the MFRC522 registers
    // For this example, we'll simulate RFID detection instead to avoid complexity
    
    return ESP_OK;
}

// Simulate RFID card detection (in a real system, this would read from the MFRC522)
static bool rfid_detect_card(void)
{
    // For demonstration purposes, we'll just return true after a small delay
    // In a real implementation, this would poll the RFID reader for card presence
    vTaskDelay(pdMS_TO_TICKS(500));
    return true;
}

// Scan keypad for pressed key with improved debouncing
static char scan_keypad(void)
{
    char pressed_key = 0;
    
    // Scan all columns
    for (int col = 0; col < 3; col++) {
        // Set current column to low
        for (int i = 0; i < 3; i++) {
            gpio_set_level(col_pins[i], i != col);
        }
        
        // Small delay to stabilize signals
        vTaskDelay(pdMS_TO_TICKS(2));
        
        // Check all rows
        for (int row = 0; row < 4; row++) {
            int reading = gpio_get_level(row_pins[row]);
            
            // If button is pressed (reading is low)
            if (reading == 0) {
                // Debounce: Wait a bit and check again
                vTaskDelay(pdMS_TO_TICKS(20));
                if (gpio_get_level(row_pins[row]) == 0) {
                    pressed_key = keypad[row][col];
                    
                    // Wait for key release with timeout
                    int timeout = 0;
                    while (gpio_get_level(row_pins[row]) == 0 && timeout < 50) {
                        vTaskDelay(pdMS_TO_TICKS(10));
                        timeout++;
                    }
                    
                    // Additional delay after key release
                    vTaskDelay(pdMS_TO_TICKS(50));
                    break;
                }
            }
        }
        
        // If key found, break the column scan
        if (pressed_key != 0) {
            break;
        }
    }
    
    // Reset all columns to high
    for (int i = 0; i < 3; i++) {
        gpio_set_level(col_pins[i], 1);
    }
    
    return pressed_key;
}

// Get destination name from ID
static const char* get_destination_name(int id)
{
    const int num_destinations = sizeof(destinations) / sizeof(destinations[0]);
    
    if (id <= 0 || id > num_destinations) {
        return "Unknown";
    }
    
    // The id is 1-based, array is 0-based
    return destinations[id-1].name;
}

// Get class name from ID
static const char* get_class_name(int id)
{
    const int num_classes = sizeof(classes) / sizeof(classes[0]);
    
    if (id <= 0 || id > num_classes) {
        return "Unknown";
    }
    
    // The id is 1-based, array is 0-based
    return classes[id-1].name;
}

// Read input number from keypad with better error handling
static int read_number_input(const char* prompt)
{
    char input[5] = {0};  // Buffer for storing input digits
    int input_len = 0;
    char key;
    bool input_complete = false;
    
    lcd_clear();
    lcd_set_cursor(0, 0);
    lcd_print(prompt);
    lcd_set_cursor(0, 1);
    lcd_print(""); // Clear second line
    
    while (!input_complete) {
        key = scan_keypad();
        
        if (key != 0) {
            ESP_LOGI(TAG, "Key pressed: %c", key);
            
            if (key == '#') {  // Enter key
                if (input_len > 0) {
                    input_complete = true;
                }
            } 
            else if (key == '*') {  // Backspace key
                if (input_len > 0) {
                    input_len--;
                    input[input_len] = '\0';
                    
                    // Update display
                    lcd_set_cursor(0, 1);
                    lcd_print("                ");  // Clear line
                    lcd_set_cursor(0, 1);
                    lcd_print(input);
                }
            } 
            else if (key >= '0' && key <= '9' && input_len < 4) {  // Only accept digits and limit length
                input[input_len++] = key;
                input[input_len] = '\0';
                
                // Blink LED1 briefly on key press for visual feedback
                gpio_set_level(LED1_PIN, 1);
                vTaskDelay(pdMS_TO_TICKS(50));
                gpio_set_level(LED1_PIN, 0);
                
                // Update display
                lcd_set_cursor(0, 1);
                lcd_print(input);
            }
        }
        
        vTaskDelay(pdMS_TO_TICKS(10));  // Short delay between scans
    }
    
    // Convert input to integer
    int result = atoi(input);
    return result;
}

// Smart Railway Payment System Main Process
static void railway_system_process(void)
{
    while (1) {
        // Welcome message
        lcd_clear();
        lcd_set_cursor(0, 0);
        lcd_print("Welcome!");
        lcd_set_cursor(0, 1);
        lcd_print("Scan RFID Card");
        
        // Turn on status LED to indicate ready state
        gpio_set_level(LED1_PIN, 1);
        
        // Wait for RFID card detection
        ESP_LOGI(TAG, "Waiting for RFID card...");
        while (!rfid_detect_card()) {
            vTaskDelay(pdMS_TO_TICKS(100));
        }
        
        // Card detected - blink LED for visual feedback
        for (int i = 0; i < 3; i++) {
            gpio_set_level(LED1_PIN, 0);
            vTaskDelay(pdMS_TO_TICKS(100));
            gpio_set_level(LED1_PIN, 1);
            vTaskDelay(pdMS_TO_TICKS(100));
        }
        
        ESP_LOGI(TAG, "RFID card detected");
        
        // Select destination
        int dest_id = read_number_input("Select Dest num:");
        const char* dest_name = get_destination_name(dest_id);
        
        // Show selected destination
        lcd_clear();
        lcd_set_cursor(0, 0);
        lcd_print("Selected Dest is");
        lcd_set_cursor(0, 1);
        lcd_print(dest_name);
        
        // Blink LED1 to indicate selection
        gpio_set_level(LED1_PIN, 0);
        vTaskDelay(pdMS_TO_TICKS(300));
        gpio_set_level(LED1_PIN, 1);
        
        // Wait 2 seconds
        vTaskDelay(pdMS_TO_TICKS(2000));
        
        // Select class
        int class_id = read_number_input("Select Class:");
        const char* class_name = get_class_name(class_id);
        
        // Show selected class
        lcd_clear();
        lcd_set_cursor(0, 0);
        lcd_print("Selected class is");
        lcd_set_cursor(0, 1);
        lcd_print(class_name);
        
        // Blink LED1 to indicate selection
        gpio_set_level(LED1_PIN, 0);
        vTaskDelay(pdMS_TO_TICKS(300));
        gpio_set_level(LED1_PIN, 1);
        
        // Wait 2 seconds
        vTaskDelay(pdMS_TO_TICKS(2000));
        
        // Show confirmation and turn on LED2 to indicate successful transaction
        lcd_clear();
        lcd_set_cursor(0, 0);
        lcd_print("Confirmed");
        
        // Turn on confirmation LED
        gpio_set_level(LED2_PIN, 1);
        
        // Wait 2 seconds before going back to the initial state
        vTaskDelay(pdMS_TO_TICKS(2000));
        
        // Turn off LEDs before restarting
        gpio_set_level(LED1_PIN, 0);
        gpio_set_level(LED2_PIN, 0);
    }
}

// Main application
void app_main(void)
{
    esp_err_t err;
    
    // Initialize I2C
    ESP_LOGI(TAG, "Initializing I2C");
    err = i2c_master_init();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "I2C initialization failed: %s", esp_err_to_name(err));
        return;
    }
    
    // Initialize LCD
    ESP_LOGI(TAG, "Initializing LCD");
    err = lcd_init();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "LCD initialization failed: %s", esp_err_to_name(err));
        return;
    }
    
    // Initialize LEDs
    ESP_LOGI(TAG, "Initializing LEDs");
    led_init();
    
    // Initialize keypad
    ESP_LOGI(TAG, "Initializing keypad");
    keypad_init();
    
    // Initialize RFID
    ESP_LOGI(TAG, "Initializing RFID reader");
    err = rfid_init();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "RFID initialization failed: %s", esp_err_to_name(err));
        // Continue anyway for demonstration purposes
    }
    
    // Clear the LCD to ensure no residual data is displayed
    lcd_clear();
    
    // Display startup message
    lcd_set_cursor(0, 0);
    lcd_print("Smart Railway");
    lcd_set_cursor(0, 1);
    lcd_print("Payment System");
    
    // Blink both LEDs to indicate system is starting
    for (int i = 0; i < 2; i++) {
        gpio_set_level(LED1_PIN, 1);
        gpio_set_level(LED2_PIN, 1);
        vTaskDelay(pdMS_TO_TICKS(300));
        gpio_set_level(LED1_PIN, 0);
        gpio_set_level(LED2_PIN, 0);
        vTaskDelay(pdMS_TO_TICKS(300));
    }
    
    // Short delay before starting the main process
    vTaskDelay(pdMS_TO_TICKS(1000));
    
    // Start the railway system process
    ESP_LOGI(TAG, "Starting Smart Railway Payment System");
    railway_system_process();
}
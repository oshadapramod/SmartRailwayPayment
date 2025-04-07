#include "rfid.h"
#include "esp_log.h"
#include "driver/spi_master.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "RFID";

// RFID PIN definitions
#define RFID_SDA_PIN      GPIO_NUM_5   // CS/SDA/SS pin
#define RFID_SCK_PIN      GPIO_NUM_18  // SCK pin
#define RFID_MOSI_PIN     GPIO_NUM_23  // MOSI pin
#define RFID_MISO_PIN     GPIO_NUM_19  // MISO pin
#define RFID_RST_PIN      GPIO_NUM_15  // RST pin

// SPI device handle
static spi_device_handle_t spi_handle;

// Write data to RFID RC522 register
static void rfid_write_register(uint8_t reg, uint8_t data)
{
    uint8_t tx_data[2];
    tx_data[0] = (reg << 1) & 0x7E;  // Format: 0XXXXXX0 where XXXXXX is the address
    tx_data[1] = data;
    
    spi_transaction_t t = {
        .length = 16,  // 16 bits = 2 bytes
        .tx_buffer = tx_data,
        .rx_buffer = NULL,
        .flags = 0
    };
    
    gpio_set_level(RFID_SDA_PIN, 0);  // Select RFID
    esp_err_t ret = spi_device_polling_transmit(spi_handle, &t);
    gpio_set_level(RFID_SDA_PIN, 1);  // Deselect RFID
    
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to write data to RFID register: %d", ret);
    }
}

// Read data from RFID RC522 register
static uint8_t rfid_read_register(uint8_t reg)
{
    uint8_t tx_data[2] = {0};
    uint8_t rx_data[2] = {0};
    
    // For reading, set the MSB of the address
    tx_data[0] = ((reg << 1) & 0x7E) | 0x80;
    
    spi_transaction_t t = {
        .length = 16,  // 16 bits = 2 bytes
        .tx_buffer = tx_data,
        .rx_buffer = rx_data,
        .flags = 0
    };
    
    gpio_set_level(RFID_SDA_PIN, 0);  // Select RFID
    esp_err_t ret = spi_device_polling_transmit(spi_handle, &t);
    gpio_set_level(RFID_SDA_PIN, 1);  // Deselect RFID
    
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to read data from RFID register: %d", ret);
        return 0;
    }
    
    return rx_data[1];  // The data is in the second byte of the response
}

// Set bits in RFID RC522 register
static void rfid_set_register_bit_mask(uint8_t reg, uint8_t mask)
{
    uint8_t tmp = rfid_read_register(reg);
    rfid_write_register(reg, tmp | mask);
}

// Clear bits in RFID RC522 register
static void rfid_clear_register_bit_mask(uint8_t reg, uint8_t mask)
{
    uint8_t tmp = rfid_read_register(reg);
    rfid_write_register(reg, tmp & (~mask));
}

// Antenna on
static void rfid_antenna_on(void)
{
    uint8_t temp = rfid_read_register(TX_CONTROL_REG);
    if ((temp & 0x03) != 0x03) {
        rfid_set_register_bit_mask(TX_CONTROL_REG, 0x03);
    }
}

// Initialize RFID module
void rfid_init(void)
{
    ESP_LOGI(TAG, "Initializing RFID module");
    
    // Initialize SPI bus
    spi_bus_config_t buscfg = {
        .miso_io_num = RFID_MISO_PIN,
        .mosi_io_num = RFID_MOSI_PIN,
        .sclk_io_num = RFID_SCK_PIN,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 0,
    };
    
    esp_err_t ret = spi_bus_initialize(VSPI_HOST, &buscfg, 1);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize SPI bus: %d", ret);
        return;
    }
    
    // Configure SPI device
    spi_device_interface_config_t devcfg = {
        .clock_speed_hz = 1000000,    // 1MHz
        .mode = 0,                    // SPI mode 0
        .spics_io_num = -1,           // CS pin manually controlled
        .queue_size = 7,
        .flags = 0
    };
    
    ret = spi_bus_add_device(VSPI_HOST, &devcfg, &spi_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to add SPI device: %d", ret);
        return;
    }
    
    // Configure CS pin as output
    gpio_config_t io_conf = {
        .intr_type = GPIO_INTR_DISABLE,
        .mode = GPIO_MODE_OUTPUT,
        .pin_bit_mask = (1ULL << RFID_SDA_PIN),
        .pull_down_en = 0,
        .pull_up_en = 0
    };
    gpio_config(&io_conf);
    gpio_set_level(RFID_SDA_PIN, 1);  // Deselect RFID
    
    // Configure RST pin as output
    io_conf.pin_bit_mask = (1ULL << RFID_RST_PIN);
    gpio_config(&io_conf);
    gpio_set_level(RFID_RST_PIN, 1);  // Release reset
    
    // Reset RFID RC522
    rfid_reset();
    
    // Configure RFID RC522
    rfid_write_register(TMR_AUTO_REG, 0x00);
    rfid_write_register(T_MODE_REG, 0x8D);
    rfid_write_register(T_PRESCALER_REG, 0x3E);
    rfid_write_register(T_RELOAD_REG_L, 30);
    rfid_write_register(T_RELOAD_REG_H, 0);
    rfid_write_register(TX_ASK_REG, 0x40);
    rfid_write_register(MODE_REG, 0x3D);
    
    // Turn antenna on
    rfid_antenna_on();
    
    ESP_LOGI(TAG, "RFID module initialized successfully");
}

// Reset RFID RC522
void rfid_reset(void)
{
    // Hardware reset
    gpio_set_level(RFID_RST_PIN, 0);
    vTaskDelay(10 / portTICK_PERIOD_MS);
    gpio_set_level(RFID_RST_PIN, 1);
    vTaskDelay(50 / portTICK_PERIOD_MS);
    
    // Software reset
    rfid_write_register(COMMAND_REG, PCD_RESETPHASE);
    vTaskDelay(50 / portTICK_PERIOD_MS);
    
    // Set timer auto reload
    rfid_write_register(T_MODE_REG, 0x8D);
    rfid_write_register(T_PRESCALER_REG, 0x3E);
    rfid_write_register(T_RELOAD_REG_L, 30);
    rfid_write_register(T_RELOAD_REG_H, 0);
    
    // Force 100% ASK modulation
    rfid_write_register(TX_ASK_REG, 0x40);
    
    // Set CRC preset value to 0x6363
    rfid_write_register(MODE_REG, 0x3D);
    
    // Turn antenna on
    rfid_antenna_on();
}

// Check if card is present
bool rfid_card_present(void)
{
    uint8_t buffer_atqa[2];
    
    // Prepare for REQA command
    rfid_write_register(BIT_FRAMING_REG, 0x07);  // TxLastBits = 7 means transmit only 7 bits of the last byte
    
    // Clear interrupts
    rfid_write_register(COM_IRQ_REG, 0x7F);
    
    // Prepare buffer
    buffer_atqa[0] = PICC_REQIDL;
    
    // Send the command
    rfid_write_register(COMMAND_REG, PCD_IDLE);  // Cancel any current command
    rfid_set_register_bit_mask(FIFO_LEVEL_REG, 0x80);  // Clear FIFO
    
    // Write data to FIFO
    rfid_write_register(FIFO_DATA_REG, buffer_atqa[0]);
    
    // Execute command
    rfid_write_register(COMMAND_REG, PCD_TRANSCEIVE);
    rfid_set_register_bit_mask(BIT_FRAMING_REG, 0x80);  // Start transmission
    
    // Wait for completion
    uint16_t timeout = 1000;
    uint8_t irq;
    bool card_detected = false;
    
    do {
        irq = rfid_read_register(COM_IRQ_REG);
        timeout--;
    } while ((!(irq & 0x30)) && timeout);
    
    rfid_clear_register_bit_mask(BIT_FRAMING_REG, 0x80);  // Stop transmission
    
    if (timeout != 0) {
        uint8_t error = rfid_read_register(ERROR_REG);
        if (!(error & 0x1B)) {  // No collision, CRC, parity, protocol or buffer overflow error
            card_detected = true;
        }
    }
    
    return card_detected;
}

// Read card UID
bool rfid_read_card_uid(uint8_t *card_uid, uint8_t *uid_size)
{
    uint8_t status;
    uint8_t buffer[MAX_LEN];
    
    // Clear interrupts
    rfid_write_register(COM_IRQ_REG, 0x7F);
    
    // Prepare for ANTICOLL command
    rfid_write_register(BIT_FRAMING_REG, 0x00);  // Normal frame
    
    // Prepare buffer
    buffer[0] = PICC_ANTICOLL;
    buffer[1] = 0x20;  // NVB = 0x20, meaning "no data sent yet"
    
    // Send command
    rfid_write_register(COMMAND_REG, PCD_IDLE);  // Cancel any current command
    rfid_set_register_bit_mask(FIFO_LEVEL_REG, 0x80);  // Clear FIFO
    
    // Write data to FIFO
    for (uint8_t i = 0; i < 2; i++) {
        rfid_write_register(FIFO_DATA_REG, buffer[i]);
    }
    
    // Execute command
    rfid_write_register(COMMAND_REG, PCD_TRANSCEIVE);
    rfid_set_register_bit_mask(BIT_FRAMING_REG, 0x80);  // Start transmission
    
    // Wait for completion
    uint16_t timeout = 1000;
    uint8_t irq;
    uint8_t n = 0;
    
    do {
        irq = rfid_read_register(COM_IRQ_REG);
        timeout--;
    } while ((!(irq & 0x30)) && timeout);
    
    rfid_clear_register_bit_mask(BIT_FRAMING_REG, 0x80);  // Stop transmission
    
    if (timeout == 0) {
        status = MI_ERR;
    } else {
        uint8_t error = rfid_read_register(ERROR_REG);
        if (error & 0x1B) {  // Check for errors
            status = MI_ERR;
        } else {
            status = MI_OK;
            
            // Check number of bytes received
            n = rfid_read_register(FIFO_LEVEL_REG);
            
            // Read the data
            if (n > MAX_LEN) {
                n = MAX_LEN;
            }
            
            for (uint8_t i = 0; i < n; i++) {
                buffer[i] = rfid_read_register(FIFO_DATA_REG);
            }
            
            *uid_size = n;
            for (uint8_t i = 0; i < n; i++) {
                card_uid[i] = buffer[i];
            }
        }
    }
    
    return (status == MI_OK);
}
/* main.c - ESP32 Compatible RFID Reader */
#include "MFRC522.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define TAG "MAIN"

void app_main(void) {
    spi_bus_config_t buscfg = {
        .miso_io_num = 19,
        .mosi_io_num = 23,
        .sclk_io_num = 18,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 0
    };
    
    spi_device_interface_config_t devcfg = {
        .clock_speed_hz = 1 * 1000 * 1000,
        .mode = 0,
        .spics_io_num = MFRC522_CS_PIN,
        .queue_size = 1
    };
    
    spi_bus_initialize(SPI2_HOST, &buscfg, SPI_DMA_CH_AUTO);
    spi_device_handle_t spi;
    spi_bus_add_device(SPI2_HOST, &devcfg, &spi);
    
    MFRC522_Init(spi);
    
    while (1) {
        uint8_t data[5];
        if (MFRC522_Request(spi, 0x26, data) == 0) {
            ESP_LOGI(TAG, "Card detected!");
            uint8_t serial[5];
            if (MFRC522_AntiCollision(spi, serial) == 0) {
                ESP_LOGI(TAG, "Card ID: %02X %02X %02X %02X %02X", serial[0], serial[1], serial[2], serial[3], serial[4]);
            }
        }
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

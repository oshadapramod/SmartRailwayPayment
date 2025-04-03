/* MFRC522.c - ESP32 Compatible Source File */
#include "MFRC522.h"
#include "driver/spi_master.h"
#include "driver/gpio.h"
#include "esp_log.h"

#define TAG "MFRC522"

static void MFRC522_SPI_Transmit(spi_device_handle_t spi, uint8_t *data, size_t len) {
    spi_transaction_t t = {
        .length = len * 8,
        .tx_buffer = data,
    };
    spi_device_transmit(spi, &t);
}

static void MFRC522_SPI_Receive(spi_device_handle_t spi, uint8_t *data, size_t len) {
    spi_transaction_t t = {
        .length = len * 8,
        .rx_buffer = data,
    };
    spi_device_transmit(spi, &t);
}

void MFRC522_Init(spi_device_handle_t spi) {
    gpio_set_direction(MFRC522_RST_PIN, GPIO_MODE_OUTPUT);
    gpio_set_level(MFRC522_RST_PIN, 1);
    vTaskDelay(pdMS_TO_TICKS(50));
    gpio_set_level(MFRC522_RST_PIN, 0);
    vTaskDelay(pdMS_TO_TICKS(50));
    gpio_set_level(MFRC522_RST_PIN, 1);
    vTaskDelay(pdMS_TO_TICKS(50));
    ESP_LOGI(TAG, "MFRC522 Initialized");
}

uint8_t MFRC522_ReadReg(spi_device_handle_t spi, uint8_t reg) {
    uint8_t txData[2] = { (reg << 1) | 0x80, 0x00 };
    uint8_t rxData[2] = { 0 };
    MFRC522_SPI_Receive(spi, rxData, 2);
    return rxData[1];
}

void MFRC522_WriteReg(spi_device_handle_t spi, uint8_t reg, uint8_t value) {
    uint8_t data[2] = { (reg << 1) & 0x7E, value };
    MFRC522_SPI_Transmit(spi, data, 2);
}

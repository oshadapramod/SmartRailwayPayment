/* MFRC522.h - ESP32 Compatible Header File */
#ifndef MFRC522_H
#define MFRC522_H

#include "driver/spi_master.h"
#include "driver/gpio.h"
#include "esp_err.h"

#define MFRC522_CS_PIN    5   // Define your SPI CS pin
#define MFRC522_RST_PIN   4   // Define your RST pin

// Function Prototypes
void MFRC522_Init(spi_device_handle_t spi);
uint8_t MFRC522_ReadReg(spi_device_handle_t spi, uint8_t reg);
void MFRC522_WriteReg(spi_device_handle_t spi, uint8_t reg, uint8_t value);
void MFRC522_AntennaOn(spi_device_handle_t spi);
uint8_t MFRC522_Request(spi_device_handle_t spi, uint8_t mode, uint8_t *data);
uint8_t MFRC522_AntiCollision(spi_device_handle_t spi, uint8_t *serial);

#endif

#include <stdio.h>
#include "driver/i2c.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "i2c-lcd1602.h"

#define I2C_MASTER_SCL_IO 22   // GPIO for SCL
#define I2C_MASTER_SDA_IO 21   // GPIO for SDA
#define I2C_MASTER_NUM I2C_NUM_0  // I2C port number
#define I2C_MASTER_FREQ_HZ 100000  // 100kHz I2C frequency
#define LCD_ADDR 0x27   // I2C address of LCD (0x27 or 0x3F)

void i2c_master_init() {
    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = I2C_MASTER_SDA_IO,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_io_num = I2C_MASTER_SCL_IO,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = I2C_MASTER_FREQ_HZ,
    };
    i2c_param_config(I2C_MASTER_NUM, &conf);
    i2c_driver_install(I2C_MASTER_NUM, conf.mode, 0, 0, 0);
}

void app_main() {
    i2c_master_init();
    i2c_lcd1602_info_t* lcd_info = i2c_lcd1602_malloc();
    i2c_lcd1602_init(lcd_info, I2C_MASTER_NUM, LCD_ADDR, true);
    i2c_lcd1602_clear(lcd_info);
    i2c_lcd1602_home(lcd_info);
    i2c_lcd1602_write_string(lcd_info, "Hello, World!");
}
#ifndef LED_H
#define LED_H

#include "driver/gpio.h"

#define LED_PIN GPIO_NUM_4 // Pin for the LED (D4)

// Function to initialize the LED
void led_init(void);

// Function to turn on the LED for a short moment
void led_on(void);

// Function to turn off the LED
void led_off(void);

#endif // LED_H
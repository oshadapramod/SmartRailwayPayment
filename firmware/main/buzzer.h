#ifndef BUZZER_H
#define BUZZER_H

#include "driver/gpio.h"

#define BUZZER_PIN GPIO_NUM_4  // Pin for the buzzer (D4)

// Function to initialize the buzzer
void buzzer_init(void);

// Function to make a short beep
void buzzer_short_beep(void);

// Function to make a long beep
void buzzer_long_beep(void);

#endif // BUZZER_H

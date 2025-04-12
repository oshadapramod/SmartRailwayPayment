#ifndef KEYPAD_H
#define KEYPAD_H

#include "driver/gpio.h"

// Keypad definitions
#define ROW_NUM     4
#define COL_NUM     3

// GPIO Pin assignments
#define ROW1_PIN    GPIO_NUM_32
#define ROW2_PIN    GPIO_NUM_33
#define ROW3_PIN    GPIO_NUM_25
#define ROW4_PIN    GPIO_NUM_26
#define COL1_PIN    GPIO_NUM_27
#define COL2_PIN    GPIO_NUM_14
#define COL3_PIN    GPIO_NUM_13

// Function declarations
void keypad_init(void);
char keypad_scan(void);

#endif /* KEYPAD_H */
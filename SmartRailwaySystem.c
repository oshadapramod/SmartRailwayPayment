#include <avr/io.h>
#include <util/delay.h>
#include <stdlib.h>
#include "i2c_lcd.h"   // Include I2C LCD library
#include "keypad.h"    // Include Keypad library

#define F_CPU 16000000UL  // Define CPU frequency for delay calculations

// Define LED & Door Control Pins (Using Port B)
#define RED_LED   PB0  // Red LED - Indicates error/idle state
#define GREEN_LED PB1  // Green LED - Indicates success
#define DOOR_PIN  PB2  // Door Control - Relay or Servo Motor

// Array storing station names (indexed from 0 to 16)
const char *destinations[17] = {
    "Polgahawela", "Alawwa", "Ambepussa", "Botale",
    "Wilwatte", "Mirigama", "Ganegoda", "Veyangoda",
    "Heendeniya", "Gampaha", "Ganemulla", "Ragama",
    "Enderamulla", "Kelaniya", "Dematagoda", "Maradana",
    "Colombo Fort"
};

// Variables to store user input
char enteredCode[3] = "";  // Buffer to store entered digits (max 2 digits)
uint8_t codeLength = 0;    // Tracks the length of entered code

// Function Prototypes
void updateDisplay();       // Update LCD display with entered numbers
void showSuccess(uint8_t index);  // Handle valid entry (success case)
void showError();           // Handle invalid entry (error case)
void resetInput();          // Reset the input for new entry

int main(void) {
    // Initialize LCD and Keypad
    lcd_init();  
    keypad_init();
    
    // Configure LED and Door Pins as Outputs
    DDRB |= (1 << RED_LED) | (1 << GREEN_LED) | (1 << DOOR_PIN);
    
    // Set initial states
    PORTB |= (1 << RED_LED);   // Turn ON Red LED (default state)
    PORTB &= ~(1 << GREEN_LED); // Turn OFF Green LED
    PORTB &= ~(1 << DOOR_PIN);  // Ensure Door is closed initially
    
    // Display prompt message on LCD
    lcd_set_cursor(0, 0);
    lcd_print("Enter Dest No:");
    
    while (1) {
        char key = keypad_get_key();  // Read keypad input
        
        if (key) {  // If a key is pressed
            if (key == '#') {  // Confirm entry
                enteredCode[codeLength] = '\0';  // Null-terminate string
                uint8_t destIndex = atoi(enteredCode); // Convert string to integer
                
                if (destIndex >= 1 && destIndex <= 17) { // Validate range
                    showSuccess(destIndex - 1); // Display corresponding destination
                } else {
                    showError(); // Display error message
                }
                
                resetInput(); // Reset for the next entry
            }
            else if (key == '*') {  // Backspace function
                if (codeLength > 0) {
                    enteredCode[--codeLength] = '\0'; // Remove last digit
                    updateDisplay();  // Update LCD display
                }
            }
            else if (codeLength < 2) {  // Accept only up to 2-digit input
                enteredCode[codeLength++] = key;  // Append new digit
                updateDisplay();  // Update display with new input
            }
        }
    }
}

// Function to update LCD display while entering digits
void updateDisplay() {
    lcd_set_cursor(0, 1);
    lcd_print(enteredCode);  // Print entered digits
    lcd_print("  ");  // Clear extra spaces for proper display
}

// Function to handle valid destination selection
void showSuccess(uint8_t index) {
    lcd_clear();
    lcd_set_cursor(0, 0);
    lcd_print("Dest: ");
    lcd_print(destinations[index]);  // Display selected destination
    lcd_set_cursor(0, 1);
    lcd_print("Thank You!");

    // Indicate success by turning on Green LED and opening the door
    PORTB &= ~(1 << RED_LED);   // Turn OFF Red LED
    PORTB |= (1 << GREEN_LED);  // Turn ON Green LED
    PORTB |= (1 << DOOR_PIN);   // Activate Door Relay

    _delay_ms(5000);  // Hold success state for 5 seconds

    // Restore default state (close door, reset LEDs)
    PORTB &= ~(1 << GREEN_LED);
    PORTB &= ~(1 << DOOR_PIN);
    PORTB |= (1 << RED_LED);
}

// Function to handle invalid destination entry
void showError() {
    lcd_clear();
    lcd_set_cursor(0, 0);
    lcd_print("Invalid Entry!");  // Error message
    lcd_set_cursor(0, 1);
    lcd_print("Try Again.");

    _delay_ms(2000);  // Display error message for 2 seconds
}

// Function to reset input buffer for new entry
void resetInput() {
    codeLength = 0;  // Reset input length
    enteredCode[0] = '\0';  // Clear input buffer
    
    lcd_clear();
    lcd_set_cursor(0, 0);
    lcd_print("Enter Dest No:");  // Re-prompt for input
}

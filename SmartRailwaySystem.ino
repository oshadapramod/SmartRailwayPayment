#include <Wire.h>  // Library for I2C communication
#include <LiquidCrystal_I2C.h>  // Library for I2C LCD display
#include <Keypad.h>  // Library for handling keypad input

// Initialize I2C LCD with address 0x27 (16 columns, 2 rows)
LiquidCrystal_I2C lcd(0x27, 16, 2);

// Keypad Settings
const byte ROWS = 4;  // Number of rows in the keypad
const byte COLS = 3;  // Number of columns in the keypad

// Define keypad layout
char keys[ROWS][COLS] = {
    {'1', '2', '3'},
    {'4', '5', '6'},
    {'7', '8', '9'},
    {'', '0', '#'}  // '' = Backspace, '#' = Enter
};

// Define pin connections for ESP32 (GPIO pins)
byte rowPins[ROWS] = {23, 19, 18, 5}; // Row connections
byte colPins[COLS] = {4, 2, 15};     // Column connections

// Initialize keypad object
Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);

// Define LED and door control pins
#define RED_LED 32     // Red LED for invalid state
#define GREEN_LED 25   // Green LED for success
#define DOOR_PIN 4     // Door control (Relay or Servo)

// List of train destinations mapped to numbers 1-17
String destinations[17] = {
    "Polgahawela", "Alawwa", "Ambepussa", "Botale",
    "Wilwatte", "Mirigama", "Ganegoda", "Veyangoda",
    "Heendeniya", "Gampaha", "Ganemulla", "Ragama",
    "Enderamulla", "Kelaniya", "Dematagoda", "Maradana",
    "Colombo Fort"
};

// Variable to store user input
String enteredCode = "";  

void setup() {
    Serial.begin(115200); // Start serial communication
    lcd.init();  // Initialize LCD
    lcd.backlight();  // Turn on LCD backlight

    // Set LED and Door pins as OUTPUT
    pinMode(RED_LED, OUTPUT);
    pinMode(GREEN_LED, OUTPUT);
    pinMode(DOOR_PIN, OUTPUT);

    // Set default states (Red LED ON, Green LED OFF, Door Closed)
    digitalWrite(RED_LED, HIGH);
    digitalWrite(GREEN_LED, LOW);
    digitalWrite(DOOR_PIN, LOW);

    // Display prompt message
    lcd.setCursor(0, 0);
    lcd.print("Enter Dest No:");
}

void loop() {
    char key = keypad.getKey();  // Read keypad input

    if (key) {  // If a key is pressed
        if (key == '#') {  // If 'Enter' is pressed
            int destIndex = enteredCode.toInt();  // Convert input to integer
            if (destIndex >= 1 && destIndex <= 17) { // Check if input is valid
                String destName = destinations[destIndex - 1]; // Get destination name
                showSuccess(destName); // Display success message
            } else {
                showError(); // Show error for invalid input
            }
            resetInput(); // Reset input for next user
        }
        else if (key == '*') {  // Backspace function
            if (enteredCode.length() > 0) {
                enteredCode.remove(enteredCode.length() - 1); // Remove last character
                updateDisplay(); // Update LCD display
            }
        }
        else if (enteredCode.length() < 2) {  // Allow max 2-digit input
            enteredCode += key; // Append key input
            updateDisplay(); // Update LCD display
        }
    }
}

// Function to update LCD display with entered digits
void updateDisplay() {
    lcd.setCursor(0, 1);
    lcd.print(enteredCode);
    lcd.print("  "); // Clears extra characters
}

// Function to handle successful destination selection
void showSuccess(String destination) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Dest: " + destination);  // Show destination name
    lcd.setCursor(0, 1);
    lcd.print("Thank You!");

    Serial.println("Destination: " + destination);
    Serial.println("Thank You!");

    // Indicate success (Green LED ON, Red LED OFF, Open Door)
    digitalWrite(RED_LED, LOW);  
    digitalWrite(GREEN_LED, HIGH);
    digitalWrite(DOOR_PIN, HIGH);  // Open door

    delay(5000);  // Keep door open for 5 seconds

    // Restore default state (Green LED OFF, Red LED ON, Close Door)
    digitalWrite(GREEN_LED, LOW);  
    digitalWrite(DOOR_PIN, LOW);  
    digitalWrite(RED_LED, HIGH);   
}

// Function to handle invalid input
void showError() {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Invalid Entry!");
    lcd.setCursor(0, 1);
    lcd.print("Try Again.");

    Serial.println("Invalid destination entered.");

    delay(2000); // Display error message for 2 seconds
}

// Function to reset input for a new entry
void resetInput() {
    enteredCode = "";  // Clear input
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Enter Dest No:");  // Reset display prompt
}

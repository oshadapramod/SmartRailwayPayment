// main.c
// ESP32 Smart Railway Payment System
// This code implements a simplified version of the project without using external libraries.
// It includes basic drivers for SPI (RFID-RC522), I2C (LCD1602C), keypad scanning, LED control, 
// and a simple HTTP client for Firebase integration.

// ===================
// Hardware Definitions
// ===================

// RFID-RC522 SPI pins
#define RFID_SDA   5   // Slave select
#define RFID_SCK   18
#define RFID_MOSI  23
#define RFID_MISO  19
#define RFID_RST   22

// 4x3 Keypad pins (change as needed for ESP32 GPIO numbers)
#define KP_ROW1    32
#define KP_ROW2    33
#define KP_ROW3    25
#define KP_ROW4    26
#define KP_COL1    27
#define KP_COL2    14
#define KP_COL3    12

// LCD1602C (I2C) pins
#define LCD_VCC    5    // Supply (note: using 5V supply pin)
#define LCD_GND    0    // Ground pin is common with system GND
#define LCD_SDA    21
#define LCD_SCL    22

// LED pins
#define LED1       4
#define LED2       2

// ===================
// Global Variables & States
// ===================

int currentState = 0;  // 0 = journey finished, 1 = journey in progress

// Firebase settings (update with your project details)
const char* FIREBASE_HOST = "your-project.firebaseio.com";
const char* FIREBASE_AUTH = "your_firebase_database_secret";

// The station at which the journey starts
const char* startStation = "Colombo Fort";

// Destination names corresponding to numbers
const char* destinations[] = {
    "Polgahawela", "Alawwa", "Ambepussa", "Botale", "Wellawatte", "Mirigama",
    "Ganegoda", "Veyangoda", "Heendeniya", "Gampaha", "Ganemulla", "Ragama",
    "Enderamulla", "Kelaniya", "Dematagoda", "Maradana", "Colombo Fort"
};

// Class names corresponding to numbers
const char* classes[] = {"First class", "Second class", "Third class"};

// ===================
// Low-Level Driver Functions
// ===================

// --------- SPI Functions for RFID-RC522 ---------

// For simplicity, assume delay_us() is implemented to provide microsecond delays.
void delay_us(int us) {
    // Implementation-specific: busy loop or timer-based delay
}

// Set a GPIO pin as output (basic bit-bang version)
void gpio_set_output(int gpio) {
    // Configure the ESP32 registers to set pin as output.
    // Implementation depends on ESP32 hardware registers.
}

// Set a GPIO pin as input
void gpio_set_input(int gpio) {
    // Configure the pin as input.
}

// Write a value (0 or 1) to a GPIO pin.
void gpio_write(int gpio, int value) {
    // Write to the output register for the given pin.
}

// Read a GPIO pin.
int gpio_read(int gpio) {
    // Return the digital input level of the pin.
    return 0;  // placeholder
}

// SPI bit-banging initialization for RFID module pins.
void spi_init() {
    // Set pins as output/input accordingly.
    gpio_set_output(RFID_SCK);
    gpio_set_output(RFID_MOSI);
    gpio_set_input(RFID_MISO);
    gpio_set_output(RFID_SDA);
    gpio_set_output(RFID_RST);
    // Initialize clock low, chip select high (inactive)
    gpio_write(RFID_SCK, 0);
    gpio_write(RFID_SDA, 1);
    gpio_write(RFID_RST, 1);
}

// SPI transfer: send one byte and return received byte.
unsigned char spi_transfer(unsigned char data) {
    unsigned char received = 0;
    for (int i = 0; i < 8; i++) {
        // Write bit (MSB first)
        gpio_write(RFID_MOSI, (data & 0x80) ? 1 : 0);
        data <<= 1;
        // Clock high
        gpio_write(RFID_SCK, 1);
        delay_us(2);
        // Read bit
        received <<= 1;
        if (gpio_read(RFID_MISO))
            received |= 1;
        // Clock low
        gpio_write(RFID_SCK, 0);
        delay_us(2);
    }
    return received;
}

// --------- I2C Functions for LCD1602C ---------

// Bit-banged I2C start condition.
void i2c_start(int sda, int scl) {
    gpio_set_output(sda);
    gpio_set_output(scl);
    gpio_write(sda, 1);
    gpio_write(scl, 1);
    delay_us(4);
    gpio_write(sda, 0);
    delay_us(4);
    gpio_write(scl, 0);
}

// Bit-banged I2C stop condition.
void i2c_stop(int sda, int scl) {
    gpio_set_output(sda);
    gpio_write(sda, 0);
    gpio_write(scl, 1);
    delay_us(4);
    gpio_write(sda, 1);
    delay_us(4);
}

// Send a byte over I2C and wait for ACK.
int i2c_write_byte(int sda, int scl, unsigned char data) {
    for (int i = 0; i < 8; i++) {
        gpio_write(sda, (data & 0x80) ? 1 : 0);
        data <<= 1;
        gpio_write(scl, 1);
        delay_us(4);
        gpio_write(scl, 0);
        delay_us(4);
    }
    // Release SDA for ACK
    gpio_set_input(sda);
    gpio_write(scl, 1);
    delay_us(4);
    int ack = gpio_read(sda);
    gpio_write(scl, 0);
    gpio_set_output(sda);
    return ack;  // 0 means ACK received
}

// Send a command to the LCD1602C via I2C
void lcd_send_command(unsigned char cmd) {
    i2c_start(LCD_SDA, LCD_SCL);
    // Send LCD I2C address (assume 0x27) and write bit (0)
    i2c_write_byte(LCD_SDA, LCD_SCL, 0x27 << 1 | 0);
    // Send command byte with proper control bits for LCD (RS=0)
    i2c_write_byte(LCD_SDA, LCD_SCL, cmd);
    i2c_stop(LCD_SDA, LCD_SCL);
}

// Send data (character) to LCD.
void lcd_send_data(unsigned char data) {
    i2c_start(LCD_SDA, LCD_SCL);
    i2c_write_byte(LCD_SDA, LCD_SCL, 0x27 << 1 | 0);
    // Data mode (RS=1)
    i2c_write_byte(LCD_SDA, LCD_SCL, data | 0x40);
    i2c_stop(LCD_SDA, LCD_SCL);
}

// Initialize the LCD (basic commands)
void lcd_init() {
    delay_us(50000);  // Wait >40ms after power-up
    lcd_send_command(0x38); // Function set: 8-bit, 2 lines, 5x8 dots
    delay_us(4500);
    lcd_send_command(0x0C); // Display on, cursor off
    delay_us(4500);
    lcd_send_command(0x01); // Clear display
    delay_us(2000);
}

// Write a string to the LCD.
void lcd_print(const char* str) {
    while (*str) {
        lcd_send_data(*str++);
    }
}

// --------- Keypad Scanning ---------

// Map keypad buttons (for a 4x3 matrix)
// This function returns the character corresponding to the pressed key.
// Note: '#' represents Enter, '*' represents Back.
char keypad_scan() {
    // Configure row pins as outputs and column pins as inputs (with pull-ups).
    // For each row, drive low and check which column reads low.
    int rows[4] = {KP_ROW1, KP_ROW2, KP_ROW3, KP_ROW4};
    int cols[3] = {KP_COL1, KP_COL2, KP_COL3};
    char keys[4][3] = {
        {'1','2','3'},
        {'4','5','6'},
        {'7','8','9'},
        {'*','0','#'}
    };
    for (int r = 0; r < 4; r++) {
        // Set current row low and others high.
        for (int i = 0; i < 4; i++) {
            gpio_set_output(rows[i]);
            gpio_write(rows[i], (i == r) ? 0 : 1);
        }
        // Read each column.
        for (int c = 0; c < 3; c++) {
            gpio_set_input(cols[c]);
            if (gpio_read(cols[c]) == 0) {
                // Debounce delay
                delay_us(20000);
                if (gpio_read(cols[c]) == 0) {
                    return keys[r][c];
                }
            }
        }
    }
    return '\0';  // No key pressed.
}

// --------- LED Control ---------

void led_on(int led_pin) {
    gpio_set_output(led_pin);
    gpio_write(led_pin, 1);
}

void led_off(int led_pin) {
    gpio_set_output(led_pin);
    gpio_write(led_pin, 0);
}

// --------- Wi-Fi & Firebase HTTP Functions ---------

// For demonstration purposes, we implement a very simple HTTP POST via TCP sockets.
// A complete implementation must handle Wi-Fi initialization, DNS, TLS, etc.
// Here, we assume Wi-Fi is already connected and we use a basic socket interface.

#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <netdb.h>

// Create a simple HTTP POST request to Firebase.
int firebase_post(const char* path, const char* jsonData) {
    char request[1024];
    char response[1024];
    int sock;
    struct hostent *host;
    struct sockaddr_in server_addr;
    
    // Resolve host name
    host = gethostbyname(FIREBASE_HOST);
    if (!host) {
        printf("DNS lookup failed\n");
        return -1;
    }
    
    // Create socket.
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        printf("Socket creation error\n");
        return -1;
    }
    
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(80);  // Using HTTP on port 80 for simplicity.
    server_addr.sin_addr = *((struct in_addr*)host->h_addr);
    
    // Connect to Firebase host.
    if (connect(sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        printf("Connection error\n");
        return -1;
    }
    
    // Construct HTTP POST request
    sprintf(request,
            "POST /%s.json?auth=%s HTTP/1.1\r\n"
            "Host: %s\r\n"
            "Content-Type: application/json\r\n"
            "Content-Length: %ld\r\n\r\n"
            "%s",
            path, FIREBASE_AUTH, FIREBASE_HOST, strlen(jsonData), jsonData);
    
    // Send request.
    send(sock, request, strlen(request), 0);
    
    // Receive response (simple implementation).
    recv(sock, response, sizeof(response)-1, 0);
    // Close socket.
    close(sock);
    // In a full implementation, parse response.
    return 0;
}

// =========
// Application Logic
// =========

// Simulated function to read RFID card ID using SPI (returns a dummy ID)
unsigned int rfid_read() {
    // Activate RFID by pulling SDA low
    gpio_write(RFID_SDA, 0);
    // For demonstration, perform a dummy SPI transfer sequence
    unsigned int dummy_id = 0;
    for (int i = 0; i < 4; i++) {
        dummy_id = (dummy_id << 8) | spi_transfer(0x00);
    }
    // Deactivate RFID chip select
    gpio_write(RFID_SDA, 1);
    return dummy_id;
}

// Check if RFID card is valid (dummy validation)
int check_rfid_valid(unsigned int id) {
    // In a real system, this would query a local DB or compare with stored IDs.
    // Here, assume any non-zero ID is valid.
    return (id != 0);
}

// Function to capture numeric input from keypad until '#' is pressed.
// Supports backspace (*) to erase.
void get_numeric_input(char* buffer, int buf_size) {
    int idx = 0;
    char key;
    while (1) {
        key = keypad_scan();
        if (key != '\0') {
            if (key == '#') {  // Enter key
                buffer[idx] = '\0';
                break;
            } else if (key == '*' && idx > 0) {  // Backspace
                idx--;
                buffer[idx] = '\0';
                // Optionally update LCD to show erased input.
            } else if (key >= '0' && key <= '9' && idx < buf_size - 1) {
                buffer[idx++] = key;
                buffer[idx] = '\0';
                // Optionally update LCD to show new digit.
            }
            delay_us(300000);  // 300ms debounce and allow user to see the digit.
        }
    }
}

// Update Firebase with journey details.
void update_firebase(const char* rfid_id, const char* destination, const char* class_name, const char* date_time, int journey_state) {
    char jsonData[512];
    // Construct JSON data string.
    sprintf(jsonData, "{\"rfid\":\"%s\",\"startStation\":\"%s\",\"destination\":\"%s\",\"class\":\"%s\",\"dateTime\":\"%s\",\"currentState\":%d}",
            rfid_id, startStation, destination, class_name, date_time, journey_state);
    firebase_post("journeys", jsonData);
}

// Dummy function to get current date and time as a string.
void get_date_time(char* buffer, int buf_size) {
    // In a real system, query an RTC or network time.
    snprintf(buffer, buf_size, "2025-03-27 12:00:00");
}

// Main application state machine.
void run_system() {
    char buffer[16];
    char date_time[32];
    unsigned int rfid_id;
    
    // Turn on LED2 always while powered.
    led_on(LED2);
    
    // Start with a welcome message.
    lcd_init();
    lcd_print("Welcome!");
    delay_us(2000000); // Display welcome for 2 seconds.
    
    while (1) {
        // Wait for RFID scan.
        lcd_send_command(0x01);  // Clear display
        lcd_print("Scan RFID...");
        rfid_id = rfid_read();
        if (!check_rfid_valid(rfid_id)) {
            lcd_send_command(0x01);
            lcd_print("Invalid Card");
            delay_us(2000000);
            continue;
        }
        
        // Convert RFID id to string (for Firebase)
        char rfid_str[16];
        sprintf(rfid_str, "%X", rfid_id);
        
        if (currentState == 0) {
            // Starting journey
            lcd_send_command(0x01);
            lcd_print("Select Dest num:");
            // Display destination numbers (example: abbreviated list)
            // In practice, you might cycle through a list on the LCD.
            
            // Get destination input.
            get_numeric_input(buffer, sizeof(buffer));
            int destIndex = atoi(buffer) - 1;
            if (destIndex < 0 || destIndex >= 17) {
                lcd_send_command(0x01);
                lcd_print("Invalid Dest");
                delay_us(1000000);
                continue;
            }
            // Show selected destination
            lcd_send_command(0x01);
            lcd_print("Selected:");
            lcd_print(destinations[destIndex]);
            // LED1 flash for valid input
            led_on(LED1);
            delay_us(1000000);
            led_off(LED1);
            
            delay_us(1000000); // 1 sec delay
            
            // Select Class
            lcd_send_command(0x01);
            lcd_print("Select Class:");
            // Display class options (1,2,3)
            get_numeric_input(buffer, sizeof(buffer));
            int classIndex = atoi(buffer) - 1;
            if (classIndex < 0 || classIndex > 2) {
                lcd_send_command(0x01);
                lcd_print("Invalid Class");
                delay_us(1000000);
                continue;
            }
            lcd_send_command(0x01);
            lcd_print("Class:");
            lcd_print(classes[classIndex]);
            led_on(LED1);
            delay_us(1000000);
            led_off(LED1);
            
            delay_us(1000000);
            lcd_send_command(0x01);
            lcd_print("Confirmed");
            
            // Get current date and time.
            get_date_time(date_time, sizeof(date_time));
            
            // Update Firebase with journey start.
            update_firebase(rfid_str, destinations[destIndex], classes[classIndex], date_time, 1);
            currentState = 1;
            
            delay_us(2000000);
            lcd_send_command(0x01);
            lcd_print("Welcome!");
        }
        else {
            // Finishing journey
            lcd_send_command(0x01);
            lcd_print("Finish Journey");
            // Update Firebase with journey finish.
            get_date_time(date_time, sizeof(date_time));
            update_firebase(rfid_str, startStation, "", date_time, 0);
            currentState = 0;
            delay_us(2000000);
            lcd_send_command(0x01);
            lcd_print("Welcome!");
        }
    }
}

// =========
// Main Function
// =========

int main(void) {
    // Initialize all interfaces.
    spi_init();
    lcd_init();
    // Initialize keypad pins, LED pins, etc.
    // (Implement GPIO initialization for each pin as needed.)
    
    // Assume Wi-Fi is already initialized and connected.
    
    // Run the system state machine.
    run_system();
    
    return 0;
}

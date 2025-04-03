#ifndef FIREBASE_H
#define FIREBASE_H

#include <stdbool.h>
#include <stdint.h>
#include <time.h>
#include "esp_http_client.h"

// Firebase configuration
#define FIREBASE_HOST "https://smartrailwaypayment-default-rtdb.firebaseio.com"
#define FIREBASE_AUTH "UuzOpxm3OBREHbeZxf7r3fdxKZaKLJfuLeoOGNBd"

// Journey state
#define JOURNEY_STATE_INACTIVE 0
#define JOURNEY_STATE_ACTIVE 1

// Journey session structure
typedef struct {
    char ticket_id[32];          // Unique ID for each journey ticket
    uint8_t rfid_uid[10];        // The card ID used for the journey
    uint8_t uid_size;            // Size of the RFID UID
    time_t start_timestamp;      // When the journey started
    time_t end_timestamp;        // When the journey finished
    int origin_station;          // Departure station ID
    int selected_class;          // Selected class ID
    int selected_destination;    // User's selected destination ID
    int actual_destination;      // Where the user actually ended the journey
    int travel_duration;         // Difference between startTimestamp and endTimestamp
    bool is_fraud_suspected;     // true if actual destination ≠ selected destination
    int current_state;           // 1 = active journey, 0 = inactive
} journey_session_t;

// User structure
typedef struct {
    char user_id[32];           // User ID in Firebase
    char name[50];              // User name
    uint8_t rfid_uid[10];       // Associated RFID UID
    uint8_t uid_size;           // Size of the RFID UID
} user_t;

// Initialize Firebase connection
void firebase_init(void);

// function declarations
bool firebase_check_active_journey(const uint8_t *rfid_uid, uint8_t uid_size, journey_session_t *journey);

// Verify if RFID is registered to a user
bool firebase_verify_rfid(const uint8_t *rfid_uid, uint8_t uid_size, user_t *user);

// Start a new journey session
bool firebase_start_journey(journey_session_t *journey);

// End a journey session
bool firebase_end_journey(journey_session_t *journey);

// Generate a unique ticket ID
void generate_ticket_id(char *ticket_id, size_t size);

// Convert RFID UID to string format
void rfid_uid_to_string(const uint8_t *uid, uint8_t size, char *output, size_t output_size);

// Get current timestamp
time_t get_current_timestamp(void);

#endif /* FIREBASE_H */
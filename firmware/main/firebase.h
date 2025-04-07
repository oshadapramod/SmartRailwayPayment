#ifndef FIREBASE_H
#define FIREBASE_H

#include <stdint.h>
#include <stdbool.h>
#include <time.h>

// Firebase configuration - replace with your values
#define FIREBASE_HOST "https://smartrailwaypayment-default-rtdb.firebaseio.com/"
#define FIREBASE_AUTH "UuzOpxm3OBREHbeZxf7r3fdxKZaKLJfuLeoOGNBd"

// Journey states
typedef enum {
    JOURNEY_STATE_INACTIVE = 0,
    JOURNEY_STATE_ACTIVE = 1
} journey_state_t;

// User structure
typedef struct {
    char name[64];
    char user_id[64];
    uint8_t rfid_uid[10];
    uint8_t uid_size;
} user_t;

// Journey session structure
typedef struct {
    char ticket_id[16];
    uint8_t rfid_uid[10];
    uint8_t uid_size;
    time_t start_timestamp;
    time_t end_timestamp;
    uint8_t origin_station;
    uint8_t selected_class;
    uint8_t selected_destination;
    uint8_t actual_destination;
    uint32_t travel_duration;
    bool is_fraud_suspected;
    journey_state_t current_state;
} journey_session_t;

// HTTP response buffer structure
typedef struct {
    char *buffer;        // Pointer to the buffer
    size_t max_len;      // Maximum length of the buffer
    size_t current_len;  // Current length of data in buffer
    bool overflow;       // Flag to indicate if buffer overflowed
} http_response_buffer_t;

// Function declarations
void firebase_init(void);
bool firebase_verify_rfid(const uint8_t *rfid_uid, uint8_t uid_size, user_t *user);
bool firebase_start_journey(journey_session_t *journey);
bool firebase_check_active_journey(const uint8_t *rfid_uid, uint8_t uid_size, journey_session_t *journey);
bool firebase_end_journey(journey_session_t *journey);
void generate_ticket_id(char *ticket_id, size_t size);
void rfid_uid_to_string(const uint8_t *uid, uint8_t size, char *output, size_t output_size);
time_t get_current_timestamp(void);

#endif // FIREBASE_H
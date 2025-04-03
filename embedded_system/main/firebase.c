#include "firebase.h"
#include "esp_log.h"
#include "esp_http_client.h"
#include "esp_wifi.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "esp_crt_bundle.h"
#include "esp_tls.h"
#include "cJSON.h"
#include <string.h>
#include <time.h>
#include <sys/time.h>

static const char *TAG = "FIREBASE";

// Global active journey session
static journey_session_t active_journey;
static bool journey_active = false;

// Generates a random string for ticket ID
void generate_ticket_id(char *ticket_id, size_t size) {
    const char charset[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";
    if (size) {
        --size; // Ensure space for null terminator
        for (size_t n = 0; n < size; n++) {
            int key = esp_random() % (sizeof charset - 1);
            ticket_id[n] = charset[key];
        }
        ticket_id[size] = '\0';
    }
}

// Convert RFID UID to string format
void rfid_uid_to_string(const uint8_t *uid, uint8_t size, char *output, size_t output_size) {
    if (uid == NULL || output == NULL || output_size < (size * 2) + 1) {
        ESP_LOGE(TAG, "Invalid parameters for rfid_uid_to_string");
        return;
    }
    
    for (int i = 0; i < size; i++) {
        snprintf(output + (i * 2), 3, "%02X", uid[i]);
    }
    output[size * 2] = '\0';
}

// Get current timestamp
time_t get_current_timestamp(void) {
    time_t now;
    time(&now);
    return now;
}

// Format timestamp as ISO 8601 string
static void format_timestamp(time_t timestamp, char *buffer, size_t size) {
    struct tm timeinfo;
    localtime_r(&timestamp, &timeinfo);
    strftime(buffer, size, "%Y-%m-%dT%H:%M:%SZ", &timeinfo);
}

// Initialize Firebase connection
void firebase_init(void) {
    ESP_LOGI(TAG, "Initializing Firebase connection");
    
    // Reset active journey flag
    journey_active = false;
    
    // Initialize cJSON (if needed)
    // Any other initialization for Firebase
    
    ESP_LOGI(TAG, "Firebase initialized");
}

// HTTP event handler
static esp_err_t http_event_handler(esp_http_client_event_t *evt) {
    switch(evt->event_id) {
        case HTTP_EVENT_ERROR:
            ESP_LOGE(TAG, "HTTP_EVENT_ERROR");
            break;
        case HTTP_EVENT_ON_CONNECTED:
            ESP_LOGI(TAG, "HTTP_EVENT_ON_CONNECTED");
            break;
        case HTTP_EVENT_HEADER_SENT:
            ESP_LOGI(TAG, "HTTP_EVENT_HEADER_SENT");
            break;
        case HTTP_EVENT_ON_HEADER:
            ESP_LOGD(TAG, "HTTP_EVENT_ON_HEADER, key=%s, value=%s", evt->header_key, evt->header_value);
            break;
        case HTTP_EVENT_ON_DATA:
            ESP_LOGD(TAG, "HTTP_EVENT_ON_DATA, len=%d", evt->data_len);
            if (evt->user_data) {
                memcpy(evt->user_data + *((int*)evt->user_data), evt->data, evt->data_len);
                *((int*)evt->user_data) += evt->data_len;
            }
            break;
        case HTTP_EVENT_ON_FINISH:
            ESP_LOGI(TAG, "HTTP_EVENT_ON_FINISH");
            break;
        case HTTP_EVENT_DISCONNECTED:
            ESP_LOGI(TAG, "HTTP_EVENT_DISCONNECTED");
            break;
        case HTTP_EVENT_REDIRECT:
            ESP_LOGI(TAG, "HTTP_EVENT_REDIRECT");
            break;
    }
    return ESP_OK;
}

// Send HTTP request to Firebase
static esp_err_t firebase_http_request(const char *path, const char *method, const char *data, char *response_buffer, size_t response_buffer_size) {
    char url[256];
    snprintf(url, sizeof(url), "https://%s%s.json?auth=%s", FIREBASE_HOST, path, FIREBASE_AUTH);
    
    esp_http_client_config_t config = {
        .url = url,
        .method = (strcmp(method, "GET") == 0) ? HTTP_METHOD_GET : 
                 (strcmp(method, "POST") == 0) ? HTTP_METHOD_POST : 
                 (strcmp(method, "PUT") == 0) ? HTTP_METHOD_PUT : 
                 (strcmp(method, "PATCH") == 0) ? HTTP_METHOD_PATCH : HTTP_METHOD_DELETE,
        .event_handler = http_event_handler,
        .buffer_size = 2048,
        .crt_bundle_attach = esp_crt_bundle_attach,
    };
    
    esp_http_client_handle_t client = esp_http_client_init(&config);
    
    if (data != NULL) {
        esp_http_client_set_header(client, "Content-Type", "application/json");
        esp_http_client_set_post_field(client, data, strlen(data));
    }
    
    int response_len = 0;
    if (response_buffer != NULL) {
        esp_http_client_set_user_data(client, &response_len);
    }
    
    esp_err_t err = esp_http_client_perform(client);
    
    if (err == ESP_OK) {
        ESP_LOGI(TAG, "HTTP request status = %d, content_length = %lld",
                 esp_http_client_get_status_code(client),
                 esp_http_client_get_content_length(client));
        
        if (response_buffer != NULL && response_len < response_buffer_size) {
            response_buffer[response_len] = '\0';
        }
    } else {
        ESP_LOGE(TAG, "HTTP request failed: %s", esp_err_to_name(err));
    }
    
    esp_http_client_cleanup(client);
    return err;
}

// Verify if RFID is registered to a user
bool firebase_verify_rfid(const uint8_t *rfid_uid, uint8_t uid_size, user_t *user) {
    if (rfid_uid == NULL || user == NULL) {
        ESP_LOGE(TAG, "Invalid parameters for verify_rfid");
        return false;
    }
    
    char uid_string[32];
    rfid_uid_to_string(rfid_uid, uid_size, uid_string, sizeof(uid_string));
    
    char path[64];
    snprintf(path, sizeof(path), "/users");
    
    char response[2048] = {0};
    esp_err_t err = firebase_http_request(path, "GET", NULL, response, sizeof(response));
    
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to fetch users data from Firebase");
        return false;
    }
    
    // Parse the response using cJSON
    cJSON *root = cJSON_Parse(response);
    if (root == NULL) {
        ESP_LOGE(TAG, "Failed to parse JSON response");
        return false;
    }
    
    bool found = false;
    
    // Iterate through users to find matching RFID
    cJSON *users = root;
    cJSON *user_item = NULL;
    cJSON *user_id = NULL;
    
    cJSON_ArrayForEach(user_item, users) {
        cJSON *stored_rfid = cJSON_GetObjectItem(user_item, "rfidUid");
        
        if (stored_rfid != NULL && cJSON_IsString(stored_rfid)) {
            if (strcmp(stored_rfid->valuestring, uid_string) == 0) {
                // Found matching RFID
                cJSON *name = cJSON_GetObjectItem(user_item, "name");
                user_id = cJSON_GetObjectItem(user_item, "id");
                
                if (name != NULL && cJSON_IsString(name) && 
                    user_id != NULL && cJSON_IsString(user_id)) {
                    // Copy user data
                    strncpy(user->name, name->valuestring, sizeof(user->name) - 1);
                    user->name[sizeof(user->name) - 1] = '\0';
                    
                    strncpy(user->user_id, user_id->valuestring, sizeof(user->user_id) - 1);
                    user->user_id[sizeof(user->user_id) - 1] = '\0';
                    
                    memcpy(user->rfid_uid, rfid_uid, uid_size);
                    user->uid_size = uid_size;
                    
                    found = true;
                    break;
                }
            }
        }
    }
    
    cJSON_Delete(root);
    return found;
}

// Start a new journey session
bool firebase_start_journey(journey_session_t *journey) {
    if (journey == NULL) {
        ESP_LOGE(TAG, "Invalid journey parameter");
        return false;
    }
    
    // Generate a ticket ID
    generate_ticket_id(journey->ticket_id, sizeof(journey->ticket_id));
    
    // Set start timestamp
    journey->start_timestamp = get_current_timestamp();
    
    // Set journey state to active
    journey->current_state = JOURNEY_STATE_ACTIVE;
    
    // Mark journey as not finished
    journey->end_timestamp = 0;
    journey->actual_destination = 0;
    journey->is_fraud_suspected = false;
    journey->travel_duration = 0;
    
    // Create JSON for Firebase
    char rfid_string[32];
    rfid_uid_to_string(journey->rfid_uid, journey->uid_size, rfid_string, sizeof(rfid_string));
    
    char timestamp_str[32];
    format_timestamp(journey->start_timestamp, timestamp_str, sizeof(timestamp_str));
    
    char json_data[512];
    snprintf(json_data, sizeof(json_data),
             "{"
             "\"ticketID\":\"%s\","
             "\"rfidUid\":\"%s\","
             "\"startTimestamp\":\"%s\","
             "\"originStation\":%d,"
             "\"selectedClass\":%d,"
             "\"selectedDestinationStation\":%d,"
             "\"currentState\":%d"
             "}",
             journey->ticket_id,
             rfid_string,
             timestamp_str,
             journey->origin_station,
             journey->selected_class,
             journey->selected_destination,
             journey->current_state);
    
    // Save to Firebase
    char path[64];
    snprintf(path, sizeof(path), "/journeys/%s", journey->ticket_id);
    
    esp_err_t err = firebase_http_request(path, "PUT", json_data, NULL, 0);
    
    if (err == ESP_OK) {
        // Store the active journey
        memcpy(&active_journey, journey, sizeof(journey_session_t));
        journey_active = true;
        
        ESP_LOGI(TAG, "Journey started successfully. Ticket ID: %s", journey->ticket_id);
        return true;
    } else {
        ESP_LOGE(TAG, "Failed to save journey to Firebase");
        return false;
    }
}

// Check if there is an active journey for this RFID
bool firebase_check_active_journey(const uint8_t *rfid_uid, uint8_t uid_size, journey_session_t *journey) {
    if (rfid_uid == NULL || journey == NULL) {
        ESP_LOGE(TAG, "Invalid parameters for check_active_journey");
        return false;
    }
    
    char uid_string[32];
    rfid_uid_to_string(rfid_uid, uid_size, uid_string, sizeof(uid_string));
    
    // Create query to find active journeys for this RFID
    char path[128];
    snprintf(path, sizeof(path), "/journeys");
    
    char response[2048] = {0};
    esp_err_t err = firebase_http_request(path, "GET", NULL, response, sizeof(response));
    
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to fetch journeys data from Firebase");
        return false;
    }
    
    // Parse the response using cJSON
    cJSON *root = cJSON_Parse(response);
    if (root == NULL) {
        ESP_LOGE(TAG, "Failed to parse JSON response");
        return false;
    }
    
    bool found = false;
    
    // Iterate through journeys to find active ones for this RFID
    cJSON *journeys = root;
    cJSON *journey_item = NULL;
    
    cJSON_ArrayForEach(journey_item, journeys) {
        cJSON *stored_rfid = cJSON_GetObjectItem(journey_item, "rfidUid");
        cJSON *current_state = cJSON_GetObjectItem(journey_item, "currentState");
        
        if (stored_rfid != NULL && cJSON_IsString(stored_rfid) && 
            current_state != NULL && cJSON_IsNumber(current_state)) {
            
            if (strcmp(stored_rfid->valuestring, uid_string) == 0 && 
                current_state->valueint == JOURNEY_STATE_ACTIVE) {
                
                // Found an active journey for this RFID
                cJSON *ticket_id = cJSON_GetObjectItem(journey_item, "ticketID");
                cJSON *start_timestamp = cJSON_GetObjectItem(journey_item, "startTimestamp");
                cJSON *origin_station = cJSON_GetObjectItem(journey_item, "originStation");
                cJSON *selected_class = cJSON_GetObjectItem(journey_item, "selectedClass");
                cJSON *selected_destination = cJSON_GetObjectItem(journey_item, "selectedDestinationStation");
                
                if (ticket_id != NULL && cJSON_IsString(ticket_id) &&
                    start_timestamp != NULL && cJSON_IsString(start_timestamp) &&
                    origin_station != NULL && cJSON_IsNumber(origin_station) &&
                    selected_class != NULL && cJSON_IsNumber(selected_class) &&
                    selected_destination != NULL && cJSON_IsNumber(selected_destination)) {
                    
                    // Copy journey data
                    strncpy(journey->ticket_id, ticket_id->valuestring, sizeof(journey->ticket_id) - 1);
                    journey->ticket_id[sizeof(journey->ticket_id) - 1] = '\0';
                    
                    memcpy(journey->rfid_uid, rfid_uid, uid_size);
                    journey->uid_size = uid_size;
                    
                    // TODO: Convert timestamp string to time_t
                    // For now, just use current time
                    journey->start_timestamp = get_current_timestamp();
                    
                    journey->origin_station = origin_station->valueint;
                    journey->selected_class = selected_class->valueint;
                    journey->selected_destination = selected_destination->valueint;
                    journey->current_state = JOURNEY_STATE_ACTIVE;
                    
                    found = true;
                    break;
                }
            }
        }
    }
    
    cJSON_Delete(root);
    return found;
}

// End a journey session
bool firebase_end_journey(journey_session_t *journey) {
    if (journey == NULL) {
        ESP_LOGE(TAG, "Invalid journey parameter");
        return false;
    }
    
    // Set end timestamp
    journey->end_timestamp = get_current_timestamp();
    
    // Calculate travel duration
    journey->travel_duration = journey->end_timestamp - journey->start_timestamp;
    
    // Check if fraud is suspected (actual destination != selected destination)
    journey->is_fraud_suspected = (journey->actual_destination != journey->selected_destination);
    
    // Set journey state to inactive
    journey->current_state = JOURNEY_STATE_INACTIVE;
    
    // Create JSON for Firebase update
    char rfid_string[32];
    rfid_uid_to_string(journey->rfid_uid, journey->uid_size, rfid_string, sizeof(rfid_string));
    
    char start_timestamp_str[32];
    format_timestamp(journey->start_timestamp, start_timestamp_str, sizeof(start_timestamp_str));
    
    char end_timestamp_str[32];
    format_timestamp(journey->end_timestamp, end_timestamp_str, sizeof(end_timestamp_str));
    
    char json_data[512];
    snprintf(json_data, sizeof(json_data),
             "{"
             "\"ticketID\":\"%s\","
             "\"rfidUid\":\"%s\","
             "\"startTimestamp\":\"%s\","
             "\"endTimestamp\":\"%s\","
             "\"originStation\":%d,"
             "\"selectedClass\":%d,"
             "\"selectedDestinationStation\":%d,"
             "\"actualDestinationStation\":%d,"
             "\"travelDuration\":%d,"
             "\"isFraudSuspected\":%s,"
             "\"currentState\":%d"
             "}",
             journey->ticket_id,
             rfid_string,
             start_timestamp_str,
             end_timestamp_str,
             journey->origin_station,
             journey->selected_class,
             journey->selected_destination,
             journey->actual_destination,
             journey->travel_duration,
             journey->is_fraud_suspected ? "true" : "false",
             journey->current_state);
    
    // Update in Firebase
    char path[64];
    snprintf(path, sizeof(path), "/journeys/%s", journey->ticket_id);
    
    esp_err_t err = firebase_http_request(path, "PUT", json_data, NULL, 0);
    
    if (err == ESP_OK) {
        // Clear active journey
        journey_active = false;
        memset(&active_journey, 0, sizeof(journey_session_t));
        
        ESP_LOGI(TAG, "Journey ended successfully. Ticket ID: %s", journey->ticket_id);
        return true;
    } else {
        ESP_LOGE(TAG, "Failed to update journey in Firebase");
        return false;
    }
}
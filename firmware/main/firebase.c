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

// HTTP event handler with improved buffer management
static esp_err_t http_event_handler(esp_http_client_event_t *evt) {
    // Cast user data to a response buffer structure
    http_response_buffer_t *response_buffer = (http_response_buffer_t *)evt->user_data;
    
    if (response_buffer == NULL) {
        ESP_LOGW(TAG, "No user data (response buffer) provided");
        return ESP_OK;
    }
    
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
            if (response_buffer) {
                // Check if we have space left in the buffer
                if (response_buffer->current_len + evt->data_len < response_buffer->max_len - 1) {
                    // Copy data to the buffer at the current position
                    memcpy(response_buffer->buffer + response_buffer->current_len, evt->data, evt->data_len);
                    response_buffer->current_len += evt->data_len;
                    response_buffer->buffer[response_buffer->current_len] = '\0'; // Ensure null termination
                } else {
                    ESP_LOGW(TAG, "Response buffer overflow, truncating response");
                    // Copy only what we can fit
                    size_t remaining_space = response_buffer->max_len - 1 - response_buffer->current_len;
                    if (remaining_space > 0) {
                        memcpy(response_buffer->buffer + response_buffer->current_len, evt->data, remaining_space);
                        response_buffer->current_len += remaining_space;
                        response_buffer->buffer[response_buffer->current_len] = '\0';
                    }
                    response_buffer->overflow = true;
                }
            }
            break;
        case HTTP_EVENT_ON_FINISH:
            ESP_LOGI(TAG, "HTTP_EVENT_ON_FINISH");
            // Make extra sure we have null termination
            if (response_buffer && response_buffer->current_len < response_buffer->max_len) {
                response_buffer->buffer[response_buffer->current_len] = '\0';
            }
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

// Improved Firebase HTTP request function with retry mechanism and better error handling
static esp_err_t firebase_http_request(const char *path, const char *method, const char *data, char *response_buffer, size_t response_buffer_size) {
    if (path == NULL || method == NULL) {
        ESP_LOGE(TAG, "Invalid parameters for firebase_http_request");
        return ESP_ERR_INVALID_ARG;
    }
    
    // Verify Firebase host and auth are properly configured
    if (strlen(FIREBASE_HOST) == 0 || strlen(FIREBASE_AUTH) == 0) {
        ESP_LOGE(TAG, "Firebase host or auth token not configured");
        return ESP_ERR_INVALID_STATE;
    }
    
    char url[256];
    snprintf(url, sizeof(url), "%s%s.json?auth=%s", FIREBASE_HOST, path, FIREBASE_AUTH);
    
    ESP_LOGI(TAG, "Request URL: %s", url);
    
    // Create response buffer structure for the event handler
    http_response_buffer_t user_buffer = {
        .buffer = response_buffer,
        .max_len = response_buffer_size,
        .current_len = 0,
        .overflow = false
    };
    
    esp_http_client_config_t config = {
        .url = url,
        .method = (strcmp(method, "GET") == 0) ? HTTP_METHOD_GET : 
                 (strcmp(method, "POST") == 0) ? HTTP_METHOD_POST : 
                 (strcmp(method, "PUT") == 0) ? HTTP_METHOD_PUT : 
                 (strcmp(method, "PATCH") == 0) ? HTTP_METHOD_PATCH : HTTP_METHOD_DELETE,
        .event_handler = http_event_handler,
        .user_data = response_buffer ? &user_buffer : NULL,
        .buffer_size = 4096,    // Increased buffer size
        .timeout_ms = 20000,    // 20 second timeout
        .crt_bundle_attach = esp_crt_bundle_attach,
        .keep_alive_enable = true,
        .disable_auto_redirect = false
    };
    
    esp_http_client_handle_t client = esp_http_client_init(&config);
    
    // Check if client initialization was successful
    if (client == NULL) {
        ESP_LOGE(TAG, "Failed to initialize HTTP client");
        return ESP_FAIL;
    }
    
    // Set headers and post data if needed
    if (data != NULL) {
        esp_http_client_set_header(client, "Content-Type", "application/json");
        esp_http_client_set_post_field(client, data, strlen(data));
    }
    
    // Add retry logic
    int retry_count = 0;
    const int max_retries = 3;
    esp_err_t err = ESP_FAIL;
    
    while (retry_count < max_retries) {
        ESP_LOGI(TAG, "Attempting request (attempt %d of %d)", retry_count + 1, max_retries);
        
        // Reset response buffer position before each attempt
        if (response_buffer) {
            user_buffer.current_len = 0;
            user_buffer.overflow = false;
            response_buffer[0] = '\0';
        }
        
        err = esp_http_client_perform(client);
        
        if (err == ESP_OK) {
            int status_code = esp_http_client_get_status_code(client);
            ESP_LOGI(TAG, "HTTP request successful with status code: %d", status_code);
            
            // Check for successful HTTP status codes (2xx)
            if (status_code >= 200 && status_code < 300) {
                break; // Success
            } else {
                ESP_LOGW(TAG, "HTTP request returned error status code: %d", status_code);
                // For client or server errors, we might want to retry
                retry_count++;
                vTaskDelay(1000 / portTICK_PERIOD_MS);
            }
        } else {
            ESP_LOGW(TAG, "HTTP request failed: %s. Retrying in 1 second...", esp_err_to_name(err));
            retry_count++;
            vTaskDelay(1000 / portTICK_PERIOD_MS);
        }
    }
    
    // Check for response buffer overflow
    if (response_buffer && user_buffer.overflow) {
        ESP_LOGW(TAG, "Response was truncated due to buffer size limitations");
    }
    
    // Safe cleanup
    esp_http_client_cleanup(client);
    
    if (retry_count >= max_retries && err != ESP_OK) {
        ESP_LOGE(TAG, "HTTP request failed after %d attempts", max_retries);
    }
    
    return err;
}

// Improved firebase_verify_rfid function with robust error handling
bool firebase_verify_rfid(const uint8_t *rfid_uid, uint8_t uid_size, user_t *user) {
    if (rfid_uid == NULL || user == NULL || uid_size == 0) {
        ESP_LOGE(TAG, "Invalid parameters for verify_rfid");
        return false;
    }
    
    // Clear user struct to avoid stale data
    memset(user, 0, sizeof(user_t));
    
    // Convert RFID UID to string format
    char uid_string[32] = {0};
    rfid_uid_to_string(rfid_uid, uid_size, uid_string, sizeof(uid_string));
    ESP_LOGI(TAG, "Looking for RFID: %s", uid_string);
    
    // For comparison, ensure we're using the right number of chars for the database format
    // RFID cards can have different formats - your database may store UIDs in different formats
    // Using just 8 characters (4 bytes) for comparison
    char uid_prefix[9] = {0}; // 8 chars + null terminator
    size_t prefix_len = uid_size > 4 ? 8 : uid_size * 2;
    strncpy(uid_prefix, uid_string, prefix_len);
    uid_prefix[prefix_len] = '\0';
    ESP_LOGI(TAG, "Using RFID prefix for comparison: %s", uid_prefix);
    
    // Request the rfidApplications data
    char path[64];
    snprintf(path, sizeof(path), "/rfidApplications");
    
    // Allocate response buffer in heap rather than stack to avoid stack overflow
    char *response = malloc(8192); // 8KB buffer
    if (response == NULL) {
        ESP_LOGE(TAG, "Failed to allocate memory for response buffer");
        return false;
    }
    memset(response, 0, 8192);
    
    esp_err_t err = firebase_http_request(path, "GET", NULL, response, 8192);
    
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to fetch RFID data from Firebase");
        free(response);
        return false;
    }
    
    // Check if response is empty or null
    if (response[0] == '\0' || strcmp(response, "null") == 0) {
        ESP_LOGE(TAG, "Empty response or null data");
        free(response);
        return false;
    }
    
    // Parse the JSON response
    cJSON *root = cJSON_Parse(response);
    
    // We don't need the response buffer anymore since it's parsed
    free(response);
    
    if (root == NULL) {
        const char *error_ptr = cJSON_GetErrorPtr();
        ESP_LOGE(TAG, "Failed to parse JSON response. Error: %s", error_ptr ? error_ptr : "Unknown error");
        return false;
    }
    
    bool found = false;
    
    // Correctly iterate through the rfidApplications - check if it's an object
    if (cJSON_IsObject(root)) {
        cJSON *app_id = NULL;
        cJSON *app_data = NULL;
        
        // Iterate through all child objects
        cJSON_ArrayForEach(app_data, root) {
            // Get the application ID (key)
            const char *id_str = app_data->string;
            if (id_str == NULL) {
                ESP_LOGW(TAG, "Application without ID, skipping");
                continue;
            }
            
            ESP_LOGI(TAG, "Checking application ID: %s", id_str);
            
            // Get application properties
            cJSON *stored_rfid = cJSON_GetObjectItem(app_data, "rfidUid");
            cJSON *status = cJSON_GetObjectItem(app_data, "status");
            
            // Check if stored_rfid exists and is a string
            if (stored_rfid && cJSON_IsString(stored_rfid) && stored_rfid->valuestring != NULL) {
                ESP_LOGI(TAG, "Comparing RFID prefix: %s with stored: %s", uid_prefix, stored_rfid->valuestring);
                
                // Safer comparison: check if our UID prefix appears in the stored RFID
                // This allows for different RFID formats (e.g. with or without checksum bytes)
                if (strstr(stored_rfid->valuestring, uid_prefix) != NULL) {
                    ESP_LOGI(TAG, "RFID match found!");
                    
                    // Check if approved (if status field exists)
                    if (status && cJSON_IsString(status) && status->valuestring != NULL) {
                        ESP_LOGI(TAG, "Application status: %s", status->valuestring);
                        if (strcmp(status->valuestring, "approved") != 0) {
                            ESP_LOGW(TAG, "RFID found but not approved");
                            continue;  // Skip non-approved RFIDs
                        }
                    }
                    
                    // Get user details
                    cJSON *name = cJSON_GetObjectItem(app_data, "name");
                    
                    if (name && cJSON_IsString(name) && name->valuestring != NULL) {
                        // Copy user data with bounds checking
                        strncpy(user->name, name->valuestring, sizeof(user->name) - 1);
                        user->name[sizeof(user->name) - 1] = '\0';
                        
                        // Use the application ID as user_id
                        strncpy(user->user_id, id_str, sizeof(user->user_id) - 1);
                        user->user_id[sizeof(user->user_id) - 1] = '\0';
                        
                        // Copy RFID UID
                        memcpy(user->rfid_uid, rfid_uid, uid_size);
                        user->uid_size = uid_size;
                        
                        ESP_LOGI(TAG, "Found valid RFID for user: %s with ID: %s", user->name, user->user_id);
                        found = true;
                        break;
                    }
                }
            }
        }
    } else {
        ESP_LOGE(TAG, "Root JSON is not an object");
    }
    
    // Clean up
    cJSON_Delete(root);
    
    if (!found) {
        ESP_LOGE(TAG, "RFID not found or not valid");
    }
    
    return found;
}

// Start a new journey session with improved error handling
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
    
    // Create JSON for Firebase with proper error handling
    char rfid_string[32] = {0};
    rfid_uid_to_string(journey->rfid_uid, journey->uid_size, rfid_string, sizeof(rfid_string));
    
    char timestamp_str[32] = {0};
    format_timestamp(journey->start_timestamp, timestamp_str, sizeof(timestamp_str));
    
    // Use cJSON for more reliable JSON creation
    cJSON *journey_json = cJSON_CreateObject();
    if (journey_json == NULL) {
        ESP_LOGE(TAG, "Failed to create JSON object");
        return false;
    }
    
    // Add all journey properties
    cJSON_AddStringToObject(journey_json, "ticketID", journey->ticket_id);
    cJSON_AddStringToObject(journey_json, "rfidUid", rfid_string);
    cJSON_AddStringToObject(journey_json, "startTimestamp", timestamp_str);
    cJSON_AddNumberToObject(journey_json, "originStation", journey->origin_station);
    cJSON_AddNumberToObject(journey_json, "selectedClass", journey->selected_class);
    cJSON_AddNumberToObject(journey_json, "selectedDestinationStation", journey->selected_destination);
    cJSON_AddNumberToObject(journey_json, "currentState", journey->current_state);
    
    // Convert to string
    char *json_str = cJSON_Print(journey_json);
    cJSON_Delete(journey_json);
    
    if (json_str == NULL) {
        ESP_LOGE(TAG, "Failed to convert JSON to string");
        return false;
    }
    
    // Save to Firebase
    char path[64];
    snprintf(path, sizeof(path), "/journeys/%s", journey->ticket_id);
    
    esp_err_t err = firebase_http_request(path, "PUT", json_str, NULL, 0);
    
    // Free the JSON string
    free(json_str);
    
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

// Check if there is an active journey for this RFID with improved error handling
bool firebase_check_active_journey(const uint8_t *rfid_uid, uint8_t uid_size, journey_session_t *journey) {
    if (rfid_uid == NULL || journey == NULL || uid_size == 0) {
        ESP_LOGE(TAG, "Invalid parameters for check_active_journey");
        return false;
    }
    
    // Clear journey struct to avoid stale data
    memset(journey, 0, sizeof(journey_session_t));
    
    char uid_string[32] = {0};
    rfid_uid_to_string(rfid_uid, uid_size, uid_string, sizeof(uid_string));
    
    // Create query to find active journeys for this RFID
    char path[128];
    snprintf(path, sizeof(path), "/journeys");
    
    // Allocate response buffer in heap
    char *response = malloc(8192);
    if (response == NULL) {
        ESP_LOGE(TAG, "Failed to allocate memory for response buffer");
        return false;
    }
    memset(response, 0, 8192);
    
    esp_err_t err = firebase_http_request(path, "GET", NULL, response, 8192);
    
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to fetch journeys data from Firebase");
        free(response);
        return false;
    }
    
    // Parse the response using cJSON
    cJSON *root = cJSON_Parse(response);
    free(response); // Free response after parsing
    
    if (root == NULL) {
        ESP_LOGE(TAG, "Failed to parse JSON response");
        return false;
    }
    
    bool found = false;
    
    // Iterate through journeys to find active ones for this RFID
    if (cJSON_IsObject(root)) {
        cJSON *journey_item = NULL;
        
        cJSON_ArrayForEach(journey_item, root) {
            if (!cJSON_IsObject(journey_item)) continue;
            
            cJSON *stored_rfid = cJSON_GetObjectItem(journey_item, "rfidUid");
            cJSON *current_state = cJSON_GetObjectItem(journey_item, "currentState");
            
            if (stored_rfid && cJSON_IsString(stored_rfid) && stored_rfid->valuestring &&
                current_state && cJSON_IsNumber(current_state)) {
                
                if (strcmp(stored_rfid->valuestring, uid_string) == 0 && 
                    current_state->valueint == JOURNEY_STATE_ACTIVE) {
                    
                    // Found an active journey for this RFID
                    cJSON *ticket_id = cJSON_GetObjectItem(journey_item, "ticketID");
                    cJSON *start_timestamp_str = cJSON_GetObjectItem(journey_item, "startTimestamp");
                    cJSON *origin_station = cJSON_GetObjectItem(journey_item, "originStation");
                    cJSON *selected_class = cJSON_GetObjectItem(journey_item, "selectedClass");
                    cJSON *selected_destination = cJSON_GetObjectItem(journey_item, "selectedDestinationStation");
                    
                    if (ticket_id && cJSON_IsString(ticket_id) && ticket_id->valuestring &&
                        start_timestamp_str && cJSON_IsString(start_timestamp_str) && start_timestamp_str->valuestring &&
                        origin_station && cJSON_IsNumber(origin_station) &&
                        selected_class && cJSON_IsNumber(selected_class) &&
                        selected_destination && cJSON_IsNumber(selected_destination)) {
                        
                        // Copy journey data with proper bounds checking
                        strncpy(journey->ticket_id, ticket_id->valuestring, sizeof(journey->ticket_id) - 1);
                        journey->ticket_id[sizeof(journey->ticket_id) - 1] = '\0';
                        
                        memcpy(journey->rfid_uid, rfid_uid, uid_size);
                        journey->uid_size = uid_size;
                        
                        // TODO: Parse timestamp string to time_t
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
    }
    
    cJSON_Delete(root);
    return found;
}

// End a journey session with improved error handling
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
    
    // Use cJSON for reliable JSON creation
    cJSON *journey_json = cJSON_CreateObject();
    if (journey_json == NULL) {
        ESP_LOGE(TAG, "Failed to create JSON object");
        return false;
    }
    
    // Format timestamps
    char rfid_string[32] = {0};
    rfid_uid_to_string(journey->rfid_uid, journey->uid_size, rfid_string, sizeof(rfid_string));
    
    char start_timestamp_str[32] = {0};
    format_timestamp(journey->start_timestamp, start_timestamp_str, sizeof(start_timestamp_str));
    
    char end_timestamp_str[32] = {0};
    format_timestamp(journey->end_timestamp, end_timestamp_str, sizeof(end_timestamp_str));
    
    // Add all journey properties
    cJSON_AddStringToObject(journey_json, "ticketID", journey->ticket_id);
    cJSON_AddStringToObject(journey_json, "rfidUid", rfid_string);
    cJSON_AddStringToObject(journey_json, "startTimestamp", start_timestamp_str);
    cJSON_AddStringToObject(journey_json, "endTimestamp", end_timestamp_str);
    cJSON_AddNumberToObject(journey_json, "originStation", journey->origin_station);
    cJSON_AddNumberToObject(journey_json, "selectedClass", journey->selected_class);
    cJSON_AddNumberToObject(journey_json, "selectedDestinationStation", journey->selected_destination);
    cJSON_AddNumberToObject(journey_json, "actualDestinationStation", journey->actual_destination);
    cJSON_AddNumberToObject(journey_json, "travelDuration", journey->travel_duration);
    cJSON_AddBoolToObject(journey_json, "isFraudSuspected", journey->is_fraud_suspected);
    cJSON_AddNumberToObject(journey_json, "currentState", journey->current_state);
    
    // Convert to string
    char *json_str = cJSON_Print(journey_json);
    cJSON_Delete(journey_json);
    
    if (json_str == NULL) {
        ESP_LOGE(TAG, "Failed to convert JSON to string");
        return false;
    }
    
    // Update in Firebase
    char path[64];
    snprintf(path, sizeof(path), "/journeys/%s", journey->ticket_id);
    
    esp_err_t err = firebase_http_request(path, "PUT", json_str, NULL, 0);
    
    // Free the JSON string
    free(json_str);
    
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
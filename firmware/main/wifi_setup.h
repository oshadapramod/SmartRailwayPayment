// Modified wifi_setup.h
#ifndef WIFI_SETUP_H
#define WIFI_SETUP_H

#include "esp_err.h"
#include <stdbool.h>

// WiFi Credentials - Update with your correct credentials
//#define WIFI_SSID "FOE-Student"
//#define WIFI_PASSWORD "abcd@1234"

#define WIFI_SSID "oshBOY"
#define WIFI_PASSWORD "11111111"

// Timeout and retry settings
#define MAXIMUM_RETRY 15          // Increased from 10
#define CONNECTION_TIMEOUT_MS 30000  // 30 seconds timeout

// Initialize WiFi connection
esp_err_t wifi_init_sta(void);

// Check if WiFi is connected
bool wifi_is_connected(void);

// Reconnect to WiFi if disconnected
esp_err_t wifi_reconnect(void);

#endif /* WIFI_SETUP_H */
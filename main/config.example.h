#pragma once

// ==========================================
// WiFi Configuration
// ==========================================
#define WIFI_SSID       "YOUR_WIFI_SSID"
#define WIFI_PASSWORD   "YOUR_WIFI_PASSWORD"
#define WIFI_MAX_RETRY  5

// Optional Static IP Configuration for sub-300ms Wi-Fi connection
// #define WIFI_STATIC_IP   "192.168.1.30"
// #define WIFI_NETMASK     "255.255.255.0"
// #define WIFI_GATEWAY     "192.168.1.1"
// #define WIFI_DNS         "192.168.1.1"
// #define WIFI_CHANNEL     6

// ==========================================
// MQTT Configuration
// ==========================================
// Replace with the IP address of your Raspberry Pi 4 running the Mosquitto broker
#define MQTT_BROKER_URI "mqtt://192.168.1.10:1883"

// ==========================================
// Gateway Configuration
// ==========================================
// Replace with the IP address of your Raspberry Pi running the Spring Boot Gateway
#define GATEWAY_API_URL "http://192.168.1.10:8080/api/events"
#define GATEWAY_API_KEY "YOUR_HARD_TO_GUESS_API_KEY"
#define DEVICE_ID       "front-door"

// ==========================================
// Media Configuration
// ==========================================
#define AUDIO_RECORD_TIME_SEC 5

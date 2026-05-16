#pragma once

// ==========================================
// WiFi Configuration
// ==========================================
#define WIFI_SSID       "YOUR_WIFI_SSID"
#define WIFI_PASSWORD   "YOUR_WIFI_PASSWORD"

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

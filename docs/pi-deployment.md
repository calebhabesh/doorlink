# Raspberry Pi Deployment Guide

This document outlines the deployment topology for the Smart Doorbell system on the local Raspberry Pi 4 gateway.

## Network Topology

- **Host:** Raspberry Pi 4 (4GB)
- **Local IP Address:** `192.168.1.10` (Ensure this is set as a static IP on your router)

### Active Services & Ports

| Service | Port | Description |
| :--- | :--- | :--- |
| **Dashboard (Next.js)** | `3000` | The web interface for viewing events and interacting with the doorbell. |
| **Gateway (Spring Boot)**| `8080` | The main backend API that the firmware communicates with. |
| **Mosquitto (MQTT)** | `1883` | Message broker for half-duplex audio and device commands. |
| **MinIO (S3 Storage)** | `9000` | Local object storage for saving event images and audio. (Console on `9001`) |
| **PostgreSQL** | `5432` | Relational database for storing event metadata. |

## Running the Services

The deployment relies on a hybrid approach, using Docker Compose for infrastructure and persistent background processes for the application layers.

### 1. Infrastructure (Docker Compose)
The database, message broker, and media storage are containerized.
```bash
cd /home/ethioking/dev/smart-doorbell
docker-compose up -d
```

### 2. Backend Gateway
The Spring Boot gateway is run using the Maven wrapper.
```bash
cd /home/ethioking/dev/smart-doorbell/gateway
./mvnw spring-boot:run
```

### 3. Frontend Dashboard
The Next.js dashboard runs in development/production mode.
```bash
cd /home/ethioking/dev/smart-doorbell/dashboard
npm run dev # or npm start for a production build
```

*(Note: Currently, the Gateway and Dashboard are running in persistent `tmux` sessions on the Pi.)*

## Firmware Configuration (`config.h`)

For the ESP32-S3 firmware to successfully communicate with this Pi, ensure your `main/config.h` reflects the following:

```c
#define WIFI_SSID "Your_WiFi_SSID"
#define WIFI_PASSWORD "Your_WiFi_Password"

// Gateway API for image/audio HTTP POSTs
#define GATEWAY_API_URL "http://192.168.1.10:8080/api/events"

// MQTT Broker for half-duplex interaction
#define MQTT_BROKER_URI "mqtt://192.168.1.10:1883"
```

## External Access (WAN)

To securely access the Next.js Dashboard and Gateway API from outside the local network without exposing ports to the public internet, we utilize **Cloudflare Tunnels** (`cloudflared`). 

*Configuration details for `cloudflared` will be documented here once established.*

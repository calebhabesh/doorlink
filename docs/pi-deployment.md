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

The deployment relies on a hybrid approach: Docker Compose runs infrastructure containers, while systemd manages the Spring Boot gateway and Next.js dashboard services. The unit files are tracked in `scripts/systemd/`.

### 1. Infrastructure (Docker Compose)
The database, message broker, and media storage are containerized.
```bash
cd /home/ethioprince/dev/smart-doorbell
docker compose up -d
```

### 2. Build Production Artifacts

The Raspberry Pi systemd services are production runtime services. They must not run `./mvnw spring-boot:run` or `npm run dev`; those commands keep build/watch tooling alive and can cause large CPU spikes on the Pi.

After pulling gateway or dashboard changes on the Pi, build the runtime artifacts once:

```bash
cd /home/ethioprince/dev/smart-doorbell
./scripts/build-pi-production.sh
```

This creates:

- `gateway/target/gateway-0.0.1-SNAPSHOT.jar`
- `dashboard/.next`

The build script runs Maven, npm, and Next.js under reduced CPU/IO priority so the Pi remains more responsive during deployment.

### 3. Install Or Refresh Systemd Units

After changing files under `scripts/systemd/`, copy them into systemd and reload:

```bash
cd /home/ethioprince/dev/smart-doorbell
sudo cp scripts/systemd/smart-doorbell-gateway.service /etc/systemd/system/
sudo cp scripts/systemd/smart-doorbell-dashboard.service /etc/systemd/system/
sudo systemctl daemon-reload
```

### 4. Backend Gateway
The Spring Boot gateway is managed by `smart-doorbell-gateway.service`.
```bash
sudo systemctl status smart-doorbell-gateway.service
sudo systemctl restart smart-doorbell-gateway.service
journalctl -u smart-doorbell-gateway.service -f
```

### 5. Frontend Dashboard
The Next.js dashboard is managed by `smart-doorbell-dashboard.service`.
```bash
sudo systemctl status smart-doorbell-dashboard.service
sudo systemctl restart smart-doorbell-dashboard.service
journalctl -u smart-doorbell-dashboard.service -f
```

After pulling new changes on the Pi, run `./scripts/build-pi-production.sh`, then restart only the service affected by the change. Backend changes generally require restarting `smart-doorbell-gateway.service`; dashboard changes generally require restarting `smart-doorbell-dashboard.service`.

## Emergency CPU Recovery

If SSH or the local TTY is already sluggish because the old dev-mode services are consuming CPU, stop the app services first, then build and restart them with the production units:

```bash
sudo systemctl stop smart-doorbell-dashboard.service smart-doorbell-gateway.service
cd /home/ethioprince/dev/smart-doorbell
git pull
./scripts/build-pi-production.sh
sudo cp scripts/systemd/smart-doorbell-gateway.service /etc/systemd/system/
sudo cp scripts/systemd/smart-doorbell-dashboard.service /etc/systemd/system/
sudo systemctl daemon-reload
sudo systemctl restart smart-doorbell-gateway.service
sudo systemctl restart smart-doorbell-dashboard.service
```

If CPU spikes persist after this, capture evidence while the spike is happening:

```bash
cd /home/ethioprince/dev/smart-doorbell
./scripts/pi-cpu-snapshot.sh
```

The script prints the path to a log under `/tmp/smart-doorbell-diagnostics/` with process CPU usage, memory pressure, Docker stats, and recent service logs.

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

See `docs/cloudflare-tunnel.md` for the Cloudflare Tunnel and Access configuration.

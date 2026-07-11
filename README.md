# Doorlink

## About The Project

- Living in an apartment without a concierge, you're often left guessing who's at the door, and asking questions like "did my package arrive?". Providing an interface for visitors to communicate in real-time to notify me of package deliveries and general presence would be really convenient.

- Doorlink is a self-hosted IoT smart doorbell built around a custom ESP32-S3-WROOM-1-N16R8 PCB, a Raspberry Pi gateway, local media storage, and real-time mobile notifications. The project is designed for image capture, visitor audio recording, and half-duplex reply audio without a proprietary subscription cloud.

## Demo

- [GIF or short video of doorbell will go here]

## Current Status

**Note:** The gateway, dashboard, storage, MQTT event pipeline, and notification path are implemented and running on the Raspberry Pi gateway. The custom hardware PCB is currently in manufacturing. Firmware capture/audio behavior and battery-life estimates are designed targets pending empirical hardware validation after the board arrives.

## Features

**Implemented Software Stack:**

- Real-time push notifications to Android and iOS via ntfy (using public ntfy.sh servers)
- Self-hosted gateway on Raspberry Pi 4 (Spring Boot, Postgres, Mosquitto, MinIO)
- High-density Next.js Dashboard for viewing historical events and active feeds
- Local network media storage without third-party vendor cloud lock-in

**Designed Hardware Capabilities (Pending PCB Arrival):**

- Button-triggered image capture and visitor audio recording (OV5640 + ICS-43434)
- Two-way audio - reply to visitors from dashboard via MQTT (MAX98357A)
- Low power deep sleep between events (Target: ~27 day battery life)

## System Architecture

- Architecture diagram pending. Current deployment topology is documented in `docs/pi-deployment.md`.

## Interaction Model (Audio & Wakeup)

The target firmware model is an asynchronous "Record-and-Send" event flow followed by a short "Half-Duplex" interaction window. Full-duplex phone-call behavior is intentionally avoided to reduce ESP32-S3 processing load and avoid acoustic echo cancellation complexity.

1. **Initial Trigger:** A visitor single-presses the doorbell button. This wakes the ESP32 from deep sleep through the routed GPIO2 wake input.
2. **Capture Phase:** The firmware will capture a JPEG from the OV5640, record a short visitor audio clip from the ICS-43434 microphone, and upload both to the gateway via HTTP POST.
3. **Interactive Phase:** After upload, the ESP32 will connect to MQTT and stay awake for a short reply window. During this window, the homeowner receives the mobile notification, opens the dashboard, and uses push-to-talk to send audio back to the doorbell speaker.
4. **Sleep Phase:** Once the interaction window expires without new activity, the ESP32 powers down peripherals and returns to deep sleep.

## Hardware

- [Photo of assembled enclosure and custom PCB]

## Components

| Component                       | Purpose               |
| ------------------------------- | --------------------- |
| ESP32-S3-WROOM-1-N16R8          | Main MCU (Octal SPI)* |
| OV5640 (24-pin FPC)             | Camera                |
| ICS-43434                       | Visitor microphone*   |
| MAX98357A + Speaker             | Homeowner reply audio |
| MCP73831 & AP2112K-3.3          | Battery Charger & 3.3V LDO |
| XC6206P282MR & XC6206P152MR     | 2.8V and 1.5V Camera LDOs |
| SRV05-4                         | ESD Protection        |
| 22mm Momentary Button           | Doorbell trigger      |
| Raspberry Pi 4 (4GB)            | Local gateway server  |
| 80×110×70mm Black ABS Enclosure (planned Rev B) | Housing |

*\* Note: The ESP32-S3 and ICS-43434 are hand-soldered onto the board to maintain economy-level PCBA constraints with JLCPCB.*

### Wiring And PCB

The KiCad design, routed netlist, Gerbers, BOM exports, and assembly notes live under `pcb/`. The firmware pin map is mirrored from the routed netlist in `main/board_pins.h`.

## Software Stack

- **Firmware:** C/C++ (ESP-IDF framework)
- **Gateway:** Spring Boot, Docker Compose
- **Messaging:** Eclipse Mosquitto (MQTT)
- **Storage:** MinIO (S3-compatible object storage)
- **Database:** PostgreSQL
- **Notifications:** ntfy
- **Dashboard:** Next.js 14

## Development And Deployment Workflow

Development happens from this Arch server checkout. The Raspberry Pi is the always-on gateway target running Docker infrastructure, the Spring Boot gateway, and the Next.js dashboard. Spring Boot and Next.js are managed by systemd units tracked under `scripts/systemd/`.

Normal workflow:

1. Edit and verify changes on the Arch development machine.
2. Commit and push changes.
3. SSH into the Raspberry Pi gateway.
4. Pull the branch in the Pi checkout.
5. Run `./scripts/build-pi-production.sh` if gateway or dashboard code changed.
6. Restart only the affected systemd service.

See `docs/pi-deployment.md` for the current Pi topology and `docs/hardware-bringup.md` for the board bring-up checklist.

## Getting Started For Development

### Prerequisites

- Java 21 for the Spring Boot gateway
- Node.js/npm for the dashboard
- ESP-IDF installed for firmware builds
- Docker + Docker Compose on the Raspberry Pi gateway
- WiFi credentials

### Infrastructure On The Raspberry Pi

```bash
git clone https://github.com/calebhabesh/smart-doorbell
cd smart-doorbell
docker compose up -d
./scripts/build-pi-production.sh
```

This starts PostgreSQL, Mosquitto, and MinIO. On the Pi, `smart-doorbell-infra.service` runs the same compose bootstrap during boot before the gateway starts. Spring Boot and Next.js run as separate systemd services; see `docs/pi-deployment.md`.

### Backend Verification

```bash
cd gateway
./mvnw compile
# or
./mvnw test
```

### Dashboard Verification

```bash
cd dashboard
npm install
npx tsc --noEmit
npm run lint
```

### Firmware Setup

```bash
cp main/config.example.h main/config.h
# Set WiFi credentials, GATEWAY_API_URL, and MQTT_BROKER_URI for the Raspberry Pi gateway.
idf.py build flash monitor
```

## Project Structure

```
├── main/              # ESP-IDF firmware source, board pin map, and config template
├── gateway/           # Spring Boot backend
├── dashboard/         # Next.js dashboard
├── pcb/               # KiCad project, Gerbers, production files, and PCB notes
├── docs/              # Pi deployment, Cloudflare tunnel, bring-up checklist, design notes
├── scripts/           # Test assets, utility scripts, and Raspberry Pi systemd unit files
├── docker-compose.yml # PostgreSQL, Mosquitto, and MinIO infrastructure
└── sdkconfig.defaults # ESP32-S3 firmware defaults
```

## Future Improvements

- Hardware bring-up and measured battery-life validation after PCB arrival
- Real OV5640 JPEG capture and ICS-43434 visitor audio recording
- Push-to-talk audio delivery from dashboard to MAX98357A speaker path
- Backend media proxy or presigned URLs for MinIO objects
- Motion detection as secondary wakeup trigger

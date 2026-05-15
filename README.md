# Smart Doorbell

## About the Project

- Living in an apartment without a concierge, you're often left guessing who's at the door, and asking questions like "did my package arrive?". Providing an interface for visitors to communicate in real-time to notify me of package deliveries and general presence would be really convenient.

- This is a self hosted IoT smart doorbell built on a custom ESP32-S3-WROOM-1-N16R8 PCB, featuring two-way audio, image capture, and real-time mobile notifications. No cloud subscription required.

## Demo

- [GIF or short video of doorbell will go here]

## Current Status

**Note:** The software stack (Gateway, Dashboard, and Notification pipeline) is fully implemented and tested. The custom hardware PCB is currently in manufacturing. Firmware capabilities and battery life estimates are designed targets pending empirical hardware validation upon arrival.

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

- [Architecture Diagram Image]

## Interaction Model (Audio & Wakeup)

To maximize battery life (~27 days), the doorbell operates using an asynchronous "Record-and-Send" model followed by a "Half-Duplex" interaction window. Full-duplex (phone call style) is avoided to eliminate the need for heavy Acoustic Echo Cancellation (AEC) processing on the ESP32-S3.

1. **Initial Trigger (Asynchronous):** A visitor single-presses the doorbell button (no need to hold). This wakes the ESP32 from deep sleep.
2. **Capture Phase:** The ESP32 immediately snaps a JPEG photo, records 5-10 seconds of audio from the ICS-43434 microphone, and uploads both to the Gateway via HTTP POST.
3. **Interactive Phase (Half-Duplex):** After uploading, the ESP32 connects to the MQTT broker and stays awake for 60 seconds listening for incoming audio packets. During this window, the homeowner receives the mobile notification, opens the dashboard, and can press and hold the "Push to Talk" button to stream their voice back to the doorbell's speaker.
4. **Sleep Phase:** Once the 60-second window expires without new interaction, the ESP32 shuts down the peripherals and returns to deep sleep.

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
| 100×68×50mm Black ABS Enclosure | Housing               |

*\* Note: The ESP32-S3 and ICS-43434 are hand-soldered onto the board to maintain economy-level PCBA constraints with JLCPCB.*

### Wiring Diagram

(Custom PCB schematic created in KiCad)

## Software Stack

- **Firmware:** C/C++ (ESP-IDF framework)
- **Gateway:** Spring Boot, Docker Compose
- **Messaging:** Eclipse Mosquitto (MQTT)
- **Storage:** MinIO (S3-compatible object storage)
- **Database:** PostgreSQL
- **Notifications:** ntfy
- **Dashboard:** Next.js 14

## Getting Started

### Prerequisites

- Docker + Docker Compose installed on Raspberry Pi 4
- ESP-IDF installed locally
- WiFi credentials

### Gateway Setup

```bash
git clone https://github.com/calebhabesh/smart-doorbell
cd smart-doorbell/gateway
docker compose up -d
```

### Firmware Setup

```bash
cd firmware
# Set your WiFi credentials and gateway IP in config.h
cp config.example.h config.h
idf.py build flash monitor
```

### Dashboard Setup

```bash
cd dashboard
npm install
npm run dev
```

## Project Structure

```
├── firmware/          # ESP32-S3 C/C++ ESP-IDF project
├── gateway/           # Docker Compose + Spring Boot backend
├── dashboard/         # Next.js frontend
├── pcb/               # KiCad project files for the custom PCB
└── docs/              # Wiring diagrams, architecture images
```

## Future Improvements

- ESP-NOW for faster WiFi-free local communication
- Motion detection as secondary wakeup trigger
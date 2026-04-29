# Smart Doorbell

## About the Project

- Living in an apartment without a concierge, you're often left guessing who's at the door, and asking questions like "did my package arrive?". Providing an interface for visitors to communicate in real-time to notify me of package deliveries and general presence would be really convenient.

- This is a self hosted IoT smart doorbell built on an ESP32-S3-DevKitC-1, featuring two-way audio, image capture, and real-time mobile notifications. No cloud subscription required.

## Demo

- [GIF or short video of doorbell will go here]

## Features

- Button-triggered image capture and visitor audio recording
- Real-time push notifications to Android and iOS via ntfy
- Two-way audio - reply to visitors from dashboard
- Self-hosted gateway on Raspberry Pi 4 (no third-party cloud)
- Low power deep sleep between events (~ 27 day battery life)

## System Architecture

- [Architecture Diagram Image]

## Hardware

- [Photo of assembled enclosure]

## Components

| Component                       | Purpose               |
| ------------------------------- | --------------------- |
| ESP32-S3-DevKitC-1              | Main MCU              |
| OV5640 (120° distortion-free)   | Camera                |
| INMP441                         | Visitor microphone    |
| MAX98357A + 8Ω speaker          | Homeowner reply audio |
| 22mm Momentary Button           | Doorbell trigger      |
| 2000mAh LiPo + TP4056 + MT3608  | Battery power stack   |
| Raspberry Pi 4 (4GB)            | Local gateway server  |
| 100×68×50mm Black ABS Enclosure | Housing               |

### Wiring Diagram

[Diagram Image]

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
└── docs/              # Wiring diagrams, architecture images
```

## Future Improvements

- Bare ESP32-S3-WROOM-2 module PCB to eliminate DevKit parasitic draw
- MOSFET power gate on camera line during deep sleep
- ESP-NOW for faster WiFi-free local communication
- Motion detection as secondary wakeup trigger

# Doorlink

## About The Project

- Living in an apartment without a concierge, you're often left guessing who's at the door, and asking questions like "did my package arrive?". Providing an interface for visitors to communicate in real-time to notify me of package deliveries and general presence would be really convenient.

- Doorlink is a self-hosted IoT smart doorbell built around a custom ESP32-S3-WROOM-1-N16R8 PCB, a Raspberry Pi gateway, local media storage, and real-time mobile notifications. The validated hardware path captures and uploads doorbell still images and visitor audio without a proprietary subscription cloud. Stored half-duplex replies have also passed an end-to-end dashboard-to-doorbell speaker test.

## Demo

- [GIF or short video of doorbell will go here]

## Current Status

The Rev C PCB is assembled and in hardware bring-up. A battery-powered QXGA
`2048x1536` capture reached the Raspberry Pi gateway, appeared in Doorlink, and
triggered the configured notification/chime path. A subsequent production
button wake also captured and displayed a correctly oriented QXGA portrait,
delivered its notification, and returned to deep sleep. The gateway, dashboard,
PostgreSQL, MinIO, MQTT, SSE, and notification services are implemented.

The event-driven C++ firmware controller now builds. The isolated GPIO2 button
and GPIO3 PIR wake/return-to-deep-sleep paths have passed on hardware,
including early ring-LED acknowledgement for a button press. The complete
production wake/capture/upload/sleep cycle has also passed from both button
and PIR triggers, including Doorlink display and notifications. End-to-end
notification latency was reduced by the early-trigger flow. Production now
arms only the GPIO2 button by default; PIR remains available behind an explicit
build option and in its isolated diagnostic, but cannot generate normal
doorbell events accidentally. GPIO48 fades in, holds, and fades out after a
button press while D3 provides immediate wake acknowledgement.
The production controller now implements release-driven hold-to-message capture,
session-grouped presses, a complete first local chime, and stored browser PTT
replies in a bounded MQTT-controlled window. The earlier fixed five-second WAV
and stored reply path were exercised end to end on the assembled board; the new
hold thresholds, full-chime handoff, multi-press behavior, and latency still need
an instrumented hardware retest. Battery life has not been measured.
The earlier MK1 heating fault was resolved by reworking its ungrounded center
pad; see `docs/hardware-bringup.md` for the measured bring-up record.

## Features

**Implemented Software Stack:**

- Real-time push notifications to Android and iOS via ntfy (using public ntfy.sh servers)
- Self-hosted gateway on Raspberry Pi 4 (Spring Boot, Postgres, Mosquitto, MinIO)
- High-density Next.js Dashboard for viewing historical events and active feeds
- Local network media storage without third-party vendor cloud lock-in

**Validated Or Implemented Hardware/Firmware:**

- OV5640 QXGA still capture at 10 MHz with bounded camera power sequencing
- Battery-only capture, Wi-Fi upload, gateway persistence, dashboard display, and notifications
- Individually validated ICS-43434 microphone and MAX98357A speaker paths
- C++ wake/capture/upload/sleep controller with C hardware drivers
- Hardware-validated visitor WAV capture and stored half-duplex PTT reply path

**Still Pending Hardware Validation:**

- Active, idle, and deep-sleep current
- Corridor-lighting exposure and moving-person blur
- Instrumented microphone-start and reply-latency measurements

## System Architecture

- Architecture diagram pending. Current deployment topology is documented in `docs/pi-deployment.md`.

## Interaction Model (Audio & Wakeup)

The firmware model is a bounded visitor session followed by a short, turn-based
reply window. Full-duplex phone-call behavior is intentionally excluded; MQTT
carries control messages while HTTP carries WAV media.

1. **Trigger:** GPIO2 wakes on an active-low button press through EXT0. GPIO3 PIR wake is disabled in the normal production build and is opt-in for future motion events.
2. **First chime and hold:** The first local chime plays once to completion while the authenticated early trigger proceeds. A visitor still holding after the chime gets microphone capture until release or 15 seconds; clips shorter than one second of microphone audio are discarded.
3. **Capture:** After the early alert and RF shutdown, firmware enables U9 and captures the initial QXGA JPEG. Later presses refresh it only after it is 15 seconds old.
4. **Later presses:** Every valid down-edge becomes another ordered press in the same session, starts hold capture immediately, and restarts LED feedback. The local chime is not replayed.
5. **Upload:** Image and visitor recordings have stable per-press identifiers. The gateway groups them into one session, deduplicates notification delivery at session scope, and publishes lifecycle updates over SSE.
6. **Reply window:** Dashboard PTT replies are stored WAV turns. A reply that arrives during visitor capture waits until the visitor releases; the microphone stays off during speaker playback.
7. **Shutdown:** At the 60-second idle or 90-second absolute deadline, firmware closes the gateway session, releases network/media resources, holds camera/audio controls safe, and enters deep sleep.

The dashboard Active Event, Event Log, and Calendar show one expandable visitor
session with all presses, visitor messages, and homeowner replies. See
`docs/behavioral_spec.md` for the locked interaction contract.

## Hardware

- [Photo of assembled enclosure and custom PCB]

## Components

| Component                       | Purpose               |
| ------------------------------- | --------------------- |
| ESP32-S3-WROOM-1-N16R8          | Main MCU (Octal SPI)* |
| OV5640 (24-pin FPC)             | Camera                |
| ICS-43434                       | Visitor microphone*   |
| MAX98357A + 4Ω, 3W-rated speaker | Homeowner reply audio |
| MCP73871                        | LiPo charger and power-path manager |
| TPS63802                        | 3.3 V buck-boost regulator |
| TPS22919                        | Firmware-controlled camera power switch |
| XC6206P282MR & XC6206P152MR     | 2.8V and 1.5V Camera LDOs |
| SRV05-4                         | ESD Protection        |
| 22mm Momentary Button           | Doorbell trigger      |
| Raspberry Pi 4 (4GB)            | Local gateway server  |
| 80×130×70mm black ABS enclosure | Housing construction target |

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

See `docs/pi-deployment.md` for the current Pi topology,
`docs/hardware-bringup.md` for the board bring-up record, and
`docs/enclosure-construction-guide.md` for the measured-fit enclosure workflow.

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
# Set Wi-Fi credentials and GATEWAY_API_URL for the Raspberry Pi gateway.

# Hardware-safe core-only build (default)
idf.py build

# Event-driven production build in an isolated directory
idf.py -B build-production -D SDKCONFIG=sdkconfig.production \
  -D 'SDKCONFIG_DEFAULTS=sdkconfig.defaults;sdkconfig.production.defaults' build
```

The production build is intentionally separate from the safe build. Do not
flash it as a substitute for the remaining wake/deep-sleep hardware test.

## Project Structure

```
├── main/              # ESP-IDF C/C++ firmware, services, board pin map, and config template
├── gateway/           # Spring Boot backend
├── dashboard/         # Next.js dashboard
├── pcb/               # KiCad project, Gerbers, production files, and PCB notes
├── docs/              # Firmware architecture, Pi deployment, bring-up checklist, design notes
├── scripts/           # Test assets, utility scripts, and Raspberry Pi systemd unit files
├── docker-compose.yml # PostgreSQL, Mosquitto, and MinIO infrastructure
├── sdkconfig.defaults # Hardware-safe ESP32-S3 defaults
└── sdkconfig.production.defaults # Production-controller configuration layer
```

## Future Improvements

- Measure active and deep-sleep current after the battery-path fault is resolved
- Corridor-lighting and moving-subject camera validation
- Retest the immediate greeting schedule and measure button-to-microphone latency
- Measure reply latency, recorded speech level, and active-session current
- Backend media proxy or presigned URLs for MinIO objects
- Motion detection as secondary wakeup trigger

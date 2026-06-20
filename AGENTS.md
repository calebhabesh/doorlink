# Smart Doorbell Agent Guide

This file is the working guide for AI agents editing this repository. Treat the KiCad project under `pcb/smart-doorbell/` as the routed PCB source of truth for net assignments. Treat `/home/ethioking/Documents/Documentation/bom-JLCPCB Assembly Order.xls` as the final JLCPCB assembly BOM source when component availability/cost swaps disagree with the preliminary KiCad BOM.

## Development Workflow

- Development/editing happens on the Arch server at `/home/ethioking/dev/smart-doorbell`.
- Runtime services are deployed on the Raspberry Pi gateway, not on the Arch development machine.
- The Raspberry Pi runs Spring Boot and Next.js through systemd service units tracked in `scripts/systemd/`; Docker Compose runs the infrastructure containers.
- Normal deployment flow: make edits on Arch, verify locally where possible, commit/push, then the user pulls the branch on the Raspberry Pi over SSH, runs `./scripts/build-pi-production.sh` when gateway/dashboard code changed, and restarts only the affected systemd service.
- Do not edit files directly on the Raspberry Pi unless explicitly asked. Treat the Arch checkout as the source working tree.
- Do not start or stop persistent gateway services on the Pi unless explicitly asked.
- **Rules of Engagement Exception:** Running local dev servers and local docker compose on Arch is permitted *only* when explicitly requested or doing local verification in "Local Dev Mode". To avoid conflict with other local projects like Fintrak, always use the isolated port configurations detailed below.

### Local Dev Mode on Arch (Isolated Stack)

To run a fully isolated local development stack on Arch without clashing with Fintrak or production Pi services, you can boot everything concurrently in a single terminal pane:

```bash
./scripts/run-dev.sh
```

This single script starts your Docker infrastructure (`.env.dev`), starts the Spring Boot gateway (dev profile, port 8081), runs the Next.js dashboard (port 3001), and cleanly shuts down all background processes and containers when you hit `Ctrl+C`.

Alternatively, you can run them manually in separate panes:

1. **Start Local Infra:** Run Docker Compose with the dev environment file:
   ```bash
   docker compose --env-file .env.dev up -d
   ```
   This starts Postgres (port 5433), Mosquitto (port 1884), and MinIO (ports 9002/9003) bound strictly to `127.0.0.1`.
2. **Start Backend Gateway:** Run Spring Boot under the `dev` profile (port 8081):
   ```bash
   cd gateway && SPRING_PROFILES_ACTIVE=dev ./mvnw spring-boot:run
   ```
3. **Start Dashboard Frontend:** Run the Next.js dev server on port 3001:
   ```bash
   cd dashboard && npm run dev -- -p 3001
   ```

### Arch-to-Pi Deployment Loop

- Commit and push changes from the Arch checkout.
- SSH into Raspberry Pi, pull latest changes, and run `./scripts/build-pi-production.sh` if gateway or dashboard code changed.
- Restart the affected service:
  - Gateway: `sudo systemctl restart smart-doorbell-gateway.service`
  - Dashboard: `sudo systemctl restart smart-doorbell-dashboard.service`

## Project Layout

- `main/`: ESP-IDF firmware component source and config template.
- `main/board_pins.h`: firmware board pin map mirrored from the routed KiCad netlist.
- `gateway/`: Spring Boot backend, event API, MQTT bridge, PostgreSQL persistence, MinIO upload, ntfy notification service.
- `dashboard/`: Next.js dashboard.
- `pcb/smart-doorbell/`: KiCad schematic/PCB, Gerbers, BOM, positions, and production netlist.
- `docker-compose.yml`: local PostgreSQL, Mosquitto, and MinIO stack at the repo root.

## Hardware Source Of Truth

The KiCad BOM/netlist may contain preliminary component values. Per the final assembly order, the effective MCU target is `ESP32-S3-WROOM-1-N16R8`; the WROOM-2 value in the KiCad files reflects an earlier cost/availability option. Always verify firmware flash/PSRAM settings against the actual module being assembled.

Confirmed routed net assignments from `pcb/smart-doorbell/smart-doorbell.net`:

- Doorbell input: `DOORBELL_IN` on ESP GPIO2.
- Battery monitor: `GPIO1` via 100k/100k divider.
- I2S audio shared clocks: LRCLK/WS GPIO4, BCLK/SCK GPIO5.
- I2S microphone: ICS-43434 SD on GPIO6.
- I2S amplifier: MAX98357A DIN on GPIO7, `AMP_EN` on GPIO43.
- USB-C native USB: D- GPIO19, D+ GPIO20, protected by SRV05-4.
- Optional PIR header: `PIR_OUT` on GPIO3.
- Status LED: GPIO47.
- Button LED: GPIO48.

The OV5640 24-pin FPC connector must match the camera ribbon cable pinout exactly. The ESP GPIO choices on the WROOM side were selected for clean PCB routing; they are flexible during design, but fixed once the board is fabricated. Current routed camera FPC to ESP GPIO mapping:

| Signal | ESP GPIO |
| --- | --- |
| CAM_D0 | GPIO18 |
| CAM_D1 | GPIO16 |
| CAM_D2 | GPIO15 |
| CAM_D3 | GPIO17 |
| CAM_D4 | GPIO8 |
| CAM_D5 | GPIO10 |
| CAM_D6 | GPIO11 |
| CAM_D7 | GPIO13 |
| CAM_PCLK | GPIO9 |
| CAM_XCLK | GPIO12 |
| CAM_HREF | GPIO14 |
| CAM_PWDN | GPIO21 |
| CAM_SDA | GPIO38 |
| CAM_SCL | GPIO39 |
| CAM_RST | GPIO40 |
| CAM_VSYNC | GPIO41 |

Do not assign firmware peripherals to GPIO19 or GPIO20 except for USB.

## Firmware Rules

- Keep secrets out of git. `main/config.h` is ignored; update `main/config.example.h` for tracked configuration shape.
- The intended runtime model is event-driven deep sleep:
  - configure EXT0 wakeup on `DOORBELL_IN`/GPIO2;
  - on button wake, connect Wi-Fi, capture a real JPEG, record visitor audio, upload media to the gateway, open a short MQTT listen window, then sleep;
  - avoid full-duplex audio and acoustic echo cancellation on the ESP32-S3.
- Current firmware is allowed to be a scaffold until hardware arrives. Verify `main/board_pins.h` against the routed PCB netlist before touching camera, audio, wake, USB, or power code.

## Backend And Dashboard Notes

- Gateway API base path is `/api/events`.
- `POST /api/events` accepts multipart `image`, optional multipart `audio`, and optional `eventType`.
- Gateway publishes saved events to MQTT and the dashboard consumes them through server-sent events at `/api/events/stream`.
- MinIO bucket/object URLs are assumed by the dashboard as `http://<host>:9000/doorbell-images/<key>`.
- Dashboard controls such as Push to Talk, CSV export, settings, and health metrics may be UI scaffolding unless matching backend/firmware support exists.

## Documentation Accuracy Rules

- Do not claim finished two-way audio, real camera capture, measured battery life, or production-ready firmware unless the implementation and measured hardware data support it.
- Mark placeholder demo media, architecture diagrams, enclosure photos, and mocked health/settings data honestly.
- Keep setup commands aligned with the actual layout: Docker Compose is currently at repo root; firmware source is currently `main/`, not a `firmware/` directory.

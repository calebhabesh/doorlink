# Smart Doorbell Agent Guide

This file is the working guide for AI agents editing this repository. Treat the KiCad project under `pcb/smart-doorbell/` as the routed PCB source of truth for net assignments. Treat `/home/ethioking/Documents/Documentation/bom-JLCPCB Assembly Order.xls` as the final JLCPCB assembly BOM source when component availability/cost swaps disagree with the preliminary KiCad BOM.

## Development Workflow

- The user may have long-running services open in separate panes. Do not start or stop persistent dev servers unless explicitly asked.
- Do not run `./mvnw spring-boot:run`, `npm run dev`, or `docker compose up` by default.
- For backend verification, use the Maven wrapper from `gateway/`: `./mvnw compile` or `./mvnw test`.
- For frontend verification, use `npx tsc --noEmit` or `npm run lint` from `dashboard/`.
- For firmware verification, use ESP-IDF tooling when available and target `esp32s3`.
- If a command fails with `EADDRINUSE`, assume the corresponding user-managed service may already be running.
- Ask the user to restart server panes only after changes to `pom.xml`, `tailwind.config.ts`, `next.config.mjs`, firmware `sdkconfig`, or other startup-only configuration.

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

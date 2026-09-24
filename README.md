# Doorlink

Doorlink is a self-hosted smart doorbell built around a custom ESP32-S3 board and a Raspberry Pi. A button press wakes the board, captures a still image, and sends the event to a local gateway. The gateway stores the media, updates the web dashboard, and sends a notification. Visitor recordings and stored, turn-based voice replies are also supported.

<p align="center">
  <img src="docs/media/finished-enclosure.jpg" alt="Assembled Doorlink prototype in its black enclosure" width="420">
</p>

## Prototype status

The assembled Rev C board has completed battery-powered button wake, QXGA image capture, Wi-Fi upload, dashboard display, notification, and return to deep sleep. The microphone, speaker, and stored voice-reply path have been exercised on hardware. The latest session timing and hardened firmware still need a final flashed hardware retest; battery life and closed-enclosure current have not been measured. This is a working prototype, not a qualified outdoor product.

## How it fits together

```mermaid
flowchart LR
    A[Doorbell PCB<br/>ESP32-S3 · camera · audio] -->|Wi-Fi| B[Raspberry Pi gateway<br/>Spring Boot]
    B --> C[(PostgreSQL<br/>events)]
    B --> D[(MinIO<br/>images and audio)]
    B --> E[Next.js dashboard]
    B --> F[ntfy notifications]
    E -->|Stored voice replies| B
```

The firmware captures the image before joining Wi-Fi, then powers the camera down. Audio is turn-based: the visitor microphone and doorbell speaker do not run as a full-duplex call. The Raspberry Pi also runs Mosquitto for device commands.

## Hardware gallery

| KiCad Rev C board | Rev C schematic |
| --- | --- |
| [![KiCad 3D view of the Rev C PCB](docs/media/pcb-3d-view.png)](docs/media/pcb-3d-view.png) | [![Rev C electrical schematic](docs/media/schematic.png)](docs/media/schematic.png) |

| Bench bring-up | Populated PCB | Enclosure wiring |
| --- | --- | --- |
| ![Board and peripherals during bench testing](docs/media/bench-bringup.jpg) | ![Rev C PCB fitted in the open enclosure](docs/media/assembled-pcb.jpg) | ![Doorbell board, battery, button, and speaker during enclosure assembly](docs/media/wired-interior.jpg) |

The board uses an ESP32-S3-WROOM-1-N16R8, OV5640 camera, ICS-43434 microphone, MAX98357A amplifier, MCP73871 charger, and TPS63802 3.3 V regulator. The [PCB overview](pcb/README.md) links the KiCad source and exact Rev C manufacturing files.

## Explore the project

| Area | Contents |
| --- | --- |
| [`main/`](main/) | ESP-IDF firmware and routed board pin map |
| [`gateway/`](gateway/) | Spring Boot event API, persistence, media storage, and notifications |
| [`dashboard/`](dashboard/) | Next.js event and intercom interface |
| [`pcb/`](pcb/) | KiCad project and manufacturing records |
| [`docs/parts.md`](docs/parts.md) | Main unit parts and CAD cost |
| [`docs/firmware-architecture.md`](docs/firmware-architecture.md) | Firmware flow and validation status |
| [`docs/hardware-bringup.md`](docs/hardware-bringup.md) | Detailed Rev C hardware test record |

## Run locally

Install Docker Compose, Java 21, Node.js/npm, and the dashboard dependencies (`cd dashboard && npm ci`). From the repository root, run `./scripts/run-dev.sh` to start the isolated development stack. The gateway uses port 8081 and the dashboard uses [localhost:3001](http://localhost:3001). On first run, enter the setup token printed by the launcher. Ctrl+C stops the stack. The local credentials in `.env.dev` are for development only.

Firmware builds require ESP-IDF. Copy `main/config.example.h` to the ignored `main/config.h`, set your Wi-Fi and gateway values, then see [firmware architecture](docs/firmware-architecture.md) for the safe and production build commands. Raspberry Pi deployment is documented in [docs/pi-deployment.md](docs/pi-deployment.md).

## License

Software and written documentation: [MIT](LICENSE). KiCad hardware designs and manufacturing files under `pcb/`: [CERN-OHL-P-2.0](pcb/LICENSE). [Project photographs](docs/media/README.md) are © 2026 Caleb Habesh, all rights reserved.

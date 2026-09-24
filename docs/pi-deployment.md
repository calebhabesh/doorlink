# Raspberry Pi Deployment Guide

This document outlines the deployment topology for the Smart Doorbell system on the local Raspberry Pi 4 gateway.

## Network Topology

- **Host:** Raspberry Pi 4 (4GB)
- **Local IP Address:** assign the Pi a stable LAN address; examples below use `192.168.1.10`.

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
The database, message broker, and media storage are containerized. On the Pi, systemd runs `smart-doorbell-infra.service` at boot to execute `docker compose up -d` from the repository root before the gateway starts.

Manual command:

```bash
cd /home/ethioprince/dev/smart-doorbell
docker compose up -d
```

### 2. Build Production Artifacts On Arch

The Raspberry Pi systemd services are production runtime services. They must not run `./mvnw spring-boot:run` or `npm run dev`; those commands keep build/watch tooling alive and can cause large CPU spikes on the Pi.

Build the runtime artifacts in the Arch development checkout. The gateway JAR is
architecture-neutral. The dashboard is built as a traced standalone server
through Docker Buildx. Compilation runs natively on the fast Arch CPU, and the
script refuses to deploy the result if tracing ever introduces an x86 runtime
binary. The current standalone runtime is portable JavaScript and assets.

```bash
cd /home/ethioking/dev/smart-doorbell
./scripts/build-pi-production.sh
```

This creates:

- `dist/pi-production/gateway/gateway.jar`
- `dist/pi-production/dashboard/server.js` and its traced standalone runtime

Native compilation and Docker layer caching make later dashboard builds
substantially faster. The Pi does not need Maven, npm dependency installation,
TypeScript compilation, or a Next.js production build.

Build and deploy both components without restarting them:

```bash
./scripts/build-pi-production.sh --deploy
```

For the quickest edit/build/deploy/restart loop, target only the changed part:

```bash
./scripts/build-pi-production.sh --dashboard-only --restart
./scripts/build-pi-production.sh --gateway-only --restart
```

`--restart` is the only option that restarts Pi services. The default command
only builds local artifacts. The host defaults to the `rpi` SSH alias; override
it with `DOORBELL_PI_HOST` if needed. Releases are installed under
`.pi-production/<component>/releases/`, and an atomic `current` symlink keeps
each systemd service on a complete artifact set. Unchanged files in consecutive
releases are hard-linked to reduce transfer time and disk use.

The first time this workflow is installed, pull the tracked changes on the Pi,
copy the gateway and dashboard units as described below, and run
`sudo systemctl daemon-reload`. The build script checks the installed unit paths
before it permits `--restart`, so an old unit cannot silently launch a stale
artifact.

### 3. Install Or Refresh Systemd Units

After changing files under `scripts/systemd/`, copy them into systemd and reload:

```bash
cd /home/ethioprince/dev/smart-doorbell
sudo cp scripts/systemd/smart-doorbell-gateway.service /etc/systemd/system/
sudo cp scripts/systemd/smart-doorbell-dashboard.service /etc/systemd/system/
sudo cp scripts/systemd/smart-doorbell-infra.service /etc/systemd/system/
sudo systemctl daemon-reload
sudo systemctl enable smart-doorbell-infra.service
```

### 4. Infrastructure Service
The Docker Compose bootstrap is managed by `smart-doorbell-infra.service`.
```bash
sudo systemctl status smart-doorbell-infra.service
sudo systemctl restart smart-doorbell-infra.service
journalctl -u smart-doorbell-infra.service -f
```

### 5. Backend Gateway
The Spring Boot gateway is managed by `smart-doorbell-gateway.service`.
```bash
sudo systemctl status smart-doorbell-gateway.service
sudo systemctl restart smart-doorbell-gateway.service
journalctl -u smart-doorbell-gateway.service -f
```

### 6. Frontend Dashboard
The Next.js dashboard is managed by `smart-doorbell-dashboard.service`.
```bash
sudo systemctl status smart-doorbell-dashboard.service
sudo systemctl restart smart-doorbell-dashboard.service
journalctl -u smart-doorbell-dashboard.service -f
```

Application builds no longer run on the Pi. Pull source changes only when the
tracked deployment configuration changed. Normal application iteration uses the
Arch build command with `--restart` and targets only the changed component.

## Emergency CPU Recovery

If SSH or the local TTY is already sluggish because the old dev-mode services are consuming CPU, stop the app services first, then build and restart them with the production units:

```bash
sudo systemctl stop smart-doorbell-dashboard.service smart-doorbell-gateway.service
cd /home/ethioprince/dev/smart-doorbell
git pull
sudo cp scripts/systemd/smart-doorbell-gateway.service /etc/systemd/system/
sudo cp scripts/systemd/smart-doorbell-dashboard.service /etc/systemd/system/
sudo cp scripts/systemd/smart-doorbell-infra.service /etc/systemd/system/
sudo systemctl daemon-reload
sudo systemctl enable smart-doorbell-infra.service
sudo systemctl restart smart-doorbell-infra.service
```

Then return to Arch and deploy prebuilt artifacts with
`./scripts/build-pi-production.sh --restart`.

If CPU spikes persist after this, capture evidence while the spike is happening:

```bash
cd /home/ethioprince/dev/smart-doorbell
./scripts/pi-cpu-snapshot.sh
```

The script prints the path to a log under `/tmp/smart-doorbell-diagnostics/` with process CPU usage, memory pressure, Docker stats, and recent service logs.

## Gateway environment

Before deploying a gateway build from the public configuration, set these values in the Pi's ignored `.env` file. The checked-in defaults use localhost and leave Home Assistant chimes disabled.

```dotenv
INTERCOM_DEVICE_BASE_URL=http://192.168.1.10:8080
DASHBOARD_URL=https://doorbell.example.com
CHIME_PROVIDER=homeassistant
CHIME_HOMEASSISTANT_WEBHOOK_URL=http://192.168.1.10:8123/api/webhook/your-private-id
```

Replace the addresses and private webhook ID with your own. If Home Assistant is not used, set `CHIME_PROVIDER=none` and omit its webhook URL. Keep the webhook ID out of git.

## Firmware Configuration (`config.h`)

For the ESP32-S3 firmware to successfully communicate with this Pi, ensure your `main/config.h` reflects the following:

```c
#define WIFI_SSID "Your_WiFi_SSID"
#define WIFI_PASSWORD "Your_WiFi_Password"

// Gateway API for image/audio HTTP POSTs
#define GATEWAY_API_URL "http://192.168.1.10:8080/api/events"
#define GATEWAY_API_KEY "the-same-high-entropy-value-as-the-Pi-environment"

// MQTT Broker for half-duplex interaction
#define MQTT_BROKER_URI "mqtt://192.168.1.10:1883"
```

`GATEWAY_API_KEY` is also the HMAC key for MQTT intercom commands. The firmware
rejects unsigned commands, changed command fields, and replayed command IDs.
Deploy the matching gateway build before flashing this firmware:

```bash
./scripts/build-pi-production.sh --gateway-only --restart
```

The gateway's `intercom.device-base-url` and firmware `GATEWAY_API_URL` must use
the same scheme, host, and port. Firmware accepts reply media only from the
gateway's `/api/events/media/<key>` route and acknowledgements only on the
matching `/api/system/ptt/messages/<uuid>/delivered` route. Redirects are not
followed when the hardware API key is attached.

## External Access (WAN)

To securely access the Next.js Dashboard and Gateway API from outside the local network without exposing ports to the public internet, we use **Cloudflare Tunnel** (`cloudflared`) plus Doorlink's revocable household-device sessions.

See `docs/cloudflare-tunnel.md` for the Cloudflare Tunnel and Access configuration.

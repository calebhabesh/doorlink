# Frontend development with live Raspberry Pi data

Use the dashboard-only development mode for rapid UI work. It runs Next.js on
the Arch development server with hot reloading while reading current sessions,
media, health, settings, and server-sent events from the Raspberry Pi gateway.

```bash
./scripts/run-dashboard-dev.sh
```

The launcher starts scanning at port 3001 and prints the URL using the first
available port. For example, when LineWatchTO already owns port 3001, the
dashboard normally starts at `http://127.0.0.1:3002`. Changes under
`dashboard/src/` should appear without rebuilding or deploying the production
dashboard.

From another computer, keep the service loopback-only and create an SSH tunnel
using the selected port. For port 3002:

```bash
ssh -L 3002:127.0.0.1:3002 ethioking@192.168.1.20
```

You can then use `http://127.0.0.1:3002` in that computer's browser. For direct
access on a trusted LAN, set `SMART_DOORBELL_DEV_HOST=0.0.0.0`. Do not expose
the raw development port to the internet; place a development hostname behind
an authenticated tunnel such as Cloudflare Access.

The browser only talks to the local Next.js server. Next.js proxies reads to the
Pi gateway at `http://192.168.1.10:8080`, so the browser does not need direct
database, MinIO, API-key, or CORS access. The development proxy rejects POST,
PUT, PATCH, and DELETE requests. Controls such as push-to-talk and settings can
still be styled and exercised in the UI, but they cannot change the Pi system
while this launcher is in use.

To use a different gateway, host binding, or starting port:

```bash
SMART_DOORBELL_DEV_GATEWAY_URL=https://doorbell.example.com \
SMART_DOORBELL_DEV_HOST=127.0.0.1 \
SMART_DOORBELL_DEV_PORT=3002 \
./scripts/run-dashboard-dev.sh
```

If the requested starting port is occupied, the launcher continues scanning
upward until it finds a free TCP port.

Keep `GATEWAY_READ_ONLY=true` when the target is the production Pi. The existing
`./scripts/run-dev.sh` remains the mode for frontend/backend integration work
that needs writable, isolated Postgres, Mosquitto, and MinIO containers.

## Why the Pi gateway is the data boundary

Do not expose PostgreSQL or MinIO from the Pi to the development machine and do
not run the local Spring gateway against the Pi database. Reusing the gateway's
read APIs keeps schema handling, media access, and session serialization in one
place and makes the read-only boundary enforceable at the frontend proxy.

If stable or destructive test scenarios are needed later, take a one-way,
sanitized snapshot of Pi Postgres and MinIO into the isolated local stack rather
than writing tests against production data.

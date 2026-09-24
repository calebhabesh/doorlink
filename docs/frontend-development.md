# Frontend development with live Raspberry Pi data

Use the dashboard-only development mode for rapid UI work. It runs Next.js on
the Arch development server with hot reloading while reading current sessions,
media, health, settings, and server-sent events from the Raspberry Pi gateway.

```bash
SMART_DOORBELL_DEV_GATEWAY_URL=http://192.168.1.10:8080 ./scripts/run-dashboard-dev.sh
```

Replace the example address with your Raspberry Pi gateway's LAN address.

The launcher starts scanning at port 3001 and prints the URL using the first
available port. For example, when another service already owns port 3001, the
dashboard normally starts at `http://localhost:3002`. Changes under
`dashboard/src/` should appear without rebuilding or deploying the production
dashboard.

From another computer, keep the service loopback-only and create an SSH tunnel
using the selected port. For port 3002:

```bash
ssh -L 3002:127.0.0.1:3002 user@dev-machine.local
```

You can then use `http://localhost:3002` in that computer's browser. For direct
access on a trusted LAN, set `SMART_DOORBELL_DEV_HOST=0.0.0.0`. Do not expose
the raw development port to the internet; place a development hostname behind
an authenticated tunnel such as Cloudflare Access.

The browser only talks to the local Next.js server. Next.js proxies reads to the
Pi gateway set in `SMART_DOORBELL_DEV_GATEWAY_URL`, so the browser does not need direct
database, MinIO, API-key, or CORS access. The development proxy blocks writes
to event, device, and settings data. Its one exception is the token-backed
device enrollment request, which creates a revocable browser session. Controls
such as push-to-talk and settings can still be styled in the UI, but cannot
change the Pi system while this launcher is in use.

## Enroll the local development browser

The local URL is a different browser origin from the deployed dashboard. Its
browser profile needs its own household device session to read private gateway
data.

1. Start `./scripts/run-dashboard-dev.sh` and note the URL it prints, including
   its selected port.
2. In an already-enrolled **owner** browser, open the deployed Doorlink
   **Household** page. Click **New device link** beside the owner account and
   copy the resulting single-use URL. Keep the token private.
3. Open the local URL from step 1 in the browser profile used for UI
   development. On the Access page, paste the entire link and click
   **Open enrollment here**. The app opens the enrollment path on `localhost`.
4. Name the browser, such as `Arch frontend dev`, and submit the enrollment
   form. The local origin now has its own revocable household session. Return
   to the printed local URL to view live data with hot reload.

The owner can later revoke this development browser from **Household**. If the
browser still returns to the access page after enrollment, check that the URL
uses the same hostname and browser profile as the enrollment page; cookies are
scoped to a hostname. Browsers that reject `Secure` cookies on a loopback HTTP
origin need an HTTPS local origin for this workflow.

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

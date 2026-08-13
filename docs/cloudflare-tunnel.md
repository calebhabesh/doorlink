# Cloudflare Tunnel and household device access

Doorlink uses Cloudflare Tunnel for private origin connectivity and its own
revocable household-device sessions for browser authentication. Cloudflare
Access email OTP must not sit in front of the hostname: its one-month maximum
session would reintroduce the login prompt before Doorlink can see the device
cookie.

## Traffic path

`doorbell.example.com` → Cloudflare Tunnel → Next.js on `localhost:3000` →
Spring gateway on `127.0.0.1:8080`

No router port is opened. Cloudflare still provides DNS proxying, TLS, Tunnel,
and normal edge/WAF protections. Doorlink authenticates every dashboard page,
API read/write, event stream, and media response.

## 1. Keep or create the tunnel

In **Cloudflare Zero Trust → Networks → Tunnels**, keep the `doorbell-pi`
connector and its public hostname:

- Hostname: `doorbell.example.com`
- Service: `HTTP`
- URL: `localhost:3000`

If the connector is not installed yet, use the Cloudflare-provided ARM64
installation command on the Raspberry Pi. Never commit its tunnel token.

## 2. Remove the interactive Access challenge

In **Zero Trust → Access → Applications**, remove the existing Smart Doorbell
self-hosted application, or add a highest-priority **Bypass** policy for this
exact hostname. Confirm in a private browser that Cloudflare no longer displays
an email/OTP page and Doorlink displays its own “device needs an invitation”
screen instead.

Do not point the tunnel directly at port 8080. Only the Next.js service on port
3000 should be public through the tunnel.

## 3. Configure the one-time owner bootstrap secret

On a trusted machine, generate a secret:

```bash
openssl rand -base64 32
```

Set it as `HOUSEHOLD_BOOTSTRAP_TOKEN` in the Pi checkout's ignored `.env` file,
then deploy/restart both gateway and dashboard. Visit:

```text
https://doorbell.example.com/setup
```

Enter the secret, owner details, and a recognizable device name. After setup,
remove `HOUSEHOLD_BOOTSTRAP_TOKEN` from `.env`; the bootstrap endpoint is also
permanently disabled by the stored owner record.

For isolated local development, provide the same variable when starting the
stack, for example `HOUSEHOLD_BOOTSTRAP_TOKEN=... ./scripts/run-dev.sh`.

The gateway stores only SHA-256 hashes of session and enrollment tokens.
Browser cookies are Secure, HttpOnly, SameSite=Lax, and renewed while a device
is used. The browser's practical 400-day cookie cap is handled by renewal, so an
active device remains signed in without a monthly prompt.

## 4. Add household devices

Open **Household** in Doorlink:

1. Add a person's name and email.
2. Copy the generated single-use enrollment link and send it privately.
3. They open the link on the exact phone/browser they want to enroll and give
   that device a name.
4. Repeat **New device link** for another browser or phone.

Enrollment links expire after 72 hours by default. They contain a short-lived
secret, so do not post them publicly. ntfy action links contain no bearer token;
they open the ordinary dashboard URL, where the device cookie authenticates the
browser.

The owner can revoke one device or remove a member (which revokes every device).
Revoked devices immediately lose new API and media access, any open event stream
is closed by the next 20-second heartbeat, and the browser is redirected to the
enrollment screen on its next page request.

## ntfy membership

Doorlink's current `ntfy.sh` topic is still a shared, hard-to-guess topic. Device
revocation in Doorlink does not remotely unsubscribe the ntfy mobile app. If
notification subscription must be revoked per person too, use authenticated
ntfy users/topic ACLs (hosted or self-hosted) and manage those subscriptions
separately. Never put a permanent Doorlink session token in an ntfy URL.

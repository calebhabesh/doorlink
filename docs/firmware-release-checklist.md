# Firmware release checklist

This is the final pre-enclosure acceptance gate for the Rev C production
firmware. Passing the software checks makes the image a release candidate; it
does not replace current, temperature, supply-droop, or battery-life
measurements on the assembled device.

## Verified in the development checkout

- Fragmented MQTT messages are accepted only when every offset and total is
  contiguous, consistent, and leaves room for the terminator.
- Gateway commands use protocol `v1`, a unique UUID, and HMAC-SHA256 over every
  field consumed by firmware. Altered, unsigned, and recently replayed commands
  are rejected.
- Reply media and acknowledgement URLs must use the exact scheme, host, and
  port from `GATEWAY_API_URL`, plus the expected API path shape. Query strings,
  fragments, escapes, traversal, user-info, and redirects are rejected.
- One chime claim has a 15-second wall limit and a 1.5-second no-progress limit.
  Microphone capture, HTTP playback, stage retries, and the greeting task join
  use finite deadlines. A missed task join proceeds to safe sleep rather than
  returning with a live stack reference.
- `CAM_PWR_EN`, `AMP_EN`, I2S WS/SCK, speaker data, and microphone data are
  latched in their safe states through deep sleep and released glitch-free on
  wake.
- Production samples the populated GPIO1 battery divider before high-current
  peripherals start and reports calibrated millivolts on both the early event
  trigger and media upload. Dashboard percentage is an estimate; it is not a
  measured state of charge or a low-battery cutoff.

Run the reproducible software gates:

```bash
./scripts/test-firmware-guards.sh

cd gateway
./mvnw test

cd ..
source /home/ethioking/.espressif/v6.0.1/esp-idf/export.sh
idf.py -B build-production -D SDKCONFIG=sdkconfig.production \
  -D 'SDKCONFIG_DEFAULTS=sdkconfig.defaults;sdkconfig.production.defaults' build
```

## Rollout order

The hardened firmware deliberately rejects commands from an older gateway.

1. Confirm the Pi and `main/config.h` use the same high-entropy
   `GATEWAY_API_KEY`.
2. Deploy and restart the gateway first:
   `./scripts/build-pi-production.sh --gateway-only --restart`.
3. Flash the production firmware and retain USB access for the first smoke run.
4. Confirm a dashboard PTT start/cancel/reply round trip. The device log must
   show the MQTT subscription and normal playback, with no
   `unauthenticated intercom command` message for gateway-generated commands.

## Required physical acceptance before sealing

1. On USB power, run one complete press, QXGA capture, image upload, optional
   held-button recording, reply playback, session expiry, and return to sleep.
2. Repeat a complete cycle on battery only with the final camera, 4-ohm
   speaker, button, PIR harness state, and enclosure cable routing fitted.
3. During the battery cycle, check 3.3 V droop/reset behavior, image integrity,
   amplifier and regulator temperature, and camera shutdown. Exercise at least
   one repress during the bounded camera-overlap chime.
4. Confirm System Health shows a plausible battery voltage from that event and
   compare it with TP4 using a multimeter. Record the pair; the earlier 4.028 V
   firmware / 4.05 V meter result is only one calibration point.
5. Measure awake and deep-sleep current. Battery life remains unqualified until
   those values are recorded; do not infer it from the earlier one-cycle test.
6. Verify `CAM_PWR_EN` and `AMP_EN` remain low in deep sleep. If practical,
   confirm I2S WS/SCK are also low rather than floating.
7. Power-cycle twice from battery and confirm GPIO2 wakes once per press, the
   held-input guard does not create a wake loop, and every cycle returns to
   deep sleep.

Record the measurements in `docs/hardware-bringup.md`. Only then treat this
firmware and the assembled battery unit as production-signed-off.

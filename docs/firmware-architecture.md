# Firmware architecture

The firmware uses controlled C++ for event orchestration while retaining C for
the proven hardware and ESP-IDF-facing layers. Exceptions and RTTI are disabled.
The default build remains the core-only safe image; production mode is an
explicit, isolated configuration.

## Runtime path

`app_main.cpp` selects either a diagnostic image or `DoorbellController`.
Production mode follows this bounded session sequence:

1. `BOOTING`: reapply the retained camera/audio-safe GPIO state, release the
   sleep holds without a pad glitch, and classify the wake.
2. `LOCAL_CHIME`: start the bounded low-power camera-overlap chime profile. One
   playback claim, including rapid repress retriggers, has a 15-second wall
   limit and a 1.5-second zero-progress I2S limit.
3. `CAPTURING`: with RF off, enable GPIO42/U9, initialize the OV5640, capture QXGA, copy the
   JPEG to owned PSRAM, return the frame, deinitialize the driver, and disable
   U9.
4. `CONNECTING` / `TRIGGERING`: connect Wi-Fi after shutter close and post the
   authenticated session/press trigger.
5. `UPLOADING`: independently POST available JPEG and visitor WAV media under
   stable per-press identifiers. All retries share one stage wall deadline.
6. `VISITOR_RECORDING`: if GPIO2 remains held after the first chime, or on any
   later press, retain 1-15 seconds of release-driven microphone audio.
7. `PTT_SESSION`: accept only HMAC-authenticated MQTT commands for the active
   random session, reject replay IDs, and queue homeowner playback behind any
   active visitor capture. Media and acknowledgement URLs must match the exact
   configured gateway origin and route; redirects are disabled.
8. `SHUTTING_DOWN`: explicitly close the gateway session, release media, stop
   Wi-Fi, drive the load gates and I2S pads safe, latch those states, configure
   wake sources, and enter deep sleep.

Camera and Wi-Fi are deliberately non-overlapping high-load phases. The camera
frame never survives camera-driver shutdown; only the owned PSRAM copy does.
The event ID also makes a retry after a lost HTTP response idempotent at the
gateway and gives media a stable object key.

## Ownership and boundaries

| Layer | Language | Responsibility |
| --- | --- | --- |
| `DoorbellController` | C++ | State machine, retry policy, event type, shutdown orchestration |
| `CameraService` / `CapturedImage` | C++ | RAII ownership of the PSRAM JPEG |
| `ConnectivityManager` | C++ | Bounded Wi-Fi/upload lifecycle and shutdown guard |
| `IntercomService` / `intercom_protocol` | C++ | Signed command validation, fragment assembly, URL policy, and bounded playback |
| `AudioService` / `chime_player` | C++ / C | Deadline-bound I2S ownership, capture, playback, and fail-safe teardown |
| `PowerManager` | C++ | Wake classification, stuck-input handling, and deep-sleep entry |
| `camera_capture` | C | OV5640 configuration and guaranteed frame/driver/power cleanup |
| `camera_power`, `ov5640_mode_fix`, bring-up modules | C | Auditable hardware sequencing and diagnostics |

There is no inheritance hierarchy, exception path, or RTTI dependency.
Follow-up turn metadata is bounded heap state and visitor WAV/JPEG payloads use
PSRAM where available. ESP-IDF errors remain `esp_err_t`.

## Wake behavior

- GPIO2 `DOORBELL_IN`: EXT0, active low, routed 10 kOhm pull-up and debounce
  capacitor.
- GPIO3 `PIR_WAKE`: EXT1, active high, routed 100 kOhm pulldown through the
  populated Rev C R30 path.
- A held button or still-high PIR is excluded from that sleep cycle to prevent
  a rapid wake loop. A 30-second timer is enabled so the input can be checked
  again.
- Cold boot, timer-recovery wake, and unknown wake causes enter sleep without
  taking a picture.
- GPIO48 uses a non-blocking LEDC fade on timer/channel 1. Camera XCLK remains
  on timer/channel 0.
- Before deep sleep, GPIO42, GPIO44, and GPIO4-7 are individually held in their
  safe state. ESP32-S3's global digital deep-sleep hold is enabled for the two
  non-RTC load gates. Wake code programs the same safe state before releasing
  the latches.

## Builds

Safe core-only build:

```bash
idf.py build
```

Production-controller build, isolated from the safe `sdkconfig`:

```bash
idf.py -B build-production -D SDKCONFIG=sdkconfig.production \
  -D 'SDKCONFIG_DEFAULTS=sdkconfig.defaults;config/firmware/sdkconfig.production.defaults' build
```

Both configurations build with ESP-IDF 6.0.1. The hardened production image is
approximately 1.56 MiB and leaves about 48% of the 3 MiB application partition
free.

## Validation status

Implemented and independently proven hardware operations include QXGA capture,
camera shutdown, battery-only Wi-Fi upload, gateway persistence, dashboard
display, notifications, and isolated GPIO2 button and GPIO3 PIR wakes followed
by return to deep sleep. An RTC-fast wake stub gives immediate visual acknowledgement on D3
before the bootloader; GPIO48 begins its nonblocking ring animation as soon as
the application starts. Cold boots still validate the
application image. The pre-fast-trigger production controller has completed QXGA capture, HTTP
upload, cleanup, and return to deep sleep from both button and PIR triggers on
hardware; notification delivery and Doorlink display were observed for both
event types. The fast-trigger/idempotent-upload flow was USB-validated on Rev C
on 2026-08-05. Corrected PIR and button events received HTTP `200` for the early
trigger in approximately 1.2-1.6 seconds, fully stopped Wi-Fi before camera
startup, completed isolated QXGA capture, uploaded under the same event ID, and
returned to deep sleep. The gateway stored one event per ID and did not invoke
the chime again during upload. Production now leaves PIR event wake disabled
unless `CONFIG_SMART_DOORBELL_ENABLE_PIR_EVENTS` is explicitly selected. The
isolated GPIO48 diagnostic and production path both validated an off-to-full
1.8-second fade, 2.5-second hold, and 1.8-second fade-out without delaying Wi-Fi
or camera work. Camera
convergence remains the dominant image-delivery delay.
Deep-sleep current remains unmeasured. The earlier fixed-duration visitor audio
and stored PTT playback path passed hardware tests; release-driven capture and
multi-press arbitration are implemented and build-tested but still require an
instrumented board validation pass.

The 2026-08-23 pre-enclosure hardening pass added strict fragmented-MQTT
assembly, HMAC-SHA256 command authentication and replay rejection, gateway-only
URL policy, redirect suppression, shared wall deadlines for retries, bounded
I2S stall handling, a bounded greeting-task join with fail-safe sleep, and
deep-sleep GPIO retention. The host guard tests, all 42 gateway tests, and a
clean ESP-IDF 6.0.1 production build pass. This revision has not yet been
flashed or battery-cycle tested; see `firmware-release-checklist.md`.

The production controller also takes a calibrated 16-sample +BATT reading from
GPIO1 before starting the speaker, camera, or radio. The gateway retains the
millivolt value and sample time; System Health exposes those authoritative
fields plus a coarse, explicitly estimated one-cell Li-ion percentage. It does
not infer charger state, thermistor temperature, remaining runtime, or a
low-battery cutoff.

The current production flow supersedes the USB-validated trigger-first order:
the first QXGA frame is captured before Wi-Fi starts, then its owned JPEG is
uploaded before the controller waits for a held-button recording. Repress
lifecycle work may remain ordered for persistence, but only the press reserved
for immediate controller handling can dispatch remote alerts; all others are
registered with alerts permanently suppressed. Repress local acknowledgements
remain opportunistic and are never queued behind microphone or homeowner audio.
This revision is compiler- and gateway-test-verified and still needs
flashed-board timing validation. EXT0 is re-armed only after session close,
cleanup, and the existing button-release guard.

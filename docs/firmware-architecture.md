# Firmware architecture

The firmware uses controlled C++ for event orchestration while retaining C for
the proven hardware and ESP-IDF-facing layers. Exceptions and RTTI are disabled.
The default build remains the core-only safe image; production mode is an
explicit, isolated configuration.

## Runtime path

`app_main.cpp` selects either a diagnostic image or `DoorbellController`.
Production mode follows this bounded state sequence:

1. `BOOTING`: apply the camera/audio-safe GPIO state and classify the wake.
2. `CONNECTING` / `TRIGGERING`: connect Wi-Fi immediately and POST a small,
   authenticated, idempotent alert using a device-generated event ID.
3. `DISCONNECTING`: fully release Wi-Fi and network resources, restoring the
   proven RF-off camera boundary.
4. `CAPTURING`: enable GPIO42/U9, initialize the OV5640, capture QXGA, copy the
   JPEG to owned PSRAM, return the frame, deinitialize the driver, and disable
   U9.
5. `CONNECTING`: initialize network resources again and reconnect Wi-Fi with a
   bounded retry count and timeout.
6. `UPLOADING`: POST the JPEG and the same event ID as `DOORBELL_PRESS` or
   `PIR_MOTION`, with one bounded upload retry. The gateway suppresses a second
   alert when the early receipt exists and falls back to alerting here when it
   does not.
7. `SHUTTING_DOWN`: release the JPEG, stop and deinitialize Wi-Fi, hold camera
   and I2S pins safe, configure wake sources, and enter deep sleep.

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
| `PowerManager` | C++ | Wake classification, stuck-input handling, and deep-sleep entry |
| `camera_capture` | C | OV5640 configuration and guaranteed frame/driver/power cleanup |
| `camera_power`, `ov5640_mode_fix`, bring-up modules | C | Auditable hardware sequencing and diagnostics |

There is no inheritance hierarchy, exception path, RTTI dependency, or C++ heap
allocation in the controller. ESP-IDF errors remain `esp_err_t`.

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
- The isolated wake diagnostic prototypes a non-blocking GPIO48 fade on LEDC
  timer/channel 1. Camera XCLK remains on timer/channel 0; production does not
  enable the fade until the diagnostic is validated on hardware.

## Builds

Safe core-only build:

```bash
idf.py build
```

Production-controller build, isolated from the safe `sdkconfig`:

```bash
idf.py -B build-production -D SDKCONFIG=sdkconfig.production \
  -D 'SDKCONFIG_DEFAULTS=sdkconfig.defaults;sdkconfig.production.defaults' build
```

Both configurations build with ESP-IDF 6.0.1. The safe image is approximately
205 KiB. The production image is approximately 1004 KiB and leaves about 67% of
the 3 MiB application partition free.

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
Deep-sleep current remains unmeasured. Audio remains outside the production
path.

Production has a single-event concurrency policy. EXT0 is re-armed only after
capture, upload, cleanup, and a stable 100 ms button release. Button edges while
that event is in flight are coalesced rather than queued, preventing rapid
repeat chimes. A press after deep-sleep re-arm creates a fresh event ID and was
validated in the sequential two-press hardware test.

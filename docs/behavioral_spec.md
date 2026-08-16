# Smart Doorbell Production Interaction Contract

This is the locked production behavior for the Rev C door-mounted device. It
defines product behavior; hardware-dependent timing and audio quality still
require validation on the assembled board after firmware changes.

## Non-negotiable hardware rules

- `PWR-01`: Wi-Fi RF and `CAM_PWR_EN`/GPIO42 are never active together.
- `PWR-02`: Camera rails and `AMP_EN`/GPIO44 are disabled immediately after use.
- `MEM-01`: the camera frame is copied to owned PSRAM before its driver buffer is
  returned and U9 is disabled.
- `AUDIO-01`: local chime, visitor microphone, and homeowner reply are
  half-duplex users of the shared I2S bus.
- `SLEEP-01`: a button still held at shutdown is handled by the GPIO2
  wake-on-release guard; it must not cause a wake loop or another visitor event.

## Visitor session

The first debounced button press creates a random 32-character `sessionId` and
press ID `<sessionId>-1`. More button-down edges before session expiry create
ordered press IDs `<sessionId>-2`, `<sessionId>-3`, and so on.

The session ends at the earlier of:

- 60 seconds since the last visitor/PTT activity; or
- 90 seconds since the first press.

Firmware explicitly closes the session before sleep when the gateway is
reachable. The gateway and dashboard infer closure from the same 60/90-second
limits if that close request is lost.

Only the first press dispatches the indoor/Home Assistant chime and primary
push notification. All later press triggers update the same persisted session.

## First press

1. GPIO2 must remain stable for 20 ms.
2. Start the ring LED and play the local chime once, from start to finish.
3. Start the authenticated early gateway trigger while the chime plays.
4. If the visitor is still holding when the complete chime releases I2S, start
   the microphone immediately and record until release or the 15-second cap.
5. Quiesce RF, capture the initial QXGA snapshot, return the camera frame,
   disable U9, reconnect RF, and upload available media.
6. Open the bounded homeowner reply window.

A visitor recording is retained only when it contains at least 1.0 second of
microphone audio. Releasing during the first chime, or before one second of
post-chime microphone audio, is a normal short press and creates no empty WAV.

## Later presses

- Restart LED acknowledgement on every valid down-edge.
- Start an onboard acknowledgement chime when I2S is idle. Never queue a
  delayed chime behind visitor microphone or homeowner playback. If a local
  chime is already audible, rewind its live stream at the next PCM chunk
  boundary so every valid physical down-edge gets prompt audible feedback.
- Treat repress chimes as interruptible: a visitor still holding after 400 ms
  stops the repress chime cleanly and receives the microphone. A tap may let
  its acknowledgement chime finish.
- Retain visitor microphone audio only after the one-second recording threshold
  is crossed. Release creates one logical recording.
- A release followed by another hold creates another ordered press/recording.
- Refresh the snapshot only when the previous capture is at least 15 seconds
  old. Otherwise register the press and upload only its visitor recording.
- There is no three-press product limit. The 60/90-second session bounds and
  memory/queue limits are the safety boundary.

Recordings stay in bounded RAM through their upload attempt; firmware does not
write visitor audio to flash.

## Half-duplex arbitration

The first chime and established visitor/homeowner turns are never intentionally
cut short except at the 90-second hard deadline. Repress acknowledgement is the
deliberately interruptible exception:

1. The first local chime completes.
2. A repress acknowledgement may play only while I2S is otherwise idle; a
   sustained visitor hold or arriving homeowner WAV interrupts it cleanly.
3. An active visitor recording completes on release or at 15 seconds.
4. A homeowner reply received during visitor capture is queued.
5. The microphone is off while the queued homeowner WAV plays through the
   speaker.
6. An already-playing homeowner reply completes; a new visitor hold waits for
   I2S and records afterward if the visitor is still holding.
7. Session cleanup overrides queued work at the hard deadline.

Dashboard PTT records a stored 16 kHz mono PCM WAV; it is not a live phone call.
The gateway reports `ARMING` when it publishes the device command and reports
reply delivery only after the ESP32 playback acknowledgement.

## Speaker and camera power arbitration

The production speaker paths apply 6 dB of digital attenuation by default to
support the 4-ohm enclosure speaker without demanding the full-scale current
used during the failed integrated test. Local chimes and homeowner replies both
use the same setting and a 25 ms linear fade-in/fade-out envelope. These values
are build-time configurable through
`CONFIG_SMART_DOORBELL_SPEAKER_ATTENUATION_DB` and
`CONFIG_SMART_DOORBELL_SPEAKER_RAMP_MS`.

The early remote notification remains ahead of camera capture. After it is
sent, firmware atomically suppresses new chime starts/retriggers, allows an
already-playing first chime to finish, and verifies that the amplifier is muted
before enabling the camera power domain. Speaker playback stays blocked until
capture has returned and the camera rails are off. Exhausting the session
deadline while draining the speaker aborts capture instead of overlapping the
two loads.

## Persisted and UI model

```text
Visitor session
├── Press 1
│   ├── optional snapshot
│   └── zero or more visitor recordings
├── Press 2
│   └── zero or more visitor recordings
└── homeowner reply turns
```

`sessionId`, `pressId`, `recordingId`, and reply `messageId` are stable retry
keys. The gateway emits lifecycle SSE updates (`session-started`,
`press-started`, `press-ended`, `session-updated`, and `session-closed`). Active
Event, Event Log, and Calendar render one grouped session, with an ordered
conversation timeline instead of one card per upload.

Legacy event rows remain readable during rollout; rows with the historical
`<sessionId>-<pressNumber>` pattern are grouped into closed sessions.

## Production constants

| Constant | Value |
| --- | ---: |
| GPIO2 stable debounce | 20 ms |
| Minimum retained visitor audio | 1,000 ms |
| Maximum visitor recording | 15,000 ms |
| Repress chime-to-microphone handoff | 400 ms |
| Production speaker attenuation | 6 dB |
| Speaker fade-in/fade-out | 25 ms |
| Stale-snapshot threshold | 15,000 ms |
| Session idle limit | 60,000 ms |
| Session absolute limit | 90,000 ms |
| Browser homeowner reply limit | 20,000 ms |

## Failure behavior

- Wi-Fi failure never prevents the complete first local chime or LED
  acknowledgement.
- The first chime is not stopped on a timer. A physical repress may restart its
  live stream from sample zero; after the final quick press, the restarted
  chime is allowed to complete.
- The complete-chime guarantee applies to the first press. Repress chimes may
  yield to a sustained visitor hold, homeowner WAV, or safe shutdown.
- Camera failure leaves the early session/press alert intact and cuts camera
  power; the dashboard may show snapshot pending.
- A lost trigger or media response is retried with stable identifiers, so it
  does not create another notification or recording.
- Quiet speech is retained; firmware does not use voice-activity detection to
  discard it.
- The 15-second recording cap does not bypass stuck-button wake-on-release
  protection.

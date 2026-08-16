# Doorlink Half-Duplex Intercom Architecture

The fixed-duration visitor WAV and stored dashboard-to-speaker reply path have
passed on assembled Rev C hardware. The release-driven multi-press policy below
is implemented and build-tested; its thresholds and latency still require a
flashed-board validation pass.

Doorlink is a turn-based voicemail relay, not a full-duplex call. The local
chime, ICS-43434 microphone, and MAX98357A speaker share a half-duplex I2S path,
so only one owns it at a time. MQTT transports small control commands and HTTP
transports stored WAV media.

## Visitor flow

```mermaid
sequenceDiagram
    actor Visitor
    participant ESP as ESP32-S3
    participant GW as Spring Gateway
    participant UI as Dashboard
    actor Homeowner

    Visitor->>ESP: First press / hold
    par Local acknowledgement
        ESP->>ESP: Play one complete local chime
    and Early alert
        ESP->>GW: POST session + press trigger
        GW-->>UI: session-started SSE
    end
    ESP->>ESP: If still held, record until release (1-15 s retained)
    ESP->>ESP: RF off, capture snapshot, camera power off
    ESP->>GW: Upload available JPEG/WAV with stable IDs
    GW-->>UI: session-updated SSE

    Visitor->>ESP: Later press / hold
    ESP->>ESP: LED + interruptible local chime if I2S idle
    ESP->>ESP: If held 1,200 ms, stop repress chime and record
    ESP->>GW: Register press and upload WAV

    Homeowner->>UI: Hold PTT, then release
    UI->>GW: Store canonical 16 kHz mono PCM WAV
    GW->>ESP: MQTT PLAY_AUDIO with HTTP URL
    Note over ESP: Queue behind active visitor recording
    ESP->>ESP: Microphone off; enable amp and play complete reply
    ESP->>GW: Playback acknowledgement
    GW-->>UI: session-updated SSE
```

The initial microphone can start only after the complete local chime. A visitor
who releases during that chime leaves no recording. Later presses request an
immediate local acknowledgement only when I2S is idle. A tap may let that chime
finish; a new valid down-edge while a local chime is audible rewinds the live
stream at its next PCM chunk boundary. A hold continuing for 1,200 ms interrupts
interruptible repress audio and hands I2S to the microphone. Repress chimes are
skipped, not delayed, while visitor or homeowner audio owns the bus. A release
followed by another hold is another ordered press and recording in the same
60/90-second session.

## Arbitration

Priority is:

1. Finish the first local chime.
2. Yield an interruptible repress chime to a sustained visitor hold or an
   arriving homeowner WAV.
3. Finish an active visitor turn on release or at 15 seconds.
4. Play a queued homeowner reply completely.
5. If a visitor presses during that playback, finish the active reply and give
   the microphone to a visitor who is still holding.
6. Enforce the 90-second hard session deadline and safe shutdown.

No voice-activity detector discards quiet speech. Recordings shorter than one
second are discarded solely to avoid empty tap/accidental-hold files.

## Control and data planes

Homeowner replies are stored in MinIO before this command is published on
`doorbell/commands/audio`:

```json
{
  "type": "PLAY_AUDIO",
  "eventId": "0123456789abcdef0123456789abcdef",
  "messageId": "6db2995e-4c0e-4717-9b95-8c0330031eb2",
  "audioUrl": "http://gateway:8080/api/events/media/reply.wav",
  "ackUrl": "http://gateway:8080/api/system/ptt/messages/6db2995e-4c0e-4717-9b95-8c0330031eb2/delivered",
  "durationMs": 3200
}
```

`eventId` is the session target. The ESP32 fetches and streams the canonical WAV
over HTTP, disables the amplifier after its final sample, and POSTs `ackUrl`.
The dashboard distinguishes `Playback queued` from `Played at door` using that
acknowledgement. A successful MQTT publish alone is reported as `ARMING`, not as
confirmed device playback.

## Persistence model

- `visitor_sessions`: session bounds, activity, and explicit close state.
- `events`: ordered physical presses and their snapshot/lifecycle state (kept as
  the table name for compatibility).
- `visitor_recordings`: stable recording ID, press relationship, media key, and
  measured WAV duration.
- `intercom_messages`: homeowner WAV, creation time, and playback delivery time.

Active Event, Event Log, and Calendar consume grouped session DTOs. Legacy rows
using numbered press IDs are grouped into closed sessions during migration.

## Why stored half-duplex turns

- It avoids acoustic echo cancellation and intentional chime contamination.
- It keeps radio activity bounded for a battery-powered device.
- Stable files and identifiers can be retried after intermittent Wi-Fi.
- The UI can present a truthful, durable conversation timeline rather than
  claiming unsupported live audio streaming.

package com.smartdoorbell.gateway.controller;

import com.fasterxml.jackson.databind.ObjectMapper;
import com.fasterxml.jackson.datatype.jsr310.JavaTimeModule;
import com.smartdoorbell.gateway.config.MqttGateway;
import com.smartdoorbell.gateway.entity.Event;
import com.smartdoorbell.gateway.entity.VisitorRecording;
import com.smartdoorbell.gateway.entity.VisitorSession;
import com.smartdoorbell.gateway.repository.EventRepository;
import com.smartdoorbell.gateway.repository.VisitorRecordingRepository;
import com.smartdoorbell.gateway.repository.VisitorSessionRepository;
import com.smartdoorbell.gateway.service.DeviceTelemetryService;
import com.smartdoorbell.gateway.service.EventAlertService;
import com.smartdoorbell.gateway.service.MinioService;
import com.smartdoorbell.gateway.service.VisitorSessionService;
import com.smartdoorbell.gateway.service.HouseholdAuthService;
import org.springframework.beans.factory.annotation.Value;
import org.springframework.data.domain.Page;
import org.springframework.data.domain.PageRequest;
import org.springframework.data.domain.Sort;
import org.springframework.http.MediaType;
import org.springframework.http.ResponseEntity;
import org.springframework.integration.annotation.ServiceActivator;
import org.springframework.scheduling.annotation.Scheduled;
import org.springframework.web.bind.annotation.*;
import org.springframework.web.multipart.MultipartFile;
import org.springframework.web.servlet.mvc.method.annotation.SseEmitter;

import java.io.IOException;
import java.time.LocalDateTime;
import java.util.ArrayList;
import java.util.List;
import java.util.Set;
import java.util.concurrent.CopyOnWriteArrayList;
import java.util.regex.Pattern;
import java.util.UUID;

import com.fasterxml.jackson.databind.SerializationFeature;
import jakarta.servlet.http.HttpServletRequest;

@RestController
@RequestMapping("/api/events")
public class EventController {

    private final MinioService minioService;
    private final EventRepository eventRepository;
    private final VisitorSessionRepository sessionRepository;
    private final VisitorRecordingRepository recordingRepository;
    private final MqttGateway mqttGateway;
    private final EventAlertService eventAlertService;
    private final DeviceTelemetryService deviceTelemetryService;
    private final VisitorSessionService visitorSessionService;
    private final ObjectMapper objectMapper;
    private final HouseholdAuthService householdAuthService;
    private final List<AuthenticatedEmitter> emitters = new CopyOnWriteArrayList<>();

    @Value("${mqtt.topic.events:doorbell/events}")
    private String eventsTopic;

    private static final Pattern EVENT_ID_PATTERN =
            Pattern.compile("[A-Za-z0-9_-]{8,64}");
    private static final Pattern NUMBERED_PRESS_PATTERN =
            Pattern.compile("^(.+)-(\\d+)$");
    private static final Pattern RECORDING_ID_PATTERN =
            Pattern.compile("[A-Za-z0-9_-]{8,96}");
    private static final Set<String> TRIGGER_EVENT_TYPES =
            Set.of("DOORBELL_PRESS", "DOORBELL_REPRESS", "PIR_MOTION");

    public EventController(MinioService minioService,
                           EventRepository eventRepository,
                           VisitorSessionRepository sessionRepository,
                           VisitorRecordingRepository recordingRepository,
                           MqttGateway mqttGateway,
                           EventAlertService eventAlertService,
                           DeviceTelemetryService deviceTelemetryService,
                           VisitorSessionService visitorSessionService,
                           HouseholdAuthService householdAuthService) {
        this.minioService = minioService;
        this.eventRepository = eventRepository;
        this.sessionRepository = sessionRepository;
        this.recordingRepository = recordingRepository;
        this.mqttGateway = mqttGateway;
        this.eventAlertService = eventAlertService;
        this.deviceTelemetryService = deviceTelemetryService;
        this.visitorSessionService = visitorSessionService;
        this.householdAuthService = householdAuthService;
        this.objectMapper = new ObjectMapper();
        this.objectMapper.registerModule(new JavaTimeModule());
        this.objectMapper.disable(SerializationFeature.WRITE_DATES_AS_TIMESTAMPS);
    }

    @PostMapping
    public ResponseEntity<String> createEvent(
            @RequestParam(value = "image", required = false) MultipartFile image,
            @RequestParam(value = "audio", required = false) MultipartFile audio,
            @RequestParam(value = "eventType", defaultValue = "DOORBELL_PRESS") String eventType,
            @RequestParam(value = "eventId", required = false) String eventId,
            @RequestParam(value = "sessionId", required = false) String requestedSessionId,
            @RequestParam(value = "pressId", required = false) String requestedPressId,
            @RequestParam(value = "pressNumber", required = false) Integer requestedPressNumber,
            @RequestParam(value = "recordingId", required = false) String requestedRecordingId,
            @RequestParam(value = "deviceId", required = false) String deviceId,
            @RequestParam(value = "firmwareVersion", required = false) String firmwareVersion,
            @RequestParam(value = "wifiRssiDbm", required = false) Integer wifiRssiDbm) {
        eventId = normalize(eventId);
        String pressId = normalize(requestedPressId);
        if (pressId == null) pressId = eventId;
        if (pressId != null && !EVENT_ID_PATTERN.matcher(pressId).matches()) {
            return ResponseEntity.badRequest().body("Invalid eventId");
        }
        String resolvedSessionId = pressId == null ? null
                : sessionId(requestedSessionId, pressId);
        if (resolvedSessionId != null &&
                !EVENT_ID_PATTERN.matcher(resolvedSessionId).matches()) {
            return ResponseEntity.badRequest().body("Invalid sessionId");
        }
        if ((image == null || image.isEmpty()) && (audio == null || audio.isEmpty())) {
            return ResponseEntity.badRequest().body("An image or visitor recording is required");
        }
        if (pressId != null && (audio == null || audio.isEmpty())) {
            var completed = eventRepository.findByEventId(pressId)
                    .filter(existing -> existing.getImageKey() != null &&
                            !Event.PENDING_IMAGE_KEY.equals(existing.getImageKey()));
            if (completed.isPresent()) {
                return ResponseEntity.ok("Event already processed with image key: "
                        + completed.get().getImageKey());
            }
        }

        try {
            LocalDateTime now = LocalDateTime.now();
            VisitorSession session = null;
            Event event;
            if (pressId != null) {
                session = getOrCreateSession(resolvedSessionId, now);
                int pressNumber = pressNumber(requestedPressNumber, pressId);
                var existingPress = eventRepository.findByEventId(pressId);
                if (existingPress.isPresent()) {
                    event = existingPress.get();
                    if (event.getSession() != null) session = event.getSession();
                } else {
                    Event created = new Event(pressId, now, eventType,
                            Event.PENDING_IMAGE_KEY, null);
                    created.assignToSession(session, pressNumber);
                    created.markTriggered();
                    event = eventRepository.save(created);
                }
            } else {
                event = new Event(null, now, eventType, null, null);
            }

            String imageKey = event.getImageKey();
            if (image != null && !image.isEmpty() &&
                    (imageKey == null || Event.PENDING_IMAGE_KEY.equals(imageKey))) {
                imageKey = minioService.uploadFile(image,
                        pressId == null ? null : pressId + "-image");
                event.setImageKey(imageKey);
            }

            String audioKey = event.getAudioKey();
            if (audio != null && !audio.isEmpty()) {
                Integer durationMs = inspectVisitorWav(audio);
                if (durationMs == null || durationMs < 1000 || durationMs > 15000) {
                    return ResponseEntity.badRequest().body(
                            "Visitor recording requires a 1-15 second, 16 kHz mono PCM WAV");
                }
                String recordingId = normalize(requestedRecordingId);
                if (recordingId == null) {
                    recordingId = (pressId == null ? UUID.randomUUID().toString() : pressId)
                            + "-visitor";
                }
                if (!RECORDING_ID_PATTERN.matcher(recordingId).matches()) {
                    return ResponseEntity.badRequest().body("Invalid recordingId");
                }
                var existingRecording = recordingRepository.findByRecordingId(recordingId);
                if (existingRecording.isPresent()) {
                    audioKey = existingRecording.get().getAudioKey();
                } else {
                    audioKey = minioService.uploadFile(audio, recordingId);
                    VisitorRecording recording = recordingRepository.save(
                            new VisitorRecording(recordingId, event, audioKey,
                                    durationMs, now));
                    event.addVisitorRecording(recording);
                }
                // Preserve the legacy field while old clients are being rolled out.
                event.setAudioKey(audioKey);
            }

            event.markReady();
            Event savedEvent = eventRepository.save(event);
            if (session != null) {
                session.touch(now);
                sessionRepository.save(session);
            }
            
            String payload = objectMapper.writeValueAsString(savedEvent);
            broadcastDoorbellEvent(payload);
            if (session != null) {
                broadcastSession("session-updated", session);
                mqttGateway.sendToMqtt(objectMapper.writeValueAsString(
                        visitorSessionService.view(session)), eventsTopic);
            } else {
                mqttGateway.sendToMqtt(payload, eventsTopic);
            }
            
            boolean fallbackNotification =
                    eventAlertService.completeUpload(
                            pressId != null ? pressId : eventId, eventType);
            deviceTelemetryService.record(deviceId, firmwareVersion, eventType,
                    pressId, sanitizeRssi(wifiRssiDbm));
            
            return ResponseEntity.ok("Event processed successfully with image key: "
                    + imageKey + "; notification="
                    + (fallbackNotification ? "fallback" : "already-triggered"));
        } catch (Exception e) {
            return ResponseEntity.internalServerError().body("Failed to process event: " + e.getMessage());
        }
    }

    @PostMapping(path = "/trigger", consumes = MediaType.APPLICATION_JSON_VALUE)
    public ResponseEntity<?> triggerEvent(@RequestBody TriggerRequest request) {
        String eventId = normalize(request.pressId());
        if (eventId == null) eventId = normalize(request.eventId());
        String eventType = normalize(request.eventType());
        if (eventId == null || !EVENT_ID_PATTERN.matcher(eventId).matches()) {
            return ResponseEntity.badRequest().body("Invalid eventId");
        }
        if (eventType == null || !TRIGGER_EVENT_TYPES.contains(eventType)) {
            return ResponseEntity.badRequest().body("Invalid eventType");
        }

        LocalDateTime now = LocalDateTime.now();
        String sessionId = sessionId(request.sessionId(), eventId);
        if (!EVENT_ID_PATTERN.matcher(sessionId).matches()) {
            return ResponseEntity.badRequest().body("Invalid sessionId");
        }
        VisitorSession session = getOrCreateSession(sessionId, now);
        int pressNumber = pressNumber(request.pressNumber(), eventId);
        boolean newPress = eventRepository.findByEventId(eventId).isEmpty();
        if (newPress) {
            Event press = new Event(eventId, now, eventType,
                    Event.PENDING_IMAGE_KEY, null);
            press.assignToSession(session, pressNumber);
            press.markTriggered();
            eventRepository.save(press);
        }
        session.touch(now);
        sessionRepository.save(session);

        EventAlertService.TriggerOutcome outcome =
                eventAlertService.trigger(eventId, eventType,
                        request.dispatchAlerts() == null || request.dispatchAlerts());
        deviceTelemetryService.record(request.deviceId(),
                request.firmwareVersion(), eventType, eventId,
                sanitizeRssi(request.wifiRssiDbm()));
        broadcastSession(newPress && pressNumber == 1 ? "session-started" : "press-started",
                session);
        return ResponseEntity.ok(new TriggerResponse(eventId, sessionId,
                newPress, outcome.created(), outcome.triggeredAt()));
    }

    private static String normalize(String value) {
        if (value == null || value.isBlank()) {
            return null;
        }
        return value.trim();
    }

    private String sessionId(String requestedSessionId, String pressId) {
        String normalized = normalize(requestedSessionId);
        if (normalized != null) return normalized;
        var matcher = NUMBERED_PRESS_PATTERN.matcher(pressId);
        return matcher.matches() ? matcher.group(1) : pressId;
    }

    private static int pressNumber(Integer requestedPressNumber, String pressId) {
        if (requestedPressNumber != null && requestedPressNumber > 0) {
            return requestedPressNumber;
        }
        var matcher = NUMBERED_PRESS_PATTERN.matcher(pressId);
        if (matcher.matches()) {
            try {
                return Math.max(1, Integer.parseInt(matcher.group(2)));
            } catch (NumberFormatException ignored) {
                // Fall through to the first press for legacy identifiers.
            }
        }
        return 1;
    }

    private synchronized VisitorSession getOrCreateSession(String sessionId,
                                                           LocalDateTime startedAt) {
        if (!EVENT_ID_PATTERN.matcher(sessionId).matches()) {
            throw new IllegalArgumentException("Invalid sessionId");
        }
        return sessionRepository.findBySessionId(sessionId)
                .orElseGet(() -> sessionRepository.save(
                        new VisitorSession(sessionId, startedAt)));
    }

    private static Integer inspectVisitorWav(MultipartFile audio) throws IOException {
        if (!"audio/wav".equalsIgnoreCase(audio.getContentType())) return null;
        byte[] wav = audio.getBytes();
        if (wav.length < 44 || !asciiEquals(wav, 0, "RIFF") ||
                !asciiEquals(wav, 8, "WAVE") || !asciiEquals(wav, 12, "fmt ") ||
                !asciiEquals(wav, 36, "data") || readLe16(wav, 20) != 1 ||
                readLe16(wav, 22) != 1 || readLe32(wav, 24) != 16000 ||
                readLe16(wav, 34) != 16) {
            return null;
        }
        long dataLength = readLe32(wav, 40);
        if (dataLength != wav.length - 44L || (dataLength & 1L) != 0) return null;
        return Math.toIntExact(dataLength * 1000L / 32000L);
    }

    private static boolean asciiEquals(byte[] bytes, int offset, String value) {
        if (offset + value.length() > bytes.length) return false;
        for (int i = 0; i < value.length(); i++) {
            if ((bytes[offset + i] & 0xff) != value.charAt(i)) return false;
        }
        return true;
    }

    private static int readLe16(byte[] bytes, int offset) {
        return (bytes[offset] & 0xff) | ((bytes[offset + 1] & 0xff) << 8);
    }

    private static long readLe32(byte[] bytes, int offset) {
        return (bytes[offset] & 0xffL) |
                ((bytes[offset + 1] & 0xffL) << 8) |
                ((bytes[offset + 2] & 0xffL) << 16) |
                ((bytes[offset + 3] & 0xffL) << 24);
    }

    private static Integer sanitizeRssi(Integer wifiRssiDbm) {
        return wifiRssiDbm != null && wifiRssiDbm >= -127 && wifiRssiDbm <= 0
                ? wifiRssiDbm : null;
    }

    public record TriggerRequest(String eventId, String sessionId, String pressId,
                                 Integer pressNumber, String eventType,
                                 String deviceId, String firmwareVersion,
                                 Integer wifiRssiDbm, Boolean dispatchAlerts) {}

    public record TriggerResponse(String pressId, String sessionId,
                                  boolean pressCreated, boolean created,
                                  LocalDateTime triggeredAt) {}

    @PostMapping("/sessions/{sessionId}/close")
    public ResponseEntity<?> closeSession(@PathVariable String sessionId) {
        return sessionRepository.findBySessionId(sessionId)
                .map(session -> {
                    session.close(LocalDateTime.now());
                    sessionRepository.save(session);
                    broadcastSession("session-closed", session);
                    return ResponseEntity.ok(visitorSessionService.view(session));
                })
                .orElseGet(() -> ResponseEntity.notFound().build());
    }

    @PostMapping(path = "/presses/{pressId}/complete",
            consumes = MediaType.APPLICATION_JSON_VALUE)
    public ResponseEntity<?> completePress(@PathVariable String pressId,
                                           @RequestBody PressCompleteRequest request) {
        if (request.durationMs() == null || request.durationMs() < 0 ||
                request.durationMs() > 15000) {
            return ResponseEntity.badRequest().body("Invalid press duration");
        }
        return eventRepository.findByEventId(pressId)
                .map(press -> {
                    press.completePress(request.durationMs());
                    eventRepository.save(press);
                    VisitorSession session = press.getSession();
                    if (session != null) {
                        session.touch(LocalDateTime.now());
                        sessionRepository.save(session);
                        broadcastSession("press-ended", session);
                    }
                    return ResponseEntity.ok().build();
                })
                .orElseGet(() -> ResponseEntity.notFound().build());
    }

    @GetMapping("/sessions")
    public ResponseEntity<List<VisitorSessionService.SessionView>> getRecentSessions(
            @RequestParam(defaultValue = "100") int size) {
        return ResponseEntity.ok(visitorSessionService.recent(size));
    }

    public record PressCompleteRequest(Integer durationMs) {}

    @GetMapping("/export")
    public ResponseEntity<String> exportEventsCsv() {
        List<Event> events = eventRepository.findAll(Sort.by(Sort.Direction.DESC, "timestamp"));
        StringBuilder csv = new StringBuilder();
        csv.append("ID,Timestamp,Event Type,Image Key,Audio Key\n");
        
        for (Event event : events) {
            csv.append(event.getId()).append(",")
               .append(event.getTimestamp()).append(",")
               .append(event.getEventType()).append(",")
               .append(event.getImageKey()).append(",")
               .append(event.getAudioKey() != null ? event.getAudioKey() : "").append("\n");
        }
        
        return ResponseEntity.ok()
                .header("Content-Disposition", "attachment; filename=doorbell_events.csv")
                .contentType(MediaType.parseMediaType("text/csv"))
                .body(csv.toString());
    }

    @GetMapping
    public ResponseEntity<List<Event>> getRecentEvents(@RequestParam(defaultValue = "0") int page, 
                                                       @RequestParam(defaultValue = "10") int size) {
        Page<Event> events = eventRepository.findAll(PageRequest.of(page, size, Sort.by(Sort.Direction.DESC, "timestamp")));
        return ResponseEntity.ok(events.getContent());
    }

    @GetMapping(path = "/stream", produces = MediaType.TEXT_EVENT_STREAM_VALUE)
    public ResponseEntity<SseEmitter> streamEvents(HttpServletRequest request) {
        SseEmitter emitter = new SseEmitter(Long.MAX_VALUE);
        Object authentication = request.getAttribute(
                HouseholdAuthService.PRINCIPAL_ATTRIBUTE);
        if (!(authentication instanceof HouseholdAuthService.Principal principal)) {
            return ResponseEntity.status(401).build();
        }
        AuthenticatedEmitter authenticatedEmitter =
                new AuthenticatedEmitter(emitter, principal.sessionId());
        
        try {
            emitter.send(SseEmitter.event().comment("connection-open"));
            emitter.send(SseEmitter.event()
                    .name("init")
                    .reconnectTime(1000)
                    .data("Connection established"));
            
            this.emitters.add(authenticatedEmitter);
        } catch (IOException e) {
            return ResponseEntity.internalServerError().build();
        }

        emitter.onCompletion(() -> this.emitters.remove(authenticatedEmitter));
        emitter.onTimeout(() -> this.emitters.remove(authenticatedEmitter));
        emitter.onError((ex) -> this.emitters.remove(authenticatedEmitter));

        org.springframework.http.HttpHeaders headers = new org.springframework.http.HttpHeaders();
        headers.add("Cache-Control", "no-cache, no-store, no-transform, max-age=0, must-revalidate");
        headers.add("X-Accel-Buffering", "no");
        headers.add("Connection", "keep-alive");

        return ResponseEntity.ok().headers(headers).body(emitter);
    }

    @GetMapping("/media/{key}")
    public ResponseEntity<org.springframework.core.io.Resource> getMediaUrl(@PathVariable String key) {
        com.amazonaws.services.s3.model.S3Object s3Object = minioService.getFile(key);
        if (s3Object != null) {
            org.springframework.core.io.InputStreamResource resource = new org.springframework.core.io.InputStreamResource(s3Object.getObjectContent());
            return ResponseEntity.ok()
                    .contentType(MediaType.parseMediaType(s3Object.getObjectMetadata().getContentType()))
                    .body(resource);
        }
        return ResponseEntity.notFound().build();
    }

    @Scheduled(fixedRate = 10000)
    public void sendHeartbeat() {
        List<AuthenticatedEmitter> deadEmitters = new ArrayList<>();
        for (AuthenticatedEmitter authenticatedEmitter : emitters) {
            SseEmitter emitter = authenticatedEmitter.emitter();
            if (!householdAuthService.sessionIsActive(
                    authenticatedEmitter.deviceSessionId())) {
                emitter.complete();
                deadEmitters.add(authenticatedEmitter);
                continue;
            }
            try {
                emitter.send(SseEmitter.event()
                        .name("heartbeat")
                        .data("keep-alive"));
            } catch (IOException e) {
                deadEmitters.add(authenticatedEmitter);
            }
        }
        emitters.removeAll(deadEmitters);
    }

    @ServiceActivator(inputChannel = "mqttInputChannel")
    public void receiveMqttMessage(String payload) {
        broadcastDoorbellEvent(payload);
    }

    private void broadcastDoorbellEvent(String payload) {
        List<AuthenticatedEmitter> deadEmitters = new ArrayList<>();
        for (AuthenticatedEmitter authenticatedEmitter : emitters) {
            if (!householdAuthService.sessionIsActive(
                    authenticatedEmitter.deviceSessionId())) {
                authenticatedEmitter.emitter().complete();
                deadEmitters.add(authenticatedEmitter);
                continue;
            }
            try {
                authenticatedEmitter.emitter().send(SseEmitter.event()
                        .name("doorbell-event")
                        .data(payload));
            } catch (IOException | IllegalStateException e) {
                deadEmitters.add(authenticatedEmitter);
            }
        }
        emitters.removeAll(deadEmitters);
    }

    private void broadcastSession(String lifecycle,
                                  VisitorSession session) {
        try {
            String payload = objectMapper.writeValueAsString(
                    visitorSessionService.view(session));
            List<AuthenticatedEmitter> deadEmitters = new ArrayList<>();
            for (AuthenticatedEmitter authenticatedEmitter : emitters) {
                if (!householdAuthService.sessionIsActive(
                        authenticatedEmitter.deviceSessionId())) {
                    authenticatedEmitter.emitter().complete();
                    deadEmitters.add(authenticatedEmitter);
                    continue;
                }
                try {
                    SseEmitter emitter = authenticatedEmitter.emitter();
                    emitter.send(SseEmitter.event().name(lifecycle).data(payload));
                    emitter.send(SseEmitter.event().name("session-update").data(payload));
                } catch (IOException | IllegalStateException exception) {
                    deadEmitters.add(authenticatedEmitter);
                }
            }
            emitters.removeAll(deadEmitters);
        } catch (Exception exception) {
            // The persisted upload remains valid even when an SSE client update fails.
        }
    }

    public void publishSessionUpdate(VisitorSession session) {
        if (session != null) broadcastSession("session-updated", session);
    }

    private record AuthenticatedEmitter(SseEmitter emitter, long deviceSessionId) {}
}

package com.smartdoorbell.gateway.controller;

import com.fasterxml.jackson.databind.ObjectMapper;
import com.fasterxml.jackson.datatype.jsr310.JavaTimeModule;
import com.smartdoorbell.gateway.config.MqttGateway;
import com.smartdoorbell.gateway.entity.Event;
import com.smartdoorbell.gateway.repository.EventRepository;
import com.smartdoorbell.gateway.service.DeviceTelemetryService;
import com.smartdoorbell.gateway.service.EventAlertService;
import com.smartdoorbell.gateway.service.MinioService;
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

import com.fasterxml.jackson.databind.SerializationFeature;

@RestController
@RequestMapping("/api/events")
public class EventController {

    private final MinioService minioService;
    private final EventRepository eventRepository;
    private final MqttGateway mqttGateway;
    private final EventAlertService eventAlertService;
    private final DeviceTelemetryService deviceTelemetryService;
    private final ObjectMapper objectMapper;
    private final List<SseEmitter> emitters = new CopyOnWriteArrayList<>();

    @Value("${mqtt.topic.events:doorbell/events}")
    private String eventsTopic;

    private static final Pattern EVENT_ID_PATTERN =
            Pattern.compile("[A-Za-z0-9_-]{8,64}");
    private static final Set<String> TRIGGER_EVENT_TYPES =
            Set.of("DOORBELL_PRESS", "DOORBELL_REPRESS", "PIR_MOTION");

    public EventController(MinioService minioService,
                           EventRepository eventRepository,
                           MqttGateway mqttGateway,
                           EventAlertService eventAlertService,
                           DeviceTelemetryService deviceTelemetryService) {
        this.minioService = minioService;
        this.eventRepository = eventRepository;
        this.mqttGateway = mqttGateway;
        this.eventAlertService = eventAlertService;
        this.deviceTelemetryService = deviceTelemetryService;
        this.objectMapper = new ObjectMapper();
        this.objectMapper.registerModule(new JavaTimeModule());
        this.objectMapper.disable(SerializationFeature.WRITE_DATES_AS_TIMESTAMPS);
    }

    @PostMapping
    public ResponseEntity<String> createEvent(
            @RequestParam("image") MultipartFile image,
            @RequestParam(value = "audio", required = false) MultipartFile audio,
            @RequestParam(value = "eventType", defaultValue = "DOORBELL_PRESS") String eventType,
            @RequestParam(value = "eventId", required = false) String eventId,
            @RequestParam(value = "deviceId", required = false) String deviceId,
            @RequestParam(value = "firmwareVersion", required = false) String firmwareVersion,
            @RequestParam(value = "wifiRssiDbm", required = false) Integer wifiRssiDbm) {
        eventId = normalize(eventId);
        if (eventId != null && !EVENT_ID_PATTERN.matcher(eventId).matches()) {
            return ResponseEntity.badRequest().body("Invalid eventId");
        }

        if (eventId != null) {
            var existing = eventRepository.findByEventId(eventId);
            if (existing.isPresent()) {
                return ResponseEntity.ok("Event already processed with image key: "
                        + existing.get().getImageKey());
            }
        }

        try {
            String imageKey = minioService.uploadFile(image,
                    eventId == null ? null : eventId + "-image");
            String audioKey = null;
            if (audio != null && !audio.isEmpty()) {
                audioKey = minioService.uploadFile(audio,
                        eventId == null ? null : eventId + "-audio");
            }
            
            Event event = new Event(eventId, LocalDateTime.now(), eventType,
                    imageKey, audioKey);
            Event savedEvent = eventRepository.save(event);
            
            String payload = objectMapper.writeValueAsString(savedEvent);
            broadcastDoorbellEvent(payload);
            mqttGateway.sendToMqtt(payload, eventsTopic);
            
            boolean fallbackNotification =
                    eventAlertService.completeUpload(eventId, eventType);
            deviceTelemetryService.record(deviceId, firmwareVersion, eventType,
                    eventId, sanitizeRssi(wifiRssiDbm));
            
            return ResponseEntity.ok("Event processed successfully with image key: "
                    + imageKey + "; notification="
                    + (fallbackNotification ? "fallback" : "already-triggered"));
        } catch (Exception e) {
            return ResponseEntity.internalServerError().body("Failed to process event: " + e.getMessage());
        }
    }

    @PostMapping(path = "/trigger", consumes = MediaType.APPLICATION_JSON_VALUE)
    public ResponseEntity<?> triggerEvent(@RequestBody TriggerRequest request) {
        String eventId = normalize(request.eventId());
        String eventType = normalize(request.eventType());
        if (eventId == null || !EVENT_ID_PATTERN.matcher(eventId).matches()) {
            return ResponseEntity.badRequest().body("Invalid eventId");
        }
        if (eventType == null || !TRIGGER_EVENT_TYPES.contains(eventType)) {
            return ResponseEntity.badRequest().body("Invalid eventType");
        }

        EventAlertService.TriggerOutcome outcome =
                eventAlertService.trigger(eventId, eventType);
        deviceTelemetryService.record(request.deviceId(),
                request.firmwareVersion(), eventType, eventId,
                sanitizeRssi(request.wifiRssiDbm()));
        return ResponseEntity.ok(new TriggerResponse(outcome.eventId(),
                outcome.created(), outcome.triggeredAt()));
    }

    private static String normalize(String value) {
        if (value == null || value.isBlank()) {
            return null;
        }
        return value.trim();
    }

    private static Integer sanitizeRssi(Integer wifiRssiDbm) {
        return wifiRssiDbm != null && wifiRssiDbm >= -127 && wifiRssiDbm <= 0
                ? wifiRssiDbm : null;
    }

    public record TriggerRequest(String eventId, String eventType,
                                 String deviceId, String firmwareVersion,
                                 Integer wifiRssiDbm) {}

    public record TriggerResponse(String eventId, boolean created,
                                  LocalDateTime triggeredAt) {}

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
    public ResponseEntity<SseEmitter> streamEvents() {
        SseEmitter emitter = new SseEmitter(Long.MAX_VALUE);
        
        try {
            emitter.send(SseEmitter.event().comment("connection-open"));
            emitter.send(SseEmitter.event()
                    .name("init")
                    .data("Connection established"));
            
            this.emitters.add(emitter);
        } catch (IOException e) {
            return ResponseEntity.internalServerError().build();
        }

        emitter.onCompletion(() -> this.emitters.remove(emitter));
        emitter.onTimeout(() -> this.emitters.remove(emitter));
        emitter.onError((ex) -> this.emitters.remove(emitter));

        org.springframework.http.HttpHeaders headers = new org.springframework.http.HttpHeaders();
        headers.add("Cache-Control", "no-cache, no-store, max-age=0, must-revalidate");
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

    @Scheduled(fixedRate = 20000)
    public void sendHeartbeat() {
        List<SseEmitter> deadEmitters = new ArrayList<>();
        for (SseEmitter emitter : emitters) {
            try {
                emitter.send(SseEmitter.event()
                        .name("heartbeat")
                        .data("keep-alive"));
            } catch (IOException e) {
                deadEmitters.add(emitter);
            }
        }
        emitters.removeAll(deadEmitters);
    }

    @ServiceActivator(inputChannel = "mqttInputChannel")
    public void receiveMqttMessage(String payload) {
        broadcastDoorbellEvent(payload);
    }

    private void broadcastDoorbellEvent(String payload) {
        List<SseEmitter> deadEmitters = new ArrayList<>();
        for (SseEmitter emitter : emitters) {
            try {
                emitter.send(SseEmitter.event()
                        .name("doorbell-event")
                        .data(payload));
            } catch (IOException | IllegalStateException e) {
                deadEmitters.add(emitter);
            }
        }
        emitters.removeAll(deadEmitters);
    }
}

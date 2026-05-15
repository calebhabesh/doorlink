package com.smartdoorbell.gateway.controller;

import com.fasterxml.jackson.databind.ObjectMapper;
import com.fasterxml.jackson.datatype.jsr310.JavaTimeModule;
import com.smartdoorbell.gateway.config.MqttGateway;
import com.smartdoorbell.gateway.entity.Event;
import com.smartdoorbell.gateway.repository.EventRepository;
import com.smartdoorbell.gateway.service.MinioService;
import com.smartdoorbell.gateway.service.NtfyService;
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
import java.util.concurrent.CopyOnWriteArrayList;

import com.fasterxml.jackson.databind.SerializationFeature;

@RestController
@RequestMapping("/api/events")
public class EventController {

    private final MinioService minioService;
    private final EventRepository eventRepository;
    private final MqttGateway mqttGateway;
    private final NtfyService ntfyService;
    private final ObjectMapper objectMapper;
    private final List<SseEmitter> emitters = new CopyOnWriteArrayList<>();

    @Value("${mqtt.topic.events:doorbell/events}")
    private String eventsTopic;

    public EventController(MinioService minioService, EventRepository eventRepository, MqttGateway mqttGateway, NtfyService ntfyService) {
        this.minioService = minioService;
        this.eventRepository = eventRepository;
        this.mqttGateway = mqttGateway;
        this.ntfyService = ntfyService;
        this.objectMapper = new ObjectMapper();
        this.objectMapper.registerModule(new JavaTimeModule());
        this.objectMapper.disable(SerializationFeature.WRITE_DATES_AS_TIMESTAMPS);
    }

    @PostMapping
    public ResponseEntity<String> createEvent(
            @RequestParam("image") MultipartFile image,
            @RequestParam(value = "audio", required = false) MultipartFile audio,
            @RequestParam(value = "eventType", defaultValue = "DOORBELL_PRESS") String eventType) {
        try {
            String imageKey = minioService.uploadFile(image);
            String audioKey = null;
            if (audio != null && !audio.isEmpty()) {
                audioKey = minioService.uploadFile(audio);
            }
            
            Event event = new Event(LocalDateTime.now(), eventType, imageKey, audioKey);
            Event savedEvent = eventRepository.save(event);
            
            String payload = objectMapper.writeValueAsString(savedEvent);
            mqttGateway.sendToMqtt(payload, eventsTopic);
            
            ntfyService.sendNotification(savedEvent);
            
            return ResponseEntity.ok("Event processed successfully with image key: " + imageKey);
        } catch (Exception e) {
            return ResponseEntity.internalServerError().body("Failed to process event: " + e.getMessage());
        }
    }

    @GetMapping
    public ResponseEntity<List<Event>> getRecentEvents(@RequestParam(defaultValue = "0") int page, 
                                                       @RequestParam(defaultValue = "10") int size) {
        Page<Event> events = eventRepository.findAll(PageRequest.of(page, size, Sort.by(Sort.Direction.DESC, "timestamp")));
        return ResponseEntity.ok(events.getContent());
    }

    @GetMapping(path = "/stream", produces = MediaType.TEXT_EVENT_STREAM_VALUE)
    public SseEmitter streamEvents() {
        SseEmitter emitter = new SseEmitter(Long.MAX_VALUE);
        
        try {
            emitter.send(SseEmitter.event().comment("connection-open"));
            emitter.send(SseEmitter.event()
                    .name("init")
                    .data("Connection established"));
            
            this.emitters.add(emitter);
        } catch (IOException e) {
            return null;
        }

        emitter.onCompletion(() -> this.emitters.remove(emitter));
        emitter.onTimeout(() -> this.emitters.remove(emitter));
        emitter.onError((ex) -> this.emitters.remove(emitter));

        return emitter;
    }

    @GetMapping("/media/{key}")
    public ResponseEntity<Void> getMediaUrl(@PathVariable String key) {
        String presignedUrl = minioService.getPresignedUrl(key);
        if (presignedUrl != null) {
            return ResponseEntity.status(org.springframework.http.HttpStatus.FOUND)
                    .location(java.net.URI.create(presignedUrl))
                    .build();
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
        for (SseEmitter emitter : emitters) {
            try {
                emitter.send(SseEmitter.event().name("doorbell-event").data(payload));
            } catch (IOException e) {
                emitters.remove(emitter);
            }
        }
    }
}

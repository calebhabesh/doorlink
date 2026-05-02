package com.smartdoorbell.gateway.controller;

import com.fasterxml.jackson.databind.ObjectMapper;
import com.fasterxml.jackson.datatype.jsr310.JavaTimeModule;
import com.smartdoorbell.gateway.config.MqttGateway;
import com.smartdoorbell.gateway.entity.Event;
import com.smartdoorbell.gateway.repository.EventRepository;
import com.smartdoorbell.gateway.service.MinioService;
import org.springframework.beans.factory.annotation.Value;
import org.springframework.data.domain.Page;
import org.springframework.data.domain.PageRequest;
import org.springframework.data.domain.Sort;
import org.springframework.http.MediaType;
import org.springframework.http.ResponseEntity;
import org.springframework.integration.annotation.ServiceActivator;
import org.springframework.web.bind.annotation.*;
import org.springframework.web.multipart.MultipartFile;
import org.springframework.web.servlet.mvc.method.annotation.SseEmitter;

import java.io.IOException;
import java.time.LocalDateTime;
import java.util.List;
import java.util.concurrent.CopyOnWriteArrayList;

@RestController
@RequestMapping("/api/events")
public class EventController {

    private final MinioService minioService;
    private final EventRepository eventRepository;
    private final MqttGateway mqttGateway;
    private final ObjectMapper objectMapper;
    private final List<SseEmitter> emitters = new CopyOnWriteArrayList<>();

    @Value("${mqtt.topic.events:doorbell/events}")
    private String eventsTopic;

    public EventController(MinioService minioService, EventRepository eventRepository, MqttGateway mqttGateway) {
        this.minioService = minioService;
        this.eventRepository = eventRepository;
        this.mqttGateway = mqttGateway;
        this.objectMapper = new ObjectMapper();
        this.objectMapper.registerModule(new JavaTimeModule());
    }

    @PostMapping
    public ResponseEntity<String> createEvent(
            @RequestParam("image") MultipartFile image,
            @RequestParam(value = "eventType", defaultValue = "DOORBELL_PRESS") String eventType) {
        try {
            String imageKey = minioService.uploadFile(image);
            Event event = new Event(LocalDateTime.now(), eventType, imageKey);
            Event savedEvent = eventRepository.save(event);
            
            String payload = objectMapper.writeValueAsString(savedEvent);
            mqttGateway.sendToMqtt(payload, eventsTopic);
            
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
        this.emitters.add(emitter);

        emitter.onCompletion(() -> this.emitters.remove(emitter));
        emitter.onTimeout(() -> this.emitters.remove(emitter));

        return emitter;
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

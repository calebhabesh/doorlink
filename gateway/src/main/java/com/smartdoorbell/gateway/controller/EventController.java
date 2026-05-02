package com.smartdoorbell.gateway.controller;

import com.fasterxml.jackson.databind.ObjectMapper;
import com.fasterxml.jackson.datatype.jsr310.JavaTimeModule;
import com.smartdoorbell.gateway.config.MqttGateway;
import com.smartdoorbell.gateway.entity.Event;
import com.smartdoorbell.gateway.repository.EventRepository;
import com.smartdoorbell.gateway.service.MinioService;
import org.springframework.beans.factory.annotation.Value;
import org.springframework.http.ResponseEntity;
import org.springframework.web.bind.annotation.*;
import org.springframework.web.multipart.MultipartFile;

import java.time.LocalDateTime;

@RestController
@RequestMapping("/api/events")
public class EventController {

    private final MinioService minioService;
    private final EventRepository eventRepository;
    private final MqttGateway mqttGateway;
    private final ObjectMapper objectMapper;

    @Value("${mqtt.topic.events:doorbell/events}")
    private String eventsTopic;

    public EventController(MinioService minioService, EventRepository eventRepository, MqttGateway mqttGateway) {
        this.minioService = minioService;
        this.eventRepository = eventRepository;
        this.mqttGateway = mqttGateway;
        this.objectMapper = new ObjectMapper();
        this.objectMapper.registerModule(new JavaTimeModule()); // Register module for LocalDateTime serialization
    }

    @PostMapping
    public ResponseEntity<String> createEvent(
            @RequestParam("image") MultipartFile image,
            @RequestParam(value = "eventType", defaultValue = "DOORBELL_PRESS") String eventType) {
        
        try {
            String imageKey = minioService.uploadFile(image);
            Event event = new Event(LocalDateTime.now(), eventType, imageKey);
            Event savedEvent = eventRepository.save(event);
            
            // Broadcast event via MQTT
            String payload = objectMapper.writeValueAsString(savedEvent);
            mqttGateway.sendToMqtt(payload, eventsTopic);
            
            return ResponseEntity.ok("Event processed successfully with image key: " + imageKey);
        } catch (Exception e) {
            return ResponseEntity.internalServerError().body("Failed to process event: " + e.getMessage());
        }
    }
}

package com.smartdoorbell.gateway.controller;

import com.smartdoorbell.gateway.config.MqttGateway;
import com.smartdoorbell.gateway.entity.SystemSettings;
import com.smartdoorbell.gateway.repository.EventRepository;
import com.smartdoorbell.gateway.repository.SystemSettingsRepository;
import com.smartdoorbell.gateway.service.MinioService;
import com.smartdoorbell.gateway.service.MqttConnectionMonitor;
import org.springframework.http.ResponseEntity;
import org.springframework.web.bind.annotation.*;
import org.springframework.web.multipart.MultipartFile;

import java.util.HashMap;
import java.util.Map;

@RestController
@RequestMapping("/api/system")
public class SystemController {

    private final SystemSettingsRepository settingsRepository;
    private final MinioService minioService;
    private final MqttConnectionMonitor mqttMonitor;
    private final EventRepository eventRepository;
    private final MqttGateway mqttGateway;
    private final com.smartdoorbell.gateway.config.MqttConfig mqttConfig;

    public SystemController(SystemSettingsRepository settingsRepository, 
                            MinioService minioService, 
                            MqttConnectionMonitor mqttMonitor,
                            EventRepository eventRepository,
                            MqttGateway mqttGateway,
                            com.smartdoorbell.gateway.config.MqttConfig mqttConfig) {
        this.settingsRepository = settingsRepository;
        this.minioService = minioService;
        this.mqttMonitor = mqttMonitor;
        this.eventRepository = eventRepository;
        this.mqttGateway = mqttGateway;
        this.mqttConfig = mqttConfig;
    }

    @PostMapping("/ptt")
    public ResponseEntity<String> handlePttAudio(@RequestParam("audio") MultipartFile audio) {
        try {
            byte[] audioData = audio.getBytes();
            mqttGateway.sendToMqtt(audioData, mqttConfig.getPttAudioTopic());
            return ResponseEntity.ok("PTT audio published to MQTT");
        } catch (Exception e) {
            return ResponseEntity.internalServerError().body("Failed to publish PTT audio: " + e.getMessage());
        }
    }

    @GetMapping("/health")
    public ResponseEntity<Map<String, Object>> getHealth() {
        Map<String, Object> health = new HashMap<>();
        
        // Database Health
        boolean dbHealthy = true;
        try {
            eventRepository.count();
        } catch (Exception e) {
            dbHealthy = false;
        }
        health.put("database", dbHealthy ? "UP" : "DOWN");
        
        // MinIO Health
        health.put("storage", minioService.isAvailable() ? "UP" : "DOWN");
        
        // MQTT Health
        health.put("mqtt", mqttMonitor.isConnected() ? "UP" : "DOWN");
        
        // Gateway Health (Implicitly UP if this endpoint responds)
        health.put("gateway", "UP");
        
        return ResponseEntity.ok(health);
    }

    @GetMapping("/settings")
    public ResponseEntity<SystemSettings> getSettings() {
        SystemSettings settings = settingsRepository.findById(1L)
                .orElseGet(() -> {
                    SystemSettings newSettings = new SystemSettings();
                    return settingsRepository.save(newSettings);
                });
        return ResponseEntity.ok(settings);
    }

    @PostMapping("/settings")
    public ResponseEntity<SystemSettings> updateSettings(@RequestBody SystemSettings newSettings) {
        SystemSettings settings = settingsRepository.findById(1L)
                .orElse(new SystemSettings());
        
        settings.setRetentionDays(newSettings.getRetentionDays());
        settings.setCameraQuality(newSettings.getCameraQuality());
        settings.setNotificationsEnabled(newSettings.getNotificationsEnabled());
        
        SystemSettings saved = settingsRepository.save(settings);
        return ResponseEntity.ok(saved);
    }
}

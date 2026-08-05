package com.smartdoorbell.gateway.controller;

import com.smartdoorbell.gateway.config.MqttGateway;
import com.smartdoorbell.gateway.entity.SystemSettings;
import com.smartdoorbell.gateway.repository.SystemSettingsRepository;
import com.smartdoorbell.gateway.service.SystemHealthService;
import org.springframework.http.ResponseEntity;
import org.springframework.web.bind.annotation.*;
import org.springframework.web.multipart.MultipartFile;

@RestController
@RequestMapping("/api/system")
public class SystemController {

    private final SystemSettingsRepository settingsRepository;
    private final MqttGateway mqttGateway;
    private final com.smartdoorbell.gateway.config.MqttConfig mqttConfig;
    private final SystemHealthService systemHealthService;

    public SystemController(SystemSettingsRepository settingsRepository,
                            MqttGateway mqttGateway,
                            com.smartdoorbell.gateway.config.MqttConfig mqttConfig,
                            SystemHealthService systemHealthService) {
        this.settingsRepository = settingsRepository;
        this.mqttGateway = mqttGateway;
        this.mqttConfig = mqttConfig;
        this.systemHealthService = systemHealthService;
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
    public ResponseEntity<SystemHealthService.HealthSnapshot> getHealth() {
        return ResponseEntity.ok(systemHealthService.snapshot());
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

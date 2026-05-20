package com.smartdoorbell.gateway.controller;

import com.smartdoorbell.gateway.service.MinioService;
import com.smartdoorbell.gateway.service.MqttConnectionMonitor;
import org.springframework.http.ResponseEntity;
import org.springframework.web.bind.annotation.GetMapping;
import org.springframework.web.bind.annotation.RequestMapping;
import org.springframework.web.bind.annotation.RestController;

import javax.sql.DataSource;
import java.sql.Connection;
import java.util.HashMap;
import java.util.Map;

@RestController
@RequestMapping("/api/health")
public class HealthController {

    private final DataSource dataSource;
    private final MinioService minioService;
    private final MqttConnectionMonitor mqttMonitor;

    public HealthController(DataSource dataSource, MinioService minioService, MqttConnectionMonitor mqttMonitor) {
        this.dataSource = dataSource;
        this.minioService = minioService;
        this.mqttMonitor = mqttMonitor;
    }

    @GetMapping
    public ResponseEntity<Map<String, Object>> getHealth() {
        Map<String, Object> health = new HashMap<>();
        
        health.put("database", checkDatabase() ? "UP" : "DOWN");
        health.put("minio", minioService.isAvailable() ? "UP" : "DOWN");
        health.put("mqtt", mqttMonitor.isConnected() ? "UP" : "DOWN");
        health.put("status", "OK");
        
        return ResponseEntity.ok(health);
    }

    private boolean checkDatabase() {
        try (Connection connection = dataSource.getConnection()) {
            return connection.isValid(1000);
        } catch (Exception e) {
            return false;
        }
    }
}

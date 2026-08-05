package com.smartdoorbell.gateway.controller;

import com.smartdoorbell.gateway.service.SystemHealthService;
import org.springframework.http.ResponseEntity;
import org.springframework.web.bind.annotation.GetMapping;
import org.springframework.web.bind.annotation.RequestMapping;
import org.springframework.web.bind.annotation.RestController;

@RestController
@RequestMapping("/api/health")
public class HealthController {

    private final SystemHealthService systemHealthService;

    public HealthController(SystemHealthService systemHealthService) {
        this.systemHealthService = systemHealthService;
    }

    @GetMapping
    public ResponseEntity<SystemHealthService.HealthSnapshot> getHealth() {
        return ResponseEntity.ok(systemHealthService.snapshot());
    }
}

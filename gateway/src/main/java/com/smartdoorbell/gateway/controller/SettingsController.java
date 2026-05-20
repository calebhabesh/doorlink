package com.smartdoorbell.gateway.controller;

import com.smartdoorbell.gateway.entity.SystemSettings;
import com.smartdoorbell.gateway.repository.SystemSettingsRepository;
import org.springframework.http.ResponseEntity;
import org.springframework.web.bind.annotation.*;

@RestController
@RequestMapping("/api/settings")
public class SettingsController {

    private final SystemSettingsRepository settingsRepository;

    public SettingsController(SystemSettingsRepository settingsRepository) {
        this.settingsRepository = settingsRepository;
    }

    @GetMapping
    public ResponseEntity<SystemSettings> getSettings() {
        SystemSettings settings = settingsRepository.findById(1L)
                .orElseGet(() -> {
                    SystemSettings defaultSettings = new SystemSettings();
                    return settingsRepository.save(defaultSettings);
                });
        return ResponseEntity.ok(settings);
    }

    @PutMapping
    public ResponseEntity<SystemSettings> updateSettings(@RequestBody SystemSettings newSettings) {
        newSettings.setId(1L); // Ensure we only update the single global record
        SystemSettings saved = settingsRepository.save(newSettings);
        return ResponseEntity.ok(saved);
    }
}

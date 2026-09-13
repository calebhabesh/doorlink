package com.smartdoorbell.gateway.service;

import com.smartdoorbell.gateway.entity.DeviceStatus;
import com.smartdoorbell.gateway.repository.DeviceStatusRepository;
import org.springframework.stereotype.Service;
import org.springframework.transaction.annotation.Transactional;

import java.time.LocalDateTime;

@Service
public class DeviceTelemetryService {
    private final DeviceStatusRepository deviceStatusRepository;

    public DeviceTelemetryService(DeviceStatusRepository deviceStatusRepository) {
        this.deviceStatusRepository = deviceStatusRepository;
    }

    @Transactional
    public void record(String deviceId, String firmwareVersion, String eventType,
                       String eventId, Integer wifiRssiDbm,
                       Integer batteryMillivolts) {
        if (deviceId == null || deviceId.isBlank()) {
            return;
        }
        DeviceStatus status = deviceStatusRepository.findById(deviceId)
                .orElseGet(() -> new DeviceStatus(deviceId));
        status.record(LocalDateTime.now(), firmwareVersion, eventType,
                eventId, wifiRssiDbm, batteryMillivolts);
        deviceStatusRepository.save(status);
    }
}

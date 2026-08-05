package com.smartdoorbell.gateway.service;

import com.smartdoorbell.gateway.entity.DeviceStatus;
import com.smartdoorbell.gateway.repository.DeviceStatusRepository;
import org.springframework.stereotype.Service;

import javax.sql.DataSource;
import java.lang.management.ManagementFactory;
import java.sql.Connection;
import java.time.Instant;
import java.time.LocalDateTime;

@Service
public class SystemHealthService {
    private final DataSource dataSource;
    private final MinioService minioService;
    private final MqttConnectionMonitor mqttMonitor;
    private final DeviceStatusRepository deviceStatusRepository;

    public SystemHealthService(DataSource dataSource,
                               MinioService minioService,
                               MqttConnectionMonitor mqttMonitor,
                               DeviceStatusRepository deviceStatusRepository) {
        this.dataSource = dataSource;
        this.minioService = minioService;
        this.mqttMonitor = mqttMonitor;
        this.deviceStatusRepository = deviceStatusRepository;
    }

    public HealthSnapshot snapshot() {
        ComponentHealth database = checkDatabase();
        ComponentHealth storage = minioService.isAvailable()
                ? new ComponentHealth("UP", "S3 API probe succeeded")
                : new ComponentHealth("DOWN", "S3 API probe failed");
        MqttHealth mqtt = new MqttHealth(mqttMonitor.getStatus(),
                mqttMonitor.getLastChangedAt());
        DeviceHealth device;
        try {
            device = deviceStatusRepository.findTopByOrderByLastSeenDesc()
                    .map(this::toHealth)
                    .orElseGet(this::unknownDevice);
        } catch (Exception ignored) {
            // Keep the gateway and other component results observable when
            // PostgreSQL is unavailable.
            device = unknownDevice();
        }

        return new HealthSnapshot(Instant.now(),
                new GatewayHealth("UP",
                        ManagementFactory.getRuntimeMXBean().getUptime() / 1000L),
                database, storage, mqtt, device);
    }

    private ComponentHealth checkDatabase() {
        try (Connection connection = dataSource.getConnection()) {
            if (connection.isValid(1000)) {
                return new ComponentHealth("UP", "Connection validation succeeded");
            }
            return new ComponentHealth("DOWN", "Connection validation returned false");
        } catch (Exception e) {
            return new ComponentHealth("DOWN", "Connection validation failed");
        }
    }

    private DeviceHealth toHealth(DeviceStatus status) {
        return new DeviceHealth("KNOWN", status.getDeviceId(),
                status.getLastSeen(), status.getFirmwareVersion(),
                status.getLastEventType(), status.getLastEventId(),
                status.getWifiRssiDbm(), "NOT_REPORTED");
    }

    private DeviceHealth unknownDevice() {
        return new DeviceHealth("UNKNOWN", null, null, null, null, null,
                null, "NOT_REPORTED");
    }

    public record HealthSnapshot(Instant checkedAt, GatewayHealth gateway,
                                 ComponentHealth database,
                                 ComponentHealth storage, MqttHealth mqtt,
                                 DeviceHealth device) {}

    public record GatewayHealth(String status, long uptimeSeconds) {}
    public record ComponentHealth(String status, String detail) {}
    public record MqttHealth(String status, Instant lastChangedAt) {}
    public record DeviceHealth(String status, String deviceId,
                               LocalDateTime lastSeen, String firmwareVersion,
                               String lastEventType, String lastEventId,
                               Integer wifiRssiDbm, String batteryStatus) {}
}

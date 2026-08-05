package com.smartdoorbell.gateway.service;

import com.smartdoorbell.gateway.repository.DeviceStatusRepository;
import org.junit.jupiter.api.Test;
import org.junit.jupiter.api.extension.ExtendWith;
import org.mockito.Mock;
import org.mockito.junit.jupiter.MockitoExtension;

import javax.sql.DataSource;
import java.sql.Connection;
import java.util.Optional;

import static org.junit.jupiter.api.Assertions.assertEquals;
import static org.mockito.Mockito.when;

@ExtendWith(MockitoExtension.class)
class SystemHealthServiceTest {
    @Mock
    private DataSource dataSource;
    @Mock
    private Connection connection;
    @Mock
    private MinioService minioService;
    @Mock
    private MqttConnectionMonitor mqttMonitor;
    @Mock
    private DeviceStatusRepository deviceStatusRepository;

    @Test
    void snapshotUsesProbesAndDoesNotInventDevicePower() throws Exception {
        when(dataSource.getConnection()).thenReturn(connection);
        when(connection.isValid(1000)).thenReturn(true);
        when(minioService.isAvailable()).thenReturn(true);
        when(mqttMonitor.getStatus()).thenReturn("UNKNOWN");
        when(deviceStatusRepository.findTopByOrderByLastSeenDesc())
                .thenReturn(Optional.empty());

        SystemHealthService.HealthSnapshot snapshot = new SystemHealthService(
                dataSource, minioService, mqttMonitor, deviceStatusRepository)
                .snapshot();

        assertEquals("UP", snapshot.gateway().status());
        assertEquals("UP", snapshot.database().status());
        assertEquals("UP", snapshot.storage().status());
        assertEquals("UNKNOWN", snapshot.mqtt().status());
        assertEquals("UNKNOWN", snapshot.device().status());
        assertEquals("NOT_REPORTED", snapshot.device().batteryStatus());
    }

    @Test
    void databaseFailureDoesNotHideGatewayOrOtherComponentState() throws Exception {
        when(dataSource.getConnection()).thenThrow(new java.sql.SQLException("offline"));
        when(minioService.isAvailable()).thenReturn(true);
        when(mqttMonitor.getStatus()).thenReturn("UP");
        when(deviceStatusRepository.findTopByOrderByLastSeenDesc())
                .thenThrow(new IllegalStateException("database offline"));

        SystemHealthService.HealthSnapshot snapshot = new SystemHealthService(
                dataSource, minioService, mqttMonitor, deviceStatusRepository)
                .snapshot();

        assertEquals("UP", snapshot.gateway().status());
        assertEquals("DOWN", snapshot.database().status());
        assertEquals("UP", snapshot.storage().status());
        assertEquals("UP", snapshot.mqtt().status());
        assertEquals("UNKNOWN", snapshot.device().status());
    }
}

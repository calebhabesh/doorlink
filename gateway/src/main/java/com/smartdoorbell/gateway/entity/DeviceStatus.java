package com.smartdoorbell.gateway.entity;

import jakarta.persistence.Column;
import jakarta.persistence.Entity;
import jakarta.persistence.Id;
import jakarta.persistence.Table;

import java.time.LocalDateTime;

@Entity
@Table(name = "device_status")
public class DeviceStatus {
    @Id
    @Column(length = 64, nullable = false)
    private String deviceId;

    @Column(nullable = false)
    private LocalDateTime lastSeen;

    private String firmwareVersion;
    private String lastEventType;
    private String lastEventId;
    private Integer wifiRssiDbm;
    private Integer batteryMillivolts;
    private LocalDateTime batteryReportedAt;

    protected DeviceStatus() {}

    public DeviceStatus(String deviceId) {
        this.deviceId = deviceId;
    }

    public void record(LocalDateTime seenAt, String firmwareVersion,
                       String eventType, String eventId, Integer wifiRssiDbm,
                       Integer batteryMillivolts) {
        this.lastSeen = seenAt;
        this.firmwareVersion = firmwareVersion;
        this.lastEventType = eventType;
        this.lastEventId = eventId;
        this.wifiRssiDbm = wifiRssiDbm;
        if (batteryMillivolts != null) {
            this.batteryMillivolts = batteryMillivolts;
            this.batteryReportedAt = seenAt;
        }
    }

    public String getDeviceId() { return deviceId; }
    public LocalDateTime getLastSeen() { return lastSeen; }
    public String getFirmwareVersion() { return firmwareVersion; }
    public String getLastEventType() { return lastEventType; }
    public String getLastEventId() { return lastEventId; }
    public Integer getWifiRssiDbm() { return wifiRssiDbm; }
    public Integer getBatteryMillivolts() { return batteryMillivolts; }
    public LocalDateTime getBatteryReportedAt() { return batteryReportedAt; }
}

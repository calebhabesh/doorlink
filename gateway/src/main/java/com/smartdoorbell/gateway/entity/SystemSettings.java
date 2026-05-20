package com.smartdoorbell.gateway.entity;

import jakarta.persistence.*;

@Entity
@Table(name = "system_settings")
public class SystemSettings {
    @Id
    private Long id = 1L; // We only ever have one record for global settings

    @Column(nullable = false)
    private Integer retentionDays = 30;

    @Column(nullable = false)
    private String cameraQuality = "1080p";

    @Column(nullable = false)
    private Boolean notificationsEnabled = true;

    public SystemSettings() {}

    public SystemSettings(Integer retentionDays, String cameraQuality, Boolean notificationsEnabled) {
        this.retentionDays = retentionDays;
        this.cameraQuality = cameraQuality;
        this.notificationsEnabled = notificationsEnabled;
    }

    public Long getId() { return id; }
    public void setId(Long id) { this.id = id; }

    public Integer getRetentionDays() { return retentionDays; }
    public void setRetentionDays(Integer retentionDays) { this.retentionDays = retentionDays; }

    public String getCameraQuality() { return cameraQuality; }
    public void setCameraQuality(String cameraQuality) { this.cameraQuality = cameraQuality; }

    public Boolean getNotificationsEnabled() { return notificationsEnabled; }
    public void setNotificationsEnabled(Boolean notificationsEnabled) { this.notificationsEnabled = notificationsEnabled; }
}

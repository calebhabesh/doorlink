package com.smartdoorbell.gateway.entity;

import jakarta.persistence.Column;
import jakarta.persistence.Entity;
import jakarta.persistence.Id;
import jakarta.persistence.Table;

import java.time.LocalDateTime;

@Entity
@Table(name = "event_trigger_receipts")
public class EventTriggerReceipt {
    @Id
    @Column(length = 64, nullable = false)
    private String eventId;

    @Column(nullable = false)
    private String eventType;

    @Column(nullable = false)
    private LocalDateTime triggeredAt;

    private LocalDateTime uploadedAt;

    protected EventTriggerReceipt() {}

    public EventTriggerReceipt(String eventId, String eventType, LocalDateTime triggeredAt) {
        this.eventId = eventId;
        this.eventType = eventType;
        this.triggeredAt = triggeredAt;
    }

    public String getEventId() { return eventId; }
    public String getEventType() { return eventType; }
    public LocalDateTime getTriggeredAt() { return triggeredAt; }
    public LocalDateTime getUploadedAt() { return uploadedAt; }

    public void markUploaded(LocalDateTime uploadedAt) {
        this.uploadedAt = uploadedAt;
    }
}

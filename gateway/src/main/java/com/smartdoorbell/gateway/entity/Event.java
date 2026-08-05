package com.smartdoorbell.gateway.entity;

import jakarta.persistence.*;
import java.time.LocalDateTime;

@Entity
@Table(name = "events")
public class Event {
    @Id
    @GeneratedValue(strategy = GenerationType.IDENTITY)
    private Long id;

    @Column(unique = true, length = 64)
    private String eventId;

    @Column(nullable = false)
    private LocalDateTime timestamp;

    @Column(nullable = false)
    private String eventType;

    @Column(nullable = false)
    private String imageKey;

    @Column(nullable = true)
    private String audioKey;

    public Event() {}

    public Event(LocalDateTime timestamp, String eventType, String imageKey) {
        this.timestamp = timestamp;
        this.eventType = eventType;
        this.imageKey = imageKey;
    }

    public Event(LocalDateTime timestamp, String eventType, String imageKey, String audioKey) {
        this.timestamp = timestamp;
        this.eventType = eventType;
        this.imageKey = imageKey;
        this.audioKey = audioKey;
    }

    public Event(String eventId, LocalDateTime timestamp, String eventType, String imageKey, String audioKey) {
        this.eventId = eventId;
        this.timestamp = timestamp;
        this.eventType = eventType;
        this.imageKey = imageKey;
        this.audioKey = audioKey;
    }

    public Long getId() { return id; }
    public String getEventId() { return eventId; }
    public LocalDateTime getTimestamp() { return timestamp; }
    public String getEventType() { return eventType; }
    public String getImageKey() { return imageKey; }
    public String getAudioKey() { return audioKey; }
}

package com.smartdoorbell.gateway.entity;

import jakarta.persistence.*;
import java.time.LocalDateTime;

@Entity
@Table(name = "events")
public class Event {
    @Id
    @GeneratedValue(strategy = GenerationType.IDENTITY)
    private Long id;

    @Column(nullable = false)
    private LocalDateTime timestamp;

    @Column(nullable = false)
    private String eventType;

    @Column(nullable = false)
    private String imageKey;

    public Event() {}

    public Event(LocalDateTime timestamp, String eventType, String imageKey) {
        this.timestamp = timestamp;
        this.eventType = eventType;
        this.imageKey = imageKey;
    }

    public Long getId() { return id; }
    public LocalDateTime getTimestamp() { return timestamp; }
    public String getEventType() { return eventType; }
    public String getImageKey() { return imageKey; }
}

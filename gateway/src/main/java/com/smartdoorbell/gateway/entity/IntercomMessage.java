package com.smartdoorbell.gateway.entity;

import jakarta.persistence.*;
import java.time.LocalDateTime;

@Entity
@Table(name = "intercom_messages")
public class IntercomMessage {
    @Id
    @GeneratedValue(strategy = GenerationType.IDENTITY)
    private Long id;

    @Column(nullable = false, unique = true, length = 36)
    private String messageId;

    @ManyToOne(fetch = FetchType.LAZY, optional = false)
    @JoinColumn(name = "event_id", nullable = false)
    private Event event;

    @Column(nullable = false, length = 16)
    private String sender;

    @Column(nullable = false)
    private String audioKey;

    @Column(nullable = false)
    private Integer durationMs;

    @Column(nullable = false)
    private LocalDateTime createdAt;

    private LocalDateTime deliveredAt;

    protected IntercomMessage() {}

    public IntercomMessage(String messageId, Event event, String sender,
                           String audioKey, Integer durationMs,
                           LocalDateTime createdAt) {
        this.messageId = messageId;
        this.event = event;
        this.sender = sender;
        this.audioKey = audioKey;
        this.durationMs = durationMs;
        this.createdAt = createdAt;
    }

    public Long getId() { return id; }
    public String getMessageId() { return messageId; }
    public Event getEvent() { return event; }
    public String getSender() { return sender; }
    public String getAudioKey() { return audioKey; }
    public Integer getDurationMs() { return durationMs; }
    public LocalDateTime getCreatedAt() { return createdAt; }
    public LocalDateTime getDeliveredAt() { return deliveredAt; }
    public void markDelivered(LocalDateTime deliveredAt) { this.deliveredAt = deliveredAt; }
}

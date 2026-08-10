package com.smartdoorbell.gateway.entity;

import com.fasterxml.jackson.annotation.JsonIgnore;
import jakarta.persistence.*;
import java.time.LocalDateTime;

@Entity
@Table(name = "visitor_recordings")
public class VisitorRecording {
    @Id
    @GeneratedValue(strategy = GenerationType.IDENTITY)
    private Long id;

    @Column(nullable = false, unique = true, length = 96)
    private String recordingId;

    @ManyToOne(fetch = FetchType.LAZY, optional = false)
    @JoinColumn(name = "event_id", nullable = false)
    @JsonIgnore
    private Event press;

    @Column(nullable = false)
    private String audioKey;

    @Column(nullable = false)
    private Integer durationMs;

    @Column(nullable = false)
    private LocalDateTime createdAt;

    protected VisitorRecording() {}

    public VisitorRecording(String recordingId, Event press, String audioKey,
                            Integer durationMs, LocalDateTime createdAt) {
        this.recordingId = recordingId;
        this.press = press;
        this.audioKey = audioKey;
        this.durationMs = durationMs;
        this.createdAt = createdAt;
    }

    public Long getId() { return id; }
    public String getRecordingId() { return recordingId; }
    public Event getPress() { return press; }
    public String getAudioKey() { return audioKey; }
    public Integer getDurationMs() { return durationMs; }
    public LocalDateTime getCreatedAt() { return createdAt; }
}

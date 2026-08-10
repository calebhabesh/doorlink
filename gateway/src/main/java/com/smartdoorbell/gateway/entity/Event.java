package com.smartdoorbell.gateway.entity;

import com.fasterxml.jackson.annotation.JsonIgnore;
import jakarta.persistence.*;
import java.time.LocalDateTime;
import java.util.ArrayList;
import java.util.List;

@Entity
@Table(name = "events")
public class Event {
    public static final String PENDING_IMAGE_KEY = "__PENDING__";
    @Id
    @GeneratedValue(strategy = GenerationType.IDENTITY)
    private Long id;

    @Column(unique = true, length = 64)
    private String eventId;

    @Column(nullable = false)
    private LocalDateTime timestamp;

    @Column(nullable = false)
    private String eventType;

    @Column(nullable = true)
    private String imageKey;

    @Column(nullable = true)
    private String audioKey;

    @ManyToOne(fetch = FetchType.LAZY)
    @JoinColumn(name = "visitor_session_id")
    @JsonIgnore
    private VisitorSession session;

    private Integer pressNumber;

    private Integer pressDurationMs;

    @Column(length = 24)
    private String lifecycleState;

    @OneToMany(mappedBy = "press", cascade = CascadeType.ALL, orphanRemoval = true,
            fetch = FetchType.EAGER)
    private List<VisitorRecording> visitorRecordings = new ArrayList<>();

    @OneToMany(mappedBy = "event", cascade = CascadeType.ALL, orphanRemoval = true)
    @JsonIgnore
    private List<IntercomMessage> intercomMessages = new ArrayList<>();

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
    public List<IntercomMessage> getIntercomMessages() { return intercomMessages; }
    public VisitorSession getSession() { return session; }
    public Integer getPressNumber() { return pressNumber; }
    public Integer getPressDurationMs() { return pressDurationMs; }
    public String getLifecycleState() { return lifecycleState; }
    public List<VisitorRecording> getVisitorRecordings() { return visitorRecordings; }

    public void assignToSession(VisitorSession session, Integer pressNumber) {
        this.session = session;
        this.pressNumber = pressNumber;
    }

    public void markTriggered() { this.lifecycleState = "PHOTO_PENDING"; }
    public void setImageKey(String imageKey) { this.imageKey = imageKey; }
    public void setAudioKey(String audioKey) { this.audioKey = audioKey; }
    public void addVisitorRecording(VisitorRecording recording) {
        if (recording != null && visitorRecordings.stream().noneMatch(existing ->
                existing.getRecordingId().equals(recording.getRecordingId()))) {
            visitorRecordings.add(recording);
        }
    }
    public void markReady() { this.lifecycleState = "READY"; }
    public void completePress(Integer durationMs) {
        this.pressDurationMs = durationMs;
        this.lifecycleState = durationMs != null && durationMs < 1000
                ? "SHORT_PRESS" : "AUDIO_UPLOADING";
    }
}

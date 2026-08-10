package com.smartdoorbell.gateway.entity;

import jakarta.persistence.*;
import java.time.LocalDateTime;

@Entity
@Table(name = "visitor_sessions")
public class VisitorSession {
    @Id
    @GeneratedValue(strategy = GenerationType.IDENTITY)
    private Long id;

    @Column(nullable = false, unique = true, length = 64)
    private String sessionId;

    @Column(nullable = false)
    private LocalDateTime startedAt;

    @Column(nullable = false)
    private LocalDateTime lastActivityAt;

    private LocalDateTime closedAt;

    @Column(nullable = false, length = 16)
    private String status;

    protected VisitorSession() {}

    public VisitorSession(String sessionId, LocalDateTime startedAt) {
        this.sessionId = sessionId;
        this.startedAt = startedAt;
        this.lastActivityAt = startedAt;
        this.status = "ACTIVE";
    }

    public Long getId() { return id; }
    public String getSessionId() { return sessionId; }
    public LocalDateTime getStartedAt() { return startedAt; }
    public LocalDateTime getLastActivityAt() { return lastActivityAt; }
    public LocalDateTime getClosedAt() { return closedAt; }
    public String getStatus() { return status; }

    public void touch(LocalDateTime at) {
        if (at != null && at.isAfter(lastActivityAt)) lastActivityAt = at;
    }

    public void close(LocalDateTime at) {
        touch(at);
        closedAt = at;
        status = "CLOSED";
    }
}

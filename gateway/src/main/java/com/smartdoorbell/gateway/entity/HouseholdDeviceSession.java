package com.smartdoorbell.gateway.entity;

import com.fasterxml.jackson.annotation.JsonIgnore;
import jakarta.persistence.*;

import java.time.LocalDateTime;

@Entity
@Table(name = "household_device_sessions", indexes = {
        @Index(name = "idx_household_device_token", columnList = "tokenHash", unique = true)
})
public class HouseholdDeviceSession {

    @Id
    @GeneratedValue(strategy = GenerationType.IDENTITY)
    private Long id;

    @JsonIgnore
    @ManyToOne(fetch = FetchType.LAZY, optional = false)
    private HouseholdMember member;

    @Column(nullable = false, length = 100)
    private String deviceName;

    @JsonIgnore
    @Column(nullable = false, unique = true, length = 64)
    private String tokenHash;

    @Column(nullable = false)
    private LocalDateTime createdAt;

    @Column(nullable = false)
    private LocalDateTime lastSeenAt;

    @Column(nullable = false)
    private LocalDateTime expiresAt;

    private LocalDateTime revokedAt;

    @Column(length = 255)
    private String userAgent;

    @Column(length = 64)
    private String lastIp;

    protected HouseholdDeviceSession() {}

    public HouseholdDeviceSession(HouseholdMember member, String deviceName,
                                  String tokenHash, LocalDateTime now,
                                  LocalDateTime expiresAt, String userAgent,
                                  String lastIp) {
        this.member = member;
        this.deviceName = deviceName;
        this.tokenHash = tokenHash;
        this.createdAt = now;
        this.lastSeenAt = now;
        this.expiresAt = expiresAt;
        this.userAgent = userAgent;
        this.lastIp = lastIp;
    }

    public Long getId() { return id; }
    public HouseholdMember getMember() { return member; }
    public String getDeviceName() { return deviceName; }
    public String getTokenHash() { return tokenHash; }
    public LocalDateTime getCreatedAt() { return createdAt; }
    public LocalDateTime getLastSeenAt() { return lastSeenAt; }
    public LocalDateTime getExpiresAt() { return expiresAt; }
    public LocalDateTime getRevokedAt() { return revokedAt; }
    public String getUserAgent() { return userAgent; }
    public String getLastIp() { return lastIp; }

    public boolean isActive(LocalDateTime now) {
        return revokedAt == null && expiresAt.isAfter(now) && member.isEnabled();
    }

    public void touch(LocalDateTime now, LocalDateTime expiresAt, String ip) {
        this.lastSeenAt = now;
        this.expiresAt = expiresAt;
        this.lastIp = ip;
    }

    public void rename(String deviceName) {
        this.deviceName = deviceName;
    }

    public void revoke(LocalDateTime at) { this.revokedAt = at; }
}

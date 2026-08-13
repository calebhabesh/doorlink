package com.smartdoorbell.gateway.entity;

import com.fasterxml.jackson.annotation.JsonIgnore;
import jakarta.persistence.*;

import java.time.LocalDateTime;

@Entity
@Table(name = "household_invitations", indexes = {
        @Index(name = "idx_household_invitation_token", columnList = "tokenHash", unique = true)
})
public class HouseholdInvitation {

    @Id
    @GeneratedValue(strategy = GenerationType.IDENTITY)
    private Long id;

    @ManyToOne(fetch = FetchType.LAZY, optional = false)
    private HouseholdMember member;

    @JsonIgnore
    @Column(nullable = false, unique = true, length = 64)
    private String tokenHash;

    @Column(nullable = false)
    private LocalDateTime createdAt;

    @Column(nullable = false)
    private LocalDateTime expiresAt;

    private LocalDateTime usedAt;

    protected HouseholdInvitation() {}

    public HouseholdInvitation(HouseholdMember member, String tokenHash,
                               LocalDateTime createdAt, LocalDateTime expiresAt) {
        this.member = member;
        this.tokenHash = tokenHash;
        this.createdAt = createdAt;
        this.expiresAt = expiresAt;
    }

    public Long getId() { return id; }
    public HouseholdMember getMember() { return member; }
    public String getTokenHash() { return tokenHash; }
    public LocalDateTime getCreatedAt() { return createdAt; }
    public LocalDateTime getExpiresAt() { return expiresAt; }
    public LocalDateTime getUsedAt() { return usedAt; }

    public boolean isUsable(LocalDateTime now) {
        return usedAt == null && expiresAt.isAfter(now) && member.isEnabled();
    }

    public void use(LocalDateTime at) { this.usedAt = at; }
}

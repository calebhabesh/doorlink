package com.smartdoorbell.gateway.entity;

import com.fasterxml.jackson.annotation.JsonIgnore;
import jakarta.persistence.*;

import java.time.LocalDateTime;
import java.util.ArrayList;
import java.util.List;

@Entity
@Table(name = "household_members")
public class HouseholdMember {

    public enum Role { OWNER, MEMBER }

    @Id
    @GeneratedValue(strategy = GenerationType.IDENTITY)
    private Long id;

    @Column(nullable = false, length = 100)
    private String name;

    @Column(nullable = false, length = 254)
    private String email;

    @Enumerated(EnumType.STRING)
    @Column(nullable = false, length = 16)
    private Role role;

    @Column(nullable = false)
    private LocalDateTime createdAt;

    private LocalDateTime disabledAt;

    @JsonIgnore
    @OneToMany(mappedBy = "member")
    private List<HouseholdDeviceSession> devices = new ArrayList<>();

    protected HouseholdMember() {}

    public HouseholdMember(String name, String email, Role role, LocalDateTime createdAt) {
        this.name = name;
        this.email = email;
        this.role = role;
        this.createdAt = createdAt;
    }

    public Long getId() { return id; }
    public String getName() { return name; }
    public String getEmail() { return email; }
    public Role getRole() { return role; }
    public LocalDateTime getCreatedAt() { return createdAt; }
    public LocalDateTime getDisabledAt() { return disabledAt; }
    public List<HouseholdDeviceSession> getDevices() { return devices; }
    public boolean isEnabled() { return disabledAt == null; }

    public void disable(LocalDateTime at) { this.disabledAt = at; }
}

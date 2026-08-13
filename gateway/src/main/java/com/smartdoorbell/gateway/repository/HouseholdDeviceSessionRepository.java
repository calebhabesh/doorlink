package com.smartdoorbell.gateway.repository;

import com.smartdoorbell.gateway.entity.HouseholdDeviceSession;
import com.smartdoorbell.gateway.entity.HouseholdMember;
import org.springframework.data.jpa.repository.JpaRepository;

import java.util.List;
import java.util.Optional;

public interface HouseholdDeviceSessionRepository extends JpaRepository<HouseholdDeviceSession, Long> {
    Optional<HouseholdDeviceSession> findByTokenHash(String tokenHash);
    List<HouseholdDeviceSession> findByMemberOrderByCreatedAtDesc(HouseholdMember member);
}

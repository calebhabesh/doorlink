package com.smartdoorbell.gateway.repository;

import com.smartdoorbell.gateway.entity.HouseholdInvitation;
import org.springframework.data.jpa.repository.JpaRepository;
import org.springframework.data.jpa.repository.Lock;

import jakarta.persistence.LockModeType;

import java.util.Optional;

public interface HouseholdInvitationRepository extends JpaRepository<HouseholdInvitation, Long> {
    Optional<HouseholdInvitation> findByTokenHash(String tokenHash);

    @Lock(LockModeType.PESSIMISTIC_WRITE)
    Optional<HouseholdInvitation> findForUpdateByTokenHash(String tokenHash);
}

package com.smartdoorbell.gateway.repository;

import com.smartdoorbell.gateway.entity.DeviceStatus;
import org.springframework.data.jpa.repository.JpaRepository;

import java.util.Optional;

public interface DeviceStatusRepository extends JpaRepository<DeviceStatus, String> {
    Optional<DeviceStatus> findTopByOrderByLastSeenDesc();
}

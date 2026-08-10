package com.smartdoorbell.gateway.repository;

import com.smartdoorbell.gateway.entity.VisitorRecording;
import org.springframework.data.jpa.repository.JpaRepository;

import java.util.Optional;

public interface VisitorRecordingRepository extends JpaRepository<VisitorRecording, Long> {
    Optional<VisitorRecording> findByRecordingId(String recordingId);
}

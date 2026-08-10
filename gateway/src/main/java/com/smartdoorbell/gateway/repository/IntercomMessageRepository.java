package com.smartdoorbell.gateway.repository;

import com.smartdoorbell.gateway.entity.IntercomMessage;
import org.springframework.data.jpa.repository.JpaRepository;

import java.util.Optional;

public interface IntercomMessageRepository extends JpaRepository<IntercomMessage, Long> {
    Optional<IntercomMessage> findByMessageId(String messageId);
}

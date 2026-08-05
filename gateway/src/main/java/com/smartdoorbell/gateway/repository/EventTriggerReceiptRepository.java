package com.smartdoorbell.gateway.repository;

import com.smartdoorbell.gateway.entity.EventTriggerReceipt;
import org.springframework.data.jpa.repository.JpaRepository;

public interface EventTriggerReceiptRepository extends JpaRepository<EventTriggerReceipt, String> {
}

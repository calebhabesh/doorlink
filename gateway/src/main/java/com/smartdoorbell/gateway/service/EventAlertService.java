package com.smartdoorbell.gateway.service;

import com.smartdoorbell.gateway.entity.EventTriggerReceipt;
import com.smartdoorbell.gateway.repository.EventTriggerReceiptRepository;
import org.springframework.stereotype.Service;
import org.springframework.transaction.annotation.Transactional;

import java.time.LocalDateTime;
import java.util.Optional;

@Service
public class EventAlertService {
    private final EventTriggerReceiptRepository receiptRepository;
    private final ChimeService chimeService;
    private final NtfyService ntfyService;

    public EventAlertService(EventTriggerReceiptRepository receiptRepository,
                             ChimeService chimeService,
                             NtfyService ntfyService) {
        this.receiptRepository = receiptRepository;
        this.chimeService = chimeService;
        this.ntfyService = ntfyService;
    }

    @Transactional
    public synchronized TriggerOutcome trigger(String eventId, String eventType) {
        Optional<EventTriggerReceipt> existing = receiptRepository.findById(eventId);
        if (existing.isPresent()) {
            return new TriggerOutcome(eventId, false, existing.get().getTriggeredAt());
        }

        LocalDateTime triggeredAt = LocalDateTime.now();
        EventTriggerReceipt receipt =
                new EventTriggerReceipt(eventId, eventType, triggeredAt);

        // Dispatch immediately so Home Assistant chime webhook fires without waiting for DB persistence
        dispatch(eventType);
        receiptRepository.saveAndFlush(receipt);
        return new TriggerOutcome(eventId, true, triggeredAt);
    }

    @Transactional
    public synchronized boolean completeUpload(String eventId, String eventType) {
        if (eventId != null) {
            Optional<EventTriggerReceipt> receipt = receiptRepository.findById(eventId);
            if (receipt.isPresent()) {
                receipt.get().markUploaded(LocalDateTime.now());
                receiptRepository.save(receipt.get());
                return false;
            }
            LocalDateTime now = LocalDateTime.now();
            dispatch(eventType);
            EventTriggerReceipt fallbackReceipt =
                    new EventTriggerReceipt(eventId, eventType, now);
            fallbackReceipt.markUploaded(now);
            receiptRepository.save(fallbackReceipt);
            return true;
        }

        dispatch(eventType);
        return true;
    }

    private void dispatch(String eventType) {
        chimeService.ring(eventType);
        ntfyService.sendNotification(eventType);
    }

    public record TriggerOutcome(String eventId, boolean created,
                                 LocalDateTime triggeredAt) {}
}

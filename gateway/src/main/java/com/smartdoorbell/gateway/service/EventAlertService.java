package com.smartdoorbell.gateway.service;

import com.smartdoorbell.gateway.entity.EventTriggerReceipt;
import com.smartdoorbell.gateway.repository.EventTriggerReceiptRepository;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.springframework.stereotype.Service;
import org.springframework.transaction.annotation.Transactional;

import java.time.LocalDateTime;
import java.util.Optional;

@Service
public class EventAlertService {
    private static final Logger logger = LoggerFactory.getLogger(EventAlertService.class);

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
        return trigger(eventId, eventType, true);
    }

    @Transactional
    public synchronized TriggerOutcome trigger(String eventId, String eventType,
                                               boolean dispatchAlerts) {
        Optional<EventTriggerReceipt> existing = receiptRepository.findById(eventId);
        if (existing.isPresent()) {
            return new TriggerOutcome(eventId, false, existing.get().getTriggeredAt());
        }

        LocalDateTime triggeredAt = LocalDateTime.now();
        EventTriggerReceipt receipt =
                new EventTriggerReceipt(eventId, eventType, triggeredAt);

        // A stale firmware lifecycle event is persisted with a receipt but
        // can never turn into a trailing chime after the cooldown has elapsed.
        if (dispatchAlerts) {
            dispatch(eventType);
        } else {
            logger.info(
                    "Persisting stale {} trigger without dispatching alerts", eventType);
        }
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
            // Upload is a last-resort alert path only for the initial press.
            // A repress must be evaluated at its physical edge; falling back
            // here would manufacture the delayed/queued chime we prohibit.
            boolean dispatchFallback = !"DOORBELL_REPRESS".equals(eventType);
            if (dispatchFallback) {
                dispatch(eventType);
            }
            EventTriggerReceipt fallbackReceipt =
                    new EventTriggerReceipt(eventId, eventType, now);
            fallbackReceipt.markUploaded(now);
            receiptRepository.save(fallbackReceipt);
            return dispatchFallback;
        }

        boolean dispatchFallback = !"DOORBELL_REPRESS".equals(eventType);
        if (dispatchFallback) {
            dispatch(eventType);
        }
        return dispatchFallback;
    }

    private void dispatch(String eventType) {
        chimeService.ring(eventType);
        ntfyService.sendNotification(eventType);
    }

    public record TriggerOutcome(String eventId, boolean created,
                                 LocalDateTime triggeredAt) {}
}

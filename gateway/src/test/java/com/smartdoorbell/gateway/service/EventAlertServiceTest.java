package com.smartdoorbell.gateway.service;

import com.smartdoorbell.gateway.entity.EventTriggerReceipt;
import com.smartdoorbell.gateway.repository.EventTriggerReceiptRepository;
import org.junit.jupiter.api.BeforeEach;
import org.junit.jupiter.api.Test;
import org.junit.jupiter.api.extension.ExtendWith;
import org.mockito.Mock;
import org.mockito.junit.jupiter.MockitoExtension;

import java.time.LocalDateTime;
import java.util.Optional;

import static org.junit.jupiter.api.Assertions.assertFalse;
import static org.junit.jupiter.api.Assertions.assertTrue;
import static org.mockito.ArgumentMatchers.any;
import static org.mockito.Mockito.never;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;

@ExtendWith(MockitoExtension.class)
class EventAlertServiceTest {
    @Mock
    private EventTriggerReceiptRepository receiptRepository;
    @Mock
    private ChimeService chimeService;
    @Mock
    private NtfyService ntfyService;

    private EventAlertService service;

    @BeforeEach
    void setUp() {
        service = new EventAlertService(receiptRepository, chimeService, ntfyService);
    }

    @Test
    void firstTriggerPersistsReceiptBeforeDispatching() {
        String eventId = "0123456789abcdef0123456789abcdef";
        when(receiptRepository.findById(eventId)).thenReturn(Optional.empty());
        when(receiptRepository.saveAndFlush(any())).thenAnswer(invocation -> invocation.getArgument(0));

        EventAlertService.TriggerOutcome outcome =
                service.trigger(eventId, "DOORBELL_PRESS");

        assertTrue(outcome.created());
        verify(receiptRepository).saveAndFlush(any(EventTriggerReceipt.class));
        verify(chimeService).ring("DOORBELL_PRESS");
        verify(ntfyService).sendNotification("DOORBELL_PRESS");
    }

    @Test
    void repeatedTriggerDoesNotDispatchAgain() {
        String eventId = "0123456789abcdef0123456789abcdef";
        EventTriggerReceipt receipt = new EventTriggerReceipt(
                eventId, "DOORBELL_PRESS", LocalDateTime.now());
        when(receiptRepository.findById(eventId)).thenReturn(Optional.of(receipt));

        EventAlertService.TriggerOutcome outcome =
                service.trigger(eventId, "DOORBELL_PRESS");

        assertFalse(outcome.created());
        verify(chimeService, never()).ring(any());
        verify(ntfyService, never()).sendNotification(any());
    }

    @Test
    void suppressedDelayedTriggerPersistsReceiptWithoutDispatching() {
        String eventId = "0123456789abcdef0123456789abcdef-2";
        when(receiptRepository.findById(eventId)).thenReturn(Optional.empty());
        when(receiptRepository.saveAndFlush(any()))
                .thenAnswer(invocation -> invocation.getArgument(0));

        EventAlertService.TriggerOutcome outcome =
                service.trigger(eventId, "DOORBELL_REPRESS", false);

        assertTrue(outcome.created());
        verify(receiptRepository).saveAndFlush(any(EventTriggerReceipt.class));
        verify(chimeService, never()).ring(any());
        verify(ntfyService, never()).sendNotification(any());
    }

    @Test
    void differentPressesInOneSessionEachDispatch() {
        String sessionId = "0123456789abcdef0123456789abcdef";
        String firstPressId = sessionId + "-1";
        String secondPressId = sessionId + "-2";
        when(receiptRepository.findById(firstPressId)).thenReturn(Optional.empty());
        when(receiptRepository.findById(secondPressId)).thenReturn(Optional.empty());
        when(receiptRepository.saveAndFlush(any()))
                .thenAnswer(invocation -> invocation.getArgument(0));

        assertTrue(service.trigger(firstPressId, "DOORBELL_PRESS").created());
        assertTrue(service.trigger(secondPressId, "DOORBELL_REPRESS").created());

        verify(chimeService).ring("DOORBELL_PRESS");
        verify(chimeService).ring("DOORBELL_REPRESS");
        verify(ntfyService).sendNotification("DOORBELL_PRESS");
        verify(ntfyService).sendNotification("DOORBELL_REPRESS");
    }

    @Test
    void uploadWithReceiptSuppressesFallback() {
        String eventId = "0123456789abcdef0123456789abcdef";
        EventTriggerReceipt receipt = new EventTriggerReceipt(
                eventId, "DOORBELL_PRESS", LocalDateTime.now());
        when(receiptRepository.findById(eventId)).thenReturn(Optional.of(receipt));

        assertFalse(service.completeUpload(eventId, "DOORBELL_PRESS"));
        verify(chimeService, never()).ring(any());
        verify(ntfyService, never()).sendNotification(any());
        verify(receiptRepository).save(receipt);
    }

    @Test
    void uploadWithoutReceiptDispatchesFallback() {
        String eventId = "0123456789abcdef0123456789abcdef";
        when(receiptRepository.findById(eventId)).thenReturn(Optional.empty());

        assertTrue(service.completeUpload(eventId, "DOORBELL_PRESS"));
        verify(chimeService).ring("DOORBELL_PRESS");
        verify(ntfyService).sendNotification("DOORBELL_PRESS");
        verify(receiptRepository).save(any(EventTriggerReceipt.class));
    }

    @Test
    void repressUploadWithoutReceiptNeverCreatesDelayedFallbackChime() {
        String eventId = "0123456789abcdef0123456789abcdef-2";
        when(receiptRepository.findById(eventId)).thenReturn(Optional.empty());

        assertFalse(service.completeUpload(eventId, "DOORBELL_REPRESS"));
        verify(chimeService, never()).ring(any());
        verify(ntfyService, never()).sendNotification(any());
        verify(receiptRepository).save(any(EventTriggerReceipt.class));
    }
}

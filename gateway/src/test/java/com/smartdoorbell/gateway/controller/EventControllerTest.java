package com.smartdoorbell.gateway.controller;

import com.smartdoorbell.gateway.config.MqttGateway;
import com.smartdoorbell.gateway.entity.Event;
import com.smartdoorbell.gateway.repository.EventRepository;
import com.smartdoorbell.gateway.repository.VisitorRecordingRepository;
import com.smartdoorbell.gateway.repository.VisitorSessionRepository;
import com.smartdoorbell.gateway.service.DeviceTelemetryService;
import com.smartdoorbell.gateway.service.EventAlertService;
import com.smartdoorbell.gateway.service.MinioService;
import com.smartdoorbell.gateway.service.VisitorSessionService;
import com.smartdoorbell.gateway.service.HouseholdAuthService;
import com.smartdoorbell.gateway.entity.VisitorSession;
import org.junit.jupiter.api.Test;
import org.junit.jupiter.api.BeforeEach;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.boot.test.autoconfigure.web.servlet.WebMvcTest;
import org.springframework.boot.test.mock.mockito.MockBean;
import org.springframework.mock.web.MockMultipartFile;
import org.springframework.test.web.servlet.MockMvc;

import java.time.LocalDateTime;
import java.util.Optional;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;

import static org.mockito.ArgumentMatchers.any;
import static org.mockito.ArgumentMatchers.anyString;
import static org.mockito.ArgumentMatchers.eq;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.never;
import static org.mockito.Mockito.when;
import static org.springframework.test.web.servlet.request.MockMvcRequestBuilders.multipart;
import static org.springframework.test.web.servlet.request.MockMvcRequestBuilders.post;
import static org.springframework.test.web.servlet.request.MockMvcRequestBuilders.get;
import static org.springframework.test.web.servlet.result.MockMvcResultMatchers.jsonPath;
import static org.springframework.test.web.servlet.result.MockMvcResultMatchers.status;
import static org.junit.jupiter.api.Assertions.assertEquals;

@WebMvcTest(controllers = EventController.class, properties = "gateway.api.key=test-api-key")
public class EventControllerTest {

    @Autowired
    private MockMvc mockMvc;

    @MockBean
    private MinioService minioService;

    @MockBean
    private EventRepository eventRepository;

    @MockBean
    private VisitorSessionRepository visitorSessionRepository;

    @MockBean
    private VisitorRecordingRepository visitorRecordingRepository;

    @MockBean
    private MqttGateway mqttGateway;
    
    @MockBean
    private EventAlertService eventAlertService;

    @MockBean
    private DeviceTelemetryService deviceTelemetryService;

    @MockBean
    private VisitorSessionService visitorSessionService;

    @MockBean
    private HouseholdAuthService householdAuthService;

    @BeforeEach
    void persistSessions() {
        when(visitorSessionRepository.save(any(VisitorSession.class)))
                .thenAnswer(invocation -> invocation.getArgument(0));
    }

    @Test
    public void testUploadEventWithAudio() throws Exception {
        MockMultipartFile imageFile = new MockMultipartFile(
                "image",
                "test.jpg",
                "image/jpeg",
                "test image content".getBytes()
        );
        
        MockMultipartFile audioFile = new MockMultipartFile(
                "audio",
                "test.wav",
                "audio/wav",
                wav(1200)
        );

        when(minioService.uploadFile(imageFile,
                "0123456789abcdef0123456789abcdef-image"))
                .thenReturn("random-uuid.jpg");
        when(minioService.uploadFile(audioFile,
                "0123456789abcdef0123456789abcdef-visitor"))
                .thenReturn("random-uuid.wav");
        when(eventRepository.save(any())).thenReturn(new Event());

        mockMvc.perform(multipart("/api/events")
                .file(imageFile)
                .file(audioFile)
                .param("eventType", "DOORBELL_PRESS")
                .param("eventId", "0123456789abcdef0123456789abcdef")
                .param("deviceId", "front-door")
                .param("firmwareVersion", "1.0.0")
                .param("wifiRssiDbm", "-54")
                .header("X-API-Key", "test-api-key"))
                .andExpect(status().isOk());

        verify(minioService).uploadFile(imageFile,
                "0123456789abcdef0123456789abcdef-image");
        verify(minioService).uploadFile(audioFile,
                "0123456789abcdef0123456789abcdef-visitor");
        verify(mqttGateway).sendToMqtt(anyString(), eq("doorbell/events"));
        verify(eventAlertService).completeUpload(
                "0123456789abcdef0123456789abcdef", "DOORBELL_PRESS");
        verify(deviceTelemetryService).record("front-door", "1.0.0",
                "DOORBELL_PRESS", "0123456789abcdef0123456789abcdef", -54);
    }

    private static byte[] wav(int durationMs) {
        int dataLength = durationMs * 32;
        ByteBuffer wav = ByteBuffer.allocate(44 + dataLength).order(ByteOrder.LITTLE_ENDIAN);
        wav.put("RIFF".getBytes()).putInt(36 + dataLength).put("WAVE".getBytes());
        wav.put("fmt ".getBytes()).putInt(16).putShort((short) 1).putShort((short) 1);
        wav.putInt(16000).putInt(32000).putShort((short) 2).putShort((short) 16);
        wav.put("data".getBytes()).putInt(dataLength);
        return wav.array();
    }

    @Test
    public void testAuthenticatedEarlyTrigger() throws Exception {
        LocalDateTime triggeredAt = LocalDateTime.of(2026, 8, 5, 12, 0);
        when(eventAlertService.trigger(
                "0123456789abcdef0123456789abcdef", "DOORBELL_PRESS", true))
                .thenReturn(new EventAlertService.TriggerOutcome(
                        "0123456789abcdef0123456789abcdef", true, triggeredAt));

        mockMvc.perform(post("/api/events/trigger")
                .contentType("application/json")
                .content("""
                        {"eventId":"0123456789abcdef0123456789abcdef",
                         "eventType":"DOORBELL_PRESS","deviceId":"front-door",
                         "firmwareVersion":"1.0.0","wifiRssiDbm":-52}
                        """)
                .header("X-API-Key", "test-api-key"))
                .andExpect(status().isOk())
                .andExpect(jsonPath("$.created").value(true));

        verify(deviceTelemetryService).record("front-door", "1.0.0",
                "DOORBELL_PRESS", "0123456789abcdef0123456789abcdef", -52);
    }

    @Test
    public void testRepressTriggersWholeHomeChimeByPressId() throws Exception {
        String sessionId = "0123456789abcdef0123456789abcdef";
        String pressId = sessionId + "-2";
        LocalDateTime triggeredAt = LocalDateTime.of(2026, 8, 16, 7, 0);
        when(eventAlertService.trigger(pressId, "DOORBELL_REPRESS", true))
                .thenReturn(new EventAlertService.TriggerOutcome(
                        pressId, true, triggeredAt));

        mockMvc.perform(post("/api/events/trigger")
                .contentType("application/json")
                .content("""
                        {"eventId":"%s","eventType":"DOORBELL_REPRESS",
                         "deviceId":"front-door","firmwareVersion":"1.0.0"}
                        """.formatted(pressId))
                .header("X-API-Key", "test-api-key"))
                .andExpect(status().isOk())
                .andExpect(jsonPath("$.sessionId").value(sessionId));

        verify(eventAlertService).trigger(pressId, "DOORBELL_REPRESS", true);
    }

    @Test
    public void testDelayedRepressIsPersistedWithoutDispatchingAlerts() throws Exception {
        String sessionId = "0123456789abcdef0123456789abcdef";
        String pressId = sessionId + "-3";
        LocalDateTime triggeredAt = LocalDateTime.of(2026, 8, 16, 7, 1);
        when(eventAlertService.trigger(pressId, "DOORBELL_REPRESS", false))
                .thenReturn(new EventAlertService.TriggerOutcome(
                        pressId, true, triggeredAt));

        mockMvc.perform(post("/api/events/trigger")
                .contentType("application/json")
                .content("""
                        {"eventId":"%s","eventType":"DOORBELL_REPRESS",
                         "deviceId":"front-door","firmwareVersion":"1.0.0",
                         "dispatchAlerts":false}
                        """.formatted(pressId))
                .header("X-API-Key", "test-api-key"))
                .andExpect(status().isOk())
                .andExpect(jsonPath("$.sessionId").value(sessionId));

        verify(eventAlertService).trigger(pressId, "DOORBELL_REPRESS", false);
    }

    @Test
    public void testEarlyTriggerRequiresApiKey() throws Exception {
        mockMvc.perform(post("/api/events/trigger")
                .contentType("application/json")
                .content("""
                        {"eventId":"0123456789abcdef0123456789abcdef",
                         "eventType":"DOORBELL_PRESS"}
                        """))
                .andExpect(status().isUnauthorized());
    }

    @Test
    public void testEventHistoryRequiresAnEnrolledBrowser() throws Exception {
        mockMvc.perform(get("/api/events"))
                .andExpect(status().isUnauthorized());
    }

    @Test
    public void testRepeatedUploadReturnsExistingEventWithoutStoringOrNotifying() throws Exception {
        String eventId = "0123456789abcdef0123456789abcdef";
        Event existing = new Event(eventId, LocalDateTime.now(),
                "DOORBELL_PRESS", "existing-image.jpg", null);
        when(eventRepository.findByEventId(eventId))
                .thenReturn(Optional.of(existing));
        MockMultipartFile imageFile = new MockMultipartFile(
                "image", "test.jpg", "image/jpeg", "jpeg".getBytes());

        mockMvc.perform(multipart("/api/events")
                .file(imageFile)
                .param("eventType", "DOORBELL_PRESS")
                .param("eventId", eventId)
                .header("X-API-Key", "test-api-key"))
                .andExpect(status().isOk());

        verify(minioService, never()).uploadFile(any(), any());
        verify(eventAlertService, never()).completeUpload(any(), any());
    }

    @Test
    public void testEarlyTriggerRejectsNonProductionEventType() throws Exception {
        mockMvc.perform(post("/api/events/trigger")
                .contentType("application/json")
                .content("""
                        {"eventId":"0123456789abcdef0123456789abcdef",
                         "eventType":"DIAGNOSTIC_UPLOAD"}
                        """)
                .header("X-API-Key", "test-api-key"))
                .andExpect(status().isBadRequest());
    }

    @Test
    public void testShortPressCompletesWithoutCreatingAudio() throws Exception {
        String pressId = "0123456789abcdef0123456789abcdef-2";
        Event press = new Event(pressId, LocalDateTime.now(),
                "DOORBELL_REPRESS", Event.PENDING_IMAGE_KEY, null);
        when(eventRepository.findByEventId(pressId)).thenReturn(Optional.of(press));

        mockMvc.perform(post("/api/events/presses/{pressId}/complete", pressId)
                        .contentType("application/json")
                        .content("{\"durationMs\":640}")
                        .header("X-API-Key", "test-api-key"))
                .andExpect(status().isOk());

        assertEquals("SHORT_PRESS", press.getLifecycleState());
        assertEquals(640, press.getPressDurationMs());
        verify(eventRepository).save(press);
        verify(minioService, never()).uploadFile(any(), any());
    }
}

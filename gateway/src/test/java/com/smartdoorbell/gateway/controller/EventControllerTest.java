package com.smartdoorbell.gateway.controller;

import com.smartdoorbell.gateway.config.MqttGateway;
import com.smartdoorbell.gateway.entity.Event;
import com.smartdoorbell.gateway.repository.EventRepository;
import com.smartdoorbell.gateway.service.DeviceTelemetryService;
import com.smartdoorbell.gateway.service.EventAlertService;
import com.smartdoorbell.gateway.service.MinioService;
import org.junit.jupiter.api.Test;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.boot.test.autoconfigure.web.servlet.WebMvcTest;
import org.springframework.boot.test.mock.mockito.MockBean;
import org.springframework.mock.web.MockMultipartFile;
import org.springframework.test.web.servlet.MockMvc;

import java.time.LocalDateTime;
import java.util.Optional;

import static org.mockito.ArgumentMatchers.any;
import static org.mockito.ArgumentMatchers.anyString;
import static org.mockito.ArgumentMatchers.eq;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.never;
import static org.mockito.Mockito.when;
import static org.springframework.test.web.servlet.request.MockMvcRequestBuilders.multipart;
import static org.springframework.test.web.servlet.request.MockMvcRequestBuilders.post;
import static org.springframework.test.web.servlet.result.MockMvcResultMatchers.jsonPath;
import static org.springframework.test.web.servlet.result.MockMvcResultMatchers.status;

@WebMvcTest(controllers = EventController.class, properties = "gateway.api.key=test-api-key")
public class EventControllerTest {

    @Autowired
    private MockMvc mockMvc;

    @MockBean
    private MinioService minioService;

    @MockBean
    private EventRepository eventRepository;

    @MockBean
    private MqttGateway mqttGateway;
    
    @MockBean
    private EventAlertService eventAlertService;

    @MockBean
    private DeviceTelemetryService deviceTelemetryService;

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
                "test audio content".getBytes()
        );

        when(minioService.uploadFile(imageFile,
                "0123456789abcdef0123456789abcdef-image"))
                .thenReturn("random-uuid.jpg");
        when(minioService.uploadFile(audioFile,
                "0123456789abcdef0123456789abcdef-audio"))
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
                "0123456789abcdef0123456789abcdef-audio");
        verify(mqttGateway).sendToMqtt(anyString(), eq("doorbell/events"));
        verify(eventAlertService).completeUpload(
                "0123456789abcdef0123456789abcdef", "DOORBELL_PRESS");
        verify(deviceTelemetryService).record("front-door", "1.0.0",
                "DOORBELL_PRESS", "0123456789abcdef0123456789abcdef", -54);
    }

    @Test
    public void testAuthenticatedEarlyTrigger() throws Exception {
        LocalDateTime triggeredAt = LocalDateTime.of(2026, 8, 5, 12, 0);
        when(eventAlertService.trigger(
                "0123456789abcdef0123456789abcdef", "DOORBELL_PRESS"))
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
}

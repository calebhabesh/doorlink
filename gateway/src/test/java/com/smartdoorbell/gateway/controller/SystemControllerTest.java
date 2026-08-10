package com.smartdoorbell.gateway.controller;

import com.smartdoorbell.gateway.config.MqttConfig;
import com.smartdoorbell.gateway.config.MqttGateway;
import com.smartdoorbell.gateway.entity.Event;
import com.smartdoorbell.gateway.repository.EventRepository;
import com.smartdoorbell.gateway.repository.IntercomMessageRepository;
import com.smartdoorbell.gateway.repository.SystemSettingsRepository;
import com.smartdoorbell.gateway.service.MinioService;
import com.smartdoorbell.gateway.service.SystemHealthService;
import org.junit.jupiter.api.BeforeEach;
import org.junit.jupiter.api.Test;
import org.mockito.ArgumentCaptor;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.boot.test.autoconfigure.web.servlet.WebMvcTest;
import org.springframework.boot.test.mock.mockito.MockBean;
import org.springframework.mock.web.MockMultipartFile;
import org.springframework.test.web.servlet.MockMvc;

import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.time.LocalDateTime;
import java.util.Optional;

import static org.junit.jupiter.api.Assertions.assertTrue;
import static org.mockito.ArgumentMatchers.any;
import static org.mockito.ArgumentMatchers.eq;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;
import static org.springframework.test.web.servlet.request.MockMvcRequestBuilders.multipart;
import static org.springframework.test.web.servlet.request.MockMvcRequestBuilders.post;
import static org.springframework.test.web.servlet.result.MockMvcResultMatchers.jsonPath;
import static org.springframework.test.web.servlet.result.MockMvcResultMatchers.status;

@WebMvcTest(controllers = SystemController.class, properties = {
        "gateway.api.key=test-api-key",
        "intercom.device-base-url=http://doorbell-gateway:8080"
})
class SystemControllerTest {
    private static final String EVENT_KEY = "0123456789abcdef0123456789abcdef-1";

    @Autowired
    private MockMvc mockMvc;

    @MockBean private SystemSettingsRepository settingsRepository;
    @MockBean private MqttGateway mqttGateway;
    @MockBean private MqttConfig mqttConfig;
    @MockBean private SystemHealthService systemHealthService;
    @MockBean private EventRepository eventRepository;
    @MockBean private IntercomMessageRepository intercomMessageRepository;
    @MockBean private MinioService minioService;

    @BeforeEach
    void configureActiveEvent() {
        Event event = new Event(EVENT_KEY, LocalDateTime.now(),
                "DOORBELL_PRESS", "image.jpg", "visitor.wav");
        when(eventRepository.findById(42L)).thenReturn(Optional.of(event));
        when(mqttConfig.getPttAudioTopic()).thenReturn("doorbell/commands/audio");
        when(intercomMessageRepository.save(any())).thenAnswer(invocation -> invocation.getArgument(0));
    }

    @Test
    void pttStartPublishesTargetedMuteCommand() throws Exception {
        mockMvc.perform(post("/api/system/ptt/start")
                        .contentType("application/json")
                        .content("{\"eventId\":42}")
                        .header("X-API-Key", "test-api-key"))
                .andExpect(status().isOk())
                .andExpect(jsonPath("$.state").value("RECORDING"));

        ArgumentCaptor<String> payload = ArgumentCaptor.forClass(String.class);
        verify(mqttGateway).sendToMqtt(payload.capture(), eq("doorbell/commands/audio"));
        assertTrue(payload.getValue().contains("\"type\":\"PTT_START\""));
        assertTrue(payload.getValue().contains(EVENT_KEY));
    }

    @Test
    void pttReplyStoresWavAndPublishesHttpPlaybackCommand() throws Exception {
        byte[] wav = oneSecondSilentWav();
        MockMultipartFile audio = new MockMultipartFile(
                "audio", "reply.wav", "audio/wav", wav);
        String storedKey = EVENT_KEY + "-reply-message.wav";
        when(minioService.uploadFile(eq(audio), any())).thenReturn(storedKey);

        mockMvc.perform(multipart("/api/system/ptt")
                        .file(audio)
                        .param("eventId", "42")
                        .param("durationMs", "1000")
                        .header("X-API-Key", "test-api-key"))
                .andExpect(status().isOk())
                .andExpect(jsonPath("$.state").value("PLAYBACK_QUEUED"))
                .andExpect(jsonPath("$.durationMs").value(1000));

        ArgumentCaptor<String> payload = ArgumentCaptor.forClass(String.class);
        verify(mqttGateway).sendToMqtt(payload.capture(), eq("doorbell/commands/audio"));
        assertTrue(payload.getValue().contains("\"type\":\"PLAY_AUDIO\""));
        assertTrue(payload.getValue().contains(
                "http://doorbell-gateway:8080/api/events/media/" + storedKey));
        verify(intercomMessageRepository).save(any());
    }

    @Test
    void pttRejectsNonWavPayloadBeforeStorage() throws Exception {
        MockMultipartFile audio = new MockMultipartFile(
                "audio", "reply.wav", "audio/wav", new byte[100]);

        mockMvc.perform(multipart("/api/system/ptt")
                        .file(audio)
                        .param("eventId", "42")
                        .param("durationMs", "1000")
                        .header("X-API-Key", "test-api-key"))
                .andExpect(status().isBadRequest());
    }

    private static byte[] oneSecondSilentWav() {
        int pcmLength = 16000 * 2;
        ByteBuffer wav = ByteBuffer.allocate(44 + pcmLength).order(ByteOrder.LITTLE_ENDIAN);
        wav.put("RIFF".getBytes()).putInt(36 + pcmLength).put("WAVE".getBytes());
        wav.put("fmt ".getBytes()).putInt(16).putShort((short) 1).putShort((short) 1);
        wav.putInt(16000).putInt(32000).putShort((short) 2).putShort((short) 16);
        wav.put("data".getBytes()).putInt(pcmLength);
        return wav.array();
    }
}

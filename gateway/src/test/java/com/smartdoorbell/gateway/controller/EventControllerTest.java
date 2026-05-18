package com.smartdoorbell.gateway.controller;

import com.smartdoorbell.gateway.config.MqttGateway;
import com.smartdoorbell.gateway.entity.Event;
import com.smartdoorbell.gateway.repository.EventRepository;
import com.smartdoorbell.gateway.service.ChimeService;
import com.smartdoorbell.gateway.service.MinioService;
import com.smartdoorbell.gateway.service.NtfyService;
import org.junit.jupiter.api.Test;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.boot.test.autoconfigure.web.servlet.WebMvcTest;
import org.springframework.boot.test.mock.mockito.MockBean;
import org.springframework.mock.web.MockMultipartFile;
import org.springframework.test.web.servlet.MockMvc;

import static org.mockito.ArgumentMatchers.any;
import static org.mockito.ArgumentMatchers.anyString;
import static org.mockito.ArgumentMatchers.eq;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;
import static org.springframework.test.web.servlet.request.MockMvcRequestBuilders.multipart;
import static org.springframework.test.web.servlet.result.MockMvcResultMatchers.status;

@WebMvcTest(EventController.class)
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
    private NtfyService ntfyService;

    @MockBean
    private ChimeService chimeService;

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

        when(minioService.uploadFile(imageFile)).thenReturn("random-uuid.jpg");
        when(minioService.uploadFile(audioFile)).thenReturn("random-uuid.wav");
        when(eventRepository.save(any())).thenReturn(new Event());

        mockMvc.perform(multipart("/api/events")
                .file(imageFile)
                .file(audioFile)
                .param("eventType", "DOORBELL_PRESS"))
                .andExpect(status().isOk());

        verify(minioService).uploadFile(imageFile);
        verify(minioService).uploadFile(audioFile);
        verify(mqttGateway).sendToMqtt(anyString(), eq("doorbell/events"));
        verify(ntfyService).sendNotification(any(Event.class));
    }
}


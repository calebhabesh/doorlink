package com.smartdoorbell.gateway.service;

import com.smartdoorbell.gateway.entity.Event;
import org.junit.jupiter.api.BeforeEach;
import org.junit.jupiter.api.Test;
import org.junit.jupiter.api.extension.ExtendWith;
import org.mockito.ArgumentCaptor;
import org.mockito.Mock;
import org.mockito.junit.jupiter.MockitoExtension;
import org.springframework.http.HttpEntity;
import org.springframework.http.HttpHeaders;
import org.springframework.web.client.RestTemplate;

import java.time.LocalDateTime;

import static org.junit.jupiter.api.Assertions.assertEquals;
import static org.junit.jupiter.api.Assertions.assertTrue;
import static org.mockito.ArgumentMatchers.eq;
import static org.mockito.Mockito.verify;

@ExtendWith(MockitoExtension.class)
public class NtfyServiceTest {

    @Mock
    private RestTemplate restTemplate;

    private NtfyService ntfyService;

    @BeforeEach
    public void setup() {
        ntfyService = new NtfyService(restTemplate);
        ntfyService.setNtfyTopic("test_topic");
        ntfyService.setDashboardUrl("http://test.com");
    }

    @Test
    public void testSendNotification() {
        Event event = new Event(LocalDateTime.now(), "DOORBELL_PRESS", "img.jpg");
        
        ntfyService.sendNotification(event);

        ArgumentCaptor<HttpEntity> entityCaptor = ArgumentCaptor.forClass(HttpEntity.class);
        verify(restTemplate).postForEntity(eq("https://ntfy.sh/test_topic"), entityCaptor.capture(), eq(String.class));
        
        HttpHeaders headers = entityCaptor.getValue().getHeaders();
        assertEquals("Someone is at the door!", headers.getFirst("Title"));
        assertEquals("doorbell, camera", headers.getFirst("Tags"));
        assertTrue(headers.getFirst("Actions").contains("http://test.com"));
    }
}

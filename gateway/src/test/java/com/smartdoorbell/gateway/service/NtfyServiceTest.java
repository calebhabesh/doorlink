package com.smartdoorbell.gateway.service;

import org.junit.jupiter.api.BeforeEach;
import org.junit.jupiter.api.Test;
import org.junit.jupiter.api.extension.ExtendWith;
import org.mockito.ArgumentCaptor;
import org.mockito.Mock;
import org.mockito.junit.jupiter.MockitoExtension;
import org.springframework.http.HttpEntity;
import org.springframework.http.HttpHeaders;
import org.springframework.web.client.RestTemplate;


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
        ntfyService.sendNotification("DOORBELL_PRESS");

        ArgumentCaptor<HttpEntity> entityCaptor = ArgumentCaptor.forClass(HttpEntity.class);
        verify(restTemplate).postForEntity(eq("https://ntfy.sh/test_topic"), entityCaptor.capture(), eq(String.class));
        
        HttpHeaders headers = entityCaptor.getValue().getHeaders();
        assertEquals("Someone rang the doorbell", headers.getFirst("Title"));
        assertEquals("doorbell, bell", headers.getFirst("Tags"));
        assertEquals("http://test.com", headers.getFirst("Click"));
        assertTrue(headers.getFirst("Actions").contains("http://test.com"));
    }
}

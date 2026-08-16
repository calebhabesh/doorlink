package com.smartdoorbell.gateway.service;

import com.smartdoorbell.gateway.service.chime.HomeAssistantChimeService;
import org.junit.jupiter.api.Test;
import org.junit.jupiter.api.extension.ExtendWith;
import org.mockito.InjectMocks;
import org.mockito.Mock;
import org.mockito.junit.jupiter.MockitoExtension;
import org.springframework.http.HttpEntity;
import org.springframework.test.util.ReflectionTestUtils;
import org.springframework.web.client.RestTemplate;

import static org.mockito.ArgumentMatchers.any;
import static org.mockito.ArgumentMatchers.eq;
import static org.mockito.Mockito.times;
import static org.mockito.Mockito.verify;

@ExtendWith(MockitoExtension.class)
class HomeAssistantChimeServiceTest {

    @Mock
    private RestTemplate restTemplate;

    @InjectMocks
    private HomeAssistantChimeService service;

    @Test
    void consecutiveRingsBothTriggerWebhookWithoutCooldown() {
        String webhookUrl = "http://home-assistant/api/webhook/REDACTED_HISTORICAL_VALUE";
        ReflectionTestUtils.setField(service, "webhookUrl", webhookUrl);

        service.ring("DOORBELL_PRESS");
        service.ring("DOORBELL_PRESS");

        verify(restTemplate, times(2)).postForEntity(
                eq(webhookUrl), any(HttpEntity.class), eq(String.class));
    }
}

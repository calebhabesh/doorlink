package com.smartdoorbell.gateway.service;

import com.smartdoorbell.gateway.service.chime.HomeAssistantChimeService;
import org.junit.jupiter.api.AfterEach;
import org.junit.jupiter.api.BeforeEach;
import org.junit.jupiter.api.Test;
import org.junit.jupiter.api.extension.ExtendWith;
import org.mockito.Mock;
import org.mockito.junit.jupiter.MockitoExtension;
import org.springframework.http.HttpEntity;
import org.springframework.test.util.ReflectionTestUtils;
import org.springframework.web.client.RestTemplate;

import static org.mockito.ArgumentMatchers.any;
import static org.mockito.ArgumentMatchers.eq;
import static org.mockito.Mockito.times;
import static org.mockito.Mockito.timeout;
import static org.mockito.Mockito.verify;

@ExtendWith(MockitoExtension.class)
class HomeAssistantChimeServiceTest {

    @Mock
    private RestTemplate restTemplate;

    private HomeAssistantChimeService service;

    @BeforeEach
    void createService() {
        service = new HomeAssistantChimeService(restTemplate);
    }

    @AfterEach
    void shutdownExecutor() {
        ReflectionTestUtils.invokeMethod(service, "shutdown");
    }

    @Test
    void rapidRingsDeliverLeadingAndOneTrailingWebhook() throws Exception {
        String webhookUrl = "http://home-assistant/api/webhook/REDACTED_HISTORICAL_VALUE";
        ReflectionTestUtils.setField(service, "webhookUrl", webhookUrl);
        ReflectionTestUtils.setField(service, "minIntervalMs", 75L);

        service.ring("DOORBELL_PRESS");
        service.ring("DOORBELL_REPRESS");
        service.ring("DOORBELL_REPRESS");

        verify(restTemplate, timeout(1000).times(2)).postForEntity(
                eq(webhookUrl), any(HttpEntity.class), eq(String.class));
        Thread.sleep(125);
        verify(restTemplate, times(2)).postForEntity(
                eq(webhookUrl), any(HttpEntity.class), eq(String.class));
    }
}

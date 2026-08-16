package com.smartdoorbell.gateway.service.chime;

import com.smartdoorbell.gateway.service.ChimeService;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.springframework.beans.factory.annotation.Value;
import org.springframework.boot.autoconfigure.condition.ConditionalOnProperty;
import org.springframework.http.HttpEntity;
import org.springframework.http.HttpHeaders;
import org.springframework.scheduling.annotation.Async;
import org.springframework.stereotype.Service;
import org.springframework.web.client.RestTemplate;

@Service
@ConditionalOnProperty(name = "chime.provider", havingValue = "homeassistant")
public class HomeAssistantChimeService implements ChimeService {

    private static final Logger logger = LoggerFactory.getLogger(HomeAssistantChimeService.class);

    private final RestTemplate restTemplate;

    @Value("${chime.homeassistant.webhook-url}")
    private String webhookUrl;

    public HomeAssistantChimeService(RestTemplate restTemplate) {
        this.restTemplate = restTemplate;
    }

    @Override
    @Async
    public void ring(String eventType) {
        if (webhookUrl == null || webhookUrl.isEmpty()) {
            logger.warn("Home Assistant webhook URL is not configured. Chime skipped.");
            return;
        }

        logger.info("Triggering Home Assistant chime webhook...");

        try {
            HttpHeaders headers = new HttpHeaders();
            headers.set("Content-Type", "application/json");
            
            // Send empty JSON or event payload based on preference
            String payload = "{}";
            HttpEntity<String> entity = new HttpEntity<>(payload, headers);

            restTemplate.postForEntity(webhookUrl, entity, String.class);
            logger.info("Chime webhook triggered successfully.");
        } catch (Exception e) {
            logger.error("Failed to trigger Home Assistant chime webhook: {}", e.getMessage());
            // Do not rethrow, to prevent blocking the event creation flow
        }
    }
}

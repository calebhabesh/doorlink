package com.smartdoorbell.gateway.service;

import org.springframework.beans.factory.annotation.Value;
import org.springframework.http.HttpEntity;
import org.springframework.http.HttpHeaders;
import org.springframework.scheduling.annotation.Async;
import org.springframework.stereotype.Service;
import org.springframework.web.client.RestTemplate;

@Service
public class NtfyService {

    private final RestTemplate restTemplate;

    @Value("${ntfy.topic:smart_doorbell_default}")
    private String ntfyTopic;

    @Value("${dashboard.url:http://localhost:3000}")
    private String dashboardUrl;

    @Value("${ntfy.enabled:true}")
    private boolean enabled = true;

    public NtfyService(RestTemplate restTemplate) {
        this.restTemplate = restTemplate;
    }

    public void setNtfyTopic(String ntfyTopic) {
        this.ntfyTopic = ntfyTopic;
    }

    public void setDashboardUrl(String dashboardUrl) {
        this.dashboardUrl = dashboardUrl;
    }

    @Async
    public void sendNotification(String eventType) {
        if (!enabled) {
            return;
        }
        String url = "https://ntfy.sh/" + ntfyTopic;
        
        HttpHeaders headers = new HttpHeaders();
        boolean motion = "PIR_MOTION".equals(eventType);
        headers.set("Title", motion ? "Motion detected at the door" : "Someone rang the doorbell");
        headers.set("Tags", motion ? "doorbell, motion" : "doorbell, bell");
        headers.set("Actions", "view, Open Dashboard, " + dashboardUrl);

        // The fast trigger intentionally runs before camera capture, so never
        // claim that media already exists in this notification.
        String message = motion
                ? "Presence was detected at the main entry. A snapshot is being prepared."
                : "The front doorbell was pressed. A snapshot is being prepared.";
        HttpEntity<String> entity = new HttpEntity<>(message, headers);

        try {
            restTemplate.postForEntity(url, entity, String.class);
        } catch (Exception e) {
            // Log error but don't fail the caller
            System.err.println("Failed to send ntfy notification: " + e.getMessage());
        }
    }
}

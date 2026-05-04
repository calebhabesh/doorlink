package com.smartdoorbell.gateway.service;

import com.smartdoorbell.gateway.entity.Event;
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
    public void sendNotification(Event event) {
        String url = "https://ntfy.sh/" + ntfyTopic;
        
        HttpHeaders headers = new HttpHeaders();
        headers.set("Title", "Someone is at the door!");
        headers.set("Tags", "doorbell, camera");
        headers.set("Actions", "view, Open Dashboard, " + dashboardUrl);

        HttpEntity<String> entity = new HttpEntity<>("A doorbell event (" + event.getEventType() + ") was triggered.", headers);

        try {
            restTemplate.postForEntity(url, entity, String.class);
        } catch (Exception e) {
            // Log error but don't fail the caller
            System.err.println("Failed to send ntfy notification: " + e.getMessage());
        }
    }
}

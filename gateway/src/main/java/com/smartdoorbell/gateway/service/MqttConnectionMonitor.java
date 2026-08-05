package com.smartdoorbell.gateway.service;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.springframework.context.event.EventListener;
import org.springframework.integration.mqtt.event.MqttConnectionFailedEvent;
import org.springframework.integration.mqtt.event.MqttIntegrationEvent;
import org.springframework.integration.mqtt.event.MqttSubscribedEvent;
import org.springframework.stereotype.Component;

import java.util.concurrent.atomic.AtomicBoolean;
import java.util.concurrent.atomic.AtomicReference;
import java.time.Instant;

@Component
public class MqttConnectionMonitor {

    private static final Logger logger = LoggerFactory.getLogger(MqttConnectionMonitor.class);
    private final AtomicBoolean connected = new AtomicBoolean(false);
    private final AtomicReference<String> status = new AtomicReference<>("UNKNOWN");
    private final AtomicReference<Instant> lastChangedAt = new AtomicReference<>();

    @EventListener
    public void handleMqttEvent(MqttIntegrationEvent event) {
        if (event instanceof MqttConnectionFailedEvent) {
            connected.set(false);
            status.set("DOWN");
            lastChangedAt.set(Instant.now());
            logger.warn("MQTT Connection Failed: {}", event.getCause() != null ? event.getCause().getMessage() : "Unknown reason");
        } else if (event instanceof MqttSubscribedEvent) {
            connected.set(true);
            status.set("UP");
            lastChangedAt.set(Instant.now());
            logger.info("MQTT Connected and Subscribed successfully");
        }
    }

    public boolean isConnected() {
        return connected.get();
    }

    public String getStatus() {
        return status.get();
    }

    public Instant getLastChangedAt() {
        return lastChangedAt.get();
    }
}

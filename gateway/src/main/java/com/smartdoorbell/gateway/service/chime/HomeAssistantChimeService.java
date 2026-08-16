package com.smartdoorbell.gateway.service.chime;

import com.smartdoorbell.gateway.service.ChimeService;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.springframework.beans.factory.annotation.Value;
import org.springframework.boot.autoconfigure.condition.ConditionalOnProperty;
import org.springframework.http.HttpEntity;
import org.springframework.http.HttpHeaders;
import org.springframework.stereotype.Service;
import org.springframework.web.client.RestTemplate;

import jakarta.annotation.PreDestroy;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;
import java.util.concurrent.RejectedExecutionException;
import java.util.concurrent.ThreadFactory;
import java.util.concurrent.TimeUnit;
import java.util.function.LongSupplier;

@Service
@ConditionalOnProperty(name = "chime.provider", havingValue = "homeassistant")
public class HomeAssistantChimeService implements ChimeService {

    private static final Logger logger = LoggerFactory.getLogger(HomeAssistantChimeService.class);

    private final RestTemplate restTemplate;
    private final ExecutorService webhookExecutor;
    private final LongSupplier nanoTime;
    private final Object deliveryLock = new Object();
    private long nextDeliveryNanos = Long.MIN_VALUE;
    private boolean deliveryInFlight;

    @Value("${chime.homeassistant.webhook-url}")
    private String webhookUrl;

    @Value("${chime.homeassistant.min-interval-ms:2600}")
    private long minIntervalMs;

    @org.springframework.beans.factory.annotation.Autowired
    public HomeAssistantChimeService(RestTemplate restTemplate) {
        this(restTemplate, Executors.newSingleThreadExecutor(
                daemonThreadFactory()), System::nanoTime);
    }

    HomeAssistantChimeService(RestTemplate restTemplate,
                              ExecutorService webhookExecutor,
                              LongSupplier nanoTime) {
        this.restTemplate = restTemplate;
        this.webhookExecutor = webhookExecutor;
        this.nanoTime = nanoTime;
    }

    @Override
    public void ring(String eventType) {
        if (webhookUrl == null || webhookUrl.isEmpty()) {
            logger.warn("Home Assistant webhook URL is not configured. Chime skipped.");
            return;
        }

        final long intervalNanos = TimeUnit.MILLISECONDS.toNanos(
                Math.max(0, minIntervalMs));
        final long now = nanoTime.getAsLong();
        synchronized (deliveryLock) {
            if (now < nextDeliveryNanos) {
                logger.info("Whole-home chime request skipped during cooldown ({} ms remaining)",
                        TimeUnit.NANOSECONDS.toMillis(nextDeliveryNanos - now));
                return;
            }

            if (deliveryInFlight) {
                logger.info("Whole-home chime request skipped while webhook delivery is in flight");
                return;
            }

            nextDeliveryNanos = saturatedAdd(now, intervalNanos);
            deliveryInFlight = true;
            try {
                webhookExecutor.execute(() -> {
                    try {
                        dispatch(eventType);
                    } finally {
                        synchronized (deliveryLock) {
                            deliveryInFlight = false;
                        }
                    }
                });
            } catch (RejectedExecutionException e) {
                deliveryInFlight = false;
                nextDeliveryNanos = now;
                if (!webhookExecutor.isShutdown()) {
                    logger.error("Failed to submit Home Assistant chime webhook: {}", e.getMessage());
                }
            }
        }
    }

    private void dispatch(String eventType) {
        logger.info("Triggering Home Assistant chime webhook for {}...", eventType);

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

    private static long saturatedAdd(long value, long increment) {
        if (increment > 0 && value > Long.MAX_VALUE - increment) {
            return Long.MAX_VALUE;
        }
        return value + increment;
    }

    private static ThreadFactory daemonThreadFactory() {
        return runnable -> {
            Thread thread = new Thread(runnable, "home-assistant-chime");
            thread.setDaemon(true);
            return thread;
        };
    }

    @PreDestroy
    void shutdown() {
        webhookExecutor.shutdownNow();
    }
}

package com.smartdoorbell.gateway.service;

import com.smartdoorbell.gateway.entity.Event;
import com.smartdoorbell.gateway.entity.SystemSettings;
import com.smartdoorbell.gateway.repository.EventRepository;
import com.smartdoorbell.gateway.repository.SystemSettingsRepository;
import com.smartdoorbell.gateway.repository.VisitorSessionRepository;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.springframework.scheduling.annotation.Scheduled;
import org.springframework.stereotype.Service;
import org.springframework.transaction.annotation.Transactional;

import java.time.LocalDateTime;
import java.util.List;

@Service
public class RetentionService {

    private static final Logger logger = LoggerFactory.getLogger(RetentionService.class);

    private final EventRepository eventRepository;
    private final MinioService minioService;
    private final SystemSettingsRepository settingsRepository;
    private final VisitorSessionRepository sessionRepository;

    public RetentionService(EventRepository eventRepository, MinioService minioService,
                            SystemSettingsRepository settingsRepository,
                            VisitorSessionRepository sessionRepository) {
        this.eventRepository = eventRepository;
        this.minioService = minioService;
        this.settingsRepository = settingsRepository;
        this.sessionRepository = sessionRepository;
    }

    /**
     * Runs every day at 3 AM to clean up old events.
     */
    @Scheduled(cron = "0 0 3 * * *")
    @Transactional
    public void performCleanup() {
        SystemSettings settings = settingsRepository.findById(1L).orElse(new SystemSettings());
        int retentionDays = settings.getRetentionDays();
        
        if (retentionDays <= 0) {
            logger.info("Retention policy set to keep forever. Skipping cleanup.");
            return;
        }

        LocalDateTime cutoff = LocalDateTime.now().minusDays(retentionDays);
        List<Event> oldEvents = eventRepository.findByTimestampBefore(cutoff);

        if (oldEvents.isEmpty()) {
            sessionRepository.deleteEmptySessionsStartedBefore(cutoff);
            logger.info("No events found to clean up (retention: {} days).", retentionDays);
            return;
        }

        logger.info("Found {} events to delete (older than {}).", oldEvents.size(), cutoff);

        for (Event event : oldEvents) {
            try {
                // Delete from MinIO
                if (event.getImageKey() != null &&
                        !Event.PENDING_IMAGE_KEY.equals(event.getImageKey())) {
                    minioService.deleteFile(event.getImageKey());
                }
                if (event.getAudioKey() != null) {
                    minioService.deleteFile(event.getAudioKey());
                }
                event.getVisitorRecordings().stream()
                        .filter(recording -> !recording.getAudioKey().equals(event.getAudioKey()))
                        .forEach(recording -> minioService.deleteFile(recording.getAudioKey()));
                event.getIntercomMessages().forEach(message ->
                        minioService.deleteFile(message.getAudioKey()));
                
                // Delete from Database
                eventRepository.delete(event);
            } catch (Exception e) {
                logger.error("Failed to clean up event {}: {}", event.getId(), e.getMessage());
            }
        }

        sessionRepository.deleteEmptySessionsStartedBefore(cutoff);

        logger.info("Retention cleanup completed.");
    }
}

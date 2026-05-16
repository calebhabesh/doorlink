package com.smartdoorbell.gateway.service;

import com.smartdoorbell.gateway.entity.Event;
import com.smartdoorbell.gateway.repository.EventRepository;
import org.springframework.beans.factory.annotation.Value;
import org.springframework.scheduling.annotation.Scheduled;
import org.springframework.stereotype.Service;
import org.springframework.transaction.annotation.Transactional;

import java.time.LocalDateTime;
import java.util.List;

@Service
public class StorageCleanupService {

    private final EventRepository eventRepository;
    private final MinioService minioService;

    // Default to 30 days retention
    @Value("${storage.retention.days:30}")
    private int retentionDays;

    public StorageCleanupService(EventRepository eventRepository, MinioService minioService) {
        this.eventRepository = eventRepository;
        this.minioService = minioService;
    }

    // Run every day at 3 AM server time
    @Scheduled(cron = "0 0 3 * * *")
    @Transactional
    public void cleanupOldEvents() {
        LocalDateTime cutoffDate = LocalDateTime.now().minusDays(retentionDays);
        List<Event> oldEvents = eventRepository.findByTimestampBefore(cutoffDate);

        if (oldEvents.isEmpty()) {
            return;
        }

        System.out.println("Starting storage cleanup. Deleting " + oldEvents.size() + " events older than " + retentionDays + " days.");

        for (Event event : oldEvents) {
            // Delete media from MinIO
            if (event.getImageKey() != null) {
                minioService.deleteFile(event.getImageKey());
            }
            if (event.getAudioKey() != null) {
                minioService.deleteFile(event.getAudioKey());
            }
        }

        // Delete records from database
        eventRepository.deleteAll(oldEvents);
        
        System.out.println("Storage cleanup completed.");
    }
}

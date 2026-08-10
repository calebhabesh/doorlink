package com.smartdoorbell.gateway.controller;

import com.fasterxml.jackson.databind.ObjectMapper;
import com.smartdoorbell.gateway.config.MqttGateway;
import com.smartdoorbell.gateway.entity.Event;
import com.smartdoorbell.gateway.entity.IntercomMessage;
import com.smartdoorbell.gateway.entity.SystemSettings;
import com.smartdoorbell.gateway.repository.EventRepository;
import com.smartdoorbell.gateway.repository.IntercomMessageRepository;
import com.smartdoorbell.gateway.repository.SystemSettingsRepository;
import com.smartdoorbell.gateway.service.MinioService;
import com.smartdoorbell.gateway.service.SystemHealthService;
import org.springframework.beans.factory.annotation.Value;
import org.springframework.http.MediaType;
import org.springframework.http.ResponseEntity;
import org.springframework.web.bind.annotation.*;
import org.springframework.web.multipart.MultipartFile;

import java.time.Duration;
import java.time.LocalDateTime;
import java.util.Map;
import java.util.UUID;

@RestController
@RequestMapping("/api/system")
public class SystemController {

    private final SystemSettingsRepository settingsRepository;
    private final MqttGateway mqttGateway;
    private final com.smartdoorbell.gateway.config.MqttConfig mqttConfig;
    private final SystemHealthService systemHealthService;
    private final EventRepository eventRepository;
    private final IntercomMessageRepository intercomMessageRepository;
    private final MinioService minioService;
    private final ObjectMapper objectMapper;

    @Value("${intercom.device-base-url:http://192.168.1.10:8080}")
    private String deviceBaseUrl;

    @Value("${intercom.session.max-age-seconds:90}")
    private long sessionMaxAgeSeconds;

    public SystemController(SystemSettingsRepository settingsRepository,
                            MqttGateway mqttGateway,
                            com.smartdoorbell.gateway.config.MqttConfig mqttConfig,
                            SystemHealthService systemHealthService,
                            EventRepository eventRepository,
                            IntercomMessageRepository intercomMessageRepository,
                            MinioService minioService,
                            ObjectMapper objectMapper) {
        this.settingsRepository = settingsRepository;
        this.mqttGateway = mqttGateway;
        this.mqttConfig = mqttConfig;
        this.systemHealthService = systemHealthService;
        this.eventRepository = eventRepository;
        this.intercomMessageRepository = intercomMessageRepository;
        this.minioService = minioService;
        this.objectMapper = objectMapper;
    }

    @PostMapping(path = "/ptt/start", consumes = MediaType.APPLICATION_JSON_VALUE)
    public ResponseEntity<?> startPtt(@RequestBody PttControlRequest request) {
        Event event = activeEvent(request.eventId());
        if (event == null) {
            return ResponseEntity.status(409).body("The visitor session is no longer active");
        }
        publishCommand(Map.of("type", "PTT_START", "eventId", event.getEventId()));
        return ResponseEntity.ok(new PttStateResponse("RECORDING", null));
    }

    @PostMapping(path = "/ptt/cancel", consumes = MediaType.APPLICATION_JSON_VALUE)
    public ResponseEntity<?> cancelPtt(@RequestBody PttControlRequest request) {
        Event event = activeEvent(request.eventId());
        if (event == null) {
            return ResponseEntity.status(409).body("The visitor session is no longer active");
        }
        publishCommand(Map.of("type", "PTT_CANCEL", "eventId", event.getEventId()));
        return ResponseEntity.ok(new PttStateResponse("LISTENING", null));
    }

    @PostMapping(path = "/ptt", consumes = MediaType.MULTIPART_FORM_DATA_VALUE)
    public ResponseEntity<?> handlePttAudio(@RequestParam("eventId") Long eventId,
                                            @RequestParam("durationMs") Integer durationMs,
                                            @RequestParam("audio") MultipartFile audio) {
        Event event = activeEvent(eventId);
        if (event == null) {
            return ResponseEntity.status(409).body("The visitor session is no longer active");
        }
        if (audio.isEmpty() || audio.getSize() < 44 || audio.getSize() > 640044 ||
                durationMs == null ||
                !"audio/wav".equalsIgnoreCase(audio.getContentType())) {
            return ResponseEntity.badRequest().body("PTT requires a 0.1-20 second, 16 kHz PCM WAV");
        }

        String audioKey = null;
        try {
            Integer wavDurationMs = inspectPcmWav(audio.getBytes());
            if (wavDurationMs == null || wavDurationMs < 100 || wavDurationMs > 20000 ||
                    Math.abs(wavDurationMs - durationMs) > 250) {
                return ResponseEntity.badRequest().body(
                        "PTT requires a canonical 16 kHz mono 16-bit PCM WAV");
            }
            String messageId = UUID.randomUUID().toString();
            audioKey = minioService.uploadFile(
                    audio, event.getEventId() + "-reply-" + messageId);
            IntercomMessage message = intercomMessageRepository.save(
                    new IntercomMessage(messageId, event, "HOMEOWNER", audioKey,
                            wavDurationMs, LocalDateTime.now()));
            String baseUrl = deviceBaseUrl.replaceAll("/+$", "");
            publishCommand(Map.of(
                    "type", "PLAY_AUDIO",
                    "eventId", event.getEventId(),
                    "messageId", messageId,
                    "audioUrl", baseUrl + "/api/events/media/" + audioKey,
                    "ackUrl", baseUrl + "/api/system/ptt/messages/" + messageId + "/delivered",
                    "durationMs", wavDurationMs));
            return ResponseEntity.ok(new PttReplyResponse(
                    "PLAYBACK_QUEUED", message.getMessageId(), audioKey, wavDurationMs));
        } catch (Exception e) {
            if (audioKey != null) {
                minioService.deleteFile(audioKey);
            }
            return ResponseEntity.internalServerError().body("Failed to queue PTT audio");
        }
    }

    @PostMapping("/ptt/messages/{messageId}/delivered")
    public ResponseEntity<?> markPttDelivered(@PathVariable String messageId) {
        return intercomMessageRepository.findByMessageId(messageId)
                .map(message -> {
                    message.markDelivered(LocalDateTime.now());
                    intercomMessageRepository.save(message);
                    return ResponseEntity.ok().build();
                })
                .orElseGet(() -> ResponseEntity.notFound().build());
    }

    private Event activeEvent(Long id) {
        if (id == null) return null;
        return eventRepository.findById(id)
                .filter(event -> event.getEventId() != null)
                .filter(event -> !event.getTimestamp().isAfter(LocalDateTime.now()))
                .filter(event -> Duration.between(event.getTimestamp(), LocalDateTime.now()).getSeconds()
                        <= sessionMaxAgeSeconds)
                .orElse(null);
    }

    private void publishCommand(Map<String, Object> command) {
        try {
            mqttGateway.sendToMqtt(objectMapper.writeValueAsString(command),
                    mqttConfig.getPttAudioTopic());
        } catch (Exception exception) {
            throw new IllegalStateException("Could not serialize intercom command", exception);
        }
    }

    private static Integer inspectPcmWav(byte[] wav) {
        if (wav.length < 44 || !asciiEquals(wav, 0, "RIFF") ||
                !asciiEquals(wav, 8, "WAVE") || !asciiEquals(wav, 12, "fmt ") ||
                !asciiEquals(wav, 36, "data") || readLe16(wav, 20) != 1 ||
                readLe16(wav, 22) != 1 || readLe32(wav, 24) != 16000 ||
                readLe16(wav, 34) != 16) {
            return null;
        }
        long dataLength = readLe32(wav, 40);
        if (dataLength != wav.length - 44L || (dataLength & 1) != 0) {
            return null;
        }
        return Math.toIntExact((dataLength * 1000L) / 32000L);
    }

    private static boolean asciiEquals(byte[] bytes, int offset, String value) {
        if (offset + value.length() > bytes.length) return false;
        for (int i = 0; i < value.length(); i++) {
            if ((bytes[offset + i] & 0xff) != value.charAt(i)) return false;
        }
        return true;
    }

    private static int readLe16(byte[] bytes, int offset) {
        return (bytes[offset] & 0xff) | ((bytes[offset + 1] & 0xff) << 8);
    }

    private static long readLe32(byte[] bytes, int offset) {
        return (bytes[offset] & 0xffL) |
                ((bytes[offset + 1] & 0xffL) << 8) |
                ((bytes[offset + 2] & 0xffL) << 16) |
                ((bytes[offset + 3] & 0xffL) << 24);
    }

    public record PttControlRequest(Long eventId) {}
    public record PttStateResponse(String state, String messageId) {}
    public record PttReplyResponse(String state, String messageId, String audioKey,
                                   Integer durationMs) {}

    @GetMapping("/health")
    public ResponseEntity<SystemHealthService.HealthSnapshot> getHealth() {
        return ResponseEntity.ok(systemHealthService.snapshot());
    }

    @GetMapping("/settings")
    public ResponseEntity<SystemSettings> getSettings() {
        SystemSettings settings = settingsRepository.findById(1L)
                .orElseGet(() -> {
                    SystemSettings newSettings = new SystemSettings();
                    return settingsRepository.save(newSettings);
                });
        return ResponseEntity.ok(settings);
    }

    @PostMapping("/settings")
    public ResponseEntity<SystemSettings> updateSettings(@RequestBody SystemSettings newSettings) {
        SystemSettings settings = settingsRepository.findById(1L)
                .orElse(new SystemSettings());
        
        settings.setRetentionDays(newSettings.getRetentionDays());
        settings.setCameraQuality(newSettings.getCameraQuality());
        settings.setNotificationsEnabled(newSettings.getNotificationsEnabled());
        
        SystemSettings saved = settingsRepository.save(settings);
        return ResponseEntity.ok(saved);
    }
}

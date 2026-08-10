package com.smartdoorbell.gateway.service;

import com.smartdoorbell.gateway.entity.Event;
import com.smartdoorbell.gateway.entity.IntercomMessage;
import com.smartdoorbell.gateway.entity.VisitorRecording;
import com.smartdoorbell.gateway.entity.VisitorSession;
import com.smartdoorbell.gateway.repository.EventRepository;
import com.smartdoorbell.gateway.repository.IntercomMessageRepository;
import com.smartdoorbell.gateway.repository.VisitorSessionRepository;
import org.springframework.beans.factory.annotation.Value;
import org.springframework.data.domain.Sort;
import org.springframework.stereotype.Service;
import org.springframework.transaction.annotation.Transactional;

import java.time.LocalDateTime;
import java.util.ArrayList;
import java.util.Comparator;
import java.util.List;
import java.util.LinkedHashMap;
import java.util.Map;
import java.util.regex.Pattern;

@Service
public class VisitorSessionService {
    private static final Pattern NUMBERED_PRESS = Pattern.compile("^(.+)-(\\d+)$");
    private final VisitorSessionRepository sessionRepository;
    private final EventRepository eventRepository;
    private final IntercomMessageRepository intercomRepository;

    @Value("${intercom.session.idle-timeout-seconds:60}")
    private long idleTimeoutSeconds;

    @Value("${intercom.session.max-age-seconds:90}")
    private long maxAgeSeconds;

    public VisitorSessionService(VisitorSessionRepository sessionRepository,
                                 EventRepository eventRepository,
                                 IntercomMessageRepository intercomRepository) {
        this.sessionRepository = sessionRepository;
        this.eventRepository = eventRepository;
        this.intercomRepository = intercomRepository;
    }

    @Transactional(readOnly = true)
    public List<SessionView> recent(int limit) {
        List<SessionView> result = new ArrayList<>();
        for (VisitorSession session : sessionRepository.findAll(
                Sort.by(Sort.Direction.DESC, "startedAt"))) {
            result.add(view(session));
        }
        Map<String, List<Event>> legacyGroups = new LinkedHashMap<>();
        for (Event event : eventRepository.findBySessionIsNullOrderByTimestampDesc()) {
            legacyGroups.computeIfAbsent(legacySessionId(event), ignored -> new ArrayList<>())
                    .add(event);
        }
        for (var entry : legacyGroups.entrySet()) {
            result.add(legacyView(entry.getKey(), entry.getValue()));
        }
        result.sort(Comparator.comparing(SessionView::startedAt).reversed());
        return result.stream().limit(Math.max(1, Math.min(limit, 1000))).toList();
    }

    @Transactional(readOnly = true)
    public SessionView view(VisitorSession session) {
        List<Event> events = eventRepository.findBySessionOrderByPressNumberAsc(session);
        List<PressView> presses = events.stream().map(this::pressView).toList();
        List<ReplyView> replies = intercomRepository
                .findByEventSessionOrderByCreatedAtAsc(session).stream()
                .map(this::replyView).toList();
        String imageKey = events.stream()
                .sorted(Comparator.comparing(Event::getTimestamp).reversed())
                .map(Event::getImageKey).filter(key -> key != null && !key.isBlank()
                        && !Event.PENDING_IMAGE_KEY.equals(key))
                .findFirst().orElse(null);
        LocalDateTime endsAt = earlier(
                session.getStartedAt().plusSeconds(maxAgeSeconds),
                session.getLastActivityAt().plusSeconds(idleTimeoutSeconds));
        String status = session.getClosedAt() != null || !endsAt.isAfter(LocalDateTime.now())
                ? "CLOSED" : session.getStatus();
        int recordingCount = presses.stream().mapToInt(press -> press.recordings().size()).sum();
        return new SessionView(session.getId(), session.getSessionId(),
                session.getStartedAt(), session.getLastActivityAt(), endsAt,
                session.getClosedAt(), status, imageKey, presses.size(),
                recordingCount, presses, replies);
    }

    private SessionView legacyView(String sessionId, List<Event> events) {
        events.sort(Comparator.comparing(Event::getTimestamp));
        List<PressView> presses = events.stream().map(this::pressView).toList();
        List<ReplyView> replies = events.stream()
                .flatMap(event -> event.getIntercomMessages().stream())
                .sorted(Comparator.comparing(IntercomMessage::getCreatedAt))
                .map(this::replyView).toList();
        Event first = events.getFirst();
        Event last = events.getLast();
        String imageKey = events.reversed().stream().map(Event::getImageKey)
                .filter(key -> key != null && !Event.PENDING_IMAGE_KEY.equals(key))
                .findFirst().orElse(null);
        int recordingCount = presses.stream().mapToInt(press -> press.recordings().size()).sum();
        return new SessionView(null, sessionId, first.getTimestamp(),
                last.getTimestamp(), last.getTimestamp(), last.getTimestamp(),
                "CLOSED", imageKey, presses.size(), recordingCount, presses, replies);
    }

    private static String legacySessionId(Event event) {
        if (event.getEventId() != null) {
            var matcher = NUMBERED_PRESS.matcher(event.getEventId());
            if (matcher.matches()) return matcher.group(1);
        }
        return "legacy-" + event.getId();
    }

    private PressView pressView(Event event) {
        List<RecordingView> recordings = event.getVisitorRecordings().stream()
                .map(this::recordingView).toList();
        if (recordings.isEmpty() && event.getAudioKey() != null) {
            recordings = List.of(new RecordingView(
                    (event.getEventId() == null ? "legacy-" + event.getId() : event.getEventId())
                            + "-visitor",
                    event.getAudioKey(), null, event.getTimestamp()));
        }
        Integer recordingDurationMs = recordings.stream().map(RecordingView::durationMs)
                .filter(value -> value != null).max(Integer::compareTo).orElse(null);
        Integer durationMs = event.getPressDurationMs() == null
                ? recordingDurationMs : event.getPressDurationMs();
        String state = event.getLifecycleState();
        if (state == null) state = event.getImageKey() == null ||
                Event.PENDING_IMAGE_KEY.equals(event.getImageKey()) ? "PHOTO_PENDING" : "READY";
        String imageKey = Event.PENDING_IMAGE_KEY.equals(event.getImageKey())
                ? null : event.getImageKey();
        return new PressView(event.getId(), event.getEventId(),
                event.getPressNumber() == null ? legacyPressNumber(event)
                        : event.getPressNumber(),
                event.getTimestamp(), durationMs, event.getEventType(),
                state, imageKey, recordings);
    }

    private RecordingView recordingView(VisitorRecording recording) {
        return new RecordingView(recording.getRecordingId(), recording.getAudioKey(),
                recording.getDurationMs(), recording.getCreatedAt());
    }

    private static int legacyPressNumber(Event event) {
        if (event.getEventId() != null) {
            var matcher = NUMBERED_PRESS.matcher(event.getEventId());
            if (matcher.matches()) {
                try {
                    return Math.max(1, Integer.parseInt(matcher.group(2)));
                } catch (NumberFormatException ignored) {
                    // Treat opaque legacy identifiers as first presses.
                }
            }
        }
        return 1;
    }

    private ReplyView replyView(IntercomMessage reply) {
        return new ReplyView(reply.getMessageId(), reply.getSender(), reply.getAudioKey(),
                reply.getDurationMs(), reply.getCreatedAt(), reply.getDeliveredAt());
    }

    private static LocalDateTime earlier(LocalDateTime left, LocalDateTime right) {
        return left.isBefore(right) ? left : right;
    }

    public record SessionView(Long id, String sessionId, LocalDateTime startedAt,
                              LocalDateTime lastActivityAt, LocalDateTime endsAt,
                              LocalDateTime closedAt, String status,
                              String latestImageKey, int pressCount,
                              int recordingCount, List<PressView> presses,
                              List<ReplyView> replies) {}

    public record PressView(Long id, String pressId, int pressNumber,
                            LocalDateTime pressedAt, Integer durationMs,
                            String eventType, String state, String imageKey,
                            List<RecordingView> recordings) {}

    public record RecordingView(String recordingId, String audioKey,
                                Integer durationMs, LocalDateTime createdAt) {}

    public record ReplyView(String messageId, String sender, String audioKey,
                            Integer durationMs, LocalDateTime createdAt,
                            LocalDateTime deliveredAt) {}
}

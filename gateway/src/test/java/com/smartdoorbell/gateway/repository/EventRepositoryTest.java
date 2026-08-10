package com.smartdoorbell.gateway.repository;

import com.smartdoorbell.gateway.entity.Event;
import com.smartdoorbell.gateway.entity.VisitorRecording;
import com.smartdoorbell.gateway.entity.VisitorSession;
import org.junit.jupiter.api.Test;
import org.junit.jupiter.api.extension.ExtendWith;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.boot.test.autoconfigure.orm.jpa.DataJpaTest;
import org.springframework.test.context.junit.jupiter.SpringExtension;

import java.time.LocalDateTime;

import static org.junit.jupiter.api.Assertions.assertEquals;
import static org.junit.jupiter.api.Assertions.assertNotNull;
import static org.junit.jupiter.api.Assertions.assertTrue;

@ExtendWith(SpringExtension.class)
@DataJpaTest
public class EventRepositoryTest {

    @Autowired
    private EventRepository eventRepository;

    @Autowired
    private VisitorSessionRepository sessionRepository;

    @Autowired
    private VisitorRecordingRepository recordingRepository;

    @Test
    public void testSaveEvent() {
        Event event = new Event(LocalDateTime.now(), "DOORBELL_PRESS", "doorbell-images/test.jpg");
        Event saved = eventRepository.save(event);
        assertNotNull(saved.getId());
    }

    @Test
    public void testSaveEventWithAudio() {
        Event event = new Event(LocalDateTime.now(), "DOORBELL_PRESS", "images/test.jpg", "audio/test.wav");
        Event saved = eventRepository.save(event);
        assertNotNull(saved.getId());
        assertEquals("audio/test.wav", saved.getAudioKey());
    }

    @Test
    public void testFindEventByStableEventId() {
        String eventId = "0123456789abcdef0123456789abcdef";
        eventRepository.save(new Event(eventId, LocalDateTime.now(),
                "DOORBELL_PRESS", "images/test.jpg", null));

        assertTrue(eventRepository.findByEventId(eventId).isPresent());
    }

    @Test
    public void testOrderedPressesAndRecordingBelongToOneSession() {
        LocalDateTime startedAt = LocalDateTime.now();
        VisitorSession session = sessionRepository.save(
                new VisitorSession("0123456789abcdef0123456789abcdef", startedAt));
        Event first = new Event(session.getSessionId() + "-1", startedAt,
                "DOORBELL_PRESS", "images/first.jpg", null);
        first.assignToSession(session, 1);
        eventRepository.save(first);
        Event second = new Event(session.getSessionId() + "-2",
                startedAt.plusSeconds(3), "DOORBELL_REPRESS",
                Event.PENDING_IMAGE_KEY, null);
        second.assignToSession(session, 2);
        second = eventRepository.save(second);
        recordingRepository.save(new VisitorRecording(
                second.getEventId() + "-visitor", second, "audio/second.wav",
                2400, startedAt.plusSeconds(5)));

        var presses = eventRepository.findBySessionOrderByPressNumberAsc(session);
        assertEquals(2, presses.size());
        assertEquals(1, presses.get(0).getPressNumber());
        assertEquals(2, presses.get(1).getPressNumber());
        assertTrue(recordingRepository.findByRecordingId(
                second.getEventId() + "-visitor").isPresent());
    }
}

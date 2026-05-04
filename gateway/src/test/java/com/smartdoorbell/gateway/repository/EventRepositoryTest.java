package com.smartdoorbell.gateway.repository;

import com.smartdoorbell.gateway.entity.Event;
import org.junit.jupiter.api.Test;
import org.junit.jupiter.api.extension.ExtendWith;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.boot.test.autoconfigure.orm.jpa.DataJpaTest;
import org.springframework.test.context.junit.jupiter.SpringExtension;

import java.time.LocalDateTime;

import static org.junit.jupiter.api.Assertions.assertEquals;
import static org.junit.jupiter.api.Assertions.assertNotNull;

@ExtendWith(SpringExtension.class)
@DataJpaTest(properties = {
    "spring.datasource.url=jdbc:postgresql://localhost:5432/smartdoorbell",
    "spring.datasource.username=user",
    "spring.datasource.password=password"
})
public class EventRepositoryTest {

    @Autowired
    private EventRepository eventRepository;

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
}

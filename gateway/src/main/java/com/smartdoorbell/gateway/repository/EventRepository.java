package com.smartdoorbell.gateway.repository;

import com.smartdoorbell.gateway.entity.Event;
import org.springframework.data.jpa.repository.JpaRepository;
import org.springframework.stereotype.Repository;

import java.time.LocalDateTime;
import java.util.List;
import java.util.Optional;
import com.smartdoorbell.gateway.entity.VisitorSession;

@Repository
public interface EventRepository extends JpaRepository<Event, Long> {
    List<Event> findByTimestampBefore(LocalDateTime date);
    Optional<Event> findByEventId(String eventId);
    List<Event> findBySessionOrderByPressNumberAsc(VisitorSession session);
    Optional<Event> findFirstBySessionOrderByPressNumberDesc(VisitorSession session);
    List<Event> findBySessionIsNullOrderByTimestampDesc();
}

package com.smartdoorbell.gateway.repository;

import com.smartdoorbell.gateway.entity.Event;
import org.springframework.data.jpa.repository.JpaRepository;
import org.springframework.stereotype.Repository;

import java.time.LocalDateTime;
import java.util.List;

@Repository
public interface EventRepository extends JpaRepository<Event, Long> {
    List<Event> findByTimestampBefore(LocalDateTime date);
}

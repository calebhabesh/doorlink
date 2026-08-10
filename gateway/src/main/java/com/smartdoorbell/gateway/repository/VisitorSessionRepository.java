package com.smartdoorbell.gateway.repository;

import com.smartdoorbell.gateway.entity.VisitorSession;
import org.springframework.data.jpa.repository.JpaRepository;

import java.util.Optional;
import java.time.LocalDateTime;
import org.springframework.data.jpa.repository.Modifying;
import org.springframework.data.jpa.repository.Query;
import org.springframework.data.repository.query.Param;

public interface VisitorSessionRepository extends JpaRepository<VisitorSession, Long> {
    Optional<VisitorSession> findBySessionId(String sessionId);

    @Modifying
    @Query("delete from VisitorSession session where session.startedAt < :cutoff " +
            "and not exists (select event.id from Event event where event.session = session)")
    int deleteEmptySessionsStartedBefore(@Param("cutoff") LocalDateTime cutoff);
}

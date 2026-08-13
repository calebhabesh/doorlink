package com.smartdoorbell.gateway.repository;

import com.smartdoorbell.gateway.entity.HouseholdMember;
import org.springframework.data.jpa.repository.JpaRepository;

import java.util.List;
import java.util.Optional;

public interface HouseholdMemberRepository extends JpaRepository<HouseholdMember, Long> {
    boolean existsByRoleAndDisabledAtIsNull(HouseholdMember.Role role);
    Optional<HouseholdMember> findByEmailIgnoreCaseAndDisabledAtIsNull(String email);
    List<HouseholdMember> findAllByOrderByCreatedAtAsc();
}

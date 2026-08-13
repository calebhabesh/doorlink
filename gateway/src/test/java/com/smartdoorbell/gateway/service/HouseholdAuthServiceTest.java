package com.smartdoorbell.gateway.service;

import com.smartdoorbell.gateway.entity.HouseholdMember;
import com.smartdoorbell.gateway.repository.HouseholdDeviceSessionRepository;
import com.smartdoorbell.gateway.repository.HouseholdInvitationRepository;
import com.smartdoorbell.gateway.repository.HouseholdMemberRepository;
import org.junit.jupiter.api.Test;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.boot.test.autoconfigure.orm.jpa.DataJpaTest;
import org.springframework.context.annotation.Import;
import org.springframework.mock.web.MockHttpServletResponse;

import static org.junit.jupiter.api.Assertions.*;

@DataJpaTest(properties = {
        "household.bootstrap-token=test-bootstrap-token",
        "household.session-days=400",
        "household.invitation-hours=72",
        "household.cookie-secure=true",
        "dashboard.url=https://doorbell.example.com",
        "spring.jpa.properties.hibernate.dialect=org.hibernate.dialect.H2Dialect"
})
@Import(HouseholdAuthService.class)
class HouseholdAuthServiceTest {

    @Autowired private HouseholdAuthService householdAuth;
    @Autowired private HouseholdMemberRepository memberRepository;
    @Autowired private HouseholdDeviceSessionRepository deviceRepository;
    @Autowired private HouseholdInvitationRepository invitationRepository;

    @Test
    void bootstrapCreatesAHashedRevocableDeviceSession() {
        var issue = householdAuth.bootstrap("test-bootstrap-token", "Caleb",
                "Caleb@example.com", "Caleb phone", "test-agent", "127.0.0.1");

        assertFalse(householdAuth.needsBootstrap());
        assertEquals(HouseholdMember.Role.OWNER, issue.principal().role());
        assertEquals("caleb@example.com", issue.principal().email());
        assertTrue(householdAuth.authenticate(issue.rawToken(), "127.0.0.1").isPresent());
        assertTrue(deviceRepository.findAll().stream()
                .noneMatch(device -> device.getTokenHash().equals(issue.rawToken())));

        var replacementInvitation = householdAuth.inviteDevice(issue.principal().memberId());
        String replacementToken = replacementInvitation.enrollmentUrl()
                .substring(replacementInvitation.enrollmentUrl().lastIndexOf('/') + 1);
        householdAuth.enroll(replacementToken, "Replacement phone",
                "test-agent", "127.0.0.1");
        householdAuth.revokeDevice(issue.principal().sessionId());
        assertTrue(householdAuth.authenticate(issue.rawToken(), "127.0.0.1").isEmpty());
    }

    @Test
    void ownerCannotRevokeTheLastRecoveryDevice() {
        var issue = householdAuth.bootstrap("test-bootstrap-token", "Caleb",
                "caleb@example.com", "Only phone", "agent", "127.0.0.1");

        assertThrows(HouseholdAuthService.ConflictException.class,
                () -> householdAuth.revokeDevice(issue.principal().sessionId()));
        assertTrue(householdAuth.authenticate(issue.rawToken(), "127.0.0.1").isPresent());
    }

    @Test
    void enrollmentLinksAreSingleUseAndDoNotContainStoredToken() {
        householdAuth.bootstrap("test-bootstrap-token", "Owner",
                "owner@example.com", "Owner phone", "test-agent", "127.0.0.1");
        var invitation = householdAuth.inviteMember("Housemate", "person@example.com");
        String rawToken = invitation.enrollmentUrl()
                .substring(invitation.enrollmentUrl().lastIndexOf('/') + 1);

        assertEquals("Housemate", householdAuth.inspectInvitation(rawToken).name());
        assertTrue(invitationRepository.findAll().stream()
                .noneMatch(stored -> stored.getTokenHash().equals(rawToken)));

        var enrolled = householdAuth.enroll(rawToken, "Housemate phone",
                "test-agent", "127.0.0.1");
        assertEquals(HouseholdMember.Role.MEMBER, enrolled.principal().role());
        assertThrows(HouseholdAuthService.NotFoundException.class,
                () -> householdAuth.enroll(rawToken, "Second phone",
                        "test-agent", "127.0.0.1"));
    }

    @Test
    void invalidBootstrapTokenDoesNotCreateAnOwner() {
        assertThrows(HouseholdAuthService.UnauthorizedException.class,
                () -> householdAuth.bootstrap("wrong", "Caleb",
                        "caleb@example.com", "Phone", "agent", "127.0.0.1"));
        assertTrue(memberRepository.findAll().isEmpty());
    }

    @Test
    void deviceCookieUsesBrowserSecurityAttributes() {
        MockHttpServletResponse response = new MockHttpServletResponse();
        householdAuth.setSessionCookie(response, "raw-session-token");

        String cookie = response.getHeader("Set-Cookie");
        assertNotNull(cookie);
        assertTrue(cookie.startsWith(HouseholdAuthService.COOKIE_NAME + "="));
        assertTrue(cookie.contains("Path=/"));
        assertTrue(cookie.contains("Max-Age="));
        assertTrue(cookie.contains("Secure"));
        assertTrue(cookie.contains("HttpOnly"));
        assertTrue(cookie.contains("SameSite=Lax"));
    }

    @Test
    void renameDeviceUpdatesDeviceName() {
        var issue = householdAuth.bootstrap("test-bootstrap-token", "Caleb",
                "caleb@example.com", "Caleb Pixel 6A", "test-agent", "127.0.0.1");

        long deviceId = issue.principal().sessionId();
        var updated = householdAuth.renameDevice(deviceId, "Pixel 6A");
        assertEquals("Pixel 6A", updated.name());

        var members = householdAuth.members();
        assertEquals(1, members.size());
        assertEquals("Pixel 6A", members.get(0).devices().get(0).name());
    }

    @Test
    void cannotRenameRevokedOrInvalidDevice() {
        var issue = householdAuth.bootstrap("test-bootstrap-token", "Caleb",
                "caleb@example.com", "First Phone", "test-agent", "127.0.0.1");

        var replacement = householdAuth.inviteDevice(issue.principal().memberId());
        String token = replacement.enrollmentUrl().substring(replacement.enrollmentUrl().lastIndexOf('/') + 1);
        householdAuth.enroll(token, "Second Phone", "test-agent", "127.0.0.1");

        householdAuth.revokeDevice(issue.principal().sessionId());

        assertThrows(HouseholdAuthService.ConflictException.class,
                () -> householdAuth.renameDevice(issue.principal().sessionId(), "New Name"));
        assertThrows(HouseholdAuthService.ValidationException.class,
                () -> householdAuth.renameDevice(issue.principal().sessionId() + 1, "   "));
        assertThrows(HouseholdAuthService.NotFoundException.class,
                () -> householdAuth.renameDevice(99999L, "New Name"));
    }
}

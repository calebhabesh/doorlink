package com.smartdoorbell.gateway.service;

import com.smartdoorbell.gateway.entity.HouseholdDeviceSession;
import com.smartdoorbell.gateway.entity.HouseholdInvitation;
import com.smartdoorbell.gateway.entity.HouseholdMember;
import com.smartdoorbell.gateway.repository.HouseholdDeviceSessionRepository;
import com.smartdoorbell.gateway.repository.HouseholdInvitationRepository;
import com.smartdoorbell.gateway.repository.HouseholdMemberRepository;
import org.springframework.beans.factory.annotation.Value;
import org.springframework.http.ResponseCookie;
import org.springframework.stereotype.Service;
import org.springframework.transaction.annotation.Transactional;

import jakarta.servlet.http.HttpServletResponse;
import java.nio.charset.StandardCharsets;
import java.security.MessageDigest;
import java.security.NoSuchAlgorithmException;
import java.security.SecureRandom;
import java.time.Duration;
import java.time.LocalDateTime;
import java.util.Base64;
import java.util.List;
import java.util.Locale;
import java.util.Optional;
import java.util.regex.Pattern;

@Service
public class HouseholdAuthService {

    public static final String COOKIE_NAME = "doorlink_device";
    public static final String PRINCIPAL_ATTRIBUTE =
            "com.smartdoorbell.gateway.householdPrincipal";

    private static final Pattern EMAIL = Pattern.compile(
            "^[^\\s@]+@[^\\s@]+\\.[^\\s@]+$");
    private static final SecureRandom RANDOM = new SecureRandom();

    private final HouseholdMemberRepository memberRepository;
    private final HouseholdDeviceSessionRepository deviceRepository;
    private final HouseholdInvitationRepository invitationRepository;

    @Value("${household.bootstrap-token:}")
    private String bootstrapToken;

    @Value("${household.session-days:400}")
    private long sessionDays;

    @Value("${household.invitation-hours:72}")
    private long invitationHours;

    @Value("${household.cookie-secure:true}")
    private boolean cookieSecure;

    @Value("${dashboard.url:http://localhost:3000}")
    private String dashboardUrl;

    public HouseholdAuthService(HouseholdMemberRepository memberRepository,
                                HouseholdDeviceSessionRepository deviceRepository,
                                HouseholdInvitationRepository invitationRepository) {
        this.memberRepository = memberRepository;
        this.deviceRepository = deviceRepository;
        this.invitationRepository = invitationRepository;
    }

    public boolean needsBootstrap() {
        return !memberRepository.existsByRoleAndDisabledAtIsNull(HouseholdMember.Role.OWNER);
    }

    public boolean bootstrapConfigured() {
        return bootstrapToken != null && !bootstrapToken.isBlank();
    }

    @Transactional
    public SessionIssue bootstrap(String suppliedToken, String name, String email,
                                  String deviceName, String userAgent, String ip) {
        if (!needsBootstrap()) throw new ConflictException("Household setup is already complete");
        if (!bootstrapConfigured()) {
            throw new ConfigurationException("HOUSEHOLD_BOOTSTRAP_TOKEN is not configured");
        }
        if (!constantTimeEquals(bootstrapToken, suppliedToken)) {
            throw new UnauthorizedException("Invalid setup token");
        }
        LocalDateTime now = LocalDateTime.now();
        HouseholdMember owner = memberRepository.save(new HouseholdMember(
                validName(name), validEmail(email), HouseholdMember.Role.OWNER, now));
        return createSession(owner, deviceName, userAgent, ip, now);
    }

    @Transactional
    public Optional<Principal> authenticate(String rawToken, String ip) {
        if (rawToken == null || rawToken.isBlank()) return Optional.empty();
        LocalDateTime now = LocalDateTime.now();
        return deviceRepository.findByTokenHash(hash(rawToken))
                .filter(device -> device.isActive(now))
                .map(device -> {
                    if (device.getLastSeenAt().isBefore(now.minusHours(1))) {
                        device.touch(now, now.plusDays(sessionDays), truncate(ip, 64));
                        deviceRepository.save(device);
                    }
                    return principal(device);
                });
    }

    public void setSessionCookie(HttpServletResponse response, String rawToken) {
        ResponseCookie cookie = ResponseCookie.from(COOKIE_NAME, rawToken)
                .httpOnly(true)
                .secure(cookieSecure)
                .sameSite("Lax")
                .path("/")
                .maxAge(Duration.ofDays(sessionDays))
                .build();
        response.addHeader("Set-Cookie", cookie.toString());
    }

    public void clearSessionCookie(HttpServletResponse response) {
        ResponseCookie cookie = ResponseCookie.from(COOKIE_NAME, "")
                .httpOnly(true)
                .secure(cookieSecure)
                .sameSite("Lax")
                .path("/")
                .maxAge(Duration.ZERO)
                .build();
        response.addHeader("Set-Cookie", cookie.toString());
    }

    @Transactional(readOnly = true)
    public InvitationView inspectInvitation(String rawToken) {
        HouseholdInvitation invitation = invitationRepository.findByTokenHash(hash(rawToken))
                .filter(value -> value.isUsable(LocalDateTime.now()))
                .orElseThrow(() -> new NotFoundException("This enrollment link is invalid or has expired"));
        return new InvitationView(invitation.getMember().getName(),
                invitation.getMember().getEmail(), invitation.getExpiresAt());
    }

    @Transactional
    public SessionIssue enroll(String rawToken, String deviceName,
                               String userAgent, String ip) {
        HouseholdInvitation invitation = invitationRepository
                .findForUpdateByTokenHash(hash(rawToken))
                .filter(value -> value.isUsable(LocalDateTime.now()))
                .orElseThrow(() -> new NotFoundException(
                        "This enrollment link is invalid, expired, or already used"));
        LocalDateTime now = LocalDateTime.now();
        invitation.use(now);
        invitationRepository.save(invitation);
        return createSession(invitation.getMember(), deviceName, userAgent, ip, now);
    }

    @Transactional
    public InvitationIssue inviteMember(String name, String email) {
        String normalizedEmail = validEmail(email);
        if (memberRepository.findByEmailIgnoreCaseAndDisabledAtIsNull(normalizedEmail).isPresent()) {
            throw new ConflictException("That email is already a household member");
        }
        HouseholdMember member = memberRepository.save(new HouseholdMember(
                validName(name), normalizedEmail, HouseholdMember.Role.MEMBER,
                LocalDateTime.now()));
        return createInvitation(member);
    }

    @Transactional
    public InvitationIssue inviteDevice(long memberId) {
        HouseholdMember member = enabledMember(memberId);
        return createInvitation(member);
    }

    @Transactional(readOnly = true)
    public List<MemberView> members() {
        return memberRepository.findAllByOrderByCreatedAtAsc().stream()
                .map(member -> new MemberView(member.getId(), member.getName(),
                        member.getEmail(), member.getRole(), member.getCreatedAt(),
                        member.getDisabledAt(), deviceRepository
                                .findByMemberOrderByCreatedAtDesc(member).stream()
                                .map(HouseholdAuthService::deviceView).toList()))
                .toList();
    }

    @Transactional
    public void revokeDevice(long deviceId) {
        HouseholdDeviceSession device = deviceRepository.findById(deviceId)
                .orElseThrow(() -> new NotFoundException("Device not found"));
        if (device.getRevokedAt() == null) {
            requireAnotherOwnerDevice(device);
            device.revoke(LocalDateTime.now());
            deviceRepository.save(device);
        }
    }

    @Transactional
    public void disableMember(long memberId) {
        HouseholdMember member = enabledMember(memberId);
        if (member.getRole() == HouseholdMember.Role.OWNER) {
            throw new ConflictException("The household owner cannot be disabled");
        }
        LocalDateTime now = LocalDateTime.now();
        member.disable(now);
        memberRepository.save(member);
        deviceRepository.findByMemberOrderByCreatedAtDesc(member).stream()
                .filter(device -> device.getRevokedAt() == null)
                .forEach(device -> device.revoke(now));
    }

    @Transactional
    public void logout(long sessionId) {
        deviceRepository.findById(sessionId).ifPresent(device -> {
            requireAnotherOwnerDevice(device);
            device.revoke(LocalDateTime.now());
            deviceRepository.save(device);
        });
    }

    @Transactional(readOnly = true)
    public boolean sessionIsActive(long sessionId) {
        LocalDateTime now = LocalDateTime.now();
        return deviceRepository.findById(sessionId)
                .map(device -> device.isActive(now))
                .orElse(false);
    }

    private SessionIssue createSession(HouseholdMember member, String deviceName,
                                       String userAgent, String ip,
                                       LocalDateTime now) {
        String rawToken = randomToken(32);
        HouseholdDeviceSession device = deviceRepository.save(
                new HouseholdDeviceSession(member, validDeviceName(deviceName),
                        hash(rawToken), now, now.plusDays(sessionDays),
                        truncate(userAgent, 255), truncate(ip, 64)));
        return new SessionIssue(rawToken, principal(device));
    }

    private InvitationIssue createInvitation(HouseholdMember member) {
        String rawToken = randomToken(32);
        LocalDateTime now = LocalDateTime.now();
        LocalDateTime expiresAt = now.plusHours(invitationHours);
        invitationRepository.save(new HouseholdInvitation(
                member, hash(rawToken), now, expiresAt));
        String baseUrl = dashboardUrl.replaceAll("/+$", "");
        return new InvitationIssue(member.getId(), member.getName(),
                member.getEmail(), baseUrl + "/enroll/" + rawToken, expiresAt);
    }

    private HouseholdMember enabledMember(long memberId) {
        return memberRepository.findById(memberId)
                .filter(HouseholdMember::isEnabled)
                .orElseThrow(() -> new NotFoundException("Household member not found"));
    }

    private void requireAnotherOwnerDevice(HouseholdDeviceSession device) {
        if (device.getMember().getRole() != HouseholdMember.Role.OWNER) return;
        LocalDateTime now = LocalDateTime.now();
        long activeDevices = deviceRepository
                .findByMemberOrderByCreatedAtDesc(device.getMember()).stream()
                .filter(candidate -> candidate.isActive(now))
                .count();
        if (activeDevices <= 1) {
            throw new ConflictException(
                    "Enroll another owner device before revoking the last one");
        }
    }

    private static Principal principal(HouseholdDeviceSession device) {
        HouseholdMember member = device.getMember();
        return new Principal(device.getId(), member.getId(), member.getName(),
                member.getEmail(), member.getRole(), device.getDeviceName());
    }

    private static DeviceView deviceView(HouseholdDeviceSession device) {
        return new DeviceView(device.getId(), device.getDeviceName(),
                device.getCreatedAt(), device.getLastSeenAt(), device.getExpiresAt(),
                device.getRevokedAt(), device.getUserAgent());
    }

    private static String validName(String value) {
        String normalized = value == null ? "" : value.trim();
        if (normalized.isEmpty() || normalized.length() > 100) {
            throw new ValidationException("Name must be between 1 and 100 characters");
        }
        return normalized;
    }

    private static String validDeviceName(String value) {
        String normalized = value == null ? "" : value.trim();
        if (normalized.isEmpty() || normalized.length() > 100) {
            throw new ValidationException("Device name must be between 1 and 100 characters");
        }
        return normalized;
    }

    private static String validEmail(String value) {
        String normalized = value == null ? "" : value.trim().toLowerCase(Locale.ROOT);
        if (normalized.length() > 254 || !EMAIL.matcher(normalized).matches()) {
            throw new ValidationException("Enter a valid email address");
        }
        return normalized;
    }

    private static String truncate(String value, int max) {
        if (value == null) return null;
        return value.length() <= max ? value : value.substring(0, max);
    }

    private static String randomToken(int bytes) {
        byte[] value = new byte[bytes];
        RANDOM.nextBytes(value);
        return Base64.getUrlEncoder().withoutPadding().encodeToString(value);
    }

    private static String hash(String rawToken) {
        try {
            byte[] digest = MessageDigest.getInstance("SHA-256")
                    .digest(rawToken.getBytes(StandardCharsets.UTF_8));
            return java.util.HexFormat.of().formatHex(digest);
        } catch (NoSuchAlgorithmException impossible) {
            throw new IllegalStateException(impossible);
        }
    }

    private static boolean constantTimeEquals(String expected, String supplied) {
        if (expected == null || supplied == null) return false;
        return MessageDigest.isEqual(expected.getBytes(StandardCharsets.UTF_8),
                supplied.getBytes(StandardCharsets.UTF_8));
    }

    public record Principal(long sessionId, long memberId, String name, String email,
                            HouseholdMember.Role role, String deviceName) {
        public boolean isOwner() { return role == HouseholdMember.Role.OWNER; }
    }
    public record SessionIssue(String rawToken, Principal principal) {}
    public record InvitationView(String name, String email, LocalDateTime expiresAt) {}
    public record InvitationIssue(long memberId, String name, String email,
                                  String enrollmentUrl, LocalDateTime expiresAt) {}
    public record MemberView(long id, String name, String email, HouseholdMember.Role role,
                             LocalDateTime createdAt, LocalDateTime disabledAt,
                             List<DeviceView> devices) {}
    public record DeviceView(long id, String name, LocalDateTime createdAt,
                             LocalDateTime lastSeenAt, LocalDateTime expiresAt,
                             LocalDateTime revokedAt, String userAgent) {}

    public static class ValidationException extends RuntimeException {
        public ValidationException(String message) { super(message); }
    }
    public static class UnauthorizedException extends RuntimeException {
        public UnauthorizedException(String message) { super(message); }
    }
    public static class ConflictException extends RuntimeException {
        public ConflictException(String message) { super(message); }
    }
    public static class ForbiddenException extends RuntimeException {
        public ForbiddenException(String message) { super(message); }
    }
    public static class NotFoundException extends RuntimeException {
        public NotFoundException(String message) { super(message); }
    }
    public static class ConfigurationException extends RuntimeException {
        public ConfigurationException(String message) { super(message); }
    }
}

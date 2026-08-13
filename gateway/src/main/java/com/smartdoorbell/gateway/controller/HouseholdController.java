package com.smartdoorbell.gateway.controller;

import com.smartdoorbell.gateway.service.HouseholdAuthService;
import org.springframework.http.ResponseEntity;
import org.springframework.web.bind.annotation.*;

import jakarta.servlet.http.HttpServletRequest;
import jakarta.servlet.http.HttpServletResponse;
import java.util.List;
import java.util.Map;

@RestController
@RequestMapping("/api/household")
public class HouseholdController {

    private final HouseholdAuthService householdAuth;

    public HouseholdController(HouseholdAuthService householdAuth) {
        this.householdAuth = householdAuth;
    }

    @GetMapping("/bootstrap/status")
    public BootstrapStatus bootstrapStatus() {
        return new BootstrapStatus(householdAuth.needsBootstrap(),
                householdAuth.bootstrapConfigured());
    }

    @PostMapping("/bootstrap")
    public HouseholdAuthService.Principal bootstrap(@RequestBody BootstrapRequest body,
                                                    HttpServletRequest request,
                                                    HttpServletResponse response) {
        var issue = householdAuth.bootstrap(body.setupToken(), body.name(), body.email(),
                body.deviceName(), request.getHeader("User-Agent"), request.getRemoteAddr());
        householdAuth.setSessionCookie(response, issue.rawToken());
        return issue.principal();
    }

    @GetMapping("/enroll/{token}")
    public HouseholdAuthService.InvitationView invitation(@PathVariable String token) {
        return householdAuth.inspectInvitation(token);
    }

    @PostMapping("/enroll/{token}")
    public HouseholdAuthService.Principal enroll(@PathVariable String token,
                                                 @RequestBody EnrollRequest body,
                                                 HttpServletRequest request,
                                                 HttpServletResponse response) {
        var issue = householdAuth.enroll(token, body.deviceName(),
                request.getHeader("User-Agent"), request.getRemoteAddr());
        householdAuth.setSessionCookie(response, issue.rawToken());
        return issue.principal();
    }

    @GetMapping("/session")
    public HouseholdAuthService.Principal session(HttpServletRequest request) {
        return principal(request);
    }

    @DeleteMapping("/session")
    public ResponseEntity<Void> logout(HttpServletRequest request,
                                       HttpServletResponse response) {
        householdAuth.logout(principal(request).sessionId());
        householdAuth.clearSessionCookie(response);
        return ResponseEntity.noContent().build();
    }

    @GetMapping("/members")
    public List<HouseholdAuthService.MemberView> members(HttpServletRequest request) {
        requireOwner(request);
        return householdAuth.members();
    }

    @PostMapping("/members")
    public HouseholdAuthService.InvitationIssue inviteMember(
            @RequestBody InviteMemberRequest body, HttpServletRequest request) {
        requireOwner(request);
        return householdAuth.inviteMember(body.name(), body.email());
    }

    @PostMapping("/members/{memberId}/invitations")
    public HouseholdAuthService.InvitationIssue inviteDevice(
            @PathVariable long memberId, HttpServletRequest request) {
        requireOwner(request);
        return householdAuth.inviteDevice(memberId);
    }

    @DeleteMapping("/members/{memberId}")
    public ResponseEntity<Void> disableMember(@PathVariable long memberId,
                                              HttpServletRequest request) {
        requireOwner(request);
        householdAuth.disableMember(memberId);
        return ResponseEntity.noContent().build();
    }

    @DeleteMapping("/devices/{deviceId}")
    public ResponseEntity<Void> revokeDevice(@PathVariable long deviceId,
                                             HttpServletRequest request) {
        requireOwner(request);
        householdAuth.revokeDevice(deviceId);
        return ResponseEntity.noContent().build();
    }

    private static HouseholdAuthService.Principal principal(HttpServletRequest request) {
        Object value = request.getAttribute(HouseholdAuthService.PRINCIPAL_ATTRIBUTE);
        if (value instanceof HouseholdAuthService.Principal principal) return principal;
        throw new HouseholdAuthService.UnauthorizedException("Authentication required");
    }

    private static void requireOwner(HttpServletRequest request) {
        if (!principal(request).isOwner()) {
            throw new HouseholdAuthService.ForbiddenException(
                    "Only the household owner can manage access");
        }
    }

    public record BootstrapStatus(boolean needsBootstrap, boolean setupTokenConfigured) {}
    public record BootstrapRequest(String setupToken, String name, String email,
                                   String deviceName) {}
    public record EnrollRequest(String deviceName) {}
    public record InviteMemberRequest(String name, String email) {}
}

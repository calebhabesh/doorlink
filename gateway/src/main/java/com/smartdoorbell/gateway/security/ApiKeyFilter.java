package com.smartdoorbell.gateway.security;

import com.fasterxml.jackson.databind.ObjectMapper;
import com.smartdoorbell.gateway.service.HouseholdAuthService;
import org.springframework.beans.factory.ObjectProvider;
import org.springframework.beans.factory.annotation.Value;
import org.springframework.http.MediaType;
import org.springframework.stereotype.Component;
import org.springframework.web.filter.OncePerRequestFilter;

import jakarta.servlet.FilterChain;
import jakarta.servlet.ServletException;
import jakarta.servlet.http.Cookie;
import jakarta.servlet.http.HttpServletRequest;
import jakarta.servlet.http.HttpServletResponse;
import java.io.IOException;
import java.util.Map;

/**
 * Keeps the hardware API key and household browser sessions as separate trust
 * paths. The ESP32 may only call its upload/playback endpoints; a browser must
 * present a revocable device cookie for every other API, including reads.
 */
@Component
public class ApiKeyFilter extends OncePerRequestFilter {

    private final ObjectProvider<HouseholdAuthService> householdAuthProvider;
    private final ObjectMapper objectMapper;

    @Value("${gateway.api.key:}")
    private String apiKey;

    public ApiKeyFilter(ObjectProvider<HouseholdAuthService> householdAuthProvider,
                        ObjectMapper objectMapper) {
        this.householdAuthProvider = householdAuthProvider;
        this.objectMapper = objectMapper;
    }

    @Override
    protected void doFilterInternal(HttpServletRequest request,
                                    HttpServletResponse response,
                                    FilterChain filterChain)
            throws ServletException, IOException {
        String path = request.getRequestURI();
        String method = request.getMethod();

        if (path.startsWith("/api/")) {
            response.setHeader("Cache-Control", "no-store");
        }

        if (!path.startsWith("/api/") || "OPTIONS".equalsIgnoreCase(method)
                || isPublicHouseholdEndpoint(path, method)) {
            filterChain.doFilter(request, response);
            return;
        }

        String requestApiKey = request.getHeader("X-API-Key");
        if (isHardwareEndpoint(path, method) && configuredApiKeyMatches(requestApiKey)) {
            filterChain.doFilter(request, response);
            return;
        }

        HouseholdAuthService householdAuth = householdAuthProvider.getIfAvailable();
        String rawToken = cookie(request, HouseholdAuthService.COOKIE_NAME);
        if (householdAuth != null && rawToken != null) {
            var principal = householdAuth.authenticate(rawToken, request.getRemoteAddr());
            if (principal.isPresent()) {
                request.setAttribute(HouseholdAuthService.PRINCIPAL_ATTRIBUTE,
                        principal.get());
                householdAuth.setSessionCookie(response, rawToken);
                filterChain.doFilter(request, response);
                return;
            }
        }

        if (householdAuth != null) householdAuth.clearSessionCookie(response);
        response.setStatus(HttpServletResponse.SC_UNAUTHORIZED);
        response.setContentType(MediaType.APPLICATION_JSON_VALUE);
        objectMapper.writeValue(response.getWriter(),
                Map.of("error", "This device is not enrolled or its access was revoked"));
    }

    private boolean configuredApiKeyMatches(String supplied) {
        // A missing production key must fail closed. Local development has an
        // explicit default in application.properties.
        return apiKey != null && !apiKey.isBlank() && apiKey.equals(supplied);
    }

    private static boolean isHardwareEndpoint(String path, String method) {
        if ("POST".equalsIgnoreCase(method) && path.startsWith("/api/events")) {
            return true;
        }
        if ("POST".equalsIgnoreCase(method)
                && path.startsWith("/api/system/ptt/messages/")) {
            return true;
        }
        return "GET".equalsIgnoreCase(method)
                && path.startsWith("/api/events/media/");
    }

    private static boolean isPublicHouseholdEndpoint(String path, String method) {
        if (path.equals("/api/household/bootstrap/status")
                && "GET".equalsIgnoreCase(method)) return true;
        if (path.equals("/api/household/bootstrap")
                && "POST".equalsIgnoreCase(method)) return true;
        return path.startsWith("/api/household/enroll/")
                && ("GET".equalsIgnoreCase(method) || "POST".equalsIgnoreCase(method));
    }

    private static String cookie(HttpServletRequest request, String name) {
        Cookie[] cookies = request.getCookies();
        if (cookies == null) return null;
        for (Cookie cookie : cookies) {
            if (name.equals(cookie.getName())) return cookie.getValue();
        }
        return null;
    }
}

package com.smartdoorbell.gateway.security;

import org.springframework.beans.factory.annotation.Value;
import org.springframework.stereotype.Component;
import org.springframework.web.filter.OncePerRequestFilter;

import jakarta.servlet.FilterChain;
import jakarta.servlet.ServletException;
import jakarta.servlet.http.HttpServletRequest;
import jakarta.servlet.http.HttpServletResponse;
import java.io.IOException;

@Component
public class ApiKeyFilter extends OncePerRequestFilter {

    @Value("${gateway.api.key:}")
    private String apiKey;

    @Override
    protected void doFilterInternal(HttpServletRequest request, HttpServletResponse response, FilterChain filterChain)
            throws ServletException, IOException {

        // Only enforce API Key on POST/PUT requests to /api/events
        String path = request.getRequestURI();
        String method = request.getMethod();

        if (path.startsWith("/api/events") && (method.equalsIgnoreCase("POST") || method.equalsIgnoreCase("PUT"))) {
            
            // If apiKey is not configured, allow all (for initial setup)
            if (apiKey == null || apiKey.trim().isEmpty()) {
                filterChain.doFilter(request, response);
                return;
            }

            String requestApiKey = request.getHeader("X-API-Key");

            if (apiKey.equals(requestApiKey)) {
                filterChain.doFilter(request, response);
            } else {
                response.setStatus(HttpServletResponse.SC_UNAUTHORIZED);
                response.getWriter().write("Unauthorized: Invalid or missing API Key");
            }
        } else {
            // For other paths (e.g. GET /api/events/stream or dashboard access), skip for now
            // or implement dashboard-specific auth later.
            filterChain.doFilter(request, response);
        }
    }
}

package com.smartdoorbell.gateway.controller;

import com.smartdoorbell.gateway.service.HouseholdAuthService;
import org.springframework.http.HttpStatus;
import org.springframework.http.ResponseEntity;
import org.springframework.web.bind.annotation.ExceptionHandler;
import org.springframework.web.bind.annotation.RestControllerAdvice;

import java.util.Map;

@RestControllerAdvice(assignableTypes = HouseholdController.class)
public class HouseholdExceptionHandler {

    @ExceptionHandler(HouseholdAuthService.ValidationException.class)
    ResponseEntity<Map<String, String>> validation(RuntimeException exception) {
        return response(HttpStatus.BAD_REQUEST, exception);
    }

    @ExceptionHandler(HouseholdAuthService.UnauthorizedException.class)
    ResponseEntity<Map<String, String>> unauthorized(RuntimeException exception) {
        return response(HttpStatus.UNAUTHORIZED, exception);
    }

    @ExceptionHandler(HouseholdAuthService.ConflictException.class)
    ResponseEntity<Map<String, String>> conflict(RuntimeException exception) {
        return response(HttpStatus.CONFLICT, exception);
    }

    @ExceptionHandler(HouseholdAuthService.ForbiddenException.class)
    ResponseEntity<Map<String, String>> forbidden(RuntimeException exception) {
        return response(HttpStatus.FORBIDDEN, exception);
    }

    @ExceptionHandler(HouseholdAuthService.NotFoundException.class)
    ResponseEntity<Map<String, String>> notFound(RuntimeException exception) {
        return response(HttpStatus.NOT_FOUND, exception);
    }

    @ExceptionHandler(HouseholdAuthService.ConfigurationException.class)
    ResponseEntity<Map<String, String>> configuration(RuntimeException exception) {
        return response(HttpStatus.SERVICE_UNAVAILABLE, exception);
    }

    private static ResponseEntity<Map<String, String>> response(
            HttpStatus status, RuntimeException exception) {
        return ResponseEntity.status(status).body(Map.of("error", exception.getMessage()));
    }
}

package com.smartdoorbell.gateway.security;

import org.junit.jupiter.api.Test;

import java.util.LinkedHashMap;
import java.util.Map;

import static org.junit.jupiter.api.Assertions.assertEquals;
import static org.junit.jupiter.api.Assertions.assertNotEquals;
import static org.junit.jupiter.api.Assertions.assertTrue;

class IntercomCommandSignerTest {

    @Test
    void canonicalSignatureMatchesFirmwareProtocolVector() {
        Map<String, Object> command = new LinkedHashMap<>();
        command.put("protocol", "v1");
        command.put("commandId", "123e4567-e89b-12d3-a456-426614174000");
        command.put("type", "PLAY_AUDIO");
        command.put("eventId", "0123456789abcdef0123456789abcdef");
        command.put("messageId", "123e4567-e89b-12d3-a456-426614174001");
        command.put("audioUrl", "http://gateway:8080/api/events/media/a.wav");
        command.put("ackUrl", "http://gateway:8080/api/system/ptt/messages/"
                + "123e4567-e89b-12d3-a456-426614174001/delivered");

        assertEquals("49cdff76238207c12ad7986b96f7cad187287ce0fee67addb950226a8d867ddd",
                IntercomCommandSigner.calculateSignature(command, "test-api-key"));
    }

    @Test
    void signAddsUniqueReplayIdentifierAndAuthenticationFields() {
        Map<String, Object> command = Map.of(
                "type", "PTT_START",
                "eventId", "0123456789abcdef0123456789abcdef");

        Map<String, Object> first = IntercomCommandSigner.sign(command, "secret");
        Map<String, Object> second = IntercomCommandSigner.sign(command, "secret");

        assertEquals("v1", first.get("protocol"));
        assertTrue(first.get("commandId").toString()
                .matches("[0-9a-f]{8}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{12}"));
        assertTrue(first.get("signature").toString().matches("[0-9a-f]{64}"));
        assertNotEquals(first.get("commandId"), second.get("commandId"));
        assertNotEquals(first.get("signature"), second.get("signature"));
    }
}

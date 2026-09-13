package com.smartdoorbell.gateway.security;

import javax.crypto.Mac;
import javax.crypto.spec.SecretKeySpec;
import java.nio.charset.StandardCharsets;
import java.security.GeneralSecurityException;
import java.util.HexFormat;
import java.util.LinkedHashMap;
import java.util.Map;
import java.util.UUID;

/**
 * Signs the exact MQTT command fields consumed by the doorbell firmware.
 * JSON ordering and whitespace are intentionally outside the signature.
 */
public final class IntercomCommandSigner {

    private static final String PROTOCOL = "v1";
    private static final String DOMAIN = "smart-doorbell-intercom-v1";

    private IntercomCommandSigner() {
    }

    public static Map<String, Object> sign(Map<String, Object> command,
                                           String sharedSecret) {
        if (command == null || sharedSecret == null || sharedSecret.isBlank()) {
            throw new IllegalArgumentException("Intercom command signing requires a shared secret");
        }
        Map<String, Object> signed = new LinkedHashMap<>(command);
        signed.put("protocol", PROTOCOL);
        signed.put("commandId", UUID.randomUUID().toString());
        signed.put("signature", calculateSignature(signed, sharedSecret));
        return signed;
    }

    static String calculateSignature(Map<String, Object> command,
                                     String sharedSecret) {
        try {
            Mac mac = Mac.getInstance("HmacSHA256");
            mac.init(new SecretKeySpec(
                    sharedSecret.getBytes(StandardCharsets.UTF_8), "HmacSHA256"));
            update(mac, DOMAIN);
            update(mac, value(command, "protocol"));
            update(mac, value(command, "commandId"));
            update(mac, value(command, "type"));
            update(mac, value(command, "eventId"));
            update(mac, value(command, "messageId"));
            update(mac, value(command, "audioUrl"));
            update(mac, value(command, "ackUrl"));
            return HexFormat.of().formatHex(mac.doFinal());
        } catch (GeneralSecurityException exception) {
            throw new IllegalStateException("HmacSHA256 is unavailable", exception);
        }
    }

    private static String value(Map<String, Object> command, String name) {
        Object value = command.get(name);
        return value == null ? "" : value.toString();
    }

    private static void update(Mac mac, String value) {
        mac.update(value.getBytes(StandardCharsets.UTF_8));
        mac.update((byte) 0);
    }
}

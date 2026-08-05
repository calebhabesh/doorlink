package com.smartdoorbell.gateway.service.chime;

import com.smartdoorbell.gateway.service.ChimeService;
import org.springframework.boot.autoconfigure.condition.ConditionalOnProperty;
import org.springframework.stereotype.Service;

@Service
@ConditionalOnProperty(name = "chime.provider", havingValue = "none", matchIfMissing = true)
public class NoopChimeService implements ChimeService {
    @Override
    public void ring(String eventType) {
        // Do nothing when disabled or provider is 'none'
    }
}

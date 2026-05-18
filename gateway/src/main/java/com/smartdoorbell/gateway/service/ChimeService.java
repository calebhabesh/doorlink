package com.smartdoorbell.gateway.service;

import com.smartdoorbell.gateway.entity.Event;

public interface ChimeService {
    void ring(Event event);
}
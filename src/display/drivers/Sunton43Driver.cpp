/**
 * @file      Sunton43Driver.cpp
 * @brief     Driver implementation for Sunton ESP32-8048S043C 4.3" 800x480 RGB LCD
 */
#include "Sunton43Driver.h"
#include <Arduino.h>
#include <display/drivers/common/LV_Helper.h>

Sunton43Driver *Sunton43Driver::instance = nullptr;

bool Sunton43Driver::isCompatible() {
    // This driver is used as a fallback when no other driver is detected
    // or can be explicitly selected via build flags
    return true;
}

void Sunton43Driver::init() {
    printf("Initializing Sunton 4.3\" 800x480 driver\n");
    if (!panel.begin()) {
        for (uint8_t i = 0; i < 20; i++) {
            Serial.println(F("Error, failed to initialize Sunton 4.3\" panel"));
            delay(1000);
        }
        ESP.restart();
    }
    beginLvglHelper(panel);
}

bool Sunton43Driver::supportsSDCard() { return true; }

bool Sunton43Driver::installSDCard() { return panel.installSD(); }

/**
 * @file      SuntonDriver.cpp
 * @brief     Driver implementation for Sunton/Elecrow ESP32-S3 5" 800x480 RGB LCD
 */
#include "SuntonDriver.h"
#include <Arduino.h>
#include <display/drivers/common/LV_Helper.h>

SuntonDriver *SuntonDriver::instance = nullptr;

bool SuntonDriver::isCompatible() {
    // This driver is used as a fallback when no other driver is detected
    // or can be explicitly selected via build flags
    return true;
}

void SuntonDriver::init() {
    printf("Initializing Sunton 5\" 800x480 driver\n");
    if (!panel.begin()) {
        for (uint8_t i = 0; i < 20; i++) {
            Serial.println(F("Error, failed to initialize Sunton panel"));
            delay(1000);
        }
        ESP.restart();
    }
    beginLvglHelper(panel);
}

bool SuntonDriver::supportsSDCard() { return true; }

bool SuntonDriver::installSDCard() { return panel.installSD(); }

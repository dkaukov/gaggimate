#ifndef PLATFORM_SYSTEM_H
#define PLATFORM_SYSTEM_H

/**
 * @brief Cross-platform system utilities
 *
 * Provides unified system functions that work on both ESP32 and STM32.
 */

#include <Arduino.h>

namespace Platform {

/**
 * @brief Restart the system
 */
inline void systemRestart() {
#ifdef ESP32
    ESP.restart();
#elif defined(STM32)
    NVIC_SystemReset();
#else
    // Generic reset - this may not work on all platforms
    void (*resetFunc)(void) = 0;
    resetFunc();
#endif
}

/**
 * @brief Read analog voltage in millivolts
 * @param pin Analog pin number
 * @return Voltage in millivolts
 */
inline uint16_t readAnalogMillivolts(uint8_t pin) {
#ifdef ESP32
    return analogReadMilliVolts(pin);
#else
    // STM32 and other platforms: Convert ADC reading to millivolts
    // Assuming 12-bit ADC and 3.3V reference
    uint16_t rawValue = analogRead(pin);
    return static_cast<uint16_t>((rawValue * 3300UL) / 4095UL);
#endif
}

/**
 * @brief Get unique chip ID
 * @return Chip ID as uint64_t
 */
inline uint64_t getChipId() {
#ifdef ESP32
    return ESP.getEfuseMac();
#elif defined(STM32)
    // STM32 unique device ID is at specific memory addresses
    uint32_t *uid = (uint32_t *)0x1FFF7A10; // Location for STM32F4xx
    return ((uint64_t)uid[2] << 32) | uid[1];
#else
    return 0;
#endif
}

/**
 * @brief Get free heap memory
 * @return Free heap in bytes
 */
inline size_t getFreeHeap() {
#ifdef ESP32
    return ESP.getFreeHeap();
#else
    // For STM32, this would require malloc tracking or heap inspection
    // Return 0 as a placeholder
    return 0;
#endif
}

} // namespace Platform

#endif // PLATFORM_SYSTEM_H

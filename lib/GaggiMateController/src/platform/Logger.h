#ifndef PLATFORM_LOGGER_H
#define PLATFORM_LOGGER_H

/**
 * @brief Cross-platform logging abstraction
 *
 * Provides unified logging macros that work on both ESP32 and STM32.
 * On ESP32, maps to ESP_LOG* macros.
 * On STM32, maps to Serial.printf or similar.
 */

#ifdef ESP32

// Use ESP-IDF logging on ESP32
#include <esp_log.h>

#define LOG_E(tag, format, ...) ESP_LOGE(tag, format, ##__VA_ARGS__)
#define LOG_W(tag, format, ...) ESP_LOGW(tag, format, ##__VA_ARGS__)
#define LOG_I(tag, format, ...) ESP_LOGI(tag, format, ##__VA_ARGS__)
#define LOG_D(tag, format, ...) ESP_LOGD(tag, format, ##__VA_ARGS__)
#define LOG_V(tag, format, ...) ESP_LOGV(tag, format, ##__VA_ARGS__)

#else // STM32 or other platforms

#include <Arduino.h>

// Define log levels
#ifndef LOG_LEVEL
#define LOG_LEVEL 3 // Default to INFO level
#endif

#define LOG_LEVEL_NONE 0
#define LOG_LEVEL_ERROR 1
#define LOG_LEVEL_WARN 2
#define LOG_LEVEL_INFO 3
#define LOG_LEVEL_DEBUG 4
#define LOG_LEVEL_VERBOSE 5

// Helper macro for formatted printing with tag
#define _LOG_PRINT(level, tag, format, ...)                                                                            \
    do {                                                                                                               \
        Serial.printf("[%s] %s: ", level, tag);                                                                        \
        Serial.printf(format, ##__VA_ARGS__);                                                                          \
        Serial.println();                                                                                              \
    } while (0)

#if LOG_LEVEL >= LOG_LEVEL_ERROR
#define LOG_E(tag, format, ...) _LOG_PRINT("E", tag, format, ##__VA_ARGS__)
#else
#define LOG_E(tag, format, ...)
#endif

#if LOG_LEVEL >= LOG_LEVEL_WARN
#define LOG_W(tag, format, ...) _LOG_PRINT("W", tag, format, ##__VA_ARGS__)
#else
#define LOG_W(tag, format, ...)
#endif

#if LOG_LEVEL >= LOG_LEVEL_INFO
#define LOG_I(tag, format, ...) _LOG_PRINT("I", tag, format, ##__VA_ARGS__)
#else
#define LOG_I(tag, format, ...)
#endif

#if LOG_LEVEL >= LOG_LEVEL_DEBUG
#define LOG_D(tag, format, ...) _LOG_PRINT("D", tag, format, ##__VA_ARGS__)
#else
#define LOG_D(tag, format, ...)
#endif

#if LOG_LEVEL >= LOG_LEVEL_VERBOSE
#define LOG_V(tag, format, ...) _LOG_PRINT("V", tag, format, ##__VA_ARGS__)
#else
#define LOG_V(tag, format, ...)
#endif

#endif // ESP32

#endif // PLATFORM_LOGGER_H

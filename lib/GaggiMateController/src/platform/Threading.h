#ifndef PLATFORM_THREADING_H
#define PLATFORM_THREADING_H

/**
 * @brief Cross-platform threading abstraction
 *
 * On ESP32, FreeRTOS is built-in. On STM32 with Arduino, FreeRTOS
 * may not be available, so we provide fallback definitions.
 */

#include <Arduino.h>

#ifdef ESP32
// ESP32 has FreeRTOS built-in with espressif paths
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#else
// STM32 with STM32duino FreeRTOS library
#include <STM32FreeRTOS.h>

// STM32duino FreeRTOS uses TaskHandle_t, provide xTaskHandle alias
typedef TaskHandle_t xTaskHandle;

// STM32duino FreeRTOS uses vTaskDelayUntil, ESP32 uses xTaskDelayUntil
#define xTaskDelayUntil vTaskDelayUntil
#endif // ESP32

/**
 * @brief Create a FreeRTOS task
 *
 * @param taskFunction Task function pointer
 * @param taskName Name of the task (for debugging)
 * @param stackSize Stack size in words (not bytes)
 * @param parameters Task parameters
 * @param priority Task priority
 * @param taskHandle Pointer to task handle
 * @return pdPASS on success, pdFAIL on failure
 */
#define PLATFORM_TASK_CREATE(taskFunction, taskName, stackSize, parameters, priority, taskHandle)                      \
    xTaskCreate(taskFunction, taskName, stackSize, parameters, priority, taskHandle)

/**
 * @brief Create a FreeRTOS task pinned to a specific core (ESP32 only)
 *
 * On non-ESP32 platforms, this falls back to regular xTaskCreate.
 *
 * @param taskFunction Task function pointer
 * @param taskName Name of the task
 * @param stackSize Stack size in words
 * @param parameters Task parameters
 * @param priority Task priority
 * @param taskHandle Pointer to task handle
 * @param core Core to pin the task to
 * @return pdPASS on success, pdFAIL on failure
 */
#ifdef ESP32
#define PLATFORM_TASK_CREATE_PINNED(taskFunction, taskName, stackSize, parameters, priority, taskHandle, core)         \
    xTaskCreatePinnedToCore(taskFunction, taskName, stackSize, parameters, priority, taskHandle, core)
#else
#define PLATFORM_TASK_CREATE_PINNED(taskFunction, taskName, stackSize, parameters, priority, taskHandle, core)         \
    xTaskCreate(taskFunction, taskName, stackSize, parameters, priority, taskHandle)
#endif

/**
 * @brief Delay task for specified milliseconds
 */
#define PLATFORM_DELAY_MS(ms) vTaskDelay(pdMS_TO_TICKS(ms))

/**
 * @brief Delay task until specified tick count
 */
#define PLATFORM_DELAY_UNTIL(lastWakeTime, intervalMs) xTaskDelayUntil(lastWakeTime, pdMS_TO_TICKS(intervalMs))

/**
 * @brief Get current tick count
 */
#define PLATFORM_GET_TICK_COUNT() xTaskGetTickCount()

/**
 * @brief Minimum stack size for a task
 */
#ifndef PLATFORM_MIN_STACK_SIZE
#define PLATFORM_MIN_STACK_SIZE configMINIMAL_STACK_SIZE
#endif

/**
 * @brief Default stack size multiplier for peripheral tasks
 */
#define PLATFORM_PERIPHERAL_STACK_SIZE (PLATFORM_MIN_STACK_SIZE * 4)

#endif // PLATFORM_THREADING_H

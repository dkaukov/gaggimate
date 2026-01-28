#ifndef PLATFORM_THREADING_H
#define PLATFORM_THREADING_H

/**
 * @brief Cross-platform threading abstraction
 *
 * Both ESP32 and STM32 support FreeRTOS, so we use FreeRTOS directly.
 * This header provides convenience macros and compatibility definitions.
 */

#include <Arduino.h>

// FreeRTOS is available on both ESP32 and STM32 (with Arduino)
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

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

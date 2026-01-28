#ifndef CONTROLLERCONFIGSTM32_H
#define CONTROLLERCONFIGSTM32_H

#include "ControllerConfig.h"

/**
 * @brief STM32 BlackPill F411CE Pin Mapping for GaggiMate Controller
 *
 * This configuration file defines the pin assignments for the STM32 BlackPill F411CE
 * development board when used as a GaggiMate controller.
 *
 * BlackPill F411CE Pinout Reference:
 * - PA0-PA15: Port A GPIO
 * - PB0-PB15: Port B GPIO
 * - PC13-PC15: Port C GPIO (limited)
 *
 * Timer Assignments:
 * - TIM1: Heater PWM (PA8)
 * - TIM3: Pump PWM (PA6/PA7)
 * - TIM4: Available for future use
 *
 * Communication:
 * - USART1: Serial comm to display (PA9/PA10)
 * - I2C1: Pressure sensor (PB6/PB7)
 * - SPI1: Thermocouple (PA5/PA6/PA7)
 */

// STM32 Pin Definitions for BlackPill F411CE
// Using Arduino pin numbering convention for STM32duino

// GPIO Pins
#define STM32_HEATER_PIN PA8     // TIM1_CH1 for PWM heater control
#define STM32_PUMP_PIN PA6       // TIM3_CH1 for pump PWM
#define STM32_PUMP_SENSE_PIN PA0 // Zero-cross detection (EXTI)
#define STM32_VALVE_PIN PB12     // Solenoid valve relay
#define STM32_ALT_PIN PB13       // Alternative relay (grind motor)
#define STM32_BREW_BTN_PIN PB0   // Brew button input
#define STM32_STEAM_BTN_PIN PB1  // Steam button input

// SPI1 for MAX31855 Thermocouple
#define STM32_MAX_SCK_PIN PA5  // SPI1_SCK
#define STM32_MAX_MISO_PIN PA7 // SPI1_MISO (shared with pump)
#define STM32_MAX_CS_PIN PA4   // SPI1_NSS (chip select)

// I2C1 for Pressure Sensor (ADS1115)
#define STM32_PRESSURE_SCL_PIN PB6 // I2C1_SCL
#define STM32_PRESSURE_SDA_PIN PB7 // I2C1_SDA

// USART1 for Serial Communication
#define STM32_SERIAL_TX_PIN PA9  // USART1_TX
#define STM32_SERIAL_RX_PIN PA10 // USART1_RX

// Optional Extension Pins
#define STM32_EXT1_PIN PB3
#define STM32_EXT2_PIN PB4
#define STM32_EXT3_PIN PB5
#define STM32_EXT4_PIN PA15
#define STM32_EXT5_PIN PB14

// LED Controller I2C (if different from pressure sensor)
#define STM32_LED_SCL_PIN PB10 // I2C2_SCL
#define STM32_LED_SDA_PIN PB9  // I2C2_SDA (alternative for second I2C bus)

/**
 * @brief STM32 BlackPill F411CE Pro Configuration (with dimming and pressure)
 */
const ControllerConfig GM_STM32_PRO = {.name = "GaggiMate STM32 Pro",
                                       .autodetectValue = 10, // Distinct value for STM32
                                       .heaterPin = PA8,
                                       .pumpPin = PA6,
                                       .pumpSensePin = PA0,
                                       .pumpOn = 1,
                                       .valvePin = PB12,
                                       .valveOn = 1,
                                       .altPin = PB13,
                                       .altOn = 1,
                                       .pressureScl = PB6,
                                       .pressureSda = PB7,
                                       .maxSckPin = PA5,
                                       .maxCsPin = PA4,
                                       .maxMisoPin = PA7,
                                       .brewButtonPin = PB0,
                                       .steamButtonPin = PB1,
                                       .scaleSclPin = 0, // Not used on STM32
                                       .scaleSdaPin = 0,
                                       .scaleSda1Pin = 0,
                                       .sunriseSclPin = PB10,
                                       .sunriseSdaPin = PB9,
                                       .ext1Pin = PB3,
                                       .ext2Pin = PB4,
                                       .ext3Pin = PB5,
                                       .ext4Pin = PA15,
                                       .ext5Pin = PB14,
                                       .capabilites = {
                                           .dimming = true,
                                           .pressure = true,
                                           .ssrPump = false,
                                           .ledControls = false,
                                           .tof = false,
                                       }};

/**
 * @brief STM32 BlackPill F411CE Standard Configuration (basic, no dimming)
 */
const ControllerConfig GM_STM32_STANDARD = {.name = "GaggiMate STM32 Standard",
                                            .autodetectValue = 11, // Distinct value for STM32
                                            .heaterPin = PA8,
                                            .pumpPin = PA6,
                                            .pumpSensePin = 0, // No zero-cross sensing
                                            .pumpOn = 1,
                                            .valvePin = PB12,
                                            .valveOn = 1,
                                            .altPin = PB13,
                                            .altOn = 1,
                                            .pressureScl = 0, // No pressure sensor
                                            .pressureSda = 0,
                                            .maxSckPin = PA5,
                                            .maxCsPin = PA4,
                                            .maxMisoPin = PA7,
                                            .brewButtonPin = PB0,
                                            .steamButtonPin = PB1,
                                            .scaleSclPin = 0,
                                            .scaleSdaPin = 0,
                                            .scaleSda1Pin = 0,
                                            .sunriseSclPin = 0,
                                            .sunriseSdaPin = 0,
                                            .ext1Pin = PB3,
                                            .ext2Pin = PB4,
                                            .ext3Pin = PB5,
                                            .ext4Pin = PA15,
                                            .ext5Pin = PB14,
                                            .capabilites = {
                                                .dimming = false,
                                                .pressure = false,
                                                .ssrPump = true,
                                                .ledControls = false,
                                                .tof = false,
                                            }};

#endif // CONTROLLERCONFIGSTM32_H

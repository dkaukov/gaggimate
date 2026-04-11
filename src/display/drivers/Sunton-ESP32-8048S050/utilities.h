/**
 * @file      utilities.h
 * @brief     Pin definitions for Sunton/Elecrow ESP32-S3 5" 800x480 RGB LCD
 * @note      ESP32-8048S050 (5.0 inch, 800x480, GT911 touch)
 */
#pragma once

// Display resolution
#define SUNTON_BOARD_TFT_WIDTH (800)
#define SUNTON_BOARD_TFT_HEIGHT (480)

// RGB timing
#define SUNTON_RGB_TIMING_FREQ_HZ (14000000UL)
#define SUNTON_RGB_HSYNC_PULSE_WIDTH (4)
#define SUNTON_RGB_HSYNC_BACK_PORCH (8)
#define SUNTON_RGB_HSYNC_FRONT_PORCH (8)
#define SUNTON_RGB_VSYNC_PULSE_WIDTH (4)
#define SUNTON_RGB_VSYNC_BACK_PORCH (8)
#define SUNTON_RGB_VSYNC_FRONT_PORCH (8)

// Backlight PWM
#define SUNTON_BOARD_TFT_BL GPIO_NUM_2
#define SUNTON_PWM_CHANNEL 0
#define SUNTON_PWM_FREQ 4000
#define SUNTON_PWM_RESOLUTION 8
#define SUNTON_BACKLIGHT_MIN 100
#define SUNTON_BACKLIGHT_MAX 255

// RGB control signals
#define SUNTON_BOARD_TFT_HSYNC (39)
#define SUNTON_BOARD_TFT_VSYNC (41)
#define SUNTON_BOARD_TFT_DE (40)
#define SUNTON_BOARD_TFT_PCLK (42)

// RGB565 data pins (directly mapped to GPIO)
// Blue channel (5 bits)
#define SUNTON_LCD_PIN_NUM_RGB_DATA0 (8)   // B0
#define SUNTON_LCD_PIN_NUM_RGB_DATA1 (3)   // B1
#define SUNTON_LCD_PIN_NUM_RGB_DATA2 (46)  // B2
#define SUNTON_LCD_PIN_NUM_RGB_DATA3 (9)   // B3
#define SUNTON_LCD_PIN_NUM_RGB_DATA4 (1)   // B4

// Green channel (6 bits)
#define SUNTON_LCD_PIN_NUM_RGB_DATA5 (5)   // G0
#define SUNTON_LCD_PIN_NUM_RGB_DATA6 (6)   // G1
#define SUNTON_LCD_PIN_NUM_RGB_DATA7 (7)   // G2
#define SUNTON_LCD_PIN_NUM_RGB_DATA8 (15)  // G3
#define SUNTON_LCD_PIN_NUM_RGB_DATA9 (16)  // G4
#define SUNTON_LCD_PIN_NUM_RGB_DATA10 (4)  // G5

// Red channel (5 bits)
#define SUNTON_LCD_PIN_NUM_RGB_DATA11 (45) // R0
#define SUNTON_LCD_PIN_NUM_RGB_DATA12 (48) // R1
#define SUNTON_LCD_PIN_NUM_RGB_DATA13 (47) // R2
#define SUNTON_LCD_PIN_NUM_RGB_DATA14 (21) // R3
#define SUNTON_LCD_PIN_NUM_RGB_DATA15 (14) // R4

// I2C for touch
#define SUNTON_BOARD_I2C_SDA (19)
#define SUNTON_BOARD_I2C_SCL (20)

// Touch interrupt (active low)
#define SUNTON_BOARD_TOUCH_IRQ (18)
#define SUNTON_BOARD_TOUCH_RST (-1) // No reset pin on this board

// SD Card (directly connected SDMMC)
#define SUNTON_BOARD_SDMMC_CLK (12)
#define SUNTON_BOARD_SDMMC_CMD (11)
#define SUNTON_BOARD_SDMMC_D0 (13)

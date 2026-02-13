#pragma once
#include <Arduino.h>

// ===================== PIN CONFIG (ESP32-S3 SuperMini) =====================
// PINES COMPATIBLES CON MagSenseUI

// Outputs to motor driver (through optocouplers if 24V logic)
#define PIN_CLOCK 1
#define PIN_DIR 2
#define PIN_BRAKE 3   // optional per profile
#define PIN_STOP 4    // optional per profile
#define PIN_ENABLE 11 // optional per profile

// Inputs from motor driver
#define PIN_FG 12 // tachometer (interrupt)
#define PIN_LD 13 // alarm input

// OLED I2C (IGUAL QUE MagSenseUI: SDA=9, SCL=10)
#define PIN_OLED_SDA 9
#define PIN_OLED_SCL 10

// Buttons (active LOW) - IGUAL QUE MagSenseUI
#define PIN_BTN_UP 5
#define PIN_BTN_DOWN 6
#define PIN_BTN_SEL 7

// LEDC CLOCK generation
#define LEDC_CH_CLOCK 0
#define LEDC_TIMER_BITS 8

// Limits
#define MAX_PROFILES 8

// Long press thresholds (ms)
#define LONG_PRESS_MS 600

// Sampling for RPM (ms)
#define RPM_SAMPLE_MS 1000

// Debug flags
#define DEBUG_BUTTONS 0    // Set to 1 to enable button debug
#define DEBUG_MOTOR 0      // Set to 1 to enable motor debug
#define DEBUG_SPEED 1      // Set to 1 to enable speed change debug

// Language
enum Language
{
    LANG_EN = 0,
    LANG_ES = 1
};

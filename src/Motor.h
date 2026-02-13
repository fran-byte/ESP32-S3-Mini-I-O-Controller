#pragma once
#include <Arduino.h>
#include <Preferences.h>
#include "Config.h"
#include "Profiles.h"

// Simple, header-only max helper to avoid <algorithm> on embedded targets.
template <typename T>
T simple_max(T a, T b) { return (a > b) ? a : b; }

class MotorRuntime
{
public:
    void begin()
    {
        // ---------------- GPIO directions ----------------
        pinMode(PIN_CLOCK,  OUTPUT);     // PWM/clock output for motor
        pinMode(PIN_DIR,    OUTPUT);     // Direction output
        pinMode(PIN_BRAKE,  OUTPUT);     // Optional brake line
        pinMode(PIN_STOP,   OUTPUT);     // Optional stop line
        pinMode(PIN_ENABLE, OUTPUT);     // Optional enable line

        pinMode(PIN_FG, INPUT_PULLUP);   // Tachometer input (FG), active edge = RISING
        pinMode(PIN_LD, INPUT_PULLUP);   // Fault/alarm input (LD), polarity set by profile

        // ---------------- LEDC clock setup ----------------
        // Attach LEDC (ESP32 PWM) to PIN_CLOCK with an initial frequency and resolution.
        // We start at 1 kHz and 8-bit resolution, but reattach dynamically in setClock().
        ledcAttach(PIN_CLOCK, 1000, LEDC_TIMER_BITS);
        ledcWrite(PIN_CLOCK, 0);         // Duty 0% (motor stopped)

        // ---------------- Tachometer ISR ------------------
        // Count FG pulses on rising edge to compute RPM periodically.
        attachInterrupt(digitalPinToInterrupt(PIN_FG), isrFG, RISING);

        // ---------------- System settings (NVS) -----------
        // Load persisted telemetry and language preferences.
        sysPrefs.begin("sys", false);
        telemetryOn = sysPrefs.getBool("tele", false);
        lang = (Language)sysPrefs.getUChar("lang", (uint8_t)LANG_ES);
        sysPrefs.end();

#if DEBUG_MOTOR
        Serial.println("Motor initialized");
        Serial.print("Telemetry: ");
        Serial.println(telemetryOn ? "ON" : "OFF");
#endif
    }

    // Apply a new motor profile (I/O capabilities, limits, polarities, etc.)
    // Resets runtime flags and targets to safe defaults.
    void applyProfile(const MotorProfile &p)
    {
        prof     = p;
        dirCW    = true;
        brakeOn  = false;
        enabled  = true;
        running  = false;
        targetHz = 1000;       // Default target clock (Hz)
        applyOutputs();

#if DEBUG_MOTOR
        Serial.print("Profile applied: ");
        Serial.println(prof.name);
#endif
    }

    // Push current runtime control state to hardware pins, honoring profile options:
    //  - Direction line always active
    //  - Brake/Enable/Stop only if present in profile, with correct active polarity
    void applyOutputs()
    {
        digitalWrite(PIN_DIR, dirCW ? HIGH : LOW);

        if (prof.hasBrake)
            digitalWrite(PIN_BRAKE, brakeOn ? HIGH : LOW);

        if (prof.hasEnable)
        {
            bool level = prof.enableActiveHigh ? enabled : !enabled;
            digitalWrite(PIN_ENABLE, level ? HIGH : LOW);
        }

        if (prof.hasStop)
        {
            // When not running, assert STOP according to profile polarity.
            bool active = !running;
            bool level = prof.stopActiveHigh ? active : !active;
            digitalWrite(PIN_STOP, level ? HIGH : LOW);
        }
    }

    // Start the motor at the current target frequency.
    void start()
    {
        running = true;
        setClock(targetHz);
        applyOutputs();

#if DEBUG_MOTOR
        Serial.print("Motor STARTED at ");
        Serial.print(targetHz);
        Serial.println(" Hz");
#endif
    }

    // Stop the motor (clock = 0 Hz) and update control lines accordingly.
    void stop()
    {
        running = false;
        setClock(0);
        applyOutputs();

#if DEBUG_MOTOR
        Serial.println("Motor STOPPED");
#endif
    }

    // Configure the LEDC clock frequency and duty cycle.
    // Re-attaches LEDC with the requested frequency to minimize jitter.
    void setClock(uint32_t hz)
    {
        if (hz == 0)
        {
            // Duty 0% to ensure no pulses; keep last configured frequency irrelevant.
            ledcWrite(PIN_CLOCK, 0);
            currentHz = 0;
            return;
        }

        // Enforce profile limit.
        if (hz > prof.maxClockHz)
            hz = prof.maxClockHz;

        // Reconfigure LEDC to the new frequency with the chosen resolution.
        // Note: ledcDetach/Attach pattern avoids artifacts when changing freq.
        ledcDetach(PIN_CLOCK);
        ledcAttach(PIN_CLOCK, hz, LEDC_TIMER_BITS);
        ledcWrite(PIN_CLOCK, 128); // ~50% duty with 8-bit resolution
        currentHz = hz;

#if DEBUG_MOTOR
        Serial.print("Clock set to ");
        Serial.print(hz);
        Serial.println(" Hz");
#endif
    }

    // Coarse speed increase with tiered step sizes for fast navigation:
    //  0   ->  100 Hz

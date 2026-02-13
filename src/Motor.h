#pragma once
#include <Arduino.h>
#include <Preferences.h>
#include "Config.h"
#include "Profiles.h"

// Función auxiliar para max
template <typename T>
T simple_max(T a, T b) { return (a > b) ? a : b; }

class MotorRuntime
{
public:
    void begin()
    {
        pinMode(PIN_CLOCK, OUTPUT);
        pinMode(PIN_DIR, OUTPUT);
        pinMode(PIN_BRAKE, OUTPUT);
        pinMode(PIN_STOP, OUTPUT);
        pinMode(PIN_ENABLE, OUTPUT);
        pinMode(PIN_FG, INPUT_PULLUP);
        pinMode(PIN_LD, INPUT_PULLUP);

        // Configurar PWM para ESP32 Core 3.x
        ledcAttach(PIN_CLOCK, 1000, LEDC_TIMER_BITS);
        ledcWrite(PIN_CLOCK, 0);

        attachInterrupt(digitalPinToInterrupt(PIN_FG), isrFG, RISING);

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

    void applyProfile(const MotorProfile &p)
    {
        prof = p;
        dirCW = true;
        brakeOn = false;
        enabled = true;
        running = false;
        targetHz = 1000;
        applyOutputs();

#if DEBUG_MOTOR
        Serial.print("Profile applied: ");
        Serial.println(prof.name);
#endif
    }

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
            bool active = !running;
            bool level = prof.stopActiveHigh ? active : !active;
            digitalWrite(PIN_STOP, level ? HIGH : LOW);
        }
    }

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

    void stop()
    {
        running = false;
        setClock(0);
        applyOutputs();

#if DEBUG_MOTOR
        Serial.println("Motor STOPPED");
#endif
    }

    void setClock(uint32_t hz)
    {
        if (hz == 0)
        {
            ledcWrite(PIN_CLOCK, 0);
            currentHz = 0;
            return;
        }

        if (hz > prof.maxClockHz)
            hz = prof.maxClockHz;

        ledcDetach(PIN_CLOCK);
        ledcAttach(PIN_CLOCK, hz, LEDC_TIMER_BITS);
        ledcWrite(PIN_CLOCK, 128); // 50% duty cycle
        currentHz = hz;

#if DEBUG_MOTOR
        Serial.print("Clock set to ");
        Serial.print(hz);
        Serial.println(" Hz");
#endif
    }

    void stepSpeedUp()
    {
        uint32_t oldTarget = targetHz;
        
        if (targetHz < prof.maxClockHz)
        {
            if (targetHz == 0)
            {
                targetHz = 100;
            }
            else if (targetHz < 1000)
            {
                targetHz += 100;
            }
            else if (targetHz < 5000)
            {
                targetHz += 500;
            }
            else
            {
                targetHz += 1000;
            }
        }
        
        if (targetHz > prof.maxClockHz)
            targetHz = prof.maxClockHz;

#if DEBUG_SPEED
        Serial.print("Speed UP: ");
        Serial.print(oldTarget);
        Serial.print(" -> ");
        Serial.print(targetHz);
        Serial.print(" Hz (running: ");
        Serial.print(running ? "YES" : "NO");
        Serial.println(")");
#endif

        if (running)
        {
            setClock(targetHz);
        }
    }

    void stepSpeedDown()
    {
        uint32_t oldTarget = targetHz;
        
        if (targetHz > 5000)
        {
            targetHz -= 1000;
        }
        else if (targetHz > 1000)
        {
            targetHz -= 500;
        }
        else if (targetHz > 100)
        {
            targetHz -= 100;
        }
        else if (targetHz > 0)
        {
            targetHz = 0;
        }

#if DEBUG_SPEED
        Serial.print("Speed DOWN: ");
        Serial.print(oldTarget);
        Serial.print(" -> ");
        Serial.print(targetHz);
        Serial.print(" Hz (running: ");
        Serial.print(running ? "YES" : "NO");
        Serial.println(")");
#endif

        if (running)
        {
            setClock(targetHz);
        }
    }

    void setDirCW(bool cw)
    {
        dirCW = cw;
        applyOutputs();

#if DEBUG_MOTOR
        Serial.print("Direction set to ");
        Serial.println(cw ? "CW" : "CCW");
#endif
    }

    void toggleDir()
    {
        dirCW = !dirCW;
        applyOutputs();

#if DEBUG_MOTOR
        Serial.print("Direction toggled to ");
        Serial.println(dirCW ? "CW" : "CCW");
#endif
    }

    void toggleBrake()
    {
        if (prof.hasBrake)
        {
            brakeOn = !brakeOn;
            applyOutputs();

#if DEBUG_MOTOR
            Serial.print("Brake toggled to ");
            Serial.println(brakeOn ? "ON" : "OFF");
#endif
        }
    }

    void toggleEnable()
    {
        if (prof.hasEnable)
        {
            enabled = !enabled;
            applyOutputs();

#if DEBUG_MOTOR
            Serial.print("Enable toggled to ");
            Serial.println(enabled ? "ON" : "OFF");
#endif
        }
    }

    bool ldAlarm() const
    {
        if (!prof.hasLD)
            return false;
        int v = digitalRead(PIN_LD);
        bool alarm = prof.ldActiveLow ? (v == LOW) : (v == HIGH);
        return alarm;
    }

    void sampleRPM()
    {
        uint32_t now = millis();
        if (now - lastRpmSample >= RPM_SAMPLE_MS)
        {
            noInterrupts();
            uint32_t p = fgPulses;
            fgPulses = 0;
            interrupts();
            
            if (prof.hasFG && prof.ppr > 0)
                rpm = (p * 60UL) / prof.ppr;
            else
                rpm = 0;
            
            lastRpmSample = now;

            // FG loss safety
            if (prof.hasFG && running)
            {
                if (rpm == 0 && currentHz > 0)
                {
                    targetHz = currentHz / 4;
                    setClock(targetHz);
#if DEBUG_MOTOR
                    Serial.println("FG loss detected - reducing speed");
#endif
                }
            }

            // Telemetría solo si está activada
            if (telemetryOn)
            {
                Serial.print("RPM:");
                Serial.print(rpm);
                Serial.print(" Hz:");
                Serial.print(currentHz);
                Serial.print(" Target:");
                Serial.print(targetHz);
                Serial.print(" DIR:");
                Serial.print(dirCW ? "CW" : "CCW");
                Serial.print(" LD:");
                Serial.println(ldAlarm() ? "ALARM" : "OK");
            }
        }
    }

    // FG ISR
    static void IRAM_ATTR isrFG();

    // System settings
    void setTelemetry(bool on)
    {
        telemetryOn = on;
        sysPrefs.begin("sys", false);
        sysPrefs.putBool("tele", telemetryOn);
        sysPrefs.end();

#if DEBUG_MOTOR
        Serial.print("Telemetry set to ");
        Serial.println(on ? "ON" : "OFF");
#endif
    }
    
    bool telemetry() const { return telemetryOn; }

    void setLanguage(Language L)
    {
        lang = L;
        sysPrefs.begin("sys", false);
        sysPrefs.putUChar("lang", (uint8_t)L);
        sysPrefs.end();

#if DEBUG_MOTOR
        Serial.print("Language set to ");
        Serial.println(L == LANG_EN ? "EN" : "ES");
#endif
    }
    
    Language getLanguage() const { return lang; }

    // Public fields
    MotorProfile prof;
    bool dirCW = true, brakeOn = false, enabled = true, running = false;
    uint32_t targetHz = 1000, currentHz = 0, rpm = 0;

private:
    static volatile uint32_t fgPulses;
    uint32_t lastRpmSample = 0;
    Preferences sysPrefs;
    bool telemetryOn = false;
    Language lang = LANG_ES;
};

// Definición de la variable estática
volatile uint32_t MotorRuntime::fgPulses = 0;

// Definición de la función ISR
void IRAM_ATTR MotorRuntime::isrFG()
{
    fgPulses++;
}

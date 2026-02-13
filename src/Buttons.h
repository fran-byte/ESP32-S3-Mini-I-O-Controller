#pragma once
#include <Arduino.h>
#include "Config.h"

class Buttons
{
public:
    void begin()
    {
        pinMode(PIN_BTN_UP, INPUT_PULLUP);
        pinMode(PIN_BTN_DOWN, INPUT_PULLUP);
        pinMode(PIN_BTN_SEL, INPUT_PULLUP);

        // Leer estado inicial
        delay(50); // Estabilizar
        lastUp = digitalRead(PIN_BTN_UP);
        lastDown = digitalRead(PIN_BTN_DOWN);
        lastSel = digitalRead(PIN_BTN_SEL);

        stableUp = lastUp;
        stableDown = lastDown;
        stableSel = lastSel;

#if DEBUG_BUTTONS
        Serial.println("Buttons initialized");
        Serial.print("Initial states - UP:");
        Serial.print(stableUp);
        Serial.print(" DOWN:");
        Serial.print(stableDown);
        Serial.print(" SEL:");
        Serial.println(stableSel);
#endif
    }

    void poll()
    {
        unsigned long now = millis();

        // Reset edges
        upEdge = false;
        downEdge = false;
        selEdge = false;

        // Procesar todos los botones
        processButton(PIN_BTN_UP, lastUp, stableUp, lastDebUp, upEdge, now, "UP");
        processButton(PIN_BTN_DOWN, lastDown, stableDown, lastDebDown, downEdge, now, "DOWN");
        processButton(PIN_BTN_SEL, lastSel, stableSel, lastDebSel, selEdge, now, "SEL");

        // Pulsación larga de SELECT
        if (stableSel == LOW)
        {
            if (selPressStart == 0)
            {
                selPressStart = now;
            }
            else if (now - selPressStart > LONG_PRESS_MS)
            {
                if (!longSelTriggered)
                {
                    longSel = true;
                    longSelTriggered = true;
#if DEBUG_BUTTONS
                    Serial.println("SEL LONG press detected");
#endif
                }
            }
        }
        else
        {
            selPressStart = 0;
            longSelTriggered = false;
            longSel = false;
        }
    }

    bool upPressed()
    {
        bool result = upEdge;
        upEdge = false;
        return result;
    }

    bool downPressed()
    {
        bool result = downEdge;
        downEdge = false;
        return result;
    }

    bool selPressed()
    {
        bool result = selEdge;
        selEdge = false;
        return result;
    }

    bool selLong()
    {
        bool result = longSel;
        longSel = false;
        return result;
    }

    bool rawUpLow() const { return stableUp == LOW; }
    bool rawDownLow() const { return stableDown == LOW; }
    bool rawSelLow() const { return stableSel == LOW; }

private:
    void processButton(uint8_t pin, int &last, int &stable,
                       unsigned long &debounceTime, bool &edge, unsigned long now, const char* name)
    {
        int reading = digitalRead(pin);

        if (reading != last)
        {
            debounceTime = now;
        }

        // Debounce de 50ms para mayor estabilidad
        if (now - debounceTime > 50)
        {
            if (reading != stable)
            {
                stable = reading;
                // Solo generar edge en flanco descendente (presionado)
                if (stable == LOW)
                {
                    edge = true;
#if DEBUG_BUTTONS
                    Serial.print("Button ");
                    Serial.print(name);
                    Serial.println(" pressed (edge)");
#endif
                }
            }
        }

        last = reading;
    }

    int lastUp = HIGH, lastDown = HIGH, lastSel = HIGH;
    int stableUp = HIGH, stableDown = HIGH, stableSel = HIGH;
    unsigned long lastDebUp = 0, lastDebDown = 0, lastDebSel = 0;
    bool upEdge = false, downEdge = false, selEdge = false, longSel = false;
    unsigned long selPressStart = 0;
    bool longSelTriggered = false;
};

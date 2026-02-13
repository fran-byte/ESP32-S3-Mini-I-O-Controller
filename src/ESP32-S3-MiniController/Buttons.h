#pragma once
#include <Arduino.h>
#include "Config.h"

class Buttons
{
public:
    void begin()
    {
        // Configure pins with internal pull-ups (active-LOW)
        pinMode(PIN_BTN_UP,    INPUT_PULLUP);
        pinMode(PIN_BTN_DOWN,  INPUT_PULLUP);
        pinMode(PIN_BTN_LEFT,  INPUT_PULLUP);
        pinMode(PIN_BTN_RIGHT, INPUT_PULLUP);

        // Read initial state after a short settling time
        delay(50);
        lastUp    = digitalRead(PIN_BTN_UP);
        lastDown  = digitalRead(PIN_BTN_DOWN);
        lastLeft  = digitalRead(PIN_BTN_LEFT);
        lastRight = digitalRead(PIN_BTN_RIGHT);

        stableUp    = lastUp;
        stableDown  = lastDown;
        stableLeft  = lastLeft;
        stableRight = lastRight;

#if DEBUG_BUTTONS
        Serial.println("Buttons initialized (UP, DOWN, LEFT, RIGHT)");
        Serial.print("Initial states - UP:");   Serial.print(stableUp);
        Serial.print(" DOWN:");                 Serial.print(stableDown);
        Serial.print(" LEFT:");                 Serial.print(stableLeft);
        Serial.print(" RIGHT:");                Serial.println(stableRight);
#endif
    }

    void poll()
    {
        unsigned long now = millis();

        // Reset one-shot edges
        upEdge = downEdge = leftEdge = rightEdge = false;

        // Process with common debounce (50 ms)
        processButton(PIN_BTN_UP,    lastUp,    stableUp,    lastDebUp,    upEdge,    now, "UP");
        processButton(PIN_BTN_DOWN,  lastDown,  stableDown,  lastDebDown,  downEdge,  now, "DOWN");
        processButton(PIN_BTN_LEFT,  lastLeft,  stableLeft,  lastDebLeft,  leftEdge,  now, "LEFT");
        processButton(PIN_BTN_RIGHT, lastRight, stableRight, lastDebRight, rightEdge, now, "RIGHT");

        // Long-press detection on RIGHT
        if (stableRight == LOW)
        {
            if (rightPressStart == 0)
            {
                rightPressStart = now;
            }
            else if (now - rightPressStart > LONG_PRESS_MS)
            {
                if (!longRightTriggered)
                {
                    rightLong = true;
                    longRightTriggered = true;
#if DEBUG_BUTTONS
                    Serial.println("RIGHT LONG press detected");
#endif
                }
            }
        }
        else
        {
            rightPressStart = 0;
            longRightTriggered = false;
            rightLong = false;
        }
    }

    // One-shot edges (falling)
    bool upPressed()    { bool r = upEdge;    upEdge    = false; return r; }
    bool downPressed()  { bool r = downEdge;  downEdge  = false; return r; }
    bool leftPressed()  { bool r = leftEdge;  leftEdge  = false; return r; }
    bool rightPressed() { bool r = rightEdge; rightEdge = false; return r; }

    // One-shot long press on RIGHT
    bool rightLongPress()
    {
        bool r = rightLong;
        rightLong = false;
        return r;
    }

    // Raw debounced levels (active-LOW)
    bool rawUpLow()    const { return stableUp    == LOW; }
    bool rawDownLow()  const { return stableDown  == LOW; }
    bool rawLeftLow()  const { return stableLeft  == LOW; }
    bool rawRightLow() const { return stableRight == LOW; }

private:
    void processButton(uint8_t pin, int &last, int &stable,
                       unsigned long &debounceTime, bool &edge, unsigned long now, const char* name)
    {
        int reading = digitalRead(pin);

        if (reading != last)
        {
            debounceTime = now;
        }

        // 50 ms debounce
        if (now - debounceTime > 50)
        {
            if (reading != stable)
            {
                stable = reading;
                // Edge only on press (LOW)
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

    // Instantaneous last raw readings
    int lastUp = HIGH, lastDown = HIGH, lastLeft = HIGH, lastRight = HIGH;
    // Debounced stable states
    int stableUp = HIGH, stableDown = HIGH, stableLeft = HIGH, stableRight = HIGH;

    // Debounce timers
    unsigned long lastDebUp = 0, lastDebDown = 0, lastDebLeft = 0, lastDebRight = 0;

    // One-shot edge flags
    bool upEdge = false, downEdge = false, leftEdge = false, rightEdge = false;

    // Long-press tracking for RIGHT
    bool rightLong = false;
    unsigned long rightPressStart = 0;
    bool longRightTriggered = false;
};

#include <Arduino.h>
#include <Wire.h>
#include <U8g2lib.h>
#include <Preferences.h>
#include "Config.h"
#include "Strings_EN.h"
#include "Strings_ES.h"
#include "Buttons.h"
#include "Profiles.h"
#include "Motor.h"
#include "Ui.h"

// OLED instance (HW I2C with custom pins)
U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, U8X8_PIN_NONE, PIN_OLED_SCL, PIN_OLED_SDA);

Buttons buttons;
ProfileStore store;
MotorRuntime motor;
UI ui;

void setup()
{
    Serial.begin(115200);
    delay(1000);

    Serial.println("\n\n=== MOTOR TESTER v2.0 ===");
    Serial.println("Build: " __DATE__ " " __TIME__);
    
    // Configuración de debug
#if DEBUG_BUTTONS
    Serial.println("DEBUG_BUTTONS: ENABLED");
#endif
#if DEBUG_MOTOR
    Serial.println("DEBUG_MOTOR: ENABLED");
#endif
#if DEBUG_SPEED
    Serial.println("DEBUG_SPEED: ENABLED");
#endif

    Serial.println("\n--- Pin Configuration ---");
    Serial.print("CLOCK: "); Serial.println(PIN_CLOCK);
    Serial.print("DIR: "); Serial.println(PIN_DIR);
    Serial.print("BRAKE: "); Serial.println(PIN_BRAKE);
    Serial.print("STOP: "); Serial.println(PIN_STOP);
    Serial.print("ENABLE: "); Serial.println(PIN_ENABLE);
    Serial.print("FG: "); Serial.println(PIN_FG);
    Serial.print("LD: "); Serial.println(PIN_LD);
    Serial.print("BTN_UP: "); Serial.println(PIN_BTN_UP);
    Serial.print("BTN_DOWN: "); Serial.println(PIN_BTN_DOWN);
    Serial.print("BTN_SEL: "); Serial.println(PIN_BTN_SEL);

    Serial.println("\n--- Initializing I2C ---");
    Wire.begin(PIN_OLED_SDA, PIN_OLED_SCL);

    Serial.println("--- Initializing Buttons ---");
    buttons.begin();

    Serial.print("Button states - UP:");
    Serial.print(digitalRead(PIN_BTN_UP));
    Serial.print(" DOWN:");
    Serial.print(digitalRead(PIN_BTN_DOWN));
    Serial.print(" SEL:");
    Serial.println(digitalRead(PIN_BTN_SEL));

    Serial.println("--- Initializing Profile Store ---");
    store.begin();
    Serial.print("Profiles found: ");
    Serial.println(store.getCount());

    Serial.println("--- Initializing Motor ---");
    motor.begin();

    // Load active profile or defaults
    MotorProfile mp;
    if (!store.loadActive(mp))
    {
        mp.setDefaults();
        Serial.println("Using default profile");
    }
    else
    {
        Serial.print("Loaded profile: ");
        Serial.println(mp.name);
    }

    motor.applyProfile(mp);

    Serial.println("--- Initializing UI ---");
    ui.begin(u8g2, buttons, store, motor);

    // Diagnostics mode if UP+DOWN held at boot
    ui.checkDiagAtBoot();

    Serial.println("\n=== SETUP COMPLETE ===");
    Serial.println("Controls:");
    Serial.println("  UP/DOWN: Change speed");
    Serial.println("  SEL short: Menu");
    Serial.println("  SEL long: Start/Stop motor");
    Serial.println("========================\n");
}

void loop()
{
    buttons.poll();
    motor.sampleRPM();
    ui.loop();
}

#pragma once
#include <Arduino.h>
#include <U8g2lib.h>
#include <Preferences.h>
#include "Config.h"
#include "Strings_EN.h"
#include "Strings_ES.h"
#include "Buttons.h"
#include "Profiles.h"
#include "Motor.h"

class UI
{
public:
    void begin(U8G2 &d, Buttons &b, ProfileStore &store, MotorRuntime &m)
    {
        disp = &d;
        btn = &b;
        pst = &store;
        motor = &m;
        d.begin();
        drawIntro();
        lang = motor->getLanguage();
    }

    void setLanguage(Language L)
    {
        lang = L;
        motor->setLanguage(L);
    }

    void home()
    {
        state = HOME;
        needRedraw = true;
    }

    void loop()
    {
        switch (state)
        {
        case HOME:
            drawHome();
            updateHome();
            break;
        case MENU:
            handleMenu();
            break;
        case SELECT_MOTOR:
            handleSelectMotor();
            break;
        case ADD_NAME:
        case ADD_Q_BRAKE:
        case ADD_Q_FG:
        case ADD_Q_LD:
        case ADD_Q_LD_LEVEL:
        case ADD_Q_STOP:
        case ADD_Q_STOP_LEVEL:
        case ADD_Q_ENABLE:
        case ADD_Q_ENABLE_LEVEL:
        case ADD_Q_PPR:
        case ADD_Q_MAXCLK:
        case ADD_SAVE:
            drawWizard();
            handleWizard();
            break;
        case SETTINGS:
            handleSettings();
            break;
        case SETTINGS_LANG:
            handleSettingsLang();
            break;
        case SETTINGS_TELE:
            handleSettingsTele();
            break;
        case ABOUT:
            handleAbout();
            break;
        case DIAG:
            handleDiag();
            break;
        }
    }

    void checkDiagAtBoot()
    {
        if (btn->rawUpLow() && btn->rawDownLow())
        {
            state = DIAG;
            needRedraw = true;
#if DEBUG_BUTTONS
            Serial.println("Entering DIAG mode");
#endif
        }
    }

private:
    enum State
    {
        HOME,
        MENU,
        SELECT_MOTOR,
        ADD_NAME,
        ADD_Q_BRAKE,
        ADD_Q_FG,
        ADD_Q_LD,
        ADD_Q_LD_LEVEL,
        ADD_Q_STOP,
        ADD_Q_STOP_LEVEL,
        ADD_Q_ENABLE,
        ADD_Q_ENABLE_LEVEL,
        ADD_Q_PPR,
        ADD_Q_MAXCLK,
        ADD_SAVE,
        SETTINGS,
        SETTINGS_LANG,
        SETTINGS_TELE,
        ABOUT,
        DIAG
    };

    const Strings &S() const { return (lang == LANG_EN) ? STR_EN : STR_ES; }

    // Dibujar marco doble redondeado
    void drawDoubleFrame()
    {
        // Marco exterior (con esquinas redondeadas)
        disp->drawRFrame(0, 0, 128, 64, 3);

        // Marco interior (2 pixeles hacia dentro, con esquinas redondeadas)
        disp->drawRFrame(2, 2, 124, 60, 2);
    }

    void header(const char *title)
    {
        disp->firstPage();
        do
        {
            disp->setFont(u8g2_font_6x12_tf);
            disp->drawBox(0, 0, 128, 13);
            disp->setDrawColor(0);
            disp->drawStr(2, 10, title);
            disp->setDrawColor(1);
        } while (disp->nextPage());
    }

    void drawIntro()
    {
        disp->firstPage();
        do
        {
            disp->setFont(u8g2_font_logisoso20_tr);
            disp->drawStr(4, 30, "Fran-Byte");
            disp->setFont(u8g2_font_6x12_tf);
            disp->drawStr(4, 52, "Motor Tester v2");
        } while (disp->nextPage());
        delay(900);
    }

    void drawHome()
    {
        if (!needRedraw)
            return;
        needRedraw = false;

        disp->firstPage();
        do
        {
            // Header con nombre del motor y estado ON/OFF
            disp->setFont(u8g2_font_6x12_tf);
            const char *title = (motor->prof.name[0] ? motor->prof.name : S().hdr_home);
            disp->drawBox(0, 0, 128, 13);
            disp->setDrawColor(0);

            // Nombre del motor a la izquierda
            disp->drawStr(2, 10, title);

            // Estado ON/OFF a la derecha
            const char *status = motor->running ? "ON" : "OFF";
            int statusWidth = strlen(status) * 6; // 6 pixels por carácter
            disp->drawStr(128 - statusWidth - 2, 10, status);

            disp->setDrawColor(1);

            int y = 22; // Posición Y inicial

            // RPM solo si tiene FG configurado (fuente normal 6x12)
            disp->setFont(u8g2_font_6x12_tf);

            if (motor->prof.hasFG)
            {
                char line[32];
                snprintf(line, sizeof(line), "%s %lu", S().rpm, (unsigned long)motor->rpm);
                disp->drawStr(2, y, line);
                y += 11; // Espaciado
            }

            // Velocidad (siempre mostrar)
            char spd[40];
            snprintf(spd, sizeof(spd), "%s %lu Hz", S().speed, (unsigned long)motor->targetHz);
            disp->drawStr(2, y, spd);
            y += 11;

            // SECUNDARIAS (fuente pequeña 5x8): DIR y señales de estado
            disp->setFont(u8g2_font_5x8_tf);

            // Array para almacenar textos de todas las señales
            char statusItems[4][32]; // DIR + BRAKE + ENABLE + LD
            int itemCount = 0;

            // DIR siempre es el primero
            char dirline[32];
            snprintf(dirline, sizeof(dirline), "%s %s", S().dir, (motor->dirCW ? S().cw : S().ccw));
            strcpy(statusItems[itemCount], dirline);
            itemCount++;

            // BRAKE (si está configurado)
            if (motor->prof.hasBrake)
            {
                snprintf(statusItems[itemCount], sizeof(statusItems[itemCount]),
                         "%s %s", S().brake, (motor->brakeOn ? S().on : S().off));
                itemCount++;
            }

            // ENABLE (si está configurado)
            if (motor->prof.hasEnable)
            {
                snprintf(statusItems[itemCount], sizeof(statusItems[itemCount]),
                         "%s %s", S().enable, (motor->enabled ? S().on : S().off));
                itemCount++;
            }

            // LD (si está configurado) - mostrar con ✓ o ✗
            if (motor->prof.hasLD)
            {
                const char *ldSymbol = motor->ldAlarm() ? "✗" : "✓";
                snprintf(statusItems[itemCount], sizeof(statusItems[itemCount]),
                         "%s: %s", S().ld, ldSymbol);
                itemCount++;
            }

            // Distribuir los elementos en líneas
            int currentX = 2;       // Posición X inicial
            int maxLineWidth = 126; // Ancho máximo de línea
            int currentLineY = y;   // Posición Y inicial

            for (int i = 0; i < itemCount; i++)
            {
                int itemWidth = strlen(statusItems[i]) * 5;

                // Si el elemento no cabe en la línea actual, pasar a nueva línea
                if (currentX + itemWidth > maxLineWidth && i > 0)
                {
                    currentLineY += 9; // Nueva línea (9 pixeles para fuente 5x8)
                    currentX = 2;      // Reiniciar X
                }

                // Dibujar el elemento
                disp->drawStr(currentX, currentLineY, statusItems[i]);

                // Actualizar posición X para el próximo elemento
                currentX += itemWidth + 8; // 8 pixeles de separación
            }

            // Actualizar Y para footer
            y = currentLineY + 9;

            // Footer (siempre al final)
            disp->setFont(u8g2_font_5x8_tf);
            disp->drawStr(2, 63, S().footer_home);
        } while (disp->nextPage());
    }

    void updateHome()
    {
        static unsigned long lastSpeedChange = 0;
        const unsigned long SPEED_DELAY = 150;
        unsigned long now = millis();

        // UP button
        if (btn->upPressed())
        {
            if (now - lastSpeedChange > SPEED_DELAY)
            {
                motor->stepSpeedUp();
                needRedraw = true;
                lastSpeedChange = now;
#if DEBUG_SPEED
                Serial.println("[UI] UP button pressed");
#endif
            }
        }

        // DOWN button
        if (btn->downPressed())
        {
            if (now - lastSpeedChange > SPEED_DELAY)
            {
                motor->stepSpeedDown();
                needRedraw = true;
                lastSpeedChange = now;
#if DEBUG_SPEED
                Serial.println("[UI] DOWN button pressed");
#endif
            }
        }

        // SELECT short -> menu
        if (btn->selPressed())
        {
#if DEBUG_BUTTONS
            Serial.println("[UI] SELECT short: Going to MENU");
#endif
            state = MENU;
            menuIndex = 0;
            needRedraw = true;
            delay(150);
        }

        // SELECT long -> start/stop
        if (btn->selLong())
        {
#if DEBUG_MOTOR
            Serial.print("[UI] SELECT long: ");
            Serial.println(motor->running ? "STOP" : "START");
#endif
            if (motor->running)
                motor->stop();
            else
                motor->start();
            needRedraw = true;
            delay(200);
        }
    }

    void drawMenuList(const char **items, int n)
    {
        if (n == 0)
            return;

        disp->firstPage();
        do
        {
            // Dibujar marco doble redondeado
            drawDoubleFrame();

            disp->setFont(u8g2_font_6x12_tf);
            // Header con fondo
            disp->drawRBox(4, 4, 120, 13, 2);
            disp->setDrawColor(0);
            disp->drawStr(6, 14, S().menu);
            disp->setDrawColor(1);

            int lineHeight = 10;
            int maxVisibleLines = 3; // Reducido para dejar espacio al marco

            if (menuIndex < menuScroll)
                menuScroll = menuIndex;
            if (menuIndex >= menuScroll + maxVisibleLines)
                menuScroll = menuIndex - maxVisibleLines + 1;

            int startY = 28; // Más abajo para dar espacio

            for (int i = 0; i < maxVisibleLines; i++)
            {
                int idx = menuScroll + i;
                if (idx >= n)
                    break;

                int y = startY + (i * lineHeight);

                if (idx == menuIndex)
                {
                    disp->drawRBox(6, y - 8, 116, lineHeight, 2);
                    disp->setDrawColor(0);
                    disp->drawStr(8, y, items[idx]);
                    disp->setDrawColor(1);
                }
                else
                {
                    disp->drawStr(8, y, items[idx]);
                }
            }

            // Footer
            disp->setFont(u8g2_font_5x8_tf);
            disp->drawStr(6, 60, S().footer_menu);

        } while (disp->nextPage());
    }

    void handleMenu()
    {
        const char *items[12];
        int n = 0;
        items[n++] = motor->running ? S().m_stop : S().m_start;
        items[n++] = motor->dirCW ? S().m_set_ccw : S().m_set_cw;
        if (motor->prof.hasBrake)
            items[n++] = motor->brakeOn ? S().m_brake_off : S().m_brake_on;
        if (pst->getCount() > 0)
            items[n++] = S().m_select_motor;
        items[n++] = S().m_add_motor;
        if (pst->getCount() > 0)
            items[n++] = S().m_delete_active;
        items[n++] = S().m_settings;
        items[n++] = S().m_about;
        items[n++] = S().m_back; // ← NUEVA OPCIÓN

        if (btn->upPressed() && menuIndex > 0)
        {
            menuIndex--;
            needRedraw = true;
        }

        if (btn->downPressed() && menuIndex < n - 1)
        {
            menuIndex++;
            needRedraw = true;
        }

        // SELECT LARGO ya no hace nada en el menú

        if (btn->selPressed())
        {
            delay(100);
            int c = 0;

            if (menuIndex == c++)
            {
                if (motor->running)
                    motor->stop();
                else
                    motor->start();
                state = HOME;
                needRedraw = true;
                return;
            }
            if (menuIndex == c++)
            {
                motor->toggleDir();
                state = HOME;
                needRedraw = true;
                return;
            }
            if (motor->prof.hasBrake)
            {
                if (menuIndex == c++)
                {
                    motor->toggleBrake();
                    state = HOME;
                    needRedraw = true;
                    return;
                }
            }
            if (pst->getCount() > 0)
            {
                if (menuIndex == c++)
                {
                    state = SELECT_MOTOR;
                    menuIndex = pst->getActiveIndex();
                    needRedraw = true;
                    return;
                }
            }
            if (menuIndex == c++)
            {
                enterAddWizard();
                return;
            }
            if (pst->getCount() > 0)
            {
                if (menuIndex == c++)
                {
                    int idx = pst->getActiveIndex();
                    pst->remove(idx);
                    MotorProfile mp;
                    if (!pst->loadActive(mp))
                        mp.setDefaults();
                    motor->applyProfile(mp);
                    state = HOME;
                    needRedraw = true;
                    return;
                }
            }
            if (menuIndex == c++)
            {
                state = SETTINGS;
                menuIndex = 0;
                needRedraw = true;
                return;
            }
            if (menuIndex == c++)
            {
                state = ABOUT;
                needRedraw = true;
                return;
            }
            // NUEVA OPCIÓN: Atrás/Back
            if (menuIndex == c++)
            {
                state = HOME;
                needRedraw = true;
                return;
            }
        }

        if (needRedraw)
        {
            drawMenuList(items, n);
            needRedraw = false;
        }
    }

    void handleSelectMotor()
    {
        int profileCount = pst->getCount();
        if (profileCount <= 0)
        {
            state = MENU;
            return;
        }

        // Crear lista con perfiles + opción Atrás
        static char names[MAX_PROFILES + 1][22];
        static const char *items[MAX_PROFILES + 1];
        int n = 0;

        for (int i = 0; i < profileCount; i++)
        {
            String nm = pst->nameOf(i);
            nm.toCharArray(names[i], sizeof(names[i]));
            items[n++] = names[i];
        }
        // Añadir opción Atrás/Back al final
        strcpy(names[profileCount], S().m_back);
        items[n++] = names[profileCount];

        if (btn->upPressed() && menuIndex > 0)
            menuIndex--;
        if (btn->downPressed() && menuIndex < n - 1)
            menuIndex++;

        // SELECT LARGO ya no hace nada

        if (btn->selPressed())
        {
            // Si es la última opción (Atrás/Back)
            if (menuIndex == profileCount)
            {
                state = MENU;
                menuIndex = 0;
                needRedraw = true;
                return;
            }
            // Si es un perfil de motor
            else
            {
                pst->setActive(menuIndex);
                MotorProfile mp;
                pst->loadActive(mp);
                motor->applyProfile(mp);
                state = HOME;
                needRedraw = true;
                return;
            }
        }

        drawMenuList(items, n);
    }

    void enterAddWizard()
    {
        memset(&tmp, 0, sizeof(tmp));
        tmp.setDefaults();
        memset(editName, 0, sizeof(editName));
        editPos = 0;
        state = ADD_NAME;
        needRedraw = true;
    }

    void wizardNext()
    {
        if (state == ADD_NAME)
            state = ADD_Q_BRAKE;
        else if (state == ADD_Q_BRAKE)
            state = ADD_Q_FG;
        else if (state == ADD_Q_FG)
            state = ADD_Q_LD;
        else if (state == ADD_Q_LD)
            state = tmp.hasLD ? ADD_Q_LD_LEVEL : ADD_Q_STOP;
        else if (state == ADD_Q_LD_LEVEL)
            state = ADD_Q_STOP;
        else if (state == ADD_Q_STOP)
            state = tmp.hasStop ? ADD_Q_STOP_LEVEL : ADD_Q_ENABLE;
        else if (state == ADD_Q_STOP_LEVEL)
            state = ADD_Q_ENABLE;
        else if (state == ADD_Q_ENABLE)
            state = tmp.hasEnable ? ADD_Q_ENABLE_LEVEL : ADD_Q_PPR;
        else if (state == ADD_Q_ENABLE_LEVEL)
            state = ADD_Q_PPR;
        else if (state == ADD_Q_PPR)
            state = ADD_Q_MAXCLK;
        else if (state == ADD_Q_MAXCLK)
            state = ADD_SAVE;
        needRedraw = true;
    }

    void drawWizard()
    {
        // En modo ADD_NAME, siempre redibujar para cursor parpadeante
        if (state == ADD_NAME)
        {
            needRedraw = true;
        }

        if (!needRedraw)
            return;
        needRedraw = false;

        char line1[32], line2[32], hint[64];
        line1[0] = 0;
        line2[0] = 0;
        strcpy(hint, S().hint_yesno);

        if (state == ADD_NAME)
        {
            strcpy(line1, S().w_name);

            // Preparar line2 mostrando "END" si el carácter actual es el marcador
            const char END_MARKER = 0x7F;
            strcpy(line2, editName);

            // Si el carácter en la posición actual es END_MARKER, mostrar como recuadro pequeño
            if (editName[editPos] == END_MARKER)
            {
                // Reemplazar temporalmente para mostrar
                line2[editPos] = 0; // Terminar string antes de END
            }

            strcpy(hint, S().hint_text);
        }
        else if (state == ADD_Q_BRAKE)
        {
            strcpy(line1, S().w_has_brake);
            strcpy(line2, tmp.hasBrake ? S().yes : S().no);
        }
        else if (state == ADD_Q_FG)
        {
            strcpy(line1, S().w_has_fg);
            strcpy(line2, tmp.hasFG ? S().yes : S().no);
        }
        else if (state == ADD_Q_LD)
        {
            strcpy(line1, S().w_has_ld);
            strcpy(line2, tmp.hasLD ? S().yes : S().no);
        }
        else if (state == ADD_Q_LD_LEVEL)
        {
            strcpy(line1, S().w_ld_active);
            strcpy(line2, tmp.ldActiveLow ? S().low : S().high);
            strcpy(hint, S().hint_choice);
        }
        else if (state == ADD_Q_STOP)
        {
            strcpy(line1, S().w_has_stop);
            strcpy(line2, tmp.hasStop ? S().yes : S().no);
        }
        else if (state == ADD_Q_STOP_LEVEL)
        {
            strcpy(line1, S().w_stop_active);
            strcpy(line2, tmp.stopActiveHigh ? S().high : S().low);
            strcpy(hint, S().hint_choice);
        }
        else if (state == ADD_Q_ENABLE)
        {
            strcpy(line1, S().w_has_enable);
            strcpy(line2, tmp.hasEnable ? S().yes : S().no);
        }
        else if (state == ADD_Q_ENABLE_LEVEL)
        {
            strcpy(line1, S().w_enable_active);
            strcpy(line2, tmp.enableActiveHigh ? S().high : S().low);
            strcpy(hint, S().hint_choice);
        }
        else if (state == ADD_Q_PPR)
        {
            strcpy(line1, S().w_ppr);
            snprintf(line2, sizeof(line2), "%d", tmp.ppr);
            strcpy(hint, S().hint_number);
        }
        else if (state == ADD_Q_MAXCLK)
        {
            strcpy(line1, S().w_maxclk);
            snprintf(line2, sizeof(line2), "%lu", (unsigned long)tmp.maxClockHz);
            strcpy(hint, S().hint_number);
        }
        else if (state == ADD_SAVE)
        {
            strcpy(line1, S().w_save);
            strcpy(line2, wizardSaveChoice ? S().yes : S().no);
            strcpy(hint, S().hint_yesno);
        }

        disp->firstPage();
        do
        {
            disp->setFont(u8g2_font_6x12_tf);
            disp->drawBox(0, 0, 128, 13);
            disp->setDrawColor(0);
            disp->drawStr(2, 10, S().m_add_motor);
            disp->setDrawColor(1);
            disp->drawStr(2, 28, line1);

            // Dibujar line2 con cursor si estamos en ADD_NAME
            if (state == ADD_NAME)
            {
                const char END_MARKER = 0x7F;

                // Dibujar el texto normal
                disp->drawStr(2, 42, line2);

                // Si el carácter actual es END_MARKER, dibujar recuadro "END"
                if (editName[editPos] == END_MARKER)
                {
                    int endX = 2 + (editPos * 6);

                    // Dibujar recuadro pequeño
                    disp->drawFrame(endX, 34, 18, 10);

                    // Dibujar texto "END" pequeño dentro
                    disp->setFont(u8g2_font_4x6_tr);
                    disp->drawStr(endX + 1, 42, "END");
                    disp->setFont(u8g2_font_6x12_tf);
                }
                else
                {
                    // Calcular posición X del cursor (6 pixels por carácter)
                    int cursorX = 2 + (editPos * 6);

                    // Dibujar cursor parpadeante (línea bajo el carácter actual)
                    if ((millis() / 500) % 2 == 0) // Parpadeo cada 500ms
                    {
                        disp->drawLine(cursorX, 44, cursorX + 5, 44);
                    }
                }
            }
            else
            {
                // Para otros estados, solo dibujar line2 normalmente
                disp->drawStr(2, 42, line2);
            }

            disp->setFont(u8g2_font_5x8_tf);
            disp->drawStr(2, 62, hint);
        } while (disp->nextPage());
    }

    void handleWizard()
    {
        if (state == ADD_NAME)
        {
            // Alfabeto incluyendo un marcador especial "END"
            // A-Z, 0-9, espacio, -, _, y un carácter especial para END
            const char END_MARKER = 0x7F; // Carácter especial que usaremos como "END"

            if (btn->upPressed())
            {
                // Inicializar con 'A' si es el primer carácter
                if (editName[editPos] == 0)
                    editName[editPos] = 'A';
                else
                {
                    editName[editPos]++;
                    // Ciclo: A-Z, 0-9, espacio, -, _, END_MARKER
                    if (editName[editPos] == 'Z' + 1)
                        editName[editPos] = '0';
                    else if (editName[editPos] == '9' + 1)
                        editName[editPos] = ' ';
                    else if (editName[editPos] == ' ' + 1)
                        editName[editPos] = '-';
                    else if (editName[editPos] == '-' + 1)
                        editName[editPos] = '_';
                    else if (editName[editPos] == '_' + 1)
                        editName[editPos] = END_MARKER;
                    else if (editName[editPos] > END_MARKER)
                        editName[editPos] = 'A';
                }
                needRedraw = true;
            }

            if (btn->downPressed())
            {
                // Inicializar con 'A' si es el primer carácter
                if (editName[editPos] == 0)
                    editName[editPos] = 'A';
                else
                {
                    editName[editPos]--;
                    // Ciclo inverso
                    if (editName[editPos] < 'A' && editName[editPos] != END_MARKER && editName[editPos] != '_' && editName[editPos] != '-' && editName[editPos] != ' ')
                    {
                        editName[editPos] = END_MARKER;
                    }
                    else if (editName[editPos] == END_MARKER - 1)
                        editName[editPos] = '_';
                    else if (editName[editPos] == '_' - 1)
                        editName[editPos] = '-';
                    else if (editName[editPos] == '-' - 1)
                        editName[editPos] = ' ';
                    else if (editName[editPos] == ' ' - 1)
                        editName[editPos] = '9';
                    else if (editName[editPos] == '0' - 1)
                        editName[editPos] = 'Z';
                }
                needRedraw = true;
            }

            // SELECT: Avanzar o finalizar si es END
            if (btn->selPressed())
            {
                // Si el carácter actual es END_MARKER, finalizar
                if (editName[editPos] == END_MARKER)
                {
                    // Eliminar el END_MARKER y finalizar
                    editName[editPos] = 0;

                    // Si el nombre está vacío, poner "Motor"
                    if (editName[0] == 0)
                    {
                        strcpy(editName, "Motor");
                    }

                    strncpy(tmp.name, editName, sizeof(tmp.name));
                    wizardNext();
#if DEBUG_BUTTONS
                    Serial.print("[Wizard] Name finalized: ");
                    Serial.println(tmp.name);
#endif
                    return;
                }

                // Si no es END, inicializar si está vacío y avanzar
                if (editName[editPos] == 0)
                    editName[editPos] = 'A';

                if (editPos < 18)
                {
                    editPos++;
#if DEBUG_BUTTONS
                    Serial.print("[Wizard] Moved to position: ");
                    Serial.println(editPos);
#endif
                }
                else
                {
                    // Llegamos al final (posición 18)
                    strncpy(tmp.name, editName, sizeof(tmp.name));
                    wizardNext();
                }
                needRedraw = true;
            }
            return;
        }

        if (state == ADD_Q_BRAKE)
        {
            if (btn->upPressed() || btn->downPressed())
            {
                tmp.hasBrake = !tmp.hasBrake;
                needRedraw = true;
            }
            if (btn->selPressed())
            {
                wizardNext();
            }
            return;
        }
        if (state == ADD_Q_FG)
        {
            if (btn->upPressed() || btn->downPressed())
            {
                tmp.hasFG = !tmp.hasFG;
                needRedraw = true;
            }
            if (btn->selPressed())
            {
                wizardNext();
            }
            return;
        }
        if (state == ADD_Q_LD)
        {
            if (btn->upPressed() || btn->downPressed())
            {
                tmp.hasLD = !tmp.hasLD;
                needRedraw = true;
            }
            if (btn->selPressed())
            {
                wizardNext();
            }
            return;
        }
        if (state == ADD_Q_LD_LEVEL)
        {
            if (btn->upPressed() || btn->downPressed())
            {
                tmp.ldActiveLow = !tmp.ldActiveLow;
                needRedraw = true;
            }
            if (btn->selPressed())
            {
                wizardNext();
            }
            return;
        }
        if (state == ADD_Q_STOP)
        {
            if (btn->upPressed() || btn->downPressed())
            {
                tmp.hasStop = !tmp.hasStop;
                needRedraw = true;
            }
            if (btn->selPressed())
            {
                wizardNext();
            }
            return;
        }
        if (state == ADD_Q_STOP_LEVEL)
        {
            if (btn->upPressed() || btn->downPressed())
            {
                tmp.stopActiveHigh = !tmp.stopActiveHigh;
                needRedraw = true;
            }
            if (btn->selPressed())
            {
                wizardNext();
            }
            return;
        }
        if (state == ADD_Q_ENABLE)
        {
            if (btn->upPressed() || btn->downPressed())
            {
                tmp.hasEnable = !tmp.hasEnable;
                needRedraw = true;
            }
            if (btn->selPressed())
            {
                wizardNext();
            }
            return;
        }
        if (state == ADD_Q_ENABLE_LEVEL)
        {
            if (btn->upPressed() || btn->downPressed())
            {
                tmp.enableActiveHigh = !tmp.enableActiveHigh;
                needRedraw = true;
            }
            if (btn->selPressed())
            {
                wizardNext();
            }
            return;
        }
        if (state == ADD_Q_PPR)
        {
            if (btn->upPressed())
            {
                tmp.ppr++;
                needRedraw = true;
            }
            if (btn->downPressed() && tmp.ppr > 1)
            {
                tmp.ppr--;
                needRedraw = true;
            }
            if (btn->selPressed())
                wizardNext();
            return;
        }
        if (state == ADD_Q_MAXCLK)
        {
            if (btn->upPressed())
            {
                tmp.maxClockHz += 1000;
                needRedraw = true;
            }
            if (btn->downPressed() && tmp.maxClockHz > 1000)
            {
                tmp.maxClockHz -= 1000;
                needRedraw = true;
            }
            if (btn->selPressed())
                wizardNext();
            return;
        }
        if (state == ADD_SAVE)
        {
            if (btn->upPressed() || btn->downPressed())
            {
                wizardSaveChoice = !wizardSaveChoice;
                needRedraw = true;
            }

            if (btn->selPressed())
            {
                if (wizardSaveChoice)
                {
                    // Guardar perfil
                    pst->append(tmp);
                    pst->setActive(pst->getCount() - 1);
                    MotorProfile mp;
                    pst->loadActive(mp);
                    motor->applyProfile(mp);
                }
                // Si es NO, no guardar
                state = HOME;
                needRedraw = true;
                wizardSaveChoice = true; // Reset para la próxima vez
                return;
            }
        }
    }

    void handleSettings()
    {
        const char *items[4];
        int n = 0;
        items[n++] = S().s_language;
        items[n++] = S().s_telemetry;
        items[n++] = S().m_back; // ← NUEVA OPCIÓN

        if (btn->upPressed() && menuIndex > 0)
            menuIndex--;
        if (btn->downPressed() && menuIndex < n - 1)
            menuIndex++;

        // SELECT LARGO ya no hace nada

        if (btn->selPressed())
        {
            int c = 0;
            if (menuIndex == c++)
            {
                state = SETTINGS_LANG;
                menuIndex = (lang == LANG_EN ? 0 : 1);
                needRedraw = true;
                return;
            }
            if (menuIndex == c++)
            {
                state = SETTINGS_TELE;
                needRedraw = true;
                return;
            }
            // NUEVA OPCIÓN: Atrás/Back
            if (menuIndex == c++)
            {
                state = MENU;
                menuIndex = 0;
                needRedraw = true;
                return;
            }
        }
        drawMenuList(items, n);
    }

    void handleSettingsLang()
    {
        const char *items[3];
        int n = 0;
        items[n++] = S().s_lang_en;
        items[n++] = S().s_lang_es;
        items[n++] = S().m_back; // ← NUEVA OPCIÓN

        if (btn->upPressed() && menuIndex > 0)
            menuIndex--;
        if (btn->downPressed() && menuIndex < n - 1)
            menuIndex++;

        // SELECT LARGO ya no hace nada

        if (btn->selPressed())
        {
            // Opción English
            if (menuIndex == 0)
            {
                lang = LANG_EN;
                motor->setLanguage(lang);
                state = HOME;
                needRedraw = true;
                return;
            }
            // Opción Español
            if (menuIndex == 1)
            {
                lang = LANG_ES;
                motor->setLanguage(lang);
                state = HOME;
                needRedraw = true;
                return;
            }
            // Opción Atrás/Back
            if (menuIndex == 2)
            {
                state = SETTINGS;
                menuIndex = 0;
                needRedraw = true;
                return;
            }
        }
        drawMenuList(items, n);
    }

    void handleSettingsTele()
    {
        const char *items[2];
        int n = 0;
        items[n++] = motor->telemetry() ? S().s_telemetry_on : S().s_telemetry_off;
        items[n++] = S().m_back; // ← OPCIÓN ATRÁS

        // Navegación del menú
        if (btn->upPressed() && menuIndex > 0)
            menuIndex--;
        if (btn->downPressed() && menuIndex < n - 1)
            menuIndex++;

        if (btn->selPressed())
        {
            if (menuIndex == 0) // Opción de telemetría ON/OFF
            {
                motor->setTelemetry(!motor->telemetry());
                needRedraw = true;
#if DEBUG_BUTTONS
                Serial.println("[UI] Telemetry toggled");
#endif
                // Cambiar el texto del primer ítem
                items[0] = motor->telemetry() ? S().s_telemetry_on : S().s_telemetry_off;
            }
            else if (menuIndex == 1) // Opción Atrás
            {
                state = SETTINGS;
                menuIndex = 0;
                needRedraw = true;
                return;
            }
        }

        // Dibujar el menú de telemetría
        drawMenuList(items, n);
    }

    void handleAbout()
    {
        // Permitir volver con SELECT (sin mostrar hint)
        if (btn->selPressed())
        {
            state = MENU;
            menuIndex = 0;
            needRedraw = true;
#if DEBUG_BUTTONS
            Serial.println("[UI] Back to MENU from About");
#endif
            return;
        }

        // Siempre redibujar
        char build[24];
        snprintf(build, sizeof(build), "%s %s", S().about_build, __DATE__);

        disp->firstPage();
        do
        {
            // Marco doble redondeado
            drawDoubleFrame();

            disp->setFont(u8g2_font_6x12_tf);
            disp->drawRBox(4, 4, 120, 13, 2);
            disp->setDrawColor(0);
            disp->drawStr(6, 14, S().about_title);
            disp->setDrawColor(1);
            disp->drawStr(8, 30, S().about_author);
            disp->drawStr(8, 42, S().about_version);
            disp->drawStr(8, 54, build);
            // Sin hint - SELECT simplemente funciona
        } while (disp->nextPage());
    }

    void handleDiag()
    {
        // Leer estado de botones PRIMERO
        if (btn->selLong())
        {
            state = HOME;
            needRedraw = true;
#if DEBUG_BUTTONS
            Serial.println("[UI] Back to HOME from Diag");
#endif
            return;
        }

        // Siempre redibujar para mostrar datos en tiempo real
        char l1[32], l2[32], l3[32];
        snprintf(l1, sizeof(l1), "UP:%d DN:%d SEL:%d",
                 btn->rawUpLow() ? 1 : 0,
                 btn->rawDownLow() ? 1 : 0,
                 btn->rawSelLow() ? 1 : 0);

        int ld = digitalRead(PIN_LD);
        snprintf(l2, sizeof(l2), "LD:%d FG-rpm:%lu",
                 (motor->prof.ldActiveLow ? (ld == LOW) : (ld == HIGH)) ? 1 : 0,
                 (unsigned long)motor->rpm);

        snprintf(l3, sizeof(l3), "Hz:%lu DIR:%s",
                 (unsigned long)motor->currentHz,
                 motor->dirCW ? "CW" : "CCW");

        disp->firstPage();
        do
        {
            disp->setFont(u8g2_font_6x12_tf);
            disp->drawBox(0, 0, 128, 13);
            disp->setDrawColor(0);
            disp->drawStr(2, 10, S().diag_title);
            disp->setDrawColor(1);
            disp->drawStr(2, 26, l1);
            disp->drawStr(2, 38, l2);
            disp->drawStr(2, 50, l3);
            disp->setFont(u8g2_font_5x8_tf);
            disp->drawStr(2, 62, S().diag_hint);
        } while (disp->nextPage());
    }

    U8G2 *disp = nullptr;
    Buttons *btn = nullptr;
    ProfileStore *pst = nullptr;
    MotorRuntime *motor = nullptr;
    State state = HOME;
    bool needRedraw = true;
    int menuIndex = 0;
    int menuScroll = 0;
    Language lang = LANG_ES;
    MotorProfile tmp;
    char editName[20] = {0};
    int editPos = 0;
    bool wizardSaveChoice = true;
};

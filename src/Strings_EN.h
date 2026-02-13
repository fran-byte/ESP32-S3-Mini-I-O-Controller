#pragma once
struct Strings
{
    const char *hdr_home;
    const char *rpm;
    const char *speed;
    const char *dir;
    const char *cw;
    const char *ccw;
    const char *brake;
    const char *on;
    const char *off;
    const char *enable;
    const char *ld;
    const char *alarm;
    const char *ok;
    const char *footer_home;
    const char *menu;
    const char *m_start;
    const char *m_stop;
    const char *m_set_ccw;
    const char *m_set_cw;
    const char *m_brake_on;
    const char *m_brake_off;
    const char *m_select_motor;
    const char *m_add_motor;
    const char *m_delete_active;
    const char *m_settings;
    const char *m_about;
    const char *m_back;
    const char *footer_menu;
    // Wizard
    const char *w_name;
    const char *w_has_brake;
    const char *w_has_fg;
    const char *w_has_ld;
    const char *w_ld_active;
    const char *low;
    const char *high;
    const char *w_has_stop;
    const char *w_stop_active;
    const char *w_has_enable;
    const char *w_enable_active;
    const char *w_ppr;
    const char *w_maxclk;
    const char *w_save;
    const char *yes;
    const char *no;
    const char *hint_yesno;
    const char *hint_choice;
    const char *hint_number;
    const char *hint_text;
    // Settings
    const char *s_title;
    const char *s_language;
    const char *s_lang_en;
    const char *s_lang_es;
    const char *s_telemetry;
    const char *s_telemetry_on;
    const char *s_telemetry_off;
    // About
    const char *about_title;
    const char *about_author;
    const char *about_version;
    const char *about_build;
    // Diag
    const char *diag_title;
    const char *diag_hint;
};

static const Strings STR_EN = {
    "Motor Tester",
    "RPM:",
    "Speed:",
    "DIR:",
    "CW",
    "CCW",
    "BRAKE:",
    "ON",
    "OFF",
    "ENABLE:",
    "LD:",
    "ALARM",
    "OK",
    "UP/DOWN: Speed  SEL: Menu  HOLD SEL: Back/Start-Stop",
    "SETTINGS",
    "Start",
    "Stop",
    "Set DIR = CCW",
    "Set DIR = CW",
    "Brake ON",
    "Brake OFF",
    "Select Motor",
    "Add Motor",
    "Delete Active",
    "Language/Telemetry",
    "About",
    "Back",
    "UP/DOWN=Move  SEL=Select",
    // Wizard
    "Name:",
    "Has BRAKE?",
    "Has FG?",
    "Has LD?",
    "LD active when:",
    "LOW",
    "HIGH",
    "Has STOP?",
    "STOP active when:",
    "Has ENABLE?",
    "ENABLE active:",
    "PPR (pulses/rev)",
    "Max CLOCK (Hz)",
    "Save profile?",
    "YES",
    "NO",
    "UP/DOWN=Change  SEL=Next",
    "UP/DOWN=Move  SEL=OK",
    "UP/DOWN=Change  SEL=OK",
    "UP/DN=Char SEL=Next/END",
    // Settings
    "SETTINGS",
    "Language",
    "English",
    "Espanol",
    "Telemetry",
    "Telemetry: ON",
    "Telemetry: OFF",
    // About
    "ABOUT",
    "Author: Fran-Byte",
    "Version: v2.0",
    "Build:",
    // Diag
    "DIAGNOSTICS",
    "HOLD SEL to exit"};

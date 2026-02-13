#pragma once
#include <Arduino.h>
#include <Preferences.h>
#include "Config.h"

struct MotorProfile {
  char   name[20];
  bool   hasBrake;
  bool   hasFG;
  bool   hasLD;
  bool   ldActiveLow;    // true if LD active when LOW
  bool   hasStop;
  bool   stopActiveHigh; // true if STOP active when HIGH
  bool   hasEnable;
  bool   enableActiveHigh; // true if ENABLE active when HIGH
  uint8_t ppr;           // pulses per revolution (for FG)
  uint32_t maxClockHz;   // safety cap

  void setDefaults() {
    strncpy(name, "Unnamed", sizeof(name));
    hasBrake=false; hasFG=false; hasLD=false; ldActiveLow=true;
    hasStop=false; stopActiveHigh=true;
    hasEnable=false; enableActiveHigh=true;
    ppr=6; maxClockHz=20000;
  }
};

class ProfileStore {
public:
  void begin() {
    prefs.begin("motors", false);
    count = prefs.getUChar("count", 0);
    activeIndex = prefs.getUChar("active", 0);
    if (count > MAX_PROFILES) count = 0;
  }

  int getCount() const { return count; }
  int getActiveIndex() const { return activeIndex; }

  bool load(int idx, MotorProfile &m) {
    if (idx < 0 || idx >= count) return false;
    char key[16];
    snprintf(key, sizeof(key), "m%d_name", idx); String s = prefs.getString(key, "Unnamed"); strncpy(m.name, s.c_str(), sizeof(m.name));
    snprintf(key, sizeof(key), "m%d_br", idx);  m.hasBrake = prefs.getBool(key, false);
    snprintf(key, sizeof(key), "m%d_fg", idx);  m.hasFG    = prefs.getBool(key, false);
    snprintf(key, sizeof(key), "m%d_ld", idx);  m.hasLD    = prefs.getBool(key, false);
    snprintf(key, sizeof(key), "m%d_lda", idx); m.ldActiveLow = prefs.getBool(key, true);
    snprintf(key, sizeof(key), "m%d_st", idx);  m.hasStop  = prefs.getBool(key, false);
    snprintf(key, sizeof(key), "m%d_sta", idx); m.stopActiveHigh = prefs.getBool(key, true);
    snprintf(key, sizeof(key), "m%d_en", idx);  m.hasEnable= prefs.getBool(key, false);
    snprintf(key, sizeof(key), "m%d_ena", idx); m.enableActiveHigh = prefs.getBool(key, true);
    snprintf(key, sizeof(key), "m%d_ppr", idx); m.ppr = prefs.getUChar(key, 6);
    snprintf(key, sizeof(key), "m%d_max", idx); m.maxClockHz = prefs.getUInt(key, 20000);
    return true;
  }

  bool save(int idx, const MotorProfile &m) {
    if (idx < 0 || idx >= MAX_PROFILES) return false;
    char key[16];
    snprintf(key, sizeof(key), "m%d_name", idx); prefs.putString(key, m.name);
    snprintf(key, sizeof(key), "m%d_br", idx);  prefs.putBool(key, m.hasBrake);
    snprintf(key, sizeof(key), "m%d_fg", idx);  prefs.putBool(key, m.hasFG);
    snprintf(key, sizeof(key), "m%d_ld", idx);  prefs.putBool(key, m.hasLD);
    snprintf(key, sizeof(key), "m%d_lda", idx); prefs.putBool(key, m.ldActiveLow);
    snprintf(key, sizeof(key), "m%d_st", idx);  prefs.putBool(key, m.hasStop);
    snprintf(key, sizeof(key), "m%d_sta", idx); prefs.putBool(key, m.stopActiveHigh);
    snprintf(key, sizeof(key), "m%d_en", idx);  prefs.putBool(key, m.hasEnable);
    snprintf(key, sizeof(key), "m%d_ena", idx); prefs.putBool(key, m.enableActiveHigh);
    snprintf(key, sizeof(key), "m%d_ppr", idx); prefs.putUChar(key, m.ppr);
    snprintf(key, sizeof(key), "m%d_max", idx); prefs.putUInt(key, m.maxClockHz);
    if (idx >= count) { count = idx+1; prefs.putUChar("count", count); }
    return true;
  }

  bool append(const MotorProfile &m) {
    if (count >= MAX_PROFILES) return false;
    return save(count, m);
  }

  void remove(int idx) {
    if (idx < 0 || idx >= count) return;
    MotorProfile tmp;
    for (int i=idx; i<count-1; ++i) { load(i+1,tmp); save(i,tmp); }
    // clear tail keys
    char key[16]; int last=count-1; const char* sfx[]={"name","br","fg","ld","lda","st","sta","en","ena","ppr","max"};
    for (auto s:sfx) { snprintf(key,sizeof(key),"m%d_%s",last,s); prefs.remove(key); }
    count--; prefs.putUChar("count", count);
    if (activeIndex >= count) { activeIndex = (count>0?0:255); prefs.putUChar("active", activeIndex); }
  }

  bool loadActive(MotorProfile &m) {
    if (count==0 || activeIndex>=count) return false; return load(activeIndex, m);
  }

  void setActive(int idx) { if (idx>=0 && idx<count) { activeIndex=idx; prefs.putUChar("active", activeIndex);} }

  String nameOf(int idx) {
    if (idx<0 || idx>=count) return String("-");
    char key[16]; snprintf(key,sizeof(key),"m%d_name",idx); return prefs.getString(key, "Unnamed");
  }

private:
  Preferences prefs;
  uint8_t count=0, activeIndex=0;
};

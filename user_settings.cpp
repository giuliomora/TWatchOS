#include <Arduino.h>
#include <Preferences.h>
#include <strings.h>
#include "user_settings.h"
#include "app.h"
#include "config.h"

UserSettings userSettings;

static const char *NVS_NAME = "settings";

void UserSettings::setDefaults()
{
    brightness    = cfg::SCREEN_BRIGHTNESS;
    faceTimeoutS  = (uint8_t)(cfg::SCREEN_TIMEOUT_MS / 1000);
    menuTimeoutS  = (uint8_t)(cfg::MENU_TIMEOUT_MS / 1000);
    wristWake     = cfg::WAKE_ON_WRIST_TILT;
    touchWake     = cfg::WAKE_ON_TOUCH;
    buttonVibrate = cfg::STOPWATCH_VIBRATE;
    stepsGoal     = cfg::STEPS_GOAL;
    stepsOnFace   = cfg::STEPS_ON_WATCHFACE;
    autoSync      = cfg::WIFI_AUTO_SYNC;
}

void UserSettings::begin()
{
    setDefaults();
    Preferences p;
    p.begin(NVS_NAME, false);
    brightness    = p.getUChar("bright", brightness);
    faceTimeoutS  = p.getUChar("faceTo", faceTimeoutS);
    menuTimeoutS  = p.getUChar("menuTo", menuTimeoutS);
    wristWake     = p.getBool("wrist", wristWake);
    touchWake     = p.getBool("touch", touchWake);
    buttonVibrate = p.getBool("vibra", buttonVibrate);
    stepsGoal     = p.getUInt("goal", stepsGoal);
    stepsOnFace   = p.getBool("stepsFace", stepsOnFace);
    autoSync      = p.getBool("autoSync", autoSync);
    p.end();

    // Valori di sicurezza (es. memoria rovinata)
    if (brightness < 20) brightness = 20;
    if (faceTimeoutS == 0) faceTimeoutS = 5;
    if (menuTimeoutS == 0) menuTimeoutS = 15;
    if (stepsGoal < 500) stepsGoal = 500;
    print();
}

void UserSettings::save()
{
    Preferences p;
    p.begin(NVS_NAME, false);
    p.putUChar("bright", brightness);
    p.putUChar("faceTo", faceTimeoutS);
    p.putUChar("menuTo", menuTimeoutS);
    p.putBool("wrist", wristWake);
    p.putBool("touch", touchWake);
    p.putBool("vibra", buttonVibrate);
    p.putUInt("goal", stepsGoal);
    p.putBool("stepsFace", stepsOnFace);
    p.putBool("autoSync", autoSync);
    p.end();
}

void UserSettings::resetToDefaults()
{
    Preferences p;
    p.begin(NVS_NAME, false);
    p.clear();
    p.end();
    setDefaults();
    App::log(name(), "Impostazioni riportate ai valori di config.h");
    print();
}

void UserSettings::print()
{
    App::log(name(), "Luminosita' %u, spegnimento quadrante %u s, menu %u s",
             brightness, faceTimeoutS, menuTimeoutS);
    App::log(name(), "Polso %s, tocco %s, vibrazione %s, obiettivo %lu passi, passi sul quadrante %s, ora automatica %s",
             wristWake ? "si" : "no", touchWake ? "si" : "no", buttonVibrate ? "si" : "no",
             (unsigned long)stepsGoal, stepsOnFace ? "si" : "no", autoSync ? "si" : "no");
}

bool UserSettings::onSerialCommand(const char *line)
{
    if (strcasecmp(line, "impostazioni") == 0) { print(); return true; }
    if (strcasecmp(line, "impostazioni reset") == 0) { resetToDefaults(); return true; }
    return false;
}

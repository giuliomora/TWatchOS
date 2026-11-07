#include <LilyGoLib.h>
#include <LV_Helper.h>
#include "screen.h"
#include "app.h"
#include "config.h"
#include "user_settings.h"

Screen screen;

void Screen::begin()
{
    // All'avvio lo schermo parte acceso
    on = false;
    if (timeoutMs == 0) timeoutMs = userSettings.faceTimeoutMs();
    wake("avvio");
}

void Screen::update()
{
    if (on && timeoutMs > 0 && millis() - lastActivity > timeoutMs) {
        sleep();
    }
}

void Screen::wake(const char *reason)
{
    lastActivity = millis();
    if (on) return;  // gia' acceso: abbiamo solo prolungato il timeout

    on = true;
    // Prima avvisiamo i moduli (cosi' il quadrante si aggiorna),
    // poi accendiamo la retroilluminazione: niente "flash" con l'ora vecchia.
    App::notifyScreenOn();
    lv_timer_handler();
    instance.powerControl(POWER_DISPLAY_BACKLIGHT, true);
    instance.setBrightness(userSettings.brightness);
    App::log(name(), "Acceso (%s)", reason);
}

void Screen::sleep()
{
    if (!on) return;
    on = false;
    instance.setBrightness(0);
    instance.powerControl(POWER_DISPLAY_BACKLIGHT, false);
    App::notifyScreenOff();
    App::log(name(), "Spento");
}

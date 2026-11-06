#include <LilyGoLib.h>
#include <esp_timer.h>
#include <stdio.h>
#include <strings.h>
#include "stopwatch_app.h"
#include "launcher.h"
#include "screen.h"
#include "app.h"
#include "config.h"

StopwatchApp stopwatchApp;

static lv_obj_t *timeLabel = nullptr;
static lv_obj_t *lapLabel = nullptr;
static lv_obj_t *startBtn = nullptr;
static lv_obj_t *startIcon = nullptr;
static lv_obj_t *lapBtn = nullptr;
static lv_obj_t *stopBtn = nullptr;

static const uint32_t COLOR_START = 0x2E7D32;  // verde
static const uint32_t COLOR_PAUSE = 0xEF6C00;  // arancione
static const uint32_t COLOR_LAP   = 0x1565C0;  // blu
static const uint32_t COLOR_STOP  = 0xC62828;  // rosso
static const uint32_t COLOR_GRAY  = 0x9AA0A6;

static int64_t nowUs() { return esp_timer_get_time(); }

// ---- Formattazione del tempo ----------------------------------

void StopwatchApp::formatLong(int64_t us, char *out, size_t size)
{
    if (us < 0) us = 0;
    const uint32_t cs = (uint32_t)(us / 10000);  // centesimi
    const uint32_t totalS = cs / 100;
    const uint32_t h = totalS / 3600, m = (totalS / 60) % 60, s = totalS % 60;
    if (h > 0) snprintf(out, size, "%lu:%02lu:%02lu", (unsigned long)h, (unsigned long)m, (unsigned long)s);
    else snprintf(out, size, "%02lu:%02lu,%02lu", (unsigned long)m, (unsigned long)s, (unsigned long)(cs % 100));
}

void StopwatchApp::formatShort(int64_t us, char *out, size_t size)
{
    if (us < 0) us = 0;
    const uint32_t totalS = (uint32_t)(us / 1000000);
    const uint32_t h = totalS / 3600, m = (totalS / 60) % 60, s = totalS % 60;
    if (h > 0) snprintf(out, size, "%lu:%02lu:%02lu", (unsigned long)h, (unsigned long)m, (unsigned long)s);
    else snprintf(out, size, "%02lu:%02lu", (unsigned long)m, (unsigned long)s);
}

// ---- Pulsanti ----------------------------------------------

// true se il tocco appena finito era uno scorrimento (non un click)
static bool wasSwipe()
{
    lv_indev_t *indev = lv_indev_active();
    return indev && lv_indev_get_gesture_dir(indev) != LV_DIR_NONE;
}

static void onButton(lv_event_t *e)
{
    if (!launcher.touchAllowed() || wasSwipe()) return;
    const int which = (int)(intptr_t)lv_event_get_user_data(e);
    if (which == 0) stopwatchApp.startPause();
    else if (which == 1) stopwatchApp.lap();
    else stopwatchApp.stop();
}

static lv_obj_t *makeButton(lv_obj_t *parent, const char *symbol, uint32_t color, int x, int which,
                            lv_obj_t **iconOut = nullptr)
{
    lv_obj_t *btn = lv_button_create(parent);
    lv_obj_set_size(btn, 64, 52);
    lv_obj_set_pos(btn, x, 108);
    lv_obj_set_style_bg_color(btn, lv_color_hex(color), 0);
    lv_obj_set_style_radius(btn, 16, 0);
    lv_obj_set_style_shadow_width(btn, 0, 0);
    lv_obj_add_event_cb(btn, onButton, LV_EVENT_CLICKED, (void *)(intptr_t)which);

    lv_obj_t *icon = lv_label_create(btn);
    lv_obj_set_style_text_font(icon, &lv_font_montserrat_24, 0);
    lv_obj_set_style_text_color(icon, lv_color_white(), 0);
    lv_label_set_text(icon, symbol);
    lv_obj_center(icon);
    if (iconOut) *iconOut = icon;
    return btn;
}

static void setEnabled(lv_obj_t *btn, bool enabled)
{
    if (enabled) {
        lv_obj_remove_state(btn, LV_STATE_DISABLED);
        lv_obj_set_style_opa(btn, LV_OPA_COVER, 0);
    } else {
        lv_obj_add_state(btn, LV_STATE_DISABLED);   // non riceve tocchi
        lv_obj_set_style_opa(btn, LV_OPA_40, 0);    // e appare sbiadito
    }
}

// ---- Cronometro ---------------------------------------------

int64_t StopwatchApp::elapsedUs() const
{
    return accumulated + (running ? nowUs() - startedAt : 0);
}

void StopwatchApp::startPause()
{
    if (running) {
        accumulated = elapsedUs();
        running = false;
        App::log(name(), "Pausa");
    } else {
        startedAt = nowUs();
        running = true;
        App::log(name(), "Avviato");
    }
    buzz();
    if (isOpen()) screen.setTimeout(screenTimeoutMs());
    refreshUi();
}

void StopwatchApp::lap()
{
    if (!running) return;
    const int64_t total = elapsedUs();
    lastLap = total - lapStart;
    lapStart = total;
    lapCount++;
    char buf[16];
    formatLong(lastLap, buf, sizeof(buf));
    App::log(name(), "Giro %lu: %s", (unsigned long)lapCount, buf);
    buzz();
    refreshUi();
}

void StopwatchApp::stop()
{
    if (!isActive()) return;
    running = false;
    accumulated = startedAt = lapStart = lastLap = 0;
    lapCount = 0;
    App::log(name(), "Azzerato");
    buzz();
    if (isOpen()) screen.setTimeout(screenTimeoutMs());
    refreshUi();
}

void StopwatchApp::buzz()
{
    if (!cfg::STOPWATCH_VIBRATE) return;
    // Effetto 1 = "Strong Click 100%": una vibrazione breve
    instance.drv.setWaveform(0, 1);
    instance.drv.setWaveform(1, 0);
    instance.drv.run();
}

uint32_t StopwatchApp::screenTimeoutMs() const
{
    if (running && cfg::STOPWATCH_KEEP_SCREEN_ON) return 0;  // sempre acceso
    return cfg::MENU_TIMEOUT_MS;
}

void StopwatchApp::update()
{
    // Aggiorna il tempo a schermo solo se l'app e' aperta e sta contando
    if (!running || !isOpen() || !screen.isOn()) return;
    if (millis() - lastUi < 40) return;
    refreshUi();
}

void StopwatchApp::onOpen()
{
    refreshUi();
}

void StopwatchApp::build(lv_obj_t *parent)
{
    timeLabel = lv_label_create(parent);
    lv_obj_set_style_text_font(timeLabel, &lv_font_montserrat_40, 0);
    lv_obj_set_style_text_color(timeLabel, lv_color_white(), 0);
    lv_obj_align(timeLabel, LV_ALIGN_TOP_MID, 0, 8);

    lapLabel = lv_label_create(parent);
    lv_obj_set_style_text_font(lapLabel, &lv_font_montserrat_18, 0);
    lv_obj_set_style_text_color(lapLabel, lv_color_hex(COLOR_GRAY), 0);
    lv_obj_align(lapLabel, LV_ALIGN_TOP_MID, 0, 64);

    startBtn = makeButton(parent, LV_SYMBOL_PLAY, COLOR_START, 12, 0, &startIcon);
    lapBtn = makeButton(parent, LV_SYMBOL_REFRESH, COLOR_LAP, 88, 1);
    stopBtn = makeButton(parent, LV_SYMBOL_STOP, COLOR_STOP, 164, 2);

    refreshUi();
}

void StopwatchApp::refreshUi()
{
    lastUi = millis();
    if (!timeLabel) return;
    char buf[16];

    formatLong(elapsedUs(), buf, sizeof(buf));
    lv_label_set_text(timeLabel, buf);
    lv_obj_align(timeLabel, LV_ALIGN_TOP_MID, 0, 8);

    if (lapCount > 0) {
        formatLong(lastLap, buf, sizeof(buf));
        lv_label_set_text_fmt(lapLabel, "Giro %lu:  %s", (unsigned long)lapCount, buf);
    } else {
        lv_label_set_text(lapLabel, "");
    }
    lv_obj_align(lapLabel, LV_ALIGN_TOP_MID, 0, 64);

    // Avvia <-> Pausa
    lv_label_set_text(startIcon, running ? LV_SYMBOL_PAUSE : LV_SYMBOL_PLAY);
    lv_obj_set_style_bg_color(startBtn, lv_color_hex(running ? COLOR_PAUSE : COLOR_START), 0);
    setEnabled(lapBtn, running);
    setEnabled(stopBtn, isActive());
}

bool StopwatchApp::onSerialCommand(const char *line)
{
    if (strcasecmp(line, "crono") != 0) return false;
    char t[16], l[16];
    formatLong(elapsedUs(), t, sizeof(t));
    formatLong(lastLap, l, sizeof(l));
    App::log(name(), "%s, tempo %s, giri %lu (ultimo %s)",
             running ? "IN CORSO" : (isActive() ? "in pausa" : "fermo"),
             t, (unsigned long)lapCount, l);
    return true;
}

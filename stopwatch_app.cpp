#include <LilyGoLib.h>
#include <esp_timer.h>
#include <stdio.h>
#include <strings.h>
#include "stopwatch_app.h"
#include "countdown_timer.h"
#include "launcher.h"
#include "screen.h"
#include "app.h"
#include "config.h"
#include "user_settings.h"

StopwatchApp stopwatchApp;

// Schede
static lv_obj_t *tabBtn[2] = {nullptr, nullptr};
static lv_obj_t *tabText[2] = {nullptr, nullptr};
static lv_obj_t *tabLine = nullptr;
static lv_obj_t *page[2] = {nullptr, nullptr};

// Cronometro
static lv_obj_t *timeLabel = nullptr;
static lv_obj_t *lapLabel = nullptr;
static lv_obj_t *startBtn = nullptr;
static lv_obj_t *startIcon = nullptr;
static lv_obj_t *lapBtn = nullptr;
static lv_obj_t *stopBtn = nullptr;

static const uint32_t COLOR_STOPWATCH = 0xEF6C00;  // arancione
static const uint32_t COLOR_TIMER     = 0x00838F;  // azzurro petrolio
static const uint32_t COLOR_TAB_OFF   = 0x2C2C2E;
static const uint32_t COLOR_START = 0x2E7D32;  // verde
static const uint32_t COLOR_PAUSE = 0xEF6C00;  // arancione
static const uint32_t COLOR_LAP   = 0x1565C0;  // blu
static const uint32_t COLOR_STOP  = 0xC62828;  // rosso
static const uint32_t COLOR_GRAY  = 0x9AA0A6;

static const int TABS_H = 30;           // altezza delle schede
static const int PAGE_Y = TABS_H + 8;   // il contenuto parte da qui

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

// ---- Icona del menu: un orologio disegnato ------------------

bool StopwatchApp::drawIcon(lv_obj_t *tile)
{
    lv_obj_t *face = lv_obj_create(tile);
    lv_obj_set_size(face, 34, 34);
    lv_obj_set_style_radius(face, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_opa(face, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_color(face, lv_color_white(), 0);
    lv_obj_set_style_border_width(face, 3, 0);
    lv_obj_set_style_pad_all(face, 0, 0);
    lv_obj_remove_flag(face, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_remove_flag(face, LV_OBJ_FLAG_CLICKABLE);  // il tocco va al riquadro
    lv_obj_align(face, LV_ALIGN_CENTER, 0, -12);

    // Lancette: dal centro (14,14) verso le 12 e verso le 3
    static lv_point_precise_t hands[] = {{14, 5}, {14, 14}, {21, 14}};
    lv_obj_t *line = lv_line_create(face);
    lv_line_set_points(line, hands, 3);
    lv_obj_set_style_line_width(line, 3, 0);
    lv_obj_set_style_line_color(line, lv_color_white(), 0);
    lv_obj_set_style_line_rounded(line, true, 0);
    lv_obj_remove_flag(line, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_pos(line, 0, 0);
    return true;
}

// ---- Schede ------------------------------------------------

static void onTab(lv_event_t *e)
{
    if (!launcher.clickAllowed()) return;
    stopwatchApp.showTab((StopwatchApp::Tab)(intptr_t)lv_event_get_user_data(e));
}

void StopwatchApp::showTab(Tab t)
{
    tab = t;
    if (!tabBtn[0]) return;
    const uint32_t active = (t == TAB_TIMER) ? COLOR_TIMER : COLOR_STOPWATCH;
    for (int i = 0; i < 2; i++) {
        const bool on = (i == (int)t);
        lv_obj_set_style_bg_color(tabBtn[i], lv_color_hex(on ? active : COLOR_TAB_OFF), 0);
        lv_obj_set_style_text_color(tabText[i], on ? lv_color_white() : lv_color_hex(COLOR_GRAY), 0);
        if (on) lv_obj_remove_flag(page[i], LV_OBJ_FLAG_HIDDEN);
        else lv_obj_add_flag(page[i], LV_OBJ_FLAG_HIDDEN);
    }
    // la linea sotto le schede prende il colore di quella attiva
    lv_obj_set_style_bg_color(tabLine, lv_color_hex(active), 0);
    if (t == TAB_TIMER) countdownTimer.refreshUi();
    else refreshUi();
}

// ---- Pulsanti del cronometro ----------------------------------

static void onButton(lv_event_t *e)
{
    if (!launcher.clickAllowed()) return;
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
    lv_obj_set_pos(btn, x, 104);
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
        App::log(name(), "Cronometro in pausa");
    } else {
        startedAt = nowUs();
        running = true;
        App::log(name(), "Cronometro avviato");
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
    App::log(name(), "Cronometro azzerato");
    buzz();
    if (isOpen()) screen.setTimeout(screenTimeoutMs());
    refreshUi();
}

void StopwatchApp::buzz()
{
    if (!userSettings.buttonVibrate) return;
    // Effetto 1 = "Strong Click 100%": una vibrazione breve
    instance.drv.setWaveform(0, 1);
    instance.drv.setWaveform(1, 0);
    instance.drv.run();
}

uint32_t StopwatchApp::screenTimeoutMs() const
{
    if (countdownTimer.isRinging()) return 0;                // squillo: sempre acceso
    if (running && cfg::STOPWATCH_KEEP_SCREEN_ON) return 0;  // sempre acceso
    return userSettings.menuTimeoutMs();
}

void StopwatchApp::update()
{
    // Aggiorna il tempo a schermo solo se si vede il cronometro e sta contando
    if (!running || !isOpen() || tab != TAB_STOPWATCH || !screen.isOn()) return;
    if (millis() - lastUi < 40) return;
    refreshUi();
}

void StopwatchApp::onOpen()
{
    showTab(tab);
}

void StopwatchApp::build(lv_obj_t *parent)
{
    // --- Schede in alto, come segnalibri ---
    // Il contenitore e' basso quanto le schede: taglia gli angoli
    // arrotondati in basso, cosi' sembrano linguette.
    lv_obj_t *tabs = lv_obj_create(parent);
    lv_obj_set_size(tabs, 240, TABS_H);
    lv_obj_set_pos(tabs, 0, 0);
    lv_obj_set_style_bg_opa(tabs, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(tabs, 0, 0);
    lv_obj_set_style_pad_all(tabs, 0, 0);
    lv_obj_set_style_radius(tabs, 0, 0);
    lv_obj_remove_flag(tabs, LV_OBJ_FLAG_SCROLLABLE);

    static const char *NAMES[2] = {"Cronometro", "Timer"};
    for (int i = 0; i < 2; i++) {
        lv_obj_t *b = lv_button_create(tabs);
        lv_obj_set_size(b, 106, TABS_H + 12);  // piu' alto del contenitore: il fondo resta nascosto
        lv_obj_set_pos(b, i == 0 ? 12 : 122, 0);
        lv_obj_set_style_radius(b, 10, 0);
        lv_obj_set_style_shadow_width(b, 0, 0);
        lv_obj_set_style_pad_all(b, 0, 0);
        lv_obj_add_event_cb(b, onTab, LV_EVENT_CLICKED, (void *)(intptr_t)i);
        lv_obj_t *t = lv_label_create(b);
        lv_obj_set_style_text_font(t, &lv_font_montserrat_16, 0);
        lv_label_set_text(t, NAMES[i]);
        lv_obj_align(t, LV_ALIGN_TOP_MID, 0, 6);
        tabBtn[i] = b;
        tabText[i] = t;
    }
    // Linea sotto le schede, del colore di quella attiva
    tabLine = lv_obj_create(parent);
    lv_obj_set_size(tabLine, 240, 3);
    lv_obj_set_pos(tabLine, 0, TABS_H);
    lv_obj_set_style_border_width(tabLine, 0, 0);
    lv_obj_set_style_radius(tabLine, 0, 0);
    lv_obj_set_style_bg_opa(tabLine, LV_OPA_COVER, 0);
    lv_obj_remove_flag(tabLine, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_remove_flag(tabLine, LV_OBJ_FLAG_CLICKABLE);

    // --- Due pagine, una per scheda ---
    // L'altezza dell'area va fatta calcolare a LVGL prima di leggerla
    // (altrimenti vale 0 e le pagine diventano invisibili)
    lv_obj_update_layout(parent);
    int32_t pageH = lv_obj_get_content_height(parent) - PAGE_Y;
    if (pageH <= 0) pageH = 200 - PAGE_Y;
    for (int i = 0; i < 2; i++) {
        lv_obj_t *p = lv_obj_create(parent);
        lv_obj_set_size(p, 240, pageH);
        lv_obj_set_pos(p, 0, PAGE_Y);
        lv_obj_set_style_bg_opa(p, LV_OPA_TRANSP, 0);
        lv_obj_set_style_border_width(p, 0, 0);
        lv_obj_set_style_pad_all(p, 0, 0);
        lv_obj_set_style_radius(p, 0, 0);
        lv_obj_remove_flag(p, LV_OBJ_FLAG_SCROLLABLE);
        page[i] = p;
    }

    // --- Pagina Cronometro ---
    lv_obj_t *sw = page[TAB_STOPWATCH];
    timeLabel = lv_label_create(sw);
    lv_obj_set_style_text_font(timeLabel, &lv_font_montserrat_40, 0);
    lv_obj_set_style_text_color(timeLabel, lv_color_white(), 0);

    lapLabel = lv_label_create(sw);
    lv_obj_set_style_text_font(lapLabel, &lv_font_montserrat_18, 0);
    lv_obj_set_style_text_color(lapLabel, lv_color_hex(COLOR_GRAY), 0);

    startBtn = makeButton(sw, LV_SYMBOL_PLAY, COLOR_START, 12, 0, &startIcon);
    lapBtn = makeButton(sw, LV_SYMBOL_REFRESH, COLOR_LAP, 88, 1);
    stopBtn = makeButton(sw, LV_SYMBOL_STOP, COLOR_STOP, 164, 2);

    // --- Pagina Timer ---
    countdownTimer.build(page[TAB_TIMER]);

    showTab(TAB_STOPWATCH);
}

void StopwatchApp::refreshUi()
{
    lastUi = millis();
    if (!timeLabel) return;
    char buf[16];

    formatLong(elapsedUs(), buf, sizeof(buf));
    lv_label_set_text(timeLabel, buf);
    lv_obj_align(timeLabel, LV_ALIGN_TOP_MID, 0, 4);

    if (lapCount > 0) {
        formatLong(lastLap, buf, sizeof(buf));
        lv_label_set_text_fmt(lapLabel, "Giro %lu:  %s", (unsigned long)lapCount, buf);
    } else {
        lv_label_set_text(lapLabel, "");
    }
    lv_obj_align(lapLabel, LV_ALIGN_TOP_MID, 0, 58);

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
    App::log(name(), "Cronometro %s, tempo %s, giri %lu (ultimo %s)",
             running ? "IN CORSO" : (isActive() ? "in pausa" : "fermo"),
             t, (unsigned long)lapCount, l);
    return true;
}

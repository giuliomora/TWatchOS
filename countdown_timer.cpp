#include <LilyGoLib.h>
#include <esp_timer.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include "countdown_timer.h"
#include "stopwatch_app.h"
#include "launcher.h"
#include "screen.h"
#include "app.h"
#include "config.h"
#include "user_settings.h"

CountdownTimer countdownTimer;

static const uint32_t COLOR_TIMER  = 0x00838F;  // azzurro petrolio
static const uint32_t COLOR_DARK   = 0x3A3A3C;
static const uint32_t COLOR_START  = 0x2E7D32;  // verde
static const uint32_t COLOR_PAUSE  = 0xEF6C00;  // arancione
static const uint32_t COLOR_CANCEL = 0xC62828;  // rosso
static const uint32_t COLOR_RING_BG = 0x2C2C2E;

// Le quattro "viste" della scheda (una sola visibile alla volta)
static lv_obj_t *setupBox = nullptr;
static lv_obj_t *pickerBox = nullptr;
static lv_obj_t *runBox = nullptr;
static lv_obj_t *ringBox = nullptr;

static lv_obj_t *minRoller = nullptr;
static lv_obj_t *secRoller = nullptr;
static lv_obj_t *arc = nullptr;
static lv_obj_t *leftLabel = nullptr;
static lv_obj_t *pauseBtn = nullptr;
static lv_obj_t *pauseIcon = nullptr;

static int64_t nowUs() { return esp_timer_get_time(); }

enum Action { PRESET_1, PRESET_2, PRESET_3, OPEN_PICKER, PICKER_CANCEL, PICKER_START,
              PAUSE_RESUME, CANCEL, STOP_RING };

static void onButton(lv_event_t *e)
{
    if (!launcher.clickAllowed()) return;
    switch ((int)(intptr_t)lv_event_get_user_data(e)) {
    case PRESET_1:      countdownTimer.start(cfg::TIMER_PRESET_1_MIN * 60); break;
    case PRESET_2:      countdownTimer.start(cfg::TIMER_PRESET_2_MIN * 60); break;
    case PRESET_3:      countdownTimer.start(cfg::TIMER_PRESET_3_MIN * 60); break;
    case OPEN_PICKER:   countdownTimer.openPicker(); break;
    case PICKER_CANCEL: countdownTimer.closePicker(); break;
    case PICKER_START:  countdownTimer.startFromPicker(); break;
    case PAUSE_RESUME:  countdownTimer.pauseResume(); break;
    case CANCEL:        countdownTimer.cancel(); break;
    case STOP_RING:     countdownTimer.stopRinging(); break;
    }
}

// Contenitore trasparente che occupa tutta la scheda
static lv_obj_t *makeBox(lv_obj_t *parent)
{
    lv_obj_t *box = lv_obj_create(parent);
    lv_obj_set_size(box, lv_pct(100), lv_pct(100));
    lv_obj_set_pos(box, 0, 0);
    lv_obj_set_style_bg_opa(box, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(box, 0, 0);
    lv_obj_set_style_pad_all(box, 0, 0);
    lv_obj_remove_flag(box, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(box, LV_OBJ_FLAG_HIDDEN);
    return box;
}

static lv_obj_t *makeButton(lv_obj_t *parent, const char *text, const lv_font_t *font, uint32_t color,
                            int x, int y, int w, int h, Action action, lv_obj_t **labelOut = nullptr)
{
    lv_obj_t *btn = lv_button_create(parent);
    lv_obj_set_size(btn, w, h);
    lv_obj_set_pos(btn, x, y);
    lv_obj_set_style_bg_color(btn, lv_color_hex(color), 0);
    lv_obj_set_style_radius(btn, 16, 0);
    lv_obj_set_style_shadow_width(btn, 0, 0);
    lv_obj_add_event_cb(btn, onButton, LV_EVENT_CLICKED, (void *)(intptr_t)action);
    lv_obj_t *label = lv_label_create(btn);
    lv_obj_set_style_text_font(label, font, 0);
    lv_obj_set_style_text_color(label, lv_color_white(), 0);
    lv_label_set_text(label, text);
    lv_obj_center(label);
    if (labelOut) *labelOut = label;
    return btn;
}

static lv_obj_t *makeRoller(lv_obj_t *parent, int maxValue, int x)
{
    // Opzioni "00\n01\n...": al massimo 100 valori da 3 caratteri
    static char options[2][100 * 3 + 1];
    static int used = 0;
    char *buf = options[used++ % 2];
    buf[0] = '\0';
    for (int i = 0; i <= maxValue; i++) {
        char item[4];
        snprintf(item, sizeof(item), i < maxValue ? "%02d\n" : "%02d", i);
        strcat(buf, item);
    }
    lv_obj_t *r = lv_roller_create(parent);
    lv_roller_set_options(r, buf, LV_ROLLER_MODE_NORMAL);
    lv_roller_set_visible_row_count(r, 3);
    lv_obj_set_size(r, 76, 100);   // non deve coprire i pulsanti sotto
    lv_obj_set_pos(r, x, 0);
    lv_obj_set_style_text_font(r, &lv_font_montserrat_24, 0);
    lv_obj_set_style_bg_color(r, lv_color_hex(0x1C1C1E), 0);
    lv_obj_set_style_border_width(r, 0, 0);
    lv_obj_set_style_text_color(r, lv_color_hex(0x9AA0A6), 0);
    lv_obj_set_style_bg_color(r, lv_color_hex(COLOR_TIMER), LV_PART_SELECTED);
    lv_obj_set_style_text_color(r, lv_color_white(), LV_PART_SELECTED);
    return r;
}

void CountdownTimer::formatRemaining(int64_t us, char *out, size_t size)
{
    if (us < 0) us = 0;
    const uint32_t totalS = (uint32_t)((us + 999999) / 1000000);  // per eccesso: 5:00 ... 0:01
    const uint32_t h = totalS / 3600, m = (totalS / 60) % 60, s = totalS % 60;
    if (h > 0) snprintf(out, size, "%lu:%02lu:%02lu", (unsigned long)h, (unsigned long)m, (unsigned long)s);
    else snprintf(out, size, "%02lu:%02lu", (unsigned long)m, (unsigned long)s);
}

// ---- Disegno della scheda ----------------------------------

void CountdownTimer::build(lv_obj_t *parent)
{
    char text[16];

    // 1) Scelta rapida: griglia 2x2
    setupBox = makeBox(parent);
    snprintf(text, sizeof(text), "%lu min", (unsigned long)cfg::TIMER_PRESET_1_MIN);
    makeButton(setupBox, text, &lv_font_montserrat_22, COLOR_TIMER, 14, 2, 102, 72, PRESET_1);
    snprintf(text, sizeof(text), "%lu min", (unsigned long)cfg::TIMER_PRESET_2_MIN);
    makeButton(setupBox, text, &lv_font_montserrat_22, COLOR_TIMER, 124, 2, 102, 72, PRESET_2);
    snprintf(text, sizeof(text), "%lu min", (unsigned long)cfg::TIMER_PRESET_3_MIN);
    makeButton(setupBox, text, &lv_font_montserrat_22, COLOR_TIMER, 14, 82, 102, 72, PRESET_3);
    makeButton(setupBox, "Altro", &lv_font_montserrat_22, COLOR_DARK, 124, 82, 102, 72, OPEN_PICKER);

    // 2) Rotelle minuti : secondi
    pickerBox = makeBox(parent);
    minRoller = makeRoller(pickerBox, 99, 36);
    secRoller = makeRoller(pickerBox, 59, 128);
    lv_roller_set_selected(minRoller, 1, LV_ANIM_OFF);
    lv_obj_t *colon = lv_label_create(pickerBox);
    lv_obj_set_style_text_font(colon, &lv_font_montserrat_24, 0);
    lv_obj_set_style_text_color(colon, lv_color_white(), 0);
    lv_label_set_text(colon, ":");
    lv_obj_align_to(colon, minRoller, LV_ALIGN_OUT_RIGHT_MID, 6, 0);
    makeButton(pickerBox, "Annulla", &lv_font_montserrat_18, COLOR_DARK, 14, 110, 102, 48, PICKER_CANCEL);
    makeButton(pickerBox, LV_SYMBOL_PLAY " Avvia", &lv_font_montserrat_18, COLOR_START, 124, 110, 102, 48, PICKER_START);

    // 3) Conto alla rovescia: anello che si svuota
    runBox = makeBox(parent);
    arc = lv_arc_create(runBox);
    lv_obj_set_size(arc, 100, 100);
    lv_obj_align(arc, LV_ALIGN_TOP_MID, 0, 0);
    lv_arc_set_rotation(arc, 270);
    lv_arc_set_bg_angles(arc, 0, 360);
    lv_arc_set_range(arc, 0, 1000);
    lv_obj_remove_style(arc, NULL, LV_PART_KNOB);
    lv_obj_remove_flag(arc, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_style_arc_width(arc, 10, LV_PART_MAIN);
    lv_obj_set_style_arc_width(arc, 10, LV_PART_INDICATOR);
    lv_obj_set_style_arc_color(arc, lv_color_hex(COLOR_RING_BG), LV_PART_MAIN);
    lv_obj_set_style_arc_color(arc, lv_color_hex(COLOR_TIMER), LV_PART_INDICATOR);
    lv_obj_set_style_arc_rounded(arc, true, LV_PART_INDICATOR);
    leftLabel = lv_label_create(runBox);
    lv_obj_set_style_text_font(leftLabel, &lv_font_montserrat_24, 0);
    lv_obj_set_style_text_color(leftLabel, lv_color_white(), 0);
    pauseBtn = makeButton(runBox, LV_SYMBOL_PAUSE, &lv_font_montserrat_24, COLOR_PAUSE, 14, 108, 102, 50,
                          PAUSE_RESUME, &pauseIcon);
    makeButton(runBox, LV_SYMBOL_CLOSE, &lv_font_montserrat_24, COLOR_CANCEL, 124, 108, 102, 50, CANCEL);

    // 4) Tempo scaduto
    ringBox = makeBox(parent);
    lv_obj_t *msg = lv_label_create(ringBox);
    lv_obj_set_style_text_font(msg, &lv_font_montserrat_24, 0);
    lv_obj_set_style_text_color(msg, lv_color_hex(COLOR_PAUSE), 0);
    lv_label_set_text(msg, "Tempo scaduto!");
    lv_obj_align(msg, LV_ALIGN_TOP_MID, 0, 6);
    makeButton(ringBox, LV_SYMBOL_STOP " Stop", &lv_font_montserrat_28, COLOR_CANCEL, 20, 50, 200, 90, STOP_RING);

    refreshUi();
}

void CountdownTimer::refreshUi()
{
    lastUi = millis();
    if (!setupBox) return;

    lv_obj_t *visible = setupBox;
    if (st == PICKER) visible = pickerBox;
    else if (st == RUNNING || st == PAUSED) visible = runBox;
    else if (st == RINGING) visible = ringBox;
    lv_obj_t *boxes[] = {setupBox, pickerBox, runBox, ringBox};
    for (lv_obj_t *b : boxes) {
        if (b == visible) lv_obj_remove_flag(b, LV_OBJ_FLAG_HIDDEN);
        else lv_obj_add_flag(b, LV_OBJ_FLAG_HIDDEN);
    }

    if (visible == runBox) {
        const int64_t left = remainingUs();
        char buf[16];
        formatRemaining(left, buf, sizeof(buf));
        lv_label_set_text(leftLabel, buf);
        lv_obj_align_to(leftLabel, arc, LV_ALIGN_CENTER, 0, 0);
        const int32_t permille = totalUs > 0 ? (int32_t)(left * 1000 / totalUs) : 0;
        lv_arc_set_value(arc, permille < 0 ? 0 : permille);
        lv_label_set_text(pauseIcon, st == RUNNING ? LV_SYMBOL_PAUSE : LV_SYMBOL_PLAY);
        lv_obj_set_style_bg_color(pauseBtn, lv_color_hex(st == RUNNING ? COLOR_PAUSE : COLOR_START), 0);
    }
}

// ---- Logica ------------------------------------------------

int64_t CountdownTimer::remainingUs() const
{
    if (st == RUNNING) {
        const int64_t left = endAt - nowUs();
        return left > 0 ? left : 0;
    }
    if (st == PAUSED) return pausedLeft;
    return 0;
}

void CountdownTimer::setState(State s)
{
    st = s;
    refreshUi();
    // il tempo di spegnimento dello schermo dipende dallo stato (squillo = sempre acceso)
    if (stopwatchApp.isOpen()) screen.setTimeout(stopwatchApp.screenTimeoutMs());
}

void CountdownTimer::start(uint32_t seconds)
{
    if (seconds == 0) return;
    totalUs = (int64_t)seconds * 1000000;
    endAt = nowUs() + totalUs;
    buzz();
    App::log(name(), "Avviato: %lu secondi", (unsigned long)seconds);
    setState(RUNNING);
}

void CountdownTimer::pauseResume()
{
    if (st == RUNNING) {
        pausedLeft = remainingUs();
        App::log(name(), "Pausa");
        buzz();
        setState(PAUSED);
    } else if (st == PAUSED) {
        endAt = nowUs() + pausedLeft;
        App::log(name(), "Ripreso");
        buzz();
        setState(RUNNING);
    }
}

void CountdownTimer::cancel()
{
    if (!isActive()) return;
    App::log(name(), "Annullato");
    buzz();
    setState(SETUP);
}

void CountdownTimer::openPicker()
{
    if (st == SETUP) setState(PICKER);
}

void CountdownTimer::closePicker()
{
    if (st == PICKER) setState(SETUP);
}

void CountdownTimer::startFromPicker()
{
    const uint32_t m = lv_roller_get_selected(minRoller);
    const uint32_t s = lv_roller_get_selected(secRoller);
    start(m * 60 + s);  // con 00:00 non parte
}

void CountdownTimer::expire()
{
    App::log(name(), "Tempo scaduto!");
    ringStartedAt = nowUs();
    lastPulseAt = 0;
    st = RINGING;
    // Accendi lo schermo e porta in primo piano la scheda Timer
    screen.wake("timer");
    if (!stopwatchApp.isOpen()) launcher.openApp(&stopwatchApp);
    stopwatchApp.showTab(StopwatchApp::TAB_TIMER);
    setState(RINGING);
    ringPulse();
}

void CountdownTimer::stopRinging()
{
    if (st != RINGING) return;
    App::log(name(), "Squillo fermato");
    setState(SETUP);
    if (!stopwatchApp.isOpen()) screen.setTimeout(userSettings.faceTimeoutMs());
    screen.wake("timer");  // riparte il conto alla rovescia dello schermo
}

void CountdownTimer::ringPulse()
{
    lastPulseAt = nowUs();
    // Due vibrazioni forti (effetto 14) con una pausa di 150 ms
    instance.drv.setWaveform(0, 14);
    instance.drv.setWaveform(1, 0x80 | 15);
    instance.drv.setWaveform(2, 14);
    instance.drv.setWaveform(3, 0);
    instance.drv.run();
}

void CountdownTimer::buzz()
{
    if (!userSettings.buttonVibrate) return;
    instance.drv.setWaveform(0, 1);  // vibrazione breve
    instance.drv.setWaveform(1, 0);
    instance.drv.run();
}

void CountdownTimer::update()
{
    if (st == RUNNING) {
        if (remainingUs() <= 0) { expire(); return; }
        if (stopwatchApp.isOpen() && screen.isOn() && millis() - lastUi >= 200) refreshUi();
    } else if (st == RINGING) {
        const int64_t now = nowUs();
        if (now - ringStartedAt >= (int64_t)cfg::TIMER_RING_S * 1000000) {
            App::log(name(), "Nessuno ha fermato lo squillo: smetto");
            stopRinging();
        } else if (now - lastPulseAt >= (int64_t)cfg::TIMER_RING_PULSE_MS * 1000) {
            ringPulse();
        }
    }
}

uint32_t CountdownTimer::msUntilNextWork()
{
    if (st == RINGING) return 0;
    if (st != RUNNING) return UINT32_MAX;
    return (uint32_t)(remainingUs() / 1000);  // svegliati alla scadenza
}

bool CountdownTimer::onSerialCommand(const char *line)
{
    if (strncasecmp(line, "timer", 5) != 0) return false;
    const char *arg = line + 5;
    if (*arg != '\0' && *arg != ' ') return false;
    while (*arg == ' ') arg++;
    if (*arg) {
        const long s = atol(arg);
        if (s <= 0) { App::log(name(), "Uso: timer <secondi>  (es. timer 10)"); return true; }
        if (st == RINGING) stopRinging();
        start((uint32_t)s);
        return true;
    }
    static const char *NAMES[] = {"scelta", "rotelle", "IN CORSO", "in pausa", "STA SUONANDO"};
    char buf[16];
    formatRemaining(remainingUs(), buf, sizeof(buf));
    App::log(name(), "Stato: %s, rimasto %s", NAMES[st], buf);
    return true;
}

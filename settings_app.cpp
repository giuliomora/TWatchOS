#include <LilyGoLib.h>
#include <Arduino.h>
#include <stdio.h>
#include <string.h>
#include "settings_app.h"
#include "user_settings.h"
#include "launcher.h"
#include "screen.h"
#include "power.h"
#include "wifi_time.h"
#include "time_service.h"
#include "steps_app.h"
#include "app.h"
#include "config.h"

SettingsApp settingsApp;

static const uint32_t COLOR_ROW     = 0x1C1C1E;  // sfondo delle righe
static const uint32_t COLOR_PRESSED = 0x3A3A3C;
static const uint32_t COLOR_BTN     = 0x3A3A3C;
static const uint32_t COLOR_GRAY    = 0x9AA0A6;
static const uint32_t COLOR_ON      = 0x34C759;  // interruttore acceso
static const uint32_t COLOR_RED     = 0xFF453A;

// Valori possibili
static const uint8_t FACE_OPTS[] = {5, 10, 15, 30};
static const uint8_t MENU_OPTS[] = {10, 15, 30, 60};
static const uint32_t GOAL_MIN = 2000, GOAL_MAX = 20000, GOAL_STEP = 500;

enum Choice { CH_FACE, CH_MENU, CH_GOAL, CH_COUNT };
enum Toggle { TG_WRIST, TG_TOUCH, TG_VIBRA, TG_STEPS_FACE, TG_AUTOSYNC, TG_COUNT };

static const char *RESET_TEXT = "Ripristina impostazioni";

// Widget
static lv_obj_t *listBox = nullptr;
static lv_obj_t *infoBox = nullptr;
static lv_obj_t *brightSlider = nullptr;
static lv_obj_t *choiceValue[CH_COUNT] = {};
static lv_obj_t *toggleSw[TG_COUNT] = {};
static lv_obj_t *syncSub = nullptr;
static lv_obj_t *resetLabel = nullptr;
static lv_obj_t *batteryText = nullptr;
static lv_obj_t *systemText = nullptr;

// ---- Funzioni di supporto -----------------------------------

// "6 h 12 min", "2 g 5 h", "12 min"
static void formatDuration(uint32_t minutes, char *out, size_t size)
{
    if (minutes >= 1440) snprintf(out, size, "%lu g %lu h", (unsigned long)(minutes / 1440), (unsigned long)((minutes % 1440) / 60));
    else if (minutes >= 60) snprintf(out, size, "%lu h %lu min", (unsigned long)(minutes / 60), (unsigned long)(minutes % 60));
    else snprintf(out, size, "%lu min", (unsigned long)minutes);
}

// Numero con una cifra decimale e la virgola: 1.8 -> "1,8"
static void formatDecimal(float v, char *out, size_t size)
{
    snprintf(out, size, "%.1f", v);
    for (char *c = out; *c; c++) if (*c == '.') *c = ',';
}

static lv_obj_t *makeText(lv_obj_t *parent, const char *text, const lv_font_t *font, uint32_t color)
{
    lv_obj_t *l = lv_label_create(parent);
    lv_obj_set_style_text_font(l, font, 0);
    lv_obj_set_style_text_color(l, lv_color_hex(color), 0);
    lv_label_set_text(l, text);
    return l;
}

// Riga della lista. clickable = la riga intera si puo' toccare.
static lv_obj_t *makeRow(lv_obj_t *list, int h, bool clickable = false)
{
    lv_obj_t *row = lv_obj_create(list);
    lv_obj_set_size(row, lv_pct(100), h);
    lv_obj_set_style_bg_color(row, lv_color_hex(COLOR_ROW), 0);
    lv_obj_set_style_bg_opa(row, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(row, 0, 0);
    lv_obj_set_style_radius(row, 12, 0);
    lv_obj_set_style_pad_hor(row, 10, 0);
    lv_obj_set_style_pad_ver(row, 6, 0);
    lv_obj_remove_flag(row, LV_OBJ_FLAG_SCROLLABLE);
    if (clickable) {
        lv_obj_set_style_bg_color(row, lv_color_hex(COLOR_PRESSED), LV_STATE_PRESSED);
    } else {
        lv_obj_remove_flag(row, LV_OBJ_FLAG_CLICKABLE);  // il dito fa solo scorrere la lista
    }
    return row;
}

// Contenitore che scorre in verticale (lista e pagina Info)
static lv_obj_t *makeScrollBox(lv_obj_t *parent, int h)
{
    lv_obj_t *box = lv_obj_create(parent);
    lv_obj_set_size(box, 240, h);
    lv_obj_set_pos(box, 0, 0);
    lv_obj_set_style_bg_color(box, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(box, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(box, 0, 0);
    lv_obj_set_style_radius(box, 0, 0);
    lv_obj_set_style_pad_hor(box, 6, 0);
    lv_obj_set_style_pad_top(box, 2, 0);
    lv_obj_set_style_pad_bottom(box, 12, 0);
    lv_obj_set_style_pad_row(box, 6, 0);
    lv_obj_set_flex_flow(box, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_scroll_dir(box, LV_DIR_VER);
    lv_obj_set_scrollbar_mode(box, LV_SCROLLBAR_MODE_ACTIVE);
    return box;
}

// ---- Eventi ------------------------------------------------

static void onChoice(lv_event_t *e)
{
    if (!launcher.clickAllowed()) return;
    const int code = (int)(intptr_t)lv_event_get_user_data(e);
    settingsApp.changeChoice(code / 2, (code % 2) ? +1 : -1);
}

static void onToggle(lv_event_t *e)
{
    lv_obj_t *sw = (lv_obj_t *)lv_event_get_target(e);
    const int id = (int)(intptr_t)lv_event_get_user_data(e);
    const bool on = lv_obj_has_state(sw, LV_STATE_CHECKED);
    if (!launcher.clickAllowed()) {
        // era uno scorrimento o un tocco subito dopo il risveglio: annulla
        if (on) lv_obj_remove_state(sw, LV_STATE_CHECKED);
        else lv_obj_add_state(sw, LV_STATE_CHECKED);
        return;
    }
    settingsApp.toggle(id, on);
}

static void onSlider(lv_event_t *e)
{
    lv_obj_t *s = (lv_obj_t *)lv_event_get_target(e);
    const bool released = lv_event_get_code(e) == LV_EVENT_RELEASED;
    settingsApp.brightnessChanged(lv_slider_get_value(s), released);
}

static void onSync(lv_event_t *)  { if (launcher.clickAllowed()) settingsApp.syncPressed(); }
static void onInfo(lv_event_t *)  { if (launcher.clickAllowed()) settingsApp.showInfo(true); }
static void onReset(lv_event_t *) { if (launcher.clickAllowed()) settingsApp.resetPressed(); }

// ---- Righe -------------------------------------------------

static void addSliderRow(lv_obj_t *list, const char *name)
{
    lv_obj_t *row = makeRow(list, 62);
    lv_obj_align(makeText(row, name, &lv_font_montserrat_16, 0xFFFFFF), LV_ALIGN_TOP_LEFT, 0, 0);
    brightSlider = lv_slider_create(row);
    lv_obj_set_size(brightSlider, 186, 10);
    lv_obj_align(brightSlider, LV_ALIGN_BOTTOM_MID, 0, -6);
    lv_slider_set_range(brightSlider, 20, 255);
    lv_obj_set_style_bg_color(brightSlider, lv_color_hex(COLOR_ON), LV_PART_INDICATOR);
    lv_obj_set_style_bg_color(brightSlider, lv_color_white(), LV_PART_KNOB);
    // trascinare il cursore verso destra non deve valere come "indietro"
    lv_obj_remove_flag(brightSlider, LV_OBJ_FLAG_GESTURE_BUBBLE);
    lv_obj_add_event_cb(brightSlider, onSlider, LV_EVENT_VALUE_CHANGED, nullptr);
    lv_obj_add_event_cb(brightSlider, onSlider, LV_EVENT_RELEASED, nullptr);
}

static lv_obj_t *makeArrow(lv_obj_t *row, const char *symbol, lv_align_t align, int x, int code)
{
    lv_obj_t *b = lv_button_create(row);
    lv_obj_set_size(b, 44, 30);
    lv_obj_align(b, align, x, 0);
    lv_obj_set_style_bg_color(b, lv_color_hex(COLOR_BTN), 0);
    lv_obj_set_style_radius(b, 10, 0);
    lv_obj_set_style_shadow_width(b, 0, 0);
    lv_obj_add_event_cb(b, onChoice, LV_EVENT_CLICKED, (void *)(intptr_t)code);
    lv_obj_center(makeText(b, symbol, &lv_font_montserrat_16, 0xFFFFFF));
    return b;
}

static void addChoiceRow(lv_obj_t *list, const char *name, Choice id)
{
    lv_obj_t *row = makeRow(list, 70);
    lv_obj_align(makeText(row, name, &lv_font_montserrat_16, 0xFFFFFF), LV_ALIGN_TOP_LEFT, 0, 0);
    makeArrow(row, LV_SYMBOL_LEFT, LV_ALIGN_BOTTOM_LEFT, 6, id * 2);
    makeArrow(row, LV_SYMBOL_RIGHT, LV_ALIGN_BOTTOM_RIGHT, -6, id * 2 + 1);
    lv_obj_t *v = makeText(row, "", &lv_font_montserrat_18, 0xFFFFFF);
    lv_obj_set_width(v, 100);
    lv_obj_set_style_text_align(v, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_align(v, LV_ALIGN_BOTTOM_MID, 0, -5);
    choiceValue[id] = v;
}

static void addToggleRow(lv_obj_t *list, const char *name, Toggle id)
{
    lv_obj_t *row = makeRow(list, 52);
    lv_obj_t *l = makeText(row, name, &lv_font_montserrat_16, 0xFFFFFF);
    lv_obj_set_width(l, 150);
    lv_label_set_long_mode(l, LV_LABEL_LONG_WRAP);
    lv_obj_align(l, LV_ALIGN_LEFT_MID, 0, 0);
    lv_obj_t *sw = lv_switch_create(row);
    lv_obj_set_size(sw, 48, 26);
    lv_obj_align(sw, LV_ALIGN_RIGHT_MID, 0, 0);
    lv_obj_set_style_bg_color(sw, lv_color_hex(COLOR_ON), LV_PART_INDICATOR | LV_STATE_CHECKED);
    lv_obj_add_event_cb(sw, onToggle, LV_EVENT_VALUE_CHANGED, (void *)(intptr_t)id);
    toggleSw[id] = sw;
}

// ---- Costruzione ---------------------------------------------

void SettingsApp::build(lv_obj_t *parent)
{
    // L'altezza dell'area va fatta calcolare a LVGL prima di leggerla
    lv_obj_update_layout(parent);
    int32_t h = lv_obj_get_content_height(parent);
    if (h <= 0) h = 170;

    // --- Lista ---
    listBox = makeScrollBox(parent, h);
    addSliderRow(listBox, "Luminosita'");
    addChoiceRow(listBox, "Spegnimento quadrante", CH_FACE);
    addChoiceRow(listBox, "Spegnimento menu e app", CH_MENU);
    addToggleRow(listBox, "Gesto del polso", TG_WRIST);
    addToggleRow(listBox, "Tocco per accendere", TG_TOUCH);
    addToggleRow(listBox, "Vibrazione pulsanti", TG_VIBRA);
    addChoiceRow(listBox, "Obiettivo passi", CH_GOAL);
    addToggleRow(listBox, "Passi sul quadrante", TG_STEPS_FACE);

    lv_obj_t *row = makeRow(listBox, 60, true);
    lv_obj_add_event_cb(row, onSync, LV_EVENT_CLICKED, nullptr);
    lv_obj_align(makeText(row, "Sincronizza ora adesso", &lv_font_montserrat_16, 0xFFFFFF), LV_ALIGN_TOP_LEFT, 0, 0);
    syncSub = makeText(row, "", &lv_font_montserrat_14, COLOR_GRAY);
    lv_obj_align(syncSub, LV_ALIGN_BOTTOM_LEFT, 0, 0);

    addToggleRow(listBox, "Ora automatica (Wi-Fi)", TG_AUTOSYNC);

    row = makeRow(listBox, 46, true);
    lv_obj_add_event_cb(row, onInfo, LV_EVENT_CLICKED, nullptr);
    lv_obj_align(makeText(row, "Info", &lv_font_montserrat_16, 0xFFFFFF), LV_ALIGN_LEFT_MID, 0, 0);
    lv_obj_align(makeText(row, LV_SYMBOL_RIGHT, &lv_font_montserrat_16, COLOR_GRAY), LV_ALIGN_RIGHT_MID, 0, 0);

    row = makeRow(listBox, 46, true);
    lv_obj_add_event_cb(row, onReset, LV_EVENT_CLICKED, nullptr);
    resetLabel = makeText(row, RESET_TEXT, &lv_font_montserrat_16, COLOR_RED);
    lv_obj_align(resetLabel, LV_ALIGN_LEFT_MID, 0, 0);

    // --- Pagina Info (copre la lista quando e' aperta) ---
    infoBox = makeScrollBox(parent, h);
    makeText(infoBox, "BATTERIA", &lv_font_montserrat_14, COLOR_GRAY);
    batteryText = makeText(infoBox, "", &lv_font_montserrat_16, 0xFFFFFF);
    lv_obj_set_width(batteryText, lv_pct(100));
    lv_label_set_long_mode(batteryText, LV_LABEL_LONG_WRAP);
    makeText(infoBox, "SISTEMA", &lv_font_montserrat_14, COLOR_GRAY);
    systemText = makeText(infoBox, "", &lv_font_montserrat_16, 0xFFFFFF);
    lv_obj_set_width(systemText, lv_pct(100));
    lv_label_set_long_mode(systemText, LV_LABEL_LONG_WRAP);
    lv_obj_add_flag(infoBox, LV_OBJ_FLAG_HIDDEN);

    refreshControls();
    refreshSync();
}

// ---- Aggiornamento dei controlli -----------------------------------

void SettingsApp::refreshControls()
{
    if (!listBox) return;
    lv_slider_set_value(brightSlider, userSettings.brightness, LV_ANIM_OFF);

    lv_label_set_text_fmt(choiceValue[CH_FACE], "%u s", userSettings.faceTimeoutS);
    lv_label_set_text_fmt(choiceValue[CH_MENU], "%u s", userSettings.menuTimeoutS);
    char buf[16];
    formatSteps(userSettings.stepsGoal, buf, sizeof(buf));
    lv_label_set_text(choiceValue[CH_GOAL], buf);

    const bool values[TG_COUNT] = {userSettings.wristWake, userSettings.touchWake, userSettings.buttonVibrate,
                                   userSettings.stepsOnFace, userSettings.autoSync};
    for (int i = 0; i < TG_COUNT; i++) {
        if (values[i]) lv_obj_add_state(toggleSw[i], LV_STATE_CHECKED);
        else lv_obj_remove_state(toggleSw[i], LV_STATE_CHECKED);
    }
}

void SettingsApp::refreshSync()
{
    if (!syncSub) return;
    char buf[64];
    if (!wifiTime.isConfigured()) {
        snprintf(buf, sizeof(buf), "Wi-Fi non configurato (secrets.h)");
    } else if (wifiTime.isSyncing()) {
        snprintf(buf, sizeof(buf), "Sincronizzazione in corso...");
    } else if (!wifiTime.hasSynced()) {
        snprintf(buf, sizeof(buf), "%s", wifiTime.lastAttemptFailed() ? "Non riuscita (rete?)" : "Mai sincronizzata");
    } else {
        static const char *MESI[] = {"gen", "feb", "mar", "apr", "mag", "giu",
                                     "lug", "ago", "set", "ott", "nov", "dic"};
        const struct tm &t = wifiTime.lastSyncTime();
        const struct tm now = timeService.now();
        char when[16];
        if (t.tm_year == now.tm_year && t.tm_yday == now.tm_yday) snprintf(when, sizeof(when), "oggi");
        else snprintf(when, sizeof(when), "%d %s", t.tm_mday, MESI[(t.tm_mon >= 0 && t.tm_mon < 12) ? t.tm_mon : 0]);
        snprintf(buf, sizeof(buf), "%s: %s %02d:%02d", wifiTime.lastAttemptFailed() ? "Fallita. Ultima ok" : "Ultima",
                 when, t.tm_hour, t.tm_min);
    }
    lv_label_set_text(syncSub, buf);
}

void SettingsApp::refreshInfo()
{
    lastInfo = millis();
    if (!batteryText) return;
    char buf[320], a[24], b[24];

    // --- Batteria ---
    int pct = (int)(instance.getBatteryPercent() + 0.5f);
    if (pct < 0) pct = 0;
    if (pct > 100) pct = 100;
    float volt = instance.getBattVoltage();
    if (volt > 100) volt /= 1000.0f;  // alcune versioni restituiscono mV
    snprintf(a, sizeof(a), "%.2f", volt);
    for (char *c = a; *c; c++) if (*c == '.') *c = ',';

    const char *status = instance.isCharging() ? "in carica" : (instance.isUsbIn() ? "cavo collegato" : "in uso");
    const Power::Usage u = power.usage();

    int n = snprintf(buf, sizeof(buf), "Livello: %d%% (%s V)\nStato: %s\n", pct, a, status);
    if (u.onUsb) {
        n += snprintf(buf + n, sizeof(buf) - n, "Consumo: -- (stacca il cavo)\n");
    } else if (!u.valid) {
        n += snprintf(buf + n, sizeof(buf) - n, "Consumo: in calcolo...\nDurata stimata: --\n");
    } else {
        formatDecimal(u.pctPerHour, b, sizeof(b));
        n += snprintf(buf + n, sizeof(buf) - n, "Consumo: %s%% all'ora\n", b);
        const uint32_t minutesLeft = (uint32_t)(pct / u.pctPerHour * 60.0f);
        formatDuration(minutesLeft, b, sizeof(b));
        n += snprintf(buf + n, sizeof(buf) - n, "Durata stimata: circa %s\n", b);
    }
    formatDuration(u.sinceMs / 60000UL, b, sizeof(b));
    n += snprintf(buf + n, sizeof(buf) - n, "%s: %s\nIn sospensione: %d%% del tempo",
                  u.onUsb ? "Misura da" : "Senza cavo da", b, (int)(u.sleepPct + 0.5f));
    lv_label_set_text(batteryText, buf);

    // --- Sistema ---
    formatDuration(millis() / 60000UL, b, sizeof(b));
    snprintf(buf, sizeof(buf), "Versione: %s (%s)\nMemoria libera: %lu KB\nAcceso da: %s",
             cfg::FIRMWARE_VERSION, __DATE__, (unsigned long)(ESP.getFreeHeap() / 1024), b);
    lv_label_set_text(systemText, buf);
}

// ---- Azioni ------------------------------------------------

static int optionIndex(const uint8_t *opts, int count, uint8_t value)
{
    for (int i = 0; i < count; i++) if (opts[i] >= value) return i;
    return count - 1;
}

void SettingsApp::changeChoice(int id, int dir)
{
    if (id == CH_FACE || id == CH_MENU) {
        const uint8_t *opts = (id == CH_FACE) ? FACE_OPTS : MENU_OPTS;
        const int count = 4;
        uint8_t &value = (id == CH_FACE) ? userSettings.faceTimeoutS : userSettings.menuTimeoutS;
        int i = optionIndex(opts, count, value);
        if (opts[i] == value) i += dir;           // valore della lista: passa al vicino
        else if (dir < 0) i -= 1;                 // valore "strano" (da config.h): il piu' vicino
        if (i < 0) i = 0;
        if (i >= count) i = count - 1;
        value = opts[i];
        if (id == CH_MENU) screen.setTimeout(userSettings.menuTimeoutMs());  // vale subito
    } else if (id == CH_GOAL) {
        int32_t g = (int32_t)userSettings.stepsGoal + dir * (int32_t)GOAL_STEP;
        if (g < (int32_t)GOAL_MIN) g = GOAL_MIN;
        if (g > (int32_t)GOAL_MAX) g = GOAL_MAX;
        userSettings.stepsGoal = (uint32_t)g;
    }
    userSettings.save();
    refreshControls();
}

void SettingsApp::toggle(int id, bool on)
{
    switch (id) {
    case TG_WRIST:      userSettings.wristWake = on; break;
    case TG_TOUCH:      userSettings.touchWake = on; break;
    case TG_VIBRA:      userSettings.buttonVibrate = on; break;
    case TG_STEPS_FACE: userSettings.stepsOnFace = on; break;
    case TG_AUTOSYNC:   userSettings.autoSync = on; wifiTime.setAutoSync(on); break;
    }
    userSettings.save();
    App::log(name(), "Impostazione %d: %s", id, on ? "si" : "no");
}

void SettingsApp::brightnessChanged(int value, bool released)
{
    userSettings.brightness = (uint8_t)value;
    if (screen.isOn()) instance.setBrightness(userSettings.brightness);  // si vede subito
    if (released) {
        userSettings.save();
        App::log(name(), "Luminosita' %d", value);
    }
}

void SettingsApp::syncPressed()
{
    if (wifiTime.isConfigured()) wifiTime.syncNow();
    refreshSync();
}

void SettingsApp::showInfo(bool show)
{
    infoOpen = show;
    if (!infoBox) return;
    if (show) {
        refreshInfo();
        lv_obj_scroll_to_y(infoBox, 0, LV_ANIM_OFF);
        lv_obj_remove_flag(infoBox, LV_OBJ_FLAG_HIDDEN);
    } else {
        lv_obj_add_flag(infoBox, LV_OBJ_FLAG_HIDDEN);
    }
}

void SettingsApp::resetPressed()
{
    if (resetArmedAt == 0) {
        // primo tocco: chiedi conferma
        resetArmedAt = millis();
        if (resetArmedAt == 0) resetArmedAt = 1;
        lv_label_set_text(resetLabel, "Tocca ancora per confermare");
        return;
    }
    // secondo tocco: ripristina
    resetArmedAt = 0;
    userSettings.resetToDefaults();
    if (screen.isOn()) instance.setBrightness(userSettings.brightness);
    wifiTime.setAutoSync(userSettings.autoSync);
    screen.setTimeout(userSettings.menuTimeoutMs());
    lv_label_set_text(resetLabel, RESET_TEXT);
    refreshControls();
}

// ---- Ciclo di vita -------------------------------------------

bool SettingsApp::onBack()
{
    if (!infoOpen) return false;  // dalla lista: torna al menu
    showInfo(false);              // dalla pagina Info: torna alla lista
    return true;
}

void SettingsApp::onOpen()
{
    refreshControls();
    refreshSync();
}

void SettingsApp::onClose()
{
    showInfo(false);
    resetArmedAt = 0;
    if (resetLabel) lv_label_set_text(resetLabel, RESET_TEXT);
    if (listBox) lv_obj_scroll_to_y(listBox, 0, LV_ANIM_OFF);
}

void SettingsApp::update()
{
    if (!isOpen() || !screen.isOn()) return;
    const uint32_t now = millis();

    // Conferma di "Ripristina" scaduta dopo 3 secondi
    if (resetArmedAt != 0 && now - resetArmedAt > 3000) {
        resetArmedAt = 0;
        lv_label_set_text(resetLabel, RESET_TEXT);
    }
    if (now - lastRefresh >= 1000) {
        lastRefresh = now;
        refreshSync();
    }
    if (infoOpen && now - lastInfo >= 2000) refreshInfo();
}

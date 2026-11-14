#include <LilyGoLib.h>
#include <Preferences.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <time.h>
#include "steps_app.h"
#include "time_service.h"
#include "screen.h"
#include "app.h"
#include "config.h"
#include "user_settings.h"
#include "theme.h"

StepsApp stepsApp;

static lv_obj_t *ring = nullptr;
static lv_obj_t *countLabel = nullptr;
static lv_obj_t *goalLabel = nullptr;
static lv_obj_t *infoLabel = nullptr;
static lv_obj_t *yesterdayLabel = nullptr;


void formatSteps(uint32_t n, char *out, size_t size)
{
    char digits[12];
    snprintf(digits, sizeof(digits), "%lu", (unsigned long)n);
    const size_t len = strlen(digits);
    size_t j = 0;
    for (size_t i = 0; i < len && j + 2 < size; i++) {
        if (i > 0 && (len - i) % 3 == 0) out[j++] = '.';
        out[j++] = digits[i];
    }
    out[j] = '\0';
}

// Data come numero AAAAMMGG (es. 20261106)
static uint32_t dayKeyOf(const struct tm &t)
{
    return (uint32_t)(t.tm_year + 1900) * 10000 + (uint32_t)(t.tm_mon + 1) * 100 + (uint32_t)t.tm_mday;
}

// Il giorno dopo "day" (AAAAMMGG)
static uint32_t nextDayKey(uint32_t day)
{
    struct tm t = {};
    t.tm_year = (int)(day / 10000) - 1900;
    t.tm_mon = (int)((day / 100) % 100) - 1;
    t.tm_mday = (int)(day % 100) + 1;
    t.tm_hour = 12;
    t.tm_isdst = -1;
    mktime(&t);  // sistema fine mese e fine anno
    return dayKeyOf(t);
}

// Millisecondi che mancano alla prossima mezzanotte
static uint32_t msToMidnight(const struct tm &t)
{
    const uint32_t s = (uint32_t)(23 - t.tm_hour) * 3600 + (uint32_t)(59 - t.tm_min) * 60 +
                       (uint32_t)(60 - t.tm_sec);
    return s * 1000UL + 500;  // mezzo secondo di margine
}

void StepsApp::begin()
{
    // LilyGoLib fa scattare un interrupt a ogni passo, che sveglia il
    // processore inutilmente: il sensore conta da solo, a noi basta
    // leggere il totale quando serve.
    bool ok = instance.bma423.enableInterrupt(InterruptPinMap::PIN1, BMA423_STEP_CNTR_INT, false);
    if (!ok) App::log(name(), "Impossibile disattivare l'interrupt dei passi");

    // Recupera i passi salvati
    Preferences p;
    p.begin("steps", false);
    const uint32_t savedDay = p.getUInt("day", 0);
    const uint32_t savedToday = p.getUInt("today", 0);
    const uint32_t savedYesterday = p.getUInt("yday", 0);
    const bool savedGoal = p.getBool("goal", false);
    p.end();

    const struct tm now = timeService.now();
    currentDay = dayKeyOf(now);
    if (savedDay == currentDay) {
        // riavvio nello stesso giorno: si continua da dove eravamo
        offset = savedToday;
        yesterdaySteps = savedYesterday;
        goalDone = savedGoal;
    } else if (savedDay != 0 && nextDayKey(savedDay) == currentDay) {
        yesterdaySteps = savedToday;  // i passi salvati erano di ieri
    }

    // All'avvio LilyGoLib azzera il contatore del sensore
    sensorSteps = 0;
    readSensor();
    loaded = true;
    lastCheck = millis();
    midnightAt = millis() + msToMidnight(now);
    if (savedDay != currentDay) save();
    else { savedSteps = today(); lastSave = millis(); }

    App::log(name(), "Passi oggi: %lu, ieri: %lu, obiettivo %lu",
             (unsigned long)today(), (unsigned long)yesterdaySteps, (unsigned long)userSettings.stepsGoal);
}

void StepsApp::readSensor()
{
    const uint32_t s = instance.bma423.getStepCount();
    // Il contatore puo' solo crescere (lo azzeriamo noi a mezzanotte):
    // un valore piu' basso e' un errore di lettura
    if (s >= sensorSteps) sensorSteps = s;
}

void StepsApp::check()
{
    lastCheck = millis();
    readSensor();

    const struct tm now = timeService.now();
    const uint32_t day = dayKeyOf(now);
    if (day > currentDay) {
        newDay(day);
    } else if (day < currentDay) {
        // ora spostata indietro (es. regolata a mano): si tengono i passi
        currentDay = day;
        save();
    }
    midnightAt = millis() + msToMidnight(now);

    // Obiettivo alzato nelle impostazioni: non e' piu' raggiunto
    if (goalDone && today() < userSettings.stepsGoal) goalDone = false;
    if (!goalDone && today() >= userSettings.stepsGoal) {
        goalDone = true;
        App::log(name(), "Obiettivo raggiunto: %lu passi!", (unsigned long)today());
        save();
        if (cfg::STEPS_GOAL_VIBRATE) celebrate();
        if (cfg::STEPS_GOAL_WAKE_SCREEN) screen.wake("obiettivo passi");
    }

    if (today() != savedSteps && millis() - lastSave >= cfg::STEPS_SAVE_MIN * 60000UL) save();

    if (isOpen()) refreshUi();
}

void StepsApp::newDay(uint32_t day)
{
    const uint32_t total = today();
    yesterdaySteps = (nextDayKey(currentDay) == day) ? total : 0;
    instance.resetStepCounter();
    sensorSteps = 0;
    offset = 0;
    goalDone = false;
    currentDay = day;
    save();
    App::log(name(), "Nuovo giorno: passi azzerati (ieri %lu)", (unsigned long)yesterdaySteps);
}

void StepsApp::save()
{
    Preferences p;
    p.begin("steps", false);
    p.putUInt("day", currentDay);
    p.putUInt("today", today());
    p.putUInt("yday", yesterdaySteps);
    p.putBool("goal", goalDone);
    p.end();
    savedSteps = today();
    lastSave = millis();
}

void StepsApp::update()
{
    if (!loaded) return;
    const uint32_t now = millis();
    // A schermo acceso ogni secondo, a schermo spento ogni STEPS_CHECK_MIN minuti
    const uint32_t interval = screen.isOn() ? 1000 : cfg::STEPS_CHECK_MIN * 60000UL;
    if (now - lastCheck >= interval || (int32_t)(now - midnightAt) >= 0) check();
}

uint32_t StepsApp::msUntilNextWork()
{
    if (!loaded) return UINT32_MAX;
    const uint32_t now = millis();
    const uint32_t interval = cfg::STEPS_CHECK_MIN * 60000UL;
    const uint32_t sinceCheck = now - lastCheck;
    uint32_t wait = sinceCheck >= interval ? 0 : interval - sinceCheck;
    const int32_t toMidnight = (int32_t)(midnightAt - now);
    if (toMidnight <= 0) return 0;
    if ((uint32_t)toMidnight < wait) wait = (uint32_t)toMidnight;
    return wait;
}

void StepsApp::onScreenOn()
{
    if (loaded) check();  // valore aggiornato appena si accende
}

void StepsApp::onOpen()
{
    if (loaded) check();
    refreshUi();
}

void StepsApp::build(lv_obj_t *parent)
{
    // Anello dell'obiettivo
    ring = lv_arc_create(parent);
    lv_obj_set_size(ring, 120, 120);
    lv_obj_align(ring, LV_ALIGN_TOP_MID, 0, 0);
    lv_arc_set_rotation(ring, 270);       // parte dall'alto
    lv_arc_set_bg_angles(ring, 0, 360);
    lv_arc_set_range(ring, 0, 1000);      // in millesimi dell'obiettivo
    lv_arc_set_value(ring, 0);
    lv_obj_remove_style(ring, NULL, LV_PART_KNOB);
    lv_obj_remove_flag(ring, LV_OBJ_FLAG_CLICKABLE);
    pk::styleArc(ring, 12);

    // Passi al centro, obiettivo sotto
    countLabel = lv_label_create(parent);
    pk::styleLabel(countLabel, &pk_digits_s);

    goalLabel = lv_label_create(parent);
    pk::styleLabel(goalLabel, &pk_font_small, pk::DIM);

    // Km e calorie, poi i passi di ieri
    infoLabel = lv_label_create(parent);
    pk::styleLabel(infoLabel, &pk_font_text);

    yesterdayLabel = lv_label_create(parent);
    pk::styleLabel(yesterdayLabel, &pk_font_small, pk::DIM);

    refreshUi();
}

void StepsApp::refreshUi()
{
    if (!ring) return;
    const uint32_t steps = today();
    char buf[16];

    uint32_t permille = (uint32_t)((uint64_t)steps * 1000 / userSettings.stepsGoal);
    if (permille > 1000) permille = 1000;
    lv_arc_set_value(ring, (int32_t)permille);

    formatSteps(steps, buf, sizeof(buf));
    lv_label_set_text(countLabel, buf);
    // Obiettivo raggiunto: numero in negativo (chiaro su fondo scuro)
    lv_obj_set_style_text_color(countLabel, pk::color(goalDone ? pk::BG : pk::INK), 0);
    lv_obj_set_style_bg_color(countLabel, pk::color(pk::INK), 0);
    lv_obj_set_style_bg_opa(countLabel, goalDone ? LV_OPA_COVER : LV_OPA_TRANSP, 0);
    lv_obj_set_style_pad_hor(countLabel, 3, 0);
    lv_obj_set_style_pad_ver(countLabel, 2, 0);
    lv_obj_align_to(countLabel, ring, LV_ALIGN_CENTER, 0, -8);

    formatSteps(userSettings.stepsGoal, buf, sizeof(buf));
    lv_label_set_text_fmt(goalLabel, "/ %s", buf);
    lv_obj_align_to(goalLabel, ring, LV_ALIGN_CENTER, 0, 17);

    // Stime: km in decimi (con la virgola) e calorie
    const uint32_t kmTenths = (uint32_t)((uint64_t)steps * cfg::STEP_LENGTH_CM / 10000);
    const uint32_t kcal = (uint32_t)((uint64_t)steps * cfg::KCAL_PER_1000_STEPS / 1000);
    lv_label_set_text_fmt(infoLabel, "%lu,%lu km    %lu kcal", (unsigned long)(kmTenths / 10),
                          (unsigned long)(kmTenths % 10), (unsigned long)kcal);
    lv_obj_align_to(infoLabel, ring, LV_ALIGN_OUT_BOTTOM_MID, 0, 5);

    formatSteps(yesterdaySteps, buf, sizeof(buf));
    lv_label_set_text_fmt(yesterdayLabel, "Ieri: %s", buf);
    lv_obj_align_to(yesterdayLabel, infoLabel, LV_ALIGN_OUT_BOTTOM_MID, 0, 2);
}

void StepsApp::celebrate()
{
    // Tre vibrazioni forti con una breve pausa in mezzo.
    // Effetto 14 = "Strong Buzz 100%"; 0x80 | n = pausa di n x 10 ms.
    instance.drv.setWaveform(0, 14);
    instance.drv.setWaveform(1, 0x80 | 15);
    instance.drv.setWaveform(2, 14);
    instance.drv.setWaveform(3, 0x80 | 15);
    instance.drv.setWaveform(4, 14);
    instance.drv.setWaveform(5, 0);  // fine sequenza
    instance.drv.run();
}

bool StepsApp::onSerialCommand(const char *line)
{
    if (strcasecmp(line, "passi") == 0) {
        if (loaded) check();
        App::log(name(), "Oggi %lu passi (sensore %lu + salvati %lu), ieri %lu, obiettivo %lu %s",
                 (unsigned long)today(), (unsigned long)sensorSteps, (unsigned long)offset,
                 (unsigned long)yesterdaySteps, (unsigned long)userSettings.stepsGoal,
                 goalDone ? "(raggiunto)" : "");
        return true;
    }
    if (strcasecmp(line, "passi vibra") == 0) {
        App::log(name(), "Prova della vibrazione dell'obiettivo");
        celebrate();
        return true;
    }
    return false;
}

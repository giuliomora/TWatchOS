#include <LilyGoLib.h>
#include <LV_Helper.h>
#include "watchface.h"
#include "time_service.h"
#include "screen.h"
#include "steps_app.h"
#include "stopwatch_app.h"
#include "countdown_timer.h"
#include "config.h"
#include "user_settings.h"
#include "theme.h"

Watchface watchface;

static lv_obj_t *timeLabel = nullptr;
static lv_obj_t *dateLabel = nullptr;
static lv_obj_t *stepsLabel = nullptr;
static lv_obj_t *stopwatchLabel = nullptr;
static lv_obj_t *timerLabel = nullptr;
static lv_point_precise_t groundPts[2][2];  // le due righe in basso, come sul Pokekron

static const char *GIORNI[] = {"Domenica", "Lunedi'", "Martedi'", "Mercoledi'",
                               "Giovedi'", "Venerdi'", "Sabato"};
static const char *MESI[] = {"gen", "feb", "mar", "apr", "mag", "giu",
                             "lug", "ago", "set", "ott", "nov", "dic"};

void Watchface::begin()
{
    lv_obj_t *scr = lv_screen_active();
    pk::styleScreen(scr);
    // Mentre lo tocchi lo schermo si schiarisce, come la retroilluminazione del Pokekron
    lv_obj_set_style_bg_color(scr, pk::color(pk::LIT), LV_STATE_PRESSED);

    // Righe decorative in basso
    pk::hline(scr, groundPts[0], 2, 238, 225, 2, pk::SHADE);
    pk::hline(scr, groundPts[1], 2, 238, 231, 2, pk::SHADE);

    // Figura in pixel art in basso a sinistra, appoggiata sulle righe,
    // nel tono "sbiadito" come sul Pokekron. E' un carattere del font
    // pk_sprite (pochi byte, niente immagini): per cambiarla vedi
    // tools/fontgen/glyphs.py (SPRITE).
    lv_obj_t *sprite = lv_label_create(scr);
    pk::styleLabel(sprite, &pk_sprite, pk::MID);
    lv_label_set_text(sprite, "A");
    lv_obj_align(sprite, LV_ALIGN_BOTTOM_LEFT, 8, -16);

    // Ora, grande e al centro, con le cifre a blocchi
    timeLabel = lv_label_create(scr);
    pk::styleLabel(timeLabel, &pk_digits_l);
    lv_label_set_text(timeLabel, "--:--");
    lv_obj_align(timeLabel, LV_ALIGN_CENTER, 0, -28);  // piu' in alto: sotto c'e' la figura

    // Data, sotto l'ora
    dateLabel = lv_label_create(scr);
    pk::styleLabel(dateLabel, &pk_font_text);
    lv_label_set_text(dateLabel, "");
    lv_obj_align_to(dateLabel, timeLabel, LV_ALIGN_OUT_BOTTOM_MID, 0, 10);

    // Passi di oggi, sotto la data (piu' scuri a obiettivo raggiunto).
    // Si possono nascondere dalle impostazioni.
    stepsLabel = lv_label_create(scr);
    pk::styleLabel(stepsLabel, &pk_font_text, pk::DIM);
    lv_label_set_text(stepsLabel, "");

    // Cronometro in alto a sinistra (solo se avviato o in pausa)
    if (cfg::STOPWATCH_ON_WATCHFACE) {
        stopwatchLabel = lv_label_create(scr);
        pk::styleLabel(stopwatchLabel, &pk_font_text);
        lv_label_set_text(stopwatchLabel, "");
        lv_obj_align(stopwatchLabel, LV_ALIGN_TOP_LEFT, 8, 8);
        lv_obj_add_flag(stopwatchLabel, LV_OBJ_FLAG_HIDDEN);
    }

    // Timer in alto a sinistra (sotto il cronometro, se c'e' anche quello)
    if (cfg::TIMER_ON_WATCHFACE) {
        timerLabel = lv_label_create(scr);
        pk::styleLabel(timerLabel, &pk_font_text);
        lv_label_set_text(timerLabel, "");
        lv_obj_add_flag(timerLabel, LV_OBJ_FLAG_HIDDEN);
    }

    refresh();
}

void Watchface::onScreenOn()
{
    refresh();  // mostra subito l'ora giusta appena si accende
}

void Watchface::update()
{
    // Aggiorna due volte al secondo, solo se lo schermo e' acceso
    if (!screen.isOn()) return;
    if (millis() - lastRefresh < 500) return;
    refresh();
}

void Watchface::refresh()
{
    if (!timeLabel) return;  // quadrante non ancora creato
    lastRefresh = millis();
    struct tm t = timeService.now();

    lv_label_set_text_fmt(timeLabel, "%02d:%02d", t.tm_hour, t.tm_min);

    int wd = (t.tm_wday >= 0 && t.tm_wday < 7) ? t.tm_wday : 0;
    int mo = (t.tm_mon >= 0 && t.tm_mon < 12) ? t.tm_mon : 0;
    lv_label_set_text_fmt(dateLabel, "%s %d %s", GIORNI[wd], t.tm_mday, MESI[mo]);
    lv_obj_align_to(dateLabel, timeLabel, LV_ALIGN_OUT_BOTTOM_MID, 0, 10);

    if (stepsLabel && !userSettings.stepsOnFace) {
        lv_obj_add_flag(stepsLabel, LV_OBJ_FLAG_HIDDEN);
    } else if (stepsLabel) {
        lv_obj_remove_flag(stepsLabel, LV_OBJ_FLAG_HIDDEN);
        char buf[16];
        formatSteps(stepsApp.today(), buf, sizeof(buf));
        lv_label_set_text_fmt(stepsLabel, "%s passi", buf);
        lv_obj_set_style_text_color(stepsLabel,
            pk::color(stepsApp.goalReached() ? pk::INK : pk::DIM), 0);
        lv_obj_align_to(stepsLabel, dateLabel, LV_ALIGN_OUT_BOTTOM_MID, 0, 4);
    }

    if (stopwatchLabel) {
        if (stopwatchApp.isActive()) {
            char buf[16];
            StopwatchApp::formatShort(stopwatchApp.elapsedUs(), buf, sizeof(buf));
            const bool run = stopwatchApp.isRunning();
            lv_label_set_text_fmt(stopwatchLabel, "%s %s", run ? LV_SYMBOL_PLAY : LV_SYMBOL_PAUSE, buf);
            lv_obj_set_style_text_color(stopwatchLabel, pk::color(run ? pk::INK : pk::DIM), 0);
            lv_obj_remove_flag(stopwatchLabel, LV_OBJ_FLAG_HIDDEN);
        } else {
            lv_obj_add_flag(stopwatchLabel, LV_OBJ_FLAG_HIDDEN);
        }
    }

    if (timerLabel) {
        if (countdownTimer.isActive()) {
            char buf[16];
            CountdownTimer::formatRemaining(countdownTimer.remainingUs(), buf, sizeof(buf));
            const bool run = countdownTimer.isRunning();
            lv_label_set_text_fmt(timerLabel, "%s %s", run ? LV_SYMBOL_DOWN : LV_SYMBOL_PAUSE, buf);
            lv_obj_set_style_text_color(timerLabel, pk::color(run ? pk::INK : pk::DIM), 0);
            const bool below = stopwatchLabel && stopwatchApp.isActive();
            lv_obj_align(timerLabel, LV_ALIGN_TOP_LEFT, 8, below ? 28 : 8);
            lv_obj_remove_flag(timerLabel, LV_OBJ_FLAG_HIDDEN);
        } else {
            lv_obj_add_flag(timerLabel, LV_OBJ_FLAG_HIDDEN);
        }
    }
}

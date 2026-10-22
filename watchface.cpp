#include <LilyGoLib.h>
#include <LV_Helper.h>
#include "watchface.h"
#include "time_service.h"
#include "screen.h"

Watchface watchface;

static lv_obj_t *timeLabel = nullptr;
static lv_obj_t *dateLabel = nullptr;

static const char *GIORNI[] = {"Domenica", "Lunedi'", "Martedi'", "Mercoledi'",
                               "Giovedi'", "Venerdi'", "Sabato"};
static const char *MESI[] = {"gen", "feb", "mar", "apr", "mag", "giu",
                             "lug", "ago", "set", "ott", "nov", "dic"};

void Watchface::begin()
{
    lv_obj_t *scr = lv_screen_active();
    lv_obj_set_style_bg_color(scr, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, 0);
    lv_obj_remove_flag(scr, LV_OBJ_FLAG_SCROLLABLE);

    // Ora, grande e al centro
    timeLabel = lv_label_create(scr);
    lv_obj_set_style_text_font(timeLabel, &lv_font_montserrat_48, 0);
    lv_obj_set_style_text_color(timeLabel, lv_color_white(), 0);
    lv_label_set_text(timeLabel, "--:--");
    lv_obj_align(timeLabel, LV_ALIGN_CENTER, 0, -15);

    // Data, sotto l'ora
    dateLabel = lv_label_create(scr);
    lv_obj_set_style_text_font(dateLabel, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(dateLabel, lv_color_hex(0x9AA0A6), 0);
    lv_label_set_text(dateLabel, "");
    lv_obj_align_to(dateLabel, timeLabel, LV_ALIGN_OUT_BOTTOM_MID, 0, 8);

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
    lv_obj_align_to(dateLabel, timeLabel, LV_ALIGN_OUT_BOTTOM_MID, 0, 8);
}

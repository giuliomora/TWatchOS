#include <LilyGoLib.h>
#include <LV_Helper.h>
#include "status_bar.h"
#include "screen.h"
#include "config.h"
#include "theme.h"

StatusBar statusBar;

static lv_obj_t *batteryLabel = nullptr;


// Icona della batteria in base alla percentuale
static const char *batteryIcon(int pct)
{
    if (pct >= 90) return LV_SYMBOL_BATTERY_FULL;
    if (pct >= 65) return LV_SYMBOL_BATTERY_3;
    if (pct >= 40) return LV_SYMBOL_BATTERY_2;
    if (pct >= 15) return LV_SYMBOL_BATTERY_1;
    return LV_SYMBOL_BATTERY_EMPTY;
}

void StatusBar::begin()
{
    // Sul livello "sopra" a tutte le schermate: si vede ovunque
    batteryLabel = lv_label_create(lv_layer_top());
    pk::styleLabel(batteryLabel, &pk_font_small);
    lv_obj_set_style_pad_hor(batteryLabel, 3, 0);
    lv_obj_set_style_pad_ver(batteryLabel, 1, 0);
    lv_label_set_text(batteryLabel, "");
    lv_obj_align(batteryLabel, LV_ALIGN_TOP_RIGHT, -6, 6);
    refresh();
}

void StatusBar::onScreenOn()
{
    refresh();  // valore aggiornato appena si accende
}

void StatusBar::update()
{
    if (!screen.isOn()) return;
    if (millis() - lastRefresh < cfg::STATUS_BAR_REFRESH_MS) return;
    refresh();
}

void StatusBar::refresh()
{
    if (!batteryLabel) return;
    lastRefresh = millis();

    int pct = (int)(instance.getBatteryPercent() + 0.5f);
    if (pct < 0) pct = 0;
    if (pct > 100) pct = 100;
    const bool charging = instance.isCharging();

    // Batteria scarica: scritta chiara su fondo scuro (in negativo), per farsi notare
    const bool low = !charging && pct < cfg::BATTERY_LOW_PCT;

    if (charging) {
        lv_label_set_text_fmt(batteryLabel, "%d%% " LV_SYMBOL_CHARGE " %s", pct, batteryIcon(pct));
    } else {
        lv_label_set_text_fmt(batteryLabel, "%d%% %s", pct, batteryIcon(pct));
    }
    lv_obj_set_style_text_color(batteryLabel, pk::color(low ? pk::BG : pk::INK), 0);
    lv_obj_set_style_bg_color(batteryLabel, pk::color(pk::INK), 0);
    lv_obj_set_style_bg_opa(batteryLabel, low ? LV_OPA_COVER : LV_OPA_TRANSP, 0);
    lv_obj_align(batteryLabel, LV_ALIGN_TOP_RIGHT, -6, 6);
}

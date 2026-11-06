#include "empty_app.h"

// Icone provvisorie: LVGL non ha icone di passi o cronometro,
// in futuro si potranno disegnare icone personalizzate.
EmptyApp batteryApp("Batteria", LV_SYMBOL_BATTERY_FULL, 0x1565C0);
EmptyApp settingsApp("Impostazioni", LV_SYMBOL_SETTINGS, 0x546E7A);

void EmptyApp::build(lv_obj_t *parent)
{
    lv_obj_t *label = lv_label_create(parent);
    lv_obj_set_style_text_font(label, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(label, lv_color_hex(0x9AA0A6), 0);
    lv_label_set_text(label, "In arrivo");
    lv_obj_center(label);
}

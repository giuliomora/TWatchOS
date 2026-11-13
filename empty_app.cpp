#include "empty_app.h"

// Per usarla: crea un'istanza, es.
//   EmptyApp provaApp("Prova", LV_SYMBOL_HOME, 0x1565C0);
// e registrala nel .ino con launcher.addApp(&provaApp);

void EmptyApp::build(lv_obj_t *parent)
{
    lv_obj_t *label = lv_label_create(parent);
    lv_obj_set_style_text_font(label, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(label, lv_color_hex(0x9AA0A6), 0);
    lv_label_set_text(label, "In arrivo");
    lv_obj_center(label);
}

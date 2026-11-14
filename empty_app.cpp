#include "empty_app.h"
#include "theme.h"

// Per usarla: crea un'istanza, es.
//   EmptyApp provaApp("Prova", LV_SYMBOL_HOME, 0x1565C0);
// e registrala nel .ino con launcher.addApp(&provaApp);

void EmptyApp::build(lv_obj_t *parent)
{
    lv_obj_t *label = lv_label_create(parent);
    pk::styleLabel(label, &pk_font_text, pk::DIM);
    lv_label_set_text(label, "In arrivo");
    lv_obj_center(label);
}

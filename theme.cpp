#include "theme.h"

namespace pk {

// =============================================================
//  Palette (come il "Cambia colore" del Pokekron)
//
//  Verde: dal Pokekron di Platino. Le altre: sfondo e inchiostro
//  presi dalle catture del gioco; i toni intermedi (LIT, SHADE,
//  MID, DIM, EDGE) sono ricavati con le stesse proporzioni del verde.
//  Ordine dei toni: BG, LIT, SHADE, MID, DIM, INK, EDGE.
// =============================================================
enum { T_BG, T_LIT, T_SHADE, T_MID, T_DIM, T_INK, T_EDGE, TONES };

struct Palette { const char *name; uint32_t tone[TONES]; };

static const Palette PALETTES[PALETTE_COUNT] = {
    {"Verde",     {0x70B070, 0x80C080, 0x60A060, 0x508850, 0x46683E, 0x385030, 0x082C10}},
    {"Giallo",    {0xC3BF6F, 0xD9D47E, 0xADAA60, 0x918E4D, 0x716E37, 0x565424, 0x212000}},
    {"Arancione", {0xC19E5C, 0xD6B069, 0xAC8C4F, 0x91753E, 0x725A2B, 0x58441B, 0x251800}},
    {"Rosso",     {0xC66D6C, 0xDB7A79, 0xB1605F, 0x964E4D, 0x773B3A, 0x5D2A29, 0x2A0908}},
    {"Viola",     {0x8F79BB, 0x9F88CF, 0x7F6AA7, 0x6A588D, 0x534270, 0x3F3057, 0x180C26}},
    {"Blu",       {0x748DB2, 0x829DC6, 0x667D9E, 0x546885, 0x3F5068, 0x2D3C50, 0x0A1520}},
    {"Azzurro",   {0x80BDC2, 0x91D2D7, 0x6FA8AD, 0x5A8D91, 0x426E71, 0x2D5456, 0x052121}},
    {"Grigio",    {0xABABAB, 0xBFBFBF, 0x979797, 0x7E7E7E, 0x616161, 0x494949, 0x191919}},
};

// Toni della palette attiva (all'avvio: Verde)
uint32_t BG    = PALETTES[0].tone[T_BG];
uint32_t LIT   = PALETTES[0].tone[T_LIT];
uint32_t SHADE = PALETTES[0].tone[T_SHADE];
uint32_t MID   = PALETTES[0].tone[T_MID];
uint32_t DIM   = PALETTES[0].tone[T_DIM];
uint32_t INK   = PALETTES[0].tone[T_INK];
uint32_t EDGE  = PALETTES[0].tone[T_EDGE];

static int current = 0;

const char *paletteName(int index)
{
    if (index < 0 || index >= PALETTE_COUNT) return "?";
    return PALETTES[index].name;
}

int palette() { return current; }

uint32_t paletteBg(int index)
{
    if (index < 0 || index >= PALETTE_COUNT) index = 0;
    return PALETTES[index].tone[T_BG];
}

bool setPalette(int index)
{
    if (index < 0 || index >= PALETTE_COUNT || index == current) return false;
    current = index;
    const uint32_t *t = PALETTES[index].tone;
    BG = t[T_BG]; LIT = t[T_LIT]; SHADE = t[T_SHADE]; MID = t[T_MID];
    DIM = t[T_DIM]; INK = t[T_INK]; EDGE = t[T_EDGE];
    return true;
}

// =============================================================
//  Ricolorazione dal vivo
//
//  Gli oggetti LVGL si "copiano" il colore quando vengono creati.
//  Qui si percorrono tutti e, per ogni proprieta' di colore impostata
//  direttamente sull'oggetto, si sostituisce il tono della vecchia
//  palette con lo stesso tono della nuova. Funziona perche' il tema
//  usa solo i 7 toni esatti (font a 1 bit, niente sfumature).
//  Costo: solo al cambio colore, una frazione di secondo.
//
//  ALTERNATIVA PIU' LEGGERA (se un giorno servisse spazio):
//  togliere recolor() e Launcher::applyPalette() e, quando cambia
//  il colore, salvare l'impostazione e fare ESP.restart(): all'avvio
//  setPalette() viene chiamata prima di disegnare, quindi basta quella.
//  Si risparmiano circa 1-2 KB, ma il riavvio ferma cronometro e
//  timer e LilyGoLib all'avvio azzera il contapassi del sensore.
// =============================================================

// Proprieta' di colore usate dal tema
static const lv_style_prop_t COLOR_PROPS[] = {
    LV_STYLE_BG_COLOR, LV_STYLE_BORDER_COLOR, LV_STYLE_TEXT_COLOR,
    LV_STYLE_ARC_COLOR, LV_STYLE_LINE_COLOR, LV_STYLE_OUTLINE_COLOR,
};

// Parti e stati in cui il tema imposta colori
static const lv_part_t PARTS[] = {
    LV_PART_MAIN, LV_PART_SCROLLBAR, LV_PART_INDICATOR,
    LV_PART_KNOB, LV_PART_SELECTED, LV_PART_ITEMS,
};
static const lv_state_t STATES[] = {
    LV_STATE_DEFAULT, LV_STATE_PRESSED, LV_STATE_CHECKED,
    (lv_state_t)(LV_STATE_CHECKED | LV_STATE_PRESSED), LV_STATE_DISABLED,
};

static void recolorObj(lv_obj_t *obj, const uint32_t *from)
{
    for (lv_part_t part : PARTS) {
        for (lv_state_t state : STATES) {
            const lv_style_selector_t sel = part | state;
            for (lv_style_prop_t prop : COLOR_PROPS) {
                lv_style_value_t v;
                if (lv_obj_get_local_style_prop(obj, prop, &v, sel) != LV_RESULT_OK) continue;
                for (int t = 0; t < TONES; t++) {
                    if (lv_color_eq(v.color, lv_color_hex(from[t]))) {
                        v.color = lv_color_hex(PALETTES[current].tone[t]);
                        lv_obj_set_local_style_prop(obj, prop, v, sel);
                        break;
                    }
                }
            }
        }
    }
    const uint32_t n = lv_obj_get_child_count(obj);
    for (uint32_t i = 0; i < n; i++) recolorObj(lv_obj_get_child(obj, i), from);
}

void recolor(lv_obj_t *root, int from)
{
    if (!root || from < 0 || from >= PALETTE_COUNT || from == current) return;
    recolorObj(root, PALETTES[from].tone);
}

// Toglie i filtri di colore del tema LVGL (scurito/grigio):
// i toni restano quelli della palette LCD
static void noFilters(lv_obj_t *obj)
{
    lv_obj_set_style_color_filter_opa(obj, LV_OPA_TRANSP, LV_STATE_PRESSED);
    lv_obj_set_style_color_filter_opa(obj, LV_OPA_TRANSP, LV_STATE_DISABLED);
    lv_obj_set_style_color_filter_opa(obj, LV_OPA_TRANSP, LV_STATE_CHECKED);
}

void styleScreen(lv_obj_t *scr)
{
    lv_obj_set_style_bg_color(scr, color(BG), 0);
    lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(scr, color(EDGE), 0);
    lv_obj_set_style_border_width(scr, 2, 0);
    lv_obj_set_style_border_opa(scr, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(scr, 0, 0);
    lv_obj_set_style_text_color(scr, color(INK), 0);
    lv_obj_set_style_text_font(scr, &pk_font_text, 0);
    lv_obj_remove_flag(scr, LV_OBJ_FLAG_SCROLLABLE);
}

void styleClear(lv_obj_t *obj)
{
    lv_obj_set_style_bg_opa(obj, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(obj, 0, 0);
    lv_obj_set_style_radius(obj, 0, 0);
    lv_obj_set_style_pad_all(obj, 0, 0);
    lv_obj_set_style_shadow_width(obj, 0, 0);
    lv_obj_set_style_text_color(obj, color(INK), 0);
}

void styleButton(lv_obj_t *btn, bool filled)
{
    lv_obj_set_style_bg_color(btn, color(filled ? INK : BG), 0);
    lv_obj_set_style_bg_opa(btn, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(btn, color(INK), 0);
    lv_obj_set_style_border_width(btn, 2, 0);
    lv_obj_set_style_border_opa(btn, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(btn, 0, 0);
    lv_obj_set_style_shadow_width(btn, 0, 0);
    lv_obj_set_style_outline_width(btn, 0, 0);
    lv_obj_set_style_text_color(btn, color(filled ? BG : INK), 0);
    lv_obj_set_style_bg_color(btn, color(filled ? MID : SHADE), LV_STATE_PRESSED);
    noFilters(btn);
}

void styleBox(lv_obj_t *box, int border)
{
    lv_obj_set_style_bg_color(box, color(BG), 0);
    lv_obj_set_style_bg_opa(box, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(box, color(INK), 0);
    lv_obj_set_style_border_width(box, border, 0);
    lv_obj_set_style_border_opa(box, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(box, 0, 0);
    lv_obj_set_style_shadow_width(box, 0, 0);
    lv_obj_set_style_text_color(box, color(INK), 0);
    noFilters(box);
}

void styleTab(lv_obj_t *btn, lv_obj_t *label, bool active)
{
    lv_obj_set_style_bg_color(btn, color(active ? INK : BG), 0);
    lv_obj_set_style_bg_opa(btn, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(btn, color(INK), 0);
    lv_obj_set_style_border_width(btn, 2, 0);
    lv_obj_set_style_radius(btn, 0, 0);
    lv_obj_set_style_shadow_width(btn, 0, 0);
    lv_obj_set_style_bg_color(btn, color(active ? MID : SHADE), LV_STATE_PRESSED);
    noFilters(btn);
    if (label) lv_obj_set_style_text_color(label, color(active ? BG : INK), 0);
}

void styleLabel(lv_obj_t *label, const lv_font_t *font, uint32_t c)
{
    lv_obj_set_style_text_font(label, font, 0);
    lv_obj_set_style_text_color(label, color(c), 0);
}

void styleArc(lv_obj_t *arc, int width)
{
    lv_obj_set_style_arc_width(arc, width, LV_PART_MAIN);
    lv_obj_set_style_arc_width(arc, width, LV_PART_INDICATOR);
    lv_obj_set_style_arc_color(arc, color(SHADE), LV_PART_MAIN);
    lv_obj_set_style_arc_color(arc, color(INK), LV_PART_INDICATOR);
    lv_obj_set_style_arc_rounded(arc, false, LV_PART_MAIN);
    lv_obj_set_style_arc_rounded(arc, false, LV_PART_INDICATOR);
}

void styleSwitch(lv_obj_t *sw)
{
    // binario: bordo scuro; acceso: pieno; pomello: quadratino
    lv_obj_set_style_bg_color(sw, color(BG), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(sw, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_border_color(sw, color(INK), LV_PART_MAIN);
    lv_obj_set_style_border_width(sw, 2, LV_PART_MAIN);
    lv_obj_set_style_radius(sw, 0, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(sw, LV_OPA_TRANSP, LV_PART_INDICATOR);
    lv_obj_set_style_bg_color(sw, color(INK), LV_PART_INDICATOR | LV_STATE_CHECKED);
    lv_obj_set_style_bg_opa(sw, LV_OPA_COVER, LV_PART_INDICATOR | LV_STATE_CHECKED);
    lv_obj_set_style_radius(sw, 0, LV_PART_INDICATOR);
    lv_obj_set_style_bg_color(sw, color(INK), LV_PART_KNOB);
    lv_obj_set_style_bg_color(sw, color(BG), LV_PART_KNOB | LV_STATE_CHECKED);
    lv_obj_set_style_bg_opa(sw, LV_OPA_COVER, LV_PART_KNOB);
    lv_obj_set_style_radius(sw, 0, LV_PART_KNOB);
    lv_obj_set_style_shadow_width(sw, 0, LV_PART_KNOB);
    lv_obj_set_style_pad_all(sw, -3, LV_PART_KNOB);
    noFilters(sw);
}

void styleSlider(lv_obj_t *slider)
{
    lv_obj_set_style_bg_color(slider, color(SHADE), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(slider, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_radius(slider, 0, LV_PART_MAIN);
    lv_obj_set_style_bg_color(slider, color(INK), LV_PART_INDICATOR);
    lv_obj_set_style_radius(slider, 0, LV_PART_INDICATOR);
    lv_obj_set_style_bg_color(slider, color(INK), LV_PART_KNOB);
    lv_obj_set_style_radius(slider, 0, LV_PART_KNOB);
    lv_obj_set_style_shadow_width(slider, 0, LV_PART_KNOB);
    noFilters(slider);
}

void styleRoller(lv_obj_t *roller)
{
    lv_obj_set_style_bg_color(roller, color(BG), 0);
    lv_obj_set_style_bg_opa(roller, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(roller, color(INK), 0);
    lv_obj_set_style_border_width(roller, 2, 0);
    lv_obj_set_style_radius(roller, 0, 0);
    lv_obj_set_style_text_color(roller, color(MID), 0);
    lv_obj_set_style_bg_color(roller, color(INK), LV_PART_SELECTED);
    lv_obj_set_style_bg_opa(roller, LV_OPA_COVER, LV_PART_SELECTED);
    lv_obj_set_style_text_color(roller, color(BG), LV_PART_SELECTED);
    noFilters(roller);
}

void styleScrollbar(lv_obj_t *obj)
{
    lv_obj_set_style_bg_color(obj, color(INK), LV_PART_SCROLLBAR);
    lv_obj_set_style_bg_opa(obj, LV_OPA_COVER, LV_PART_SCROLLBAR);
    lv_obj_set_style_radius(obj, 0, LV_PART_SCROLLBAR);
    lv_obj_set_style_width(obj, 3, LV_PART_SCROLLBAR);
}

lv_obj_t *hline(lv_obj_t *parent, lv_point_precise_t *pts, int x0, int x1, int y, int width, uint32_t c)
{
    pts[0].x = x0; pts[0].y = y;
    pts[1].x = x1; pts[1].y = y;
    lv_obj_t *l = lv_line_create(parent);
    lv_line_set_points(l, pts, 2);
    lv_obj_set_style_line_width(l, width, 0);
    lv_obj_set_style_line_color(l, color(c), 0);
    lv_obj_set_style_line_rounded(l, false, 0);
    lv_obj_remove_flag(l, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_pos(l, 0, 0);
    return l;
}

}  // namespace pk

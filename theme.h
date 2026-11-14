// =============================================================
//  theme.h - Tema grafico "Pokekron" (Pokemon Platino)
//
//  Schermo LCD verde a 4 toni, testo pixel, cifre a blocchi,
//  riquadri con il bordo scuro e angoli vivi. Tutto e' disegnato
//  con oggetti LVGL e font a 1 bit (pk_fonts.c): niente immagini.
//
//  Colori: 8 palette come il "Cambia colore" del Pokekron (tabella
//  PALETTES in theme.cpp). Si sceglie in Impostazioni > Colore
//  schermo; il cambio si vede subito su tutte le schermate.
// =============================================================
#pragma once
#include <lvgl.h>
#include <stdint.h>

// Font pixel (generati in pk_fonts.c)
extern "C" {
LV_FONT_DECLARE(pk_font_small)   // maiuscoletto 3x5, alto 10 px (etichette)
LV_FONT_DECLARE(pk_font_text)    // testo 5x7, alto 14 px (testo normale)
LV_FONT_DECLARE(pk_font_big)     // testo 5x7, alto 21 px (testo grande)
LV_FONT_DECLARE(pk_digits_s)     // cifre LCD alte 27 px
LV_FONT_DECLARE(pk_digits_m)     // cifre LCD alte 45 px (cronometro)
LV_FONT_DECLARE(pk_digits_l)     // cifre LCD alte 72 px (quadrante)
LV_FONT_DECLARE(pk_sprite)       // segnaposto del quadrante (carattere 'A'), 66x42 px
}

namespace pk {

// --- Toni della palette attiva (cambiano con setPalette) ---------
// Usali sempre cosi' (pk::INK, pk::BG...), mai con il valore scritto
// a mano: solo cosi' il cambio di colore li trova.
extern uint32_t BG;     // sfondo dello schermo
extern uint32_t LIT;    // sfondo mentre lo tocchi (come la retroilluminazione)
extern uint32_t SHADE;  // tono chiaro: linee, tracce, pulsante premuto
extern uint32_t MID;    // tono medio: elementi spenti, decorazioni
extern uint32_t DIM;    // testo secondario
extern uint32_t INK;    // "inchiostro": testo e disegni
extern uint32_t EDGE;   // bordo dello schermo

inline lv_color_t color(uint32_t hex) { return lv_color_hex(hex); }

// --- Palette ---------------------------------------------------
constexpr int PALETTE_COUNT = 8;   // Verde, Giallo, Arancione, Rosso, Viola, Blu, Azzurro, Grigio

const char *paletteName(int index);   // "Verde", "Giallo"...
int palette();                        // palette attiva (0 = Verde)

// Colore di sfondo di una palette qualsiasi (per i puntini del Radar)
uint32_t paletteBg(int index);

// Rende attiva una palette: da qui in poi gli oggetti nuovi usano i suoi
// toni. Restituisce false se era gia' quella attiva (o indice non valido).
// Gli oggetti GIA' disegnati non cambiano: per quelli c'e' recolor().
bool setPalette(int index);

// Ricolora un albero di oggetti gia' disegnato (una schermata, un livello):
// ogni tono della palette "from" diventa lo stesso tono della palette attiva.
// Lo usa il launcher subito dopo setPalette (vedi Launcher::applyPalette).
void recolor(lv_obj_t *root, int from);

// Schermata: sfondo verde, bordo scuro, testo color inchiostro
void styleScreen(lv_obj_t *scr);

// Contenitore trasparente senza bordo (per raggruppare oggetti)
void styleClear(lv_obj_t *obj);

// Pulsante: riquadro con bordo scuro, angoli vivi; premuto si scurisce.
// filled = true: pieno color inchiostro con il testo chiaro.
void styleButton(lv_obj_t *btn, bool filled = false);

// Riquadro (righe, schede di dettaglio): sfondo verde e bordo scuro
void styleBox(lv_obj_t *box, int border = 2);

// Linguetta di una scheda: attiva = piena, non attiva = solo bordo
void styleTab(lv_obj_t *btn, lv_obj_t *label, bool active);

// Etichetta con font e colore
void styleLabel(lv_obj_t *label, const lv_font_t *font, uint32_t color = INK);

// Anello di avanzamento (passi, timer)
void styleArc(lv_obj_t *arc, int width);

// Controlli delle impostazioni
void styleSwitch(lv_obj_t *sw);
void styleSlider(lv_obj_t *slider);
void styleRoller(lv_obj_t *roller);
void styleScrollbar(lv_obj_t *obj);

// Linea orizzontale da x0 a x1 all'altezza y (punti salvati in "pts")
lv_obj_t *hline(lv_obj_t *parent, lv_point_precise_t *pts, int x0, int x1, int y, int width, uint32_t color);

}  // namespace pk

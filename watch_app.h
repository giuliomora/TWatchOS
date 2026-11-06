// =============================================================
//  watch_app.h - Interfaccia comune delle app del menu
//
//  Un'app e' un modulo (WatchModule) con in piu' un titolo,
//  un'icona, un colore e una schermata da disegnare.
//  Si registra nel .ino con launcher.addApp(&miaApp).
//
//  Il launcher crea per ogni app una schermata con la freccia
//  "indietro" e il titolo in alto; l'app disegna il resto
//  dentro build(parent).
// =============================================================
#pragma once
#include <lvgl.h>
#include "watch_module.h"
#include "config.h"

class WatchApp : public WatchModule {
public:
    // Nome mostrato sotto l'icona e in cima alla schermata
    virtual const char *title() const = 0;

    // Icona del menu (un simbolo LVGL, es. LV_SYMBOL_SETTINGS)
    virtual const char *icon() const = 0;

    // Colore del riquadro nel menu (es. 0x1E88E5)
    virtual uint32_t color() const { return 0x3A3A3C; }

    // Disegna il contenuto dell'app dentro "parent" (chiamato una volta all'avvio)
    virtual void build(lv_obj_t * /*parent*/) {}

    // Chiamati quando l'app viene aperta / chiusa
    virtual void onOpen() {}
    virtual void onClose() {}

    // Dopo quanto lo schermo si spegne mentre l'app e' aperta.
    // 0 = resta sempre acceso (es. cronometro in funzione).
    virtual uint32_t screenTimeoutMs() const { return cfg::MENU_TIMEOUT_MS; }

    // Il nome del modulo (per i messaggi di debug) e' il titolo
    const char *name() const override { return title(); }

    // true mentre l'app e' aperta (lo imposta il launcher)
    bool isOpen() const { return open; }

private:
    friend class Launcher;
    bool open = false;
};

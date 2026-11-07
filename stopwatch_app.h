// =============================================================
//  stopwatch_app.h - App "Tempo": Cronometro e Timer
//
//  In alto due schede, come segnalibri: "Cronometro" e "Timer".
//  Restano sempre visibili; toccandole si cambia modalita'.
//  La scheda Timer e' disegnata e gestita da countdown_timer.*
//
//  Cronometro - tre pulsanti:  Avvia/Pausa,  Giro,  Stop (azzera).
//  Sotto il tempo si vede solo l'ultimo giro.
//
//  Cronometro e timer contano anche a schermo spento e con il
//  processore in sospensione. Sul quadrante, in alto a sinistra,
//  compaiono con il loro tempo quando sono attivi.
//
//  Comando dal Serial Monitor:
//      crono   -> stato del cronometro
// =============================================================
#pragma once
#include <stddef.h>
#include <stdint.h>
#include "watch_app.h"

class StopwatchApp : public WatchApp {
public:
    enum Tab { TAB_STOPWATCH = 0, TAB_TIMER = 1 };

    const char *title() const override { return "Tempo"; }
    const char *icon() const override { return LV_SYMBOL_PLAY; }  // non usata: c'e' drawIcon
    uint32_t color() const override { return 0xEF6C00; }
    bool drawIcon(lv_obj_t *tile) override;   // icona dell'orologio nel menu
    bool hideTitle() const override { return true; }  // al posto del titolo ci sono le schede

    void update() override;
    void build(lv_obj_t *parent) override;
    void onOpen() override;
    bool onSerialCommand(const char *line) override;
    uint32_t screenTimeoutMs() const override;

    // Cambia scheda
    void showTab(Tab tab);

    // Comandi del cronometro (usati dai pulsanti)
    void startPause();
    void lap();
    void stop();

    // Stato del cronometro (usato anche dal quadrante)
    bool isRunning() const { return running; }
    bool isActive() const { return running || elapsedUs() > 0; }  // avviato o in pausa
    int64_t elapsedUs() const;

    // Tempo come "MM:SS,cc" (o "H:MM:SS" oltre l'ora)
    static void formatLong(int64_t us, char *out, size_t size);
    // Tempo come "MM:SS" (o "H:MM:SS"), per il quadrante
    static void formatShort(int64_t us, char *out, size_t size);

private:
    void refreshUi();
    void buzz();

    Tab tab = TAB_STOPWATCH;
    bool running = false;
    int64_t startedAt = 0;     // quando e' partito l'ultimo tratto (us)
    int64_t accumulated = 0;   // tempo dei tratti precedenti (us)
    int64_t lapStart = 0;      // tempo totale all'inizio del giro in corso
    int64_t lastLap = 0;       // durata dell'ultimo giro
    uint32_t lapCount = 0;
    uint32_t lastUi = 0;
};

extern StopwatchApp stopwatchApp;

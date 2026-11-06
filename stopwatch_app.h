// =============================================================
//  stopwatch_app.h - App "Cronometro"
//
//  Tre pulsanti:  Avvia/Pausa,  Giro,  Stop (azzera).
//  Sotto il tempo si vede solo l'ultimo giro.
//
//  Il cronometro conta anche a schermo spento e con il
//  processore in sospensione (usa l'orologio interno del chip,
//  che non si ferma). Mentre e' avviato o in pausa, sul quadrante
//  in alto a sinistra compare una piccola icona con il tempo.
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
    const char *title() const override { return "Cronometro"; }
    const char *icon() const override { return LV_SYMBOL_PLAY; }  // provvisoria
    uint32_t color() const override { return 0xEF6C00; }

    void update() override;
    void build(lv_obj_t *parent) override;
    void onOpen() override;
    bool onSerialCommand(const char *line) override;
    uint32_t screenTimeoutMs() const override;

    // Comandi (usati dai pulsanti)
    void startPause();
    void lap();
    void stop();

    // Stato (usato anche dal quadrante)
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

    bool running = false;
    int64_t startedAt = 0;     // quando e' partito l'ultimo tratto (us)
    int64_t accumulated = 0;   // tempo dei tratti precedenti (us)
    int64_t lapStart = 0;      // tempo totale all'inizio del giro in corso
    int64_t lastLap = 0;       // durata dell'ultimo giro
    uint32_t lapCount = 0;
    uint32_t lastUi = 0;
};

extern StopwatchApp stopwatchApp;

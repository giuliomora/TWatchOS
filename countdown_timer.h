// =============================================================
//  countdown_timer.h - Timer (conto alla rovescia)
//
//  Vive nella scheda "Timer" dell'app Tempo (stopwatch_app):
//  l'app disegna le schede, questo modulo disegna e gestisce
//  il contenuto della scheda Timer.
//
//    - pulsanti rapidi: 5, 10, 15 minuti (in config.h)
//    - "Altro": rotelle minuti / secondi
//    - durante il conto: anello che si svuota, Pausa e Annulla
//    - allo scadere: schermo acceso, app aperta sul Timer,
//      vibrazione a intervalli finche' non tocchi Stop
//      (al massimo TIMER_RING_S secondi)
//
//  Conta anche a schermo spento: il processore si risveglia
//  da solo alla scadenza.
//
//  Comandi dal Serial Monitor:
//      timer        -> stato del timer
//      timer 10     -> fa partire un timer di 10 secondi (per le prove)
// =============================================================
#pragma once
#include <lvgl.h>
#include <stddef.h>
#include <stdint.h>
#include "watch_module.h"

class CountdownTimer : public WatchModule {
public:
    enum State { SETUP, PICKER, RUNNING, PAUSED, RINGING };

    const char *name() const override { return "timer"; }
    void update() override;
    bool onSerialCommand(const char *line) override;
    uint32_t msUntilNextWork() override;

    // Disegna la scheda Timer dentro "parent" (lo chiama l'app Tempo)
    void build(lv_obj_t *parent);
    void refreshUi();

    // Comandi (usati dai pulsanti)
    void start(uint32_t seconds);
    void pauseResume();
    void cancel();
    void stopRinging();
    void openPicker();
    void closePicker();
    void startFromPicker();

    // Stato (usato dall'app Tempo e dal quadrante)
    bool isActive() const { return st == RUNNING || st == PAUSED; }  // in corso o in pausa
    bool isRunning() const { return st == RUNNING; }
    bool isRinging() const { return st == RINGING; }
    int64_t remainingUs() const;

    // Tempo rimasto come "MM:SS" (o "H:MM:SS"), arrotondato per eccesso
    static void formatRemaining(int64_t us, char *out, size_t size);

private:
    void setState(State s);
    void expire();
    void ringPulse();
    void buzz();

    State st = SETUP;
    int64_t totalUs = 0;        // durata impostata
    int64_t endAt = 0;          // quando scade (us), se in corso
    int64_t pausedLeft = 0;     // tempo rimasto, se in pausa
    int64_t ringStartedAt = 0;
    int64_t lastPulseAt = 0;
    uint32_t lastUi = 0;
};

extern CountdownTimer countdownTimer;

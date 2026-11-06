// =============================================================
//  screen.h - Accensione e spegnimento automatico dello schermo
//
//  Qualunque modulo puo' chiamare screen.wake() per accendere
//  lo schermo (o tenerlo acceso, se lo e' gia').
// =============================================================
#pragma once
#include "watch_module.h"
#include <stdint.h>

class Screen : public WatchModule {
public:
    const char *name() const override { return "screen"; }
    void begin() override;
    void update() override;
    bool canSleep() override { return !on; }  // si dorme solo a schermo spento

    // Accende lo schermo e fa ripartire il conto alla rovescia.
    // "reason" serve solo per i messaggi di debug.
    void wake(const char *reason);

    // Spegne subito lo schermo
    void sleep();

    bool isOn() const { return on; }

    // Dopo quanti ms senza tocchi lo schermo si spegne (0 = mai).
    // Lo imposta il launcher a seconda della schermata.
    void setTimeout(uint32_t ms) { timeoutMs = ms; }

private:
    bool on = false;
    uint32_t lastActivity = 0;
    uint32_t timeoutMs = 0;
};

// L'unico schermo dell'orologio, utilizzabile da tutti i moduli
extern Screen screen;

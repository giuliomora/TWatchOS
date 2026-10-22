// =============================================================
//  power.h - Risparmio energetico
//
//  Quando lo schermo e' spento e nessun modulo sta lavorando,
//  mette il processore in "light sleep": si ferma quasi del tutto
//  e consuma pochissimo. Si risveglia da solo in meno di un
//  millisecondo con:
//    - rotazione del polso (e in generale gli eventi del sensore)
//    - tocco sullo schermo
//    - click sulla corona
//    - un timer, quando un modulo deve fare qualcosa (es. Wi-Fi)
//
//  Con il cavo USB collegato NON va in sospensione (per non
//  disturbare il Serial Monitor e il caricamento dei programmi),
//  a meno che SLEEP_ON_USB in config.h sia true.
//
//  Comandi dal Serial Monitor:
//      power        -> batteria, consumi e statistiche
//      power reset  -> azzera le statistiche
// =============================================================
#pragma once
#include "watch_module.h"
#include <stdint.h>

class Power : public WatchModule {
public:
    const char *name() const override { return "power"; }
    void begin() override;
    void update() override;
    bool onSerialCommand(const char *line) override;

private:
    void lightSleep(uint32_t maxMs);
    bool usbConnected();
    void printStatus();
    void resetStats();

    // Statistiche
    uint32_t statsSince = 0;      // millis() di inizio statistiche
    uint64_t sleptMs = 0;         // tempo totale in sospensione
    uint32_t sleepCount = 0;
    uint32_t wakeTouch = 0, wakeTilt = 0, wakeCrown = 0;
    uint32_t wakeSensorOther = 0, wakeTimer = 0;
    float batteryAtStart = -1;

    // Cache dello stato USB (leggerlo costa una richiesta al chip)
    bool usbCached = false;
    uint32_t usbCheckedAt = 0;
};

extern Power power;

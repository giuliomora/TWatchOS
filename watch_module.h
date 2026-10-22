// =============================================================
//  watch_module.h - Interfaccia comune di ogni funzionalita'
//
//  Ogni funzionalita' dell'orologio (quadrante, sveglia, passi,
//  Wi-Fi, ...) e' una classe che eredita da WatchModule.
//  Implementi solo i metodi che ti servono: gli altri restano
//  vuoti e non fanno niente.
// =============================================================
#pragma once
#include <stdint.h>

class WatchModule {
public:
    virtual ~WatchModule() {}

    // Nome del modulo, usato nei messaggi di debug
    virtual const char *name() const = 0;

    // Chiamato una volta all'avvio, dopo l'inizializzazione dell'hardware
    virtual void begin() {}

    // Chiamato continuamente dal loop()
    virtual void update() {}

    // Chiamati quando lo schermo si accende / si spegne
    virtual void onScreenOn() {}
    virtual void onScreenOff() {}

    // Chiamato quando arriva una riga di testo dal Serial Monitor.
    // Restituisci true se il comando era tuo e l'hai gestito.
    virtual bool onSerialCommand(const char * /*line*/) { return false; }

    // --- Risparmio energetico (modulo power) ---
    // Restituisci false se il modulo sta lavorando e il processore
    // non deve andare in sospensione (es. Wi-Fi in collegamento).
    virtual bool canSleep() { return true; }

    // Tra quanti millisecondi il modulo deve essere risvegliato
    // per fare qualcosa (UINT32_MAX = non serve).
    virtual uint32_t msUntilNextWork() { return UINT32_MAX; }
};

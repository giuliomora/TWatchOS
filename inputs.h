// =============================================================
//  inputs.h - Legge i "gesti" che accendono lo schermo:
//  rotazione del polso, tocco sullo schermo, click sulla corona.
//
//  Per aggiungere un nuovo gesto (es. doppio tocco) basta
//  gestirlo in onDeviceEvent() dentro inputs.cpp.
//
//  Comando dal Serial Monitor:
//      polso   -> accende/spegne i messaggi di prova del gesto:
//                 a ogni gesto riconosciuto stampa l'inclinazione
//                 dell'orologio (0 gradi = schermo verso l'alto)
// =============================================================
#pragma once
#include "watch_module.h"
#include <stdint.h>

class Inputs : public WatchModule {
public:
    const char *name() const override { return "inputs"; }
    void begin() override;
    void update() override;
    bool onSerialCommand(const char *line) override;

private:
    void pollTouch();
public:
    bool wristDebug = false;   // messaggi di prova del gesto (comando "polso")
};

extern Inputs inputs;

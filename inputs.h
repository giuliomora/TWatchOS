// =============================================================
//  inputs.h - Legge i "gesti" che accendono lo schermo:
//  rotazione del polso, tocco sullo schermo, click sulla corona.
//
//  Per aggiungere un nuovo gesto (es. doppio tocco) basta
//  gestirlo in onDeviceEvent() dentro inputs.cpp.
// =============================================================
#pragma once
#include "watch_module.h"
#include <stdint.h>

class Inputs : public WatchModule {
public:
    const char *name() const override { return "inputs"; }
    void begin() override;
    void update() override;

private:
    void pollTouch();
};

extern Inputs inputs;

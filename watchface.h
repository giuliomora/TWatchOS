// =============================================================
//  watchface.h - Il quadrante: ora grande e data sotto
//
//  Si aggiorna solo quando lo schermo e' acceso, per non
//  consumare batteria inutilmente.
// =============================================================
#pragma once
#include "watch_module.h"
#include <stdint.h>

class Watchface : public WatchModule {
public:
    const char *name() const override { return "watchface"; }
    void begin() override;
    void update() override;
    void onScreenOn() override;

private:
    void refresh();
    uint32_t lastRefresh = 0;
};

extern Watchface watchface;

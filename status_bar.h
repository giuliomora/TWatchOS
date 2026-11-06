// =============================================================
//  status_bar.h - Barra di stato: batteria in alto a destra
//
//  Visibile su tutte le schermate (quadrante, menu, app).
//  Mostra la percentuale e l'icona della batteria:
//    - grigia normalmente
//    - verde con il fulmine quando e' in carica
//    - rossa sotto la soglia BATTERY_LOW_PCT (config.h)
//  Si aggiorna quando lo schermo si accende e poi ogni
//  STATUS_BAR_REFRESH_MS, solo a schermo acceso.
// =============================================================
#pragma once
#include "watch_module.h"
#include <stdint.h>

class StatusBar : public WatchModule {
public:
    const char *name() const override { return "status"; }
    void begin() override;
    void update() override;
    void onScreenOn() override;

private:
    void refresh();
    uint32_t lastRefresh = 0;
};

extern StatusBar statusBar;

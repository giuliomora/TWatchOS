// =============================================================
//  wifi_time.h - Sincronizzazione automatica dell'ora via Wi-Fi
//
//  All'avvio (e poi una volta al giorno) si collega al Wi-Fi,
//  chiede l'ora esatta a un server NTP, la salva nell'RTC e
//  spegne subito il Wi-Fi per risparmiare batteria.
//  Non blocca l'orologio: tutto avviene "in sottofondo".
//
//  Nome e password della rete vanno in secrets.h
//  Comandi dal Serial Monitor:
//      sync   -> sincronizza subito
//      wifi   -> stato dell'ultima sincronizzazione
// =============================================================
#pragma once
#include "watch_module.h"
#include <stdint.h>
#include <time.h>

class WifiTime : public WatchModule {
public:
    const char *name() const override { return "wifi"; }
    void begin() override;
    void update() override;
    bool onSerialCommand(const char *line) override;
    bool canSleep() override { return state == IDLE; }
    uint32_t msUntilNextWork() override;

    // Avvia una sincronizzazione (se non ce n'e' gia' una in corso)
    void syncNow();

    // Accende/spegne la sincronizzazione automatica (dalle impostazioni)
    void setAutoSync(bool on);

    // Stato, per le impostazioni
    bool isConfigured() const { return enabled; }       // secrets.h compilato
    bool isSyncing() const { return state != IDLE; }
    bool hasSynced() const { return everSynced; }
    bool lastAttemptFailed() const { return lastFailed; }
    const struct tm &lastSyncTime() const { return lastSyncTm; }  // ora locale

private:
    enum State { IDLE, CONNECTING, WAITING_TIME };
    void stop(bool success, const char *msg);

    State state = IDLE;
    bool enabled = false;
    uint32_t stateSince = 0;     // quando e' iniziata la fase attuale
    uint32_t nextAttempt = 0;    // quando riprovare (millis)
    uint32_t lastSyncOk = 0;     // ultima sincronizzazione riuscita (millis)
    bool everSynced = false;
    bool lastFailed = false;
    struct tm lastSyncTm = {};
};

extern WifiTime wifiTime;

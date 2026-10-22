// =============================================================
//  time_service.h - Gestione dell'ora
//
//  L'RTC (orologio interno) conserva l'ora universale (UTC) e
//  continua a contare anche a orologio spento, finche' c'e'
//  batteria. La conversione in ora italiana, compreso il cambio
//  ora solare/legale, la fa il programma (vedi TIMEZONE in config.h).
//
//  L'ora si regola da sola via Wi-Fi (modulo wifi_time).
//  Per regolarla a mano: Serial Monitor (115200 baud, "Newline")
//      ora 2026-10-22 15:30:00      (ora italiana)
// =============================================================
#pragma once
#include "watch_module.h"
#include <time.h>

class TimeService : public WatchModule {
public:
    const char *name() const override { return "time"; }
    void begin() override;
    bool onSerialCommand(const char *line) override;

    // Data e ora correnti, in ora locale
    struct tm now();

    // Imposta data e ora, in ora locale
    void set(int year, int month, int day, int hour, int minute, int second);

    // Salva nell'RTC l'ora attuale del sistema
    // (chiamato dal modulo Wi-Fi dopo la sincronizzazione)
    void saveSystemTimeToRtc();
};

extern TimeService timeService;

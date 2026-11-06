// =============================================================
//  steps_app.h - App "Passi": contapassi del giorno
//
//  I passi li conta da solo il sensore di movimento (BMA423),
//  sempre, anche a schermo spento e con il processore in
//  sospensione: il programma legge il totale solo quando serve
//  (schermo acceso, e ogni STEPS_CHECK_MIN minuti per
//  l'obiettivo e la mezzanotte).
//
//  Schermata: anello che si riempie verso l'obiettivo, con
//  i passi al centro; sotto km e calorie stimati e i passi di ieri.
//  A mezzanotte si riparte da zero. Al raggiungimento
//  dell'obiettivo l'orologio vibra.
//  I passi sono salvati in memoria: dopo un riavvio si riparte
//  dall'ultimo salvataggio (ogni STEPS_SAVE_MIN minuti).
//
//  Comandi dal Serial Monitor:
//      passi         -> passi di oggi e di ieri
//      passi vibra   -> prova la vibrazione dell'obiettivo
// =============================================================
#pragma once
#include <stddef.h>
#include <stdint.h>
#include "watch_app.h"

class StepsApp : public WatchApp {
public:
    const char *title() const override { return "Passi"; }
    const char *icon() const override { return LV_SYMBOL_GPS; }  // provvisoria
    uint32_t color() const override { return 0x2E7D32; }

    void begin() override;
    void update() override;
    void build(lv_obj_t *parent) override;
    void onOpen() override;
    void onScreenOn() override;
    bool onSerialCommand(const char *line) override;
    uint32_t msUntilNextWork() override;

    // Passi di oggi e di ieri (usati anche dal quadrante)
    uint32_t today() const { return offset + sensorSteps; }
    uint32_t yesterday() const { return yesterdaySteps; }
    bool goalReached() const { return goalDone; }

private:
    void check();                  // legge il sensore, mezzanotte, obiettivo, salvataggio
    void readSensor();
    void newDay(uint32_t day);
    void save();
    void refreshUi();
    void celebrate();              // vibrazione dell'obiettivo

    uint32_t offset = 0;           // passi fatti oggi prima dell'ultimo riavvio
    uint32_t sensorSteps = 0;      // passi contati dal sensore dall'ultimo azzeramento
    uint32_t yesterdaySteps = 0;
    uint32_t currentDay = 0;       // data di oggi come AAAAMMGG
    bool goalDone = false;
    bool loaded = false;

    uint32_t lastCheck = 0;
    uint32_t midnightAt = 0;       // millis() della prossima mezzanotte
    uint32_t lastSave = 0;
    uint32_t savedSteps = 0;
};

// Scrive un numero con il punto delle migliaia: 12345 -> "12.345"
void formatSteps(uint32_t n, char *out, size_t size);

extern StepsApp stepsApp;

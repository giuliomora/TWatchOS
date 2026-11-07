// =============================================================
//  user_settings.h - Impostazioni scelte sull'orologio
//
//  I valori di config.h sono quelli di partenza; quelli scelti
//  nell'app Impostazioni vengono salvati in memoria (NVS) e
//  hanno la precedenza, anche dopo un riavvio.
//
//  Va registrato per PRIMO nel .ino: gli altri moduli leggono
//  questi valori gia' nel loro begin().
//
//  Comandi dal Serial Monitor:
//      impostazioni         -> mostra i valori attuali
//      impostazioni reset   -> torna ai valori di config.h
// =============================================================
#pragma once
#include <stdint.h>
#include "watch_module.h"

class UserSettings : public WatchModule {
public:
    const char *name() const override { return "settings"; }
    void begin() override;   // carica i valori salvati
    bool onSerialCommand(const char *line) override;

    void save();             // salva tutti i valori
    void resetToDefaults();  // torna ai valori di config.h (e li salva)

    uint32_t faceTimeoutMs() const { return (uint32_t)faceTimeoutS * 1000UL; }
    uint32_t menuTimeoutMs() const { return (uint32_t)menuTimeoutS * 1000UL; }

    // --- I valori ---
    uint8_t brightness;      // luminosita' 20-255
    uint8_t faceTimeoutS;    // spegnimento sul quadrante (secondi)
    uint8_t menuTimeoutS;    // spegnimento nel menu e nelle app (secondi)
    bool wristWake;          // accensione con il gesto del polso
    bool touchWake;          // accensione con il tocco
    bool buttonVibrate;      // vibrazione dei pulsanti (cronometro, timer)
    uint32_t stepsGoal;      // obiettivo dei passi
    bool stepsOnFace;        // passi sul quadrante
    bool autoSync;           // sincronizzazione automatica dell'ora via Wi-Fi

private:
    void setDefaults();
    void print();
};

extern UserSettings userSettings;

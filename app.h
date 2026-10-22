// =============================================================
//  app.h - Il "motore" che gestisce tutti i moduli
//
//  Non serve modificarlo per aggiungere funzionalita':
//  basta registrare il nuovo modulo con App::add() nel file .ino
// =============================================================
#pragma once
#include "watch_module.h"
#include <stdint.h>

namespace App {

// Registra un modulo (da chiamare nel setup, prima di App::begin)
void add(WatchModule *module);

// Avvia tutti i moduli registrati, nell'ordine in cui sono stati aggiunti
void begin();

// Da chiamare nel loop(): aggiorna i moduli e legge i comandi seriali
void update();

// Avvisa tutti i moduli che lo schermo si e' acceso / spento
void notifyScreenOn();
void notifyScreenOff();

// Risparmio energetico: true se tutti i moduli possono dormire
bool canSleep();
// Il minimo, tra tutti i moduli, di msUntilNextWork()
uint32_t msUntilNextWork();

// Stampa un messaggio sul Serial Monitor (se DEBUG_LOG e' attivo)
void log(const char *tag, const char *fmt, ...);

}  // namespace App

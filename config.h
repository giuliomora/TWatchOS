// =============================================================
//  config.h - Impostazioni generali dell'orologio
//  Cambia i valori qui, senza toccare il resto del codice.
// =============================================================
#pragma once
#include <stdint.h>

namespace cfg {

// --- Schermo ------------------------------------------------
// Dopo quanti millisecondi senza interazioni lo schermo si spegne
constexpr uint32_t SCREEN_TIMEOUT_MS = 5000;

// Dopo quanti millisecondi si spegne quando si e' nel menu o in un'app
// (un'app puo' chiedere un tempo diverso, o 0 = sempre acceso)
constexpr uint32_t MENU_TIMEOUT_MS = 15000;

// Luminosita' quando lo schermo e' acceso (0 - 255)
constexpr uint8_t SCREEN_BRIGHTNESS = 180;

// --- Cosa accende lo schermo --------------------------------
constexpr bool WAKE_ON_WRIST_TILT  = true;   // rotazione del polso
constexpr bool WAKE_ON_TOUCH       = true;   // tocco sullo schermo
constexpr bool WAKE_ON_CROWN_CLICK = true;   // click breve sulla corona

// Riconoscimento del gesto del polso nel sensore di movimento:
// true = algoritmo per orologi/braccialetti (consigliato),
// false = algoritmo per smartphone (quello impostato da LilyGoLib)
constexpr bool WRIST_MODE_WRISTBAND = true;

// --- Menu -------------------------------------------------
// Durata dell'animazione tra le schermate (0 = nessuna animazione)
constexpr uint32_t MENU_ANIM_MS = 200;

// Dopo un risveglio, per quanti millisecondi ignorare i tocchi
// (cosi' il tocco che accende lo schermo non apre anche il menu)
constexpr uint32_t WAKE_TOUCH_IGNORE_MS = 600;

// --- Barra di stato (batteria in alto a destra) ----------
// Ogni quanti millisecondi aggiornare la batteria (a schermo acceso)
constexpr uint32_t STATUS_BAR_REFRESH_MS = 10000;

// Sotto questa percentuale la batteria diventa rossa
constexpr int BATTERY_LOW_PCT = 20;

// --- Ora ---------------------------------------------------
// Fuso orario con le regole di ora solare/legale (questo e' l'Italia).
// Altri fusi: https://github.com/nayarsystems/posix_tz_db/blob/master/zones.csv
constexpr const char *TIMEZONE = "CET-1CEST,M3.5.0,M10.5.0/3";

// --- Sincronizzazione Wi-Fi (nome e password in secrets.h) ---
constexpr const char *NTP_SERVER = "pool.ntp.org";
constexpr uint32_t WIFI_SYNC_INTERVAL_H = 24;   // ogni quante ore risincronizzare
constexpr uint32_t WIFI_RETRY_MIN = 15;         // se fallisce, riprova dopo N minuti
constexpr uint32_t WIFI_TIMEOUT_MS = 20000;     // tempo massimo per collegarsi

// --- Risparmio energetico ----------------------------------
// Velocita' del processore in MHz (80 basta e consuma meno; massimo 240)
constexpr uint32_t CPU_MHZ = 80;

// A schermo spento il processore va in sospensione
constexpr bool SLEEP_ENABLED = true;

// Va in sospensione anche con il cavo USB collegato?
// Lascia false: in sospensione il Serial Monitor si disconnette
// e il caricamento dei programmi puo' non riuscire.
constexpr bool SLEEP_ON_USB = false;

// --- Debug --------------------------------------------------
// Se true, stampa messaggi sul Serial Monitor (115200 baud)
constexpr bool DEBUG_LOG = true;

}  // namespace cfg

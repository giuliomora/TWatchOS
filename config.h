// =============================================================
//  config.h - Impostazioni generali dell'orologio
//  Cambia i valori qui, senza toccare il resto del codice.
// =============================================================
#pragma once
#include <stdint.h>

namespace cfg {

// Versione del firmware (mostrata in Impostazioni > Info)
constexpr const char *FIRMWARE_VERSION = "0.8";

// NOTA: luminosita', colore, spegnimento schermo, polso, tocco, vibrazione,
// obiettivo dei passi e ora automatica si cambiano anche dall'app
// Impostazioni. I valori qui sotto sono quelli di partenza (e quelli
// a cui si torna con "Ripristina impostazioni").

// --- Schermo ------------------------------------------------
// Dopo quanti millisecondi senza interazioni lo schermo si spegne
constexpr uint32_t SCREEN_TIMEOUT_MS = 5000;

// Dopo quanti millisecondi si spegne quando si e' nel menu o in un'app
// (un'app puo' chiedere un tempo diverso, o 0 = sempre acceso)
constexpr uint32_t MENU_TIMEOUT_MS = 15000;

// Luminosita' quando lo schermo e' acceso (0 - 255)
constexpr uint8_t SCREEN_BRIGHTNESS = 180;

// Colore dello schermo (come il "Cambia colore" del Pokekron):
// 0 Verde, 1 Giallo, 2 Arancione, 3 Rosso, 4 Viola, 5 Blu, 6 Azzurro, 7 Grigio
constexpr uint8_t SCREEN_COLOR = 0;

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

// --- Suono ----------------------------------------------------
// Suoni dell'altoparlante (Pokeradar). Valore di partenza: si cambia
// anche da Impostazioni > Suono. L'altoparlante si accende solo
// mentre un'app che suona e' aperta.
constexpr bool SOUND_ON = true;
// Volume (0 - 100)
constexpr uint8_t SOUND_VOLUME = 70;

// --- Barra di stato (batteria in alto a destra) ----------
// Ogni quanti millisecondi aggiornare la batteria (a schermo acceso)
constexpr uint32_t STATUS_BAR_REFRESH_MS = 10000;

// Sotto questa percentuale la batteria diventa rossa
constexpr int BATTERY_LOW_PCT = 20;

// --- Passi ------------------------------------------------
// Obiettivo giornaliero (anello pieno e vibrazione)
constexpr uint32_t STEPS_GOAL = 6000;

// Al raggiungimento dell'obiettivo: vibra? accende lo schermo?
constexpr bool STEPS_GOAL_VIBRATE     = true;
constexpr bool STEPS_GOAL_WAKE_SCREEN = true;

// Mostra i passi sul quadrante, sotto la data
constexpr bool STEPS_ON_WATCHFACE = true;   // valore di partenza (si cambia anche da Impostazioni)

// Per le stime: lunghezza del passo (cm) e calorie ogni 1000 passi
constexpr uint32_t STEP_LENGTH_CM      = 70;
constexpr uint32_t KCAL_PER_1000_STEPS = 40;

// A schermo spento, ogni quanti minuti controllare obiettivo e mezzanotte
constexpr uint32_t STEPS_CHECK_MIN = 5;

// Ogni quanti minuti salvare i passi in memoria (contro i riavvii)
constexpr uint32_t STEPS_SAVE_MIN = 15;

// --- Cronometro --------------------------------------------
// Vibrazione breve a ogni tocco sui pulsanti di cronometro e timer
constexpr bool STOPWATCH_VIBRATE = true;

// Mentre il cronometro conta, tenere lo schermo sempre acceso
// (solo con l'app aperta)? false = si spegne come al solito,
// il cronometro continua a contare lo stesso.
constexpr bool STOPWATCH_KEEP_SCREEN_ON = false;

// Mostra il cronometro sul quadrante (in alto a sinistra) se e' avviato o in pausa
constexpr bool STOPWATCH_ON_WATCHFACE = true;

// --- Timer (nell'app Tempo, insieme al cronometro) ----------
// I tre pulsanti rapidi, in minuti
constexpr uint32_t TIMER_PRESET_1_MIN = 5;
constexpr uint32_t TIMER_PRESET_2_MIN = 10;
constexpr uint32_t TIMER_PRESET_3_MIN = 15;

// Allo scadere: vibra ogni TIMER_RING_PULSE_MS millisecondi,
// finche' non tocchi Stop, al massimo per TIMER_RING_S secondi
constexpr uint32_t TIMER_RING_PULSE_MS = 1500;
constexpr uint32_t TIMER_RING_S = 60;

// Mostra il timer sul quadrante (in alto a sinistra) se e' in corso o in pausa
constexpr bool TIMER_ON_WATCHFACE = true;

// --- Radar (app Radar: dispositivi Bluetooth vicini) --------
// Durata di una passata del sonar (la riga che sale o che scende), in ms
constexpr uint32_t RADAR_SWEEP_MS = 2000;
// Ogni quanti millisecondi si ridisegna la riga del sonar (50 = 20 volte al secondo)
constexpr uint32_t RADAR_FRAME_MS = 50;
// Un dispositivo che non si sente da N secondi sparisce dal radar
constexpr uint32_t RADAR_DEVICE_TIMEOUT_S = 30;
// Quanti dispositivi al massimo sul radar
constexpr int RADAR_MAX_DEVICES = 32;
// Potenza del segnale (dBm) che corrisponde al centro e al bordo del radar
constexpr int RADAR_RSSI_NEAR = -40;
constexpr int RADAR_RSSI_FAR = -100;
// false = solo ascolto. true = chiede ai dispositivi anche il nome
// (si vedono piu' nomi, ma l'orologio trasmette una breve richiesta)
constexpr bool RADAR_ACTIVE_SCAN = false;
// Scheda Wi-Fi (ascolto passivo delle probe request)
constexpr int RADAR_PROBE_MAX_DEVICES = 40;    // quanti dispositivi al massimo in lista
constexpr uint32_t RADAR_PROBE_HOP_MS = 250;   // ogni quanto cambiare canale Wi-Fi
constexpr int RADAR_PROBE_CHANNELS = 13;       // canali su cui ascoltare (1..13)
constexpr int RADAR_PROBE_MAX_SSID = 8;        // nomi di rete al massimo per dispositivo

// --- Pokeradar (ricerca strumenti nascosti) ------------------
// Ogni quanti passi si "tira il dado" per far comparire uno strumento
constexpr uint32_t POKERADAR_ROLL_STEPS = 50;
// Strumenti nascosti al massimo nello stesso momento
constexpr int POKERADAR_MAX_ITEMS = 3;
// Distanza (in passi) a cui compaiono: da ... a ...
// (lo schermo arriva a circa 100 passi dal centro)
constexpr int POKERADAR_SPAWN_MIN = 40;
constexpr int POKERADAR_SPAWN_MAX = 450;

// --- Ora ---------------------------------------------------
// Fuso orario con le regole di ora solare/legale (questo e' l'Italia).
// Altri fusi: https://github.com/nayarsystems/posix_tz_db/blob/master/zones.csv
constexpr const char *TIMEZONE = "CET-1CEST,M3.5.0,M10.5.0/3";

// --- Sincronizzazione Wi-Fi (nome e password in secrets.h) ---
constexpr const char *NTP_SERVER = "pool.ntp.org";
constexpr uint32_t WIFI_SYNC_INTERVAL_H = 24;   // ogni quante ore risincronizzare
constexpr uint32_t WIFI_RETRY_MIN = 15;         // se fallisce, riprova dopo N minuti
constexpr uint32_t WIFI_TIMEOUT_MS = 20000;     // tempo massimo per collegarsi
constexpr bool WIFI_AUTO_SYNC = true;           // sincronizzazione automatica (valore di partenza)

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

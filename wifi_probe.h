// =============================================================
//  wifi_probe.h - Scheda "Wi-Fi" del Radar: dispositivi che probano
//
//  Ascolto PASSIVO delle "probe request" Wi-Fi: i messaggi che i
//  dispositivi mandano per cercare le reti. L'orologio si mette in
//  "monitor mode", li ascolta e basta: non risponde, non si collega,
//  non disturba niente. Salta tra i canali per sentirli tutti.
//
//  Mostra la lista dei dispositivi (indirizzo MAC) e, toccandone uno,
//  i nomi di rete che quel dispositivo cerca ("(generica)" quando non
//  dichiara un nome). Pensato per prove in laboratorio sui PROPRI
//  dispositivi; i telefoni recenti usano un MAC casuale e per lo piu'
//  probe generiche, apposta per non farsi tracciare.
//
//  Comando dal Serial Monitor:
//      probe   -> stato e dispositivi sentiti
// =============================================================
#pragma once
#include <lvgl.h>
#include "watch_module.h"

class WifiProbe : public WatchModule {
public:
    const char *name() const override { return "wifiprobe"; }
    void update() override;
    bool canSleep() override { return !scanning; }
    bool onSerialCommand(const char *line) override;

    // Disegna la lista dentro "parent" (lo chiama l'app Radar)
    void build(lv_obj_t *parent);

    void start();                 // accende il monitor mode
    void stop(bool feedback = true);  // spegne e cancella tutto
    bool isScanning() const { return scanning; }

    // "Indietro": chiude la scheda dei dettagli se aperta
    bool onBack();

    // Dettagli di un dispositivo (usati dalle righe della lista)
    void showDetails(int index);
    void hideDetails();

private:
    void processQueue();
    void rebuildList();
    void buzz();

    bool scanning = false;
    uint32_t startedAt = 0;
    uint32_t lastHop = 0;
    uint8_t channel = 1;
    bool dirty = false;      // la lista va ridisegnata
    uint32_t lastRedraw = 0;
};

extern WifiProbe wifiProbe;

// =============================================================
//  radar_app.h - App "Radar": dispositivi nelle vicinanze
//
//  Due schede in alto (alte la meta' di quelle dell'app Tempo):
//    - Bluetooth: radar quadrato, scansione a riga stile sonar dei
//      dispositivi BLE (cuffie, telefoni, orologi, TV...). Solo ascolto.
//    - Wi-Fi: lista dei dispositivi che "probano" (vedi wifi_probe.*).
//
//  Il pulsante Avvia/Stop e' un quadratino in basso a destra, sopra
//  il radar. Avvia la scansione della scheda aperta; cambiando scheda
//  l'altra scansione si ferma (c'e' una sola radio). Lo schermo resta
//  acceso mentre una scansione e' in corso. Uscendo dall'app si ferma
//  tutto.
//
//  Scheda Bluetooth:
//    - distanza dal centro = potenza del segnale (vicino = al centro)
//    - direzione = finta ma fissa per ogni dispositivo
//    - colore = produttore (stesso produttore, stesso colore): i colori
//      sono quelli delle altre palette del Pokekron; vuoto = sconosciuto
//  Toccando un puntino: nome, produttore, indirizzo.
//
//  Comando dal Serial Monitor:
//      radar   -> stato e dispositivi Bluetooth trovati
// =============================================================
#pragma once
#include <stdint.h>
#include "watch_app.h"

class RadarApp : public WatchApp {
public:
    enum Tab { TAB_BT = 0, TAB_WIFI = 1 };

    const char *title() const override { return "Radar"; }
    const char *icon() const override { return LV_SYMBOL_BLUETOOTH; }  // non usata: c'e' drawIcon
    uint32_t color() const override { return 0x1B5E20; }
    bool drawIcon(lv_obj_t *tile) override;   // icona del radar nel menu
    bool hideTitle() const override { return true; }  // al posto del titolo ci sono le schede

    void update() override;
    void build(lv_obj_t *parent) override;
    void onClose() override;
    bool onBack() override;
    void onPaletteChanged() override;   // ricolora i puntini
    bool onSerialCommand(const char *line) override;
    bool canSleep() override;
    uint32_t screenTimeoutMs() const override;

    // Cambia scheda (ferma la scansione in corso)
    void showTab(Tab tab);

    // Pulsante Avvia/Stop: agisce sulla scheda aperta
    void toggleScan();
    bool anyScanning() const;

    // Scansione Bluetooth (scheda Bluetooth)
    void startScan();
    void stopScan(bool feedback = true);
    bool isScanning() const { return scanning; }

    // Scheda di dettaglio di un dispositivo Bluetooth (dai puntini)
    void showDetails(int index);
    void hideDetails();

private:
    void buildTabs(lv_obj_t *parent);
    void buildRadar(lv_obj_t *parent);   // disegna il radar dentro la pagina Bluetooth
    void processQueue();
    void removeOld();
    void updateSweep();
    void refreshUi();
    void buzz();

    Tab tab = TAB_BT;
    bool scanning = false;       // scansione Bluetooth
    uint32_t scanStartedAt = 0;
    uint32_t lastFrame = 0;
    uint32_t lastCleanup = 0;
};

extern RadarApp radarApp;

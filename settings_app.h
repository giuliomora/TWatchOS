// =============================================================
//  settings_app.h - App "Impostazioni"
//
//  Una lista che scorre in verticale:
//    Luminosita' (cursore), Colore schermo, Spegnimento quadrante, Spegnimento
//    menu e app, Gesto del polso, Tocco per accendere, Vibrazione
//    pulsanti, Suono, Obiettivo passi, Passi sul quadrante, Sincronizza
//    ora adesso, Ora automatica (Wi-Fi), Info, Ripristina.
//
//  "Info" apre una pagina a parte (batteria e sistema): si torna
//  alla lista con la freccia in alto o scorrendo verso destra.
//
//  Ogni modifica si salva subito (vedi user_settings.*).
// =============================================================
#pragma once
#include "watch_app.h"

class SettingsApp : public WatchApp {
public:
    const char *title() const override { return "Impostazioni"; }
    const char *icon() const override { return LV_SYMBOL_SETTINGS; }
    uint32_t color() const override { return 0x546E7A; }

    void build(lv_obj_t *parent) override;
    void update() override;
    void onOpen() override;
    void onClose() override;
    bool onBack() override;   // dalla pagina Info si torna alla lista

    // Usate dai controlli della lista
    void changeChoice(int id, int dir);
    void toggle(int id, bool on);
    void brightnessChanged(int value, bool released);
    void syncPressed();
    void showInfo(bool show);
    void resetPressed();

private:
    void refreshControls();
    void refreshSync();
    void refreshInfo();

    bool infoOpen = false;
    uint32_t resetArmedAt = 0;   // primo tocco su "Ripristina" (0 = nessuno)
    uint32_t lastRefresh = 0;
    uint32_t lastInfo = 0;
};

extern SettingsApp settingsApp;

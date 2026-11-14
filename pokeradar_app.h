// =============================================================
//  pokeradar_app.h - App "Pokeradar"
//
//  Come la ricerca strumenti del Pokekron: lo schermo e' un piano
//  cartesiano (15 x 17 caselle) con te al centro. Tocca un punto:
//  da li' parte un cerchio ("ping"). Se c'e' uno strumento nascosto
//  vicino al tocco, lampeggia per un attimo; se e' poco piu' in la',
//  il ping si ripete qualche volta (sei vicino, tocca accanto).
//
//  Gli strumenti non esistono davvero: compaiono a caso mentre
//  cammini (anche ad app chiusa), in un punto e a una distanza
//  casuali, e ogni passo li avvicina al centro. Quando uno arriva
//  nella casella centrale, trovalo con un ping: "Hai trovato...!".
//  L'orologio non ha la bussola: la direzione e' inventata, ma
//  resta fissa per ogni strumento.
//
//  Comparsa "imprevedibile": ogni POKERADAR_ROLL_STEPS passi si tira
//  il dado, con una fortuna che cambia piano piano, ondate di
//  fortuna, periodi di secca e ogni tanto uno sciame. Al massimo
//  POKERADAR_MAX_ITEMS strumenti insieme (config.h).
//
//  Comandi dal Serial Monitor:
//      pokeradar           -> strumenti nascosti, fortuna
//      pokeradar spawn     -> fa comparire uno strumento vicino
//      pokeradar passi 50  -> simula 50 passi
//      pokeradar reset     -> toglie tutti gli strumenti
// =============================================================
#pragma once
#include <stdint.h>
#include "watch_app.h"
#include "config.h"

class PokeradarApp : public WatchApp {
public:
    const char *title() const override { return "Pokeradar"; }
    const char *icon() const override { return LV_SYMBOL_GPS; }  // non usata: c'e' drawIcon
    bool drawIcon(lv_obj_t *tile) override;
    bool hideTitle() const override { return true; }   // area 240x200: c'e' piu' spazio per la griglia

    void begin() override;
    void update() override;
    void build(lv_obj_t *parent) override;
    void onOpen() override;
    void onClose() override;
    void onScreenOff() override;
    bool onSerialCommand(const char *line) override;

    // Tocco sulla griglia (coordinate dentro l'area dell'app)
    void pingAt(int x, int y);

private:
    struct Item {
        uint8_t used;
        uint16_t angle;    // direzione, gradi (0 = in alto)
        uint16_t dist;     // distanza dal centro, in passi
    };

    void addSteps(uint32_t steps);
    void roll();                       // tira il dado per la comparsa
    bool spawn(int minDist, int maxDist);
    int itemCount() const;
    bool itemTile(const Item &it, int &tx, int &ty) const;   // false = fuori dallo schermo
    void startPing(int x, int y);
    void reveal(int i);
    void showFound();
    void hideMessage();
    void load();
    void save();
    void print();

    Item items[cfg::POKERADAR_MAX_ITEMS] = {};
    float luck = 0.15f;        // probabilita' di comparsa a ogni tiro
    uint8_t mood = 0;          // 0 normale, 1 ondata di fortuna, 2 secca
    uint8_t moodLeft = 0;      // tiri rimasti all'ondata / alla secca
    uint32_t stepAcc = 0;      // passi accumulati per il prossimo tiro
    uint32_t lastSteps = 0;    // passi di oggi alla lettura precedente
    int16_t lastDay = -1;      // giorno dell'anno della lettura precedente
    bool dirty = false;
    uint32_t lastSave = 0;
    uint32_t lastCheck = 0;

    // Interfaccia
    uint32_t revealUntil[cfg::POKERADAR_MAX_ITEMS] = {};
    int foundIndex = -1;         // strumento trovato, in attesa del messaggio
    uint32_t foundAt = 0;
    uint32_t messageUntil = 0;
    int echoX = 0, echoY = 0;    // ping che si ripete (strumento vicino)
    int echoLeft = 0;
    uint32_t echoAt = 0;
};

extern PokeradarApp pokeradarApp;

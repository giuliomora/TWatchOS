// =============================================================
//  sound.h - Suoni sintetizzati con l'altoparlante
//
//  Niente file audio: i suoni sono note generate al momento
//  (onde sinusoidali con una piccola sfumatura), quindi non
//  pesano in memoria.
//
//  L'altoparlante consuma: va acceso solo quando serve.
//    sound.enable(true)   -> acceso (es. all'apertura di un'app)
//    sound.enable(false)  -> spento (es. alla chiusura)
//  Se in Impostazioni "Suono" e' spento, enable(true) non fa niente.
//
//  I suoni si scrivono a piccoli pezzi in update(), cosi'
//  l'interfaccia non si blocca mentre suonano.
//
//  Comandi dal Serial Monitor:
//      suono         -> prova il bip del Pokeradar
// =============================================================
#pragma once
#include <stdint.h>
#include "watch_module.h"

class Sound : public WatchModule {
public:
    const char *name() const override { return "sound"; }
    void update() override;
    bool onSerialCommand(const char *line) override;
    bool canSleep() override { return !playing; }

    void enable(bool on);
    bool isEnabled() const { return opened; }

    // Suoni pronti
    void ping();     // "pyuun" del radar

    // Una nota: frequenza che scorre da f0 a f1 Hz in ms millisecondi.
    // f0 = 0 -> pausa. Le note si mettono in coda (max 8).
    void note(uint16_t f0, uint16_t f1, uint16_t ms);

private:
    struct Note { uint16_t f0, f1, ms; };
    static constexpr int QUEUE = 8;
    Note queue[QUEUE];
    int qHead = 0, qCount = 0;

    bool opened = false;      // altoparlante acceso
    bool playing = false;
    uint32_t pos = 0;         // campione dentro la nota attuale
    uint32_t len = 0;         // campioni della nota attuale
    float phase = 0;
    int tail = 0;             // campioni di silenzio ancora da scrivere alla fine
};

extern Sound sound;

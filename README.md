# TWatchOS

Firmware modulare per lo smartwatch **LilyGO T-Watch S3**, scritto con Arduino IDE e [LilyGoLib](https://github.com/Xinyuan-LilyGO/LilyGoLib).

## Cosa fa adesso
- Mostra ora e data.
- Menu: tocca il quadrante per aprire la griglia con le app (per ora vuote); scorri verso destra o tocca la freccia in alto per tornare indietro.
- Passi: anello verso l'obiettivo (6.000, in `config.h`), km e calorie stimati, passi di ieri; i passi si vedono anche sul quadrante. Azzeramento a mezzanotte e vibrazione all'obiettivo.
- Batteria in alto a destra: grigia, verde con il fulmine in carica, rossa sotto il 20%.
- Lo schermo si accende ruotando il polso, toccando lo schermo o con un click breve sulla corona.
- Si spegne da solo dopo 5 secondi (modificabile in `config.h`).
- Sincronizza l'ora via Wi-Fi e gestisce da solo l'ora legale.
- A schermo spento mette il processore in sospensione per risparmiare batteria.

## Requisiti
- Arduino IDE 2 con **esp32 by Espressif** 3.3.x
- Librerie **LilyGoLib** e **LilyGoLib-ThirdParty** nella cartella `libraries`

## Come caricarlo
1. Copia la cartella `TWatchOS` nella cartella degli sketch
   (quella indicata in *File → Preferences → Sketchbook location*).
   Copia `secrets.example.h` in `secrets.h` (vedi sotto).
2. In Arduino IDE: *File → Open…* → `TWatchOS/TWatchOS.ino`.
   Gli altri file compaiono come schede in alto.
3. Impostazioni in *Tools*: le stesse dell'esempio helloworld.
4. Premi **Upload**.

## Wi-Fi (ora automatica)
Copia `secrets.example.h` in `secrets.h` e scrivi nome e password della rete (solo reti a 2.4 GHz).
`secrets.h` è escluso da Git, quindi la password non viene pubblicata.
All'avvio e poi ogni 24 ore l'orologio si collega, prende l'ora esatta e spegne il Wi-Fi.
Comandi nel Serial Monitor: `sync` (sincronizza subito), `wifi` (stato).

## Risparmio energetico
A schermo spento il processore va in sospensione e si risveglia con polso, tocco o corona.
Con il cavo USB collegato la sospensione è in pausa (per non disturbare il Serial Monitor).
Comandi: `power` (batteria e statistiche), `power reset` (azzera le statistiche).

## Gesto del polso
Il sensore usa l'algoritmo per orologi (`WRIST_MODE_WRISTBAND` in `config.h`).
Comando `polso`: accende/spegne i messaggi di prova; a ogni gesto riconosciuto stampa l'inclinazione dell'orologio (0 gradi = schermo verso l'alto).

## Passi
Li conta il sensore di movimento da solo, anche a schermo spento.
Comandi: `passi` (passi di oggi e di ieri), `passi vibra` (prova la vibrazione dell'obiettivo).

## Regolare l'ora a mano
1. Apri *Tools → Serial Monitor*, imposta **115200 baud** e **Newline**.
2. Scrivi, per esempio, `ora 2026-10-22 15:30:00` e premi Invio.

L'ora resta salvata nell'orologio (RTC) finché la batteria non si scarica del tutto.

## Struttura
| File | A cosa serve |
|---|---|
| `TWatchOS.ino` | avvio: registra i moduli |
| `config.h` | tutte le impostazioni |
| `watch_module.h` | interfaccia comune dei moduli |
| `app.*` | fa girare i moduli e legge i comandi seriali |
| `screen.*` | accende/spegne lo schermo |
| `inputs.*` | polso, tocco, corona |
| `time_service.*` | ora (RTC) |
| `watchface.*` | quadrante |
| `status_bar.*` | batteria in alto a destra |
| `launcher.*` | navigazione quadrante → menu → app |
| `watch_app.h` | interfaccia comune delle app |
| `steps_app.*` | app Passi |
| `empty_app.*` | app vuote (segnaposto) |
| `wifi_time.*` | ora automatica via Wi-Fi |
| `secrets.example.h` | modello per `secrets.h` (nome e password del Wi-Fi) |
| `power.*` | risparmio energetico |

## Aggiungere una funzionalità
1. Crea `nome.h` e `nome.cpp`, con una classe che eredita da `WatchModule`
   (usa `watchface` come esempio).
2. Nel file `.ino`: `#include "nome.h"` e `App::add(&nome);` nel `setup()`.

Metodi disponibili in un modulo:
`begin()`, `update()`, `onScreenOn()`, `onScreenOff()`, `onSerialCommand(riga)`.
Per accendere lo schermo da qualunque modulo: `screen.wake("motivo");`.

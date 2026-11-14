# TWatchOS

Firmware modulare per lo smartwatch **LilyGO T-Watch S3**, scritto con Arduino IDE e [LilyGoLib](https://github.com/Xinyuan-LilyGO/LilyGoLib).

## Cosa fa adesso
- Tema grafico in stile **Pokekron** (Pokemon Platino): schermo LCD (verde o uno degli altri 7 colori del Pokekron, da Impostazioni > Colore schermo), cifre a blocchi, testo pixel, riquadri con il bordo scuro. Toccando il quadrante lo schermo si schiarisce come la retroilluminazione del Pokekron. In basso a sinistra c'e' una figura in pixel art (per ora un gufetto segnaposto, si cambia in `tools/fontgen/glyphs.py`).
- Mostra ora e data.
- Menu: tocca il quadrante per aprire la griglia con le app, 4 per pagina (pagina 1: Passi, Tempo, Radar, Pokeradar; pagina 2: Impostazioni). Scorri in su o in giu' per cambiare pagina (i quadratini a destra dicono dove sei); scorri verso destra o tocca la freccia in alto per tornare indietro.
- Passi: anello verso l'obiettivo (6.000, in `config.h`), km e calorie stimati, passi di ieri; i passi si vedono anche sul quadrante. Azzeramento a mezzanotte e vibrazione all'obiettivo.
- App Tempo (icona dell'orologio), con due schede in alto:
  - Cronometro: Avvia/Pausa, Giro (si vede l'ultimo), Stop.
  - Timer: 5, 10, 15 minuti o "Altro" (rotelle minuti/secondi); allo scadere vibra finché non tocchi Stop.
  - Contano anche a schermo spento e compaiono sul quadrante in alto a sinistra.
- Impostazioni: luminosita', colore schermo, suono, spegnimento schermo, polso, tocco, vibrazione, obiettivo passi, ora via Wi-Fi, pagina Info (batteria e sistema), ripristino. Si salvano in memoria.
- App Radar: premi Avvia e sul radar compaiono i dispositivi Bluetooth vicini (cuffie, telefoni, orologi, TV...), come quadratini colorati per produttore (stesso produttore, stesso colore, presi dalle altre palette del Pokekron; vuoti se il produttore non e' noto); toccandone uno vedi nome, produttore, tipo e indirizzo. Solo ascolto. Il Bluetooth è acceso solo durante la scansione: si ferma con Stop o uscendo dall'app. Comando: `radar`.
- App Pokeradar: come la ricerca strumenti del Pokekron. Piano cartesiano con te al centro: tocca un punto e parte un cerchio; se c'e' uno strumento nascosto vicino lampeggia, se e' poco piu' in la' il cerchio si ripete. Gli strumenti compaiono a caso mentre cammini (al massimo 3) e ogni passo li avvicina al centro: quando uno e' nella casella centrale, trovalo con un tocco (compare "Hai trovato...!"). Il tocco fa il suono del radar (si spegne da Impostazioni > Suono). Comandi: `pokeradar`, `pokeradar spawn`, `pokeradar passi 50`, `pokeradar reset`, `suono`.
- Batteria in alto a destra: con il fulmine in carica, in negativo (chiara su fondo scuro) sotto il 20%.
- Lo schermo si accende ruotando il polso, toccando lo schermo o con un click breve sulla corona.
- Si spegne da solo dopo 5 secondi (modificabile in `config.h`).
- Sincronizza l'ora via Wi-Fi e gestisce da solo l'ora legale.
- A schermo spento mette il processore in sospensione per risparmiare batteria.

## Requisiti
- Arduino IDE 2 con **esp32 by Espressif** 3.3.x
- Librerie **LilyGoLib** e **LilyGoLib-ThirdParty** nella cartella `libraries`
- Libreria **NimBLE-Arduino** 2.x (per l'app Radar)

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

## Cronometro e Timer
Comandi: `crono` (stato del cronometro), `timer` (stato del timer), `timer 10` (timer di prova di 10 secondi).
Impostazioni in `config.h` (vibrazione, pulsanti rapidi, durata dello squillo).

## Impostazioni
I valori di `config.h` sono quelli di partenza; quelli scelti sull'orologio restano salvati anche dopo un riavvio.
Comandi: `impostazioni` (valori attuali), `impostazioni reset` (torna ai valori di `config.h`).

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
| `stopwatch_app.*` | app Tempo (cronometro + schede) |
| `countdown_timer.*` | timer (scheda dell'app Tempo) |
| `settings_app.*` | app Impostazioni (lista e pagina Info) |
| `user_settings.*` | impostazioni salvate in memoria |
| `radar_app.*` | app Radar (dispositivi Bluetooth vicini) |
| `pokeradar_app.*` | app Pokeradar (ricerca strumenti nascosti, come nel gioco) |
| `sound.*` | suoni sintetizzati con l'altoparlante (niente file audio) |
| `empty_app.*` | modello di app vuota (non usato) |
| `wifi_time.*` | ora automatica via Wi-Fi |
| `secrets.example.h` | modello per `secrets.h` (nome e password del Wi-Fi) |
| `power.*` | risparmio energetico |
| `theme.*` | tema grafico "Pokekron": le 8 palette dello schermo LCD (con cambio colore dal vivo) e stili di pulsanti, schede, interruttori... |
| `pk_fonts.c` | font pixel del tema (file generato, 1 bit per pixel: circa 10 KB in tutto) |
| `tools/fontgen/` | generatore di `pk_fonts.c` (Python): i caratteri sono disegnati in `glyphs.py` |

## Tema grafico
Colori e stili sono in `theme.h`: per cambiare la palette basta modificare i valori in cima al file.
Nelle nuove app usa le funzioni `pk::styleButton`, `pk::styleLabel`, `pk::styleBox`... e i font
`pk_font_small`, `pk_font_text`, `pk_font_big` (testo) e `pk_digits_s/m/l` (cifre a blocchi).
I simboli LVGL (`LV_SYMBOL_...`) funzionano anche con i font pixel: vengono presi da Montserrat.
Per ridisegnare un carattere: modifica `tools/fontgen/glyphs.py` e, dalla cartella del progetto,
lancia `python tools/fontgen/gen.py .` (rigenera `pk_fonts.c`).

## Aggiungere una funzionalità
1. Crea `nome.h` e `nome.cpp`, con una classe che eredita da `WatchModule`
   (usa `watchface` come esempio).
2. Nel file `.ino`: `#include "nome.h"` e `App::add(&nome);` nel `setup()`.

Metodi disponibili in un modulo:
`begin()`, `update()`, `onScreenOn()`, `onScreenOff()`, `onSerialCommand(riga)`.
Per accendere lo schermo da qualunque modulo: `screen.wake("motivo");`.

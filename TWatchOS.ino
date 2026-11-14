// =============================================================
//  TWatchOS - firmware modulare per LilyGO T-Watch S3
//
//  Struttura del progetto (ogni file e' una scheda nell'IDE):
//    config.h          impostazioni (timeout, luminosita', ...)
//    watch_module.h    interfaccia comune dei moduli
//    app.h/.cpp        motore che fa girare i moduli
//    screen.h/.cpp     accensione/spegnimento automatico schermo
//    inputs.h/.cpp     polso, tocco, corona -> accendono lo schermo
//    time_service.*    ora dall'RTC + comando seriale "ora"
//    watchface.*       quadrante con ora e data
//    status_bar.*      batteria in alto a destra
//    launcher.*        navigazione quadrante -> menu -> app
//    watch_app.h       interfaccia comune delle app
//    steps_app.*       app Passi (contapassi, anche sul quadrante)
//    stopwatch_app.*   app Tempo: cronometro + schede
//    countdown_timer.* timer (scheda dell'app Tempo)
//    settings_app.*    app Impostazioni
//    user_settings.*   impostazioni salvate in memoria
//    radar_app.*       app Radar (dispositivi Bluetooth vicini)
//    pokeradar_app.*   app Pokeradar (ricerca strumenti nascosti, come nel gioco)
//    sound.*           suoni sintetizzati con l'altoparlante
//    empty_app.*       modello di app vuota (per ora non usato)
//    wifi_time.*       sincronizzazione automatica dell'ora via Wi-Fi
//    secrets.h         nome e password del Wi-Fi
//    power.*           risparmio energetico (sospensione a schermo spento)
//    theme.*           tema grafico "Pokekron" (colori LCD e stili)
//    pk_fonts.c        font pixel del tema (generato)
//
//  Per aggiungere una funzionalita':
//    1. crea nuovo_modulo.h / nuovo_modulo.cpp (copia watchface come esempio)
//    2. includilo qui sotto
//    3. registralo con App::add(&nuovoModulo) nel setup()
// =============================================================

#include <LilyGoLib.h>
#include <LV_Helper.h>

#include "app.h"
#include "user_settings.h"
#include "screen.h"
#include "inputs.h"
#include "time_service.h"
#include "watchface.h"
#include "status_bar.h"
#include "launcher.h"
#include "steps_app.h"
#include "stopwatch_app.h"
#include "countdown_timer.h"
#include "settings_app.h"
#include "radar_app.h"
#include "pokeradar_app.h"
#include "sound.h"
#include "wifi_probe.h"
#include "wifi_time.h"
#include "power.h"

void setup()
{
    Serial.begin(115200);

    // Hardware e grafica
    instance.begin();
    beginLvglHelper(instance);

    // Moduli: vengono avviati in questo ordine.
    // Lo schermo va per ultimo, cosi' quando si accende
    // il quadrante e' gia' pronto.
    App::add(&userSettings);   // per primo: gli altri moduli leggono le impostazioni
    App::add(&timeService);
    App::add(&wifiTime);
    App::add(&watchface);
    App::add(&statusBar);

    App::add(&sound);          // altoparlante (lo usano le app)

    // App del menu, in questo ordine (4 per pagina, griglia 2x2)
    launcher.addApp(&stepsApp);       // pagina 1
    launcher.addApp(&stopwatchApp);
    launcher.addApp(&radarApp);
    launcher.addApp(&pokeradarApp);
    launcher.addApp(&settingsApp);    // pagina 2 (in fondo)
    App::add(&countdownTimer);   // il timer (lo disegna l'app Tempo)
    App::add(&wifiProbe);        // scheda Wi-Fi del Radar (la disegna l'app Radar)
    App::add(&launcher);

    App::add(&inputs);
    App::add(&screen);
    App::add(&power);   // per ultimo: decide se dormire dopo che tutti hanno lavorato

    App::begin();
}

void loop()
{
    instance.loop();     // eventi hardware (sensore, corona, batteria)
    App::update();       // tutti i moduli
    lv_timer_handler();  // disegno dell'interfaccia
    delay(5);
}

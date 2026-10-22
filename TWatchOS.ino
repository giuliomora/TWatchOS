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
//    wifi_time.*       sincronizzazione automatica dell'ora via Wi-Fi
//    secrets.h         nome e password del Wi-Fi
//    power.*           risparmio energetico (sospensione a schermo spento)
//
//  Per aggiungere una funzionalita':
//    1. crea nuovo_modulo.h / nuovo_modulo.cpp (copia watchface come esempio)
//    2. includilo qui sotto
//    3. registralo con App::add(&nuovoModulo) nel setup()
// =============================================================

#include <LilyGoLib.h>
#include <LV_Helper.h>

#include "app.h"
#include "screen.h"
#include "inputs.h"
#include "time_service.h"
#include "watchface.h"
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
    App::add(&timeService);
    App::add(&wifiTime);
    App::add(&watchface);
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

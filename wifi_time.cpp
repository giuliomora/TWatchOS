#include <Arduino.h>
#include <WiFi.h>
#include <esp_sntp.h>
#include <string.h>
#include <strings.h>
#include "wifi_time.h"
#include "time_service.h"
#include "app.h"
#include "config.h"
#include "secrets.h"

WifiTime wifiTime;

// Impostato dalla libreria quando arriva l'ora dal server
static volatile bool timeReceived = false;
static void onTimeSync(struct timeval *) { timeReceived = true; }

void WifiTime::begin()
{
    enabled = strlen(secrets::WIFI_SSID) > 0;
    if (!enabled) {
        App::log(name(), "Disattivato: inserisci nome e password del Wi-Fi in secrets.h");
        return;
    }
    sntp_set_time_sync_notification_cb(onTimeSync);
    syncNow();  // prima sincronizzazione all'avvio
}

void WifiTime::syncNow()
{
    if (!enabled || state != IDLE) return;
    App::log(name(), "Connessione a \"%s\"...", secrets::WIFI_SSID);
    timeReceived = false;
    WiFi.mode(WIFI_STA);
    WiFi.begin(secrets::WIFI_SSID, secrets::WIFI_PASSWORD);
    state = CONNECTING;
    stateSince = millis();
}

void WifiTime::update()
{
    if (!enabled) return;
    uint32_t now = millis();

    switch (state) {
    case IDLE:
        if ((int32_t)(now - nextAttempt) >= 0 && nextAttempt != 0) {
            nextAttempt = 0;
            syncNow();
        }
        break;

    case CONNECTING:
        if (WiFi.status() == WL_CONNECTED) {
            App::log(name(), "Connesso, chiedo l'ora a %s", cfg::NTP_SERVER);
            configTzTime(cfg::TIMEZONE, cfg::NTP_SERVER, "time.google.com");
            state = WAITING_TIME;
            stateSince = now;
        } else if (now - stateSince > cfg::WIFI_TIMEOUT_MS) {
            stop(false, "rete non raggiungibile (nome/password giusti? rete a 2.4 GHz?)");
        }
        break;

    case WAITING_TIME:
        if (timeReceived) {
            timeService.saveSystemTimeToRtc();
            struct tm t = timeService.now();
            char msg[64];
            snprintf(msg, sizeof(msg), "ora sincronizzata: %02d:%02d:%02d",
                     t.tm_hour, t.tm_min, t.tm_sec);
            stop(true, msg);
        } else if (now - stateSince > cfg::WIFI_TIMEOUT_MS) {
            stop(false, "il server dell'ora non risponde");
        }
        break;
    }
}

void WifiTime::stop(bool success, const char *msg)
{
    esp_sntp_stop();
    WiFi.disconnect(true);
    WiFi.mode(WIFI_OFF);
    state = IDLE;

    uint32_t delayMs;
    if (success) {
        everSynced = true;
        lastSyncOk = millis();
        delayMs = cfg::WIFI_SYNC_INTERVAL_H * 3600UL * 1000UL;
        App::log(name(), "OK, %s. Wi-Fi spento.", msg);
    } else {
        delayMs = cfg::WIFI_RETRY_MIN * 60UL * 1000UL;
        App::log(name(), "Fallito: %s. Riprovo tra %u minuti.", msg,
                 (unsigned)cfg::WIFI_RETRY_MIN);
    }
    nextAttempt = millis() + delayMs;
    if (nextAttempt == 0) nextAttempt = 1;
}

uint32_t WifiTime::msUntilNextWork()
{
    if (!enabled || nextAttempt == 0) return UINT32_MAX;
    int32_t left = (int32_t)(nextAttempt - millis());
    return left > 0 ? (uint32_t)left : 0;
}

bool WifiTime::onSerialCommand(const char *line)
{
    if (strcasecmp(line, "sync") == 0) {
        if (!enabled) App::log(name(), "Disattivato: compila secrets.h");
        else if (state != IDLE) App::log(name(), "Sincronizzazione gia' in corso");
        else syncNow();
        return true;
    }
    if (strcasecmp(line, "wifi") == 0) {
        if (!enabled) {
            App::log(name(), "Disattivato: compila secrets.h");
        } else if (!everSynced) {
            App::log(name(), "Mai sincronizzato finora");
        } else {
            App::log(name(), "Ultima sincronizzazione %lu minuti fa",
                     (unsigned long)((millis() - lastSyncOk) / 60000UL));
        }
        return true;
    }
    return false;
}

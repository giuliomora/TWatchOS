#include <LilyGoLib.h>
#include <WiFi.h>
#include "esp_wifi.h"
#include <stdio.h>
#include <string.h>
#include "wifi_probe.h"
#include "launcher.h"
#include "screen.h"
#include "app.h"
#include "config.h"
#include "user_settings.h"
#include "theme.h"

WifiProbe wifiProbe;

// Colori: quelli del tema (theme.h). Le righe con MAC casuale
// hanno lo sfondo un po' piu' scuro.

// ---- Dispositivi sentiti --------------------------------------
struct Prober {
    bool used;
    uint8_t mac[6];
    bool randomMac;                              // MAC casuale (bit locale)
    char ssids[cfg::RADAR_PROBE_MAX_SSID][33];   // reti cercate ("" = generica)
    uint8_t ssidCount;
    bool hasGeneric;                             // ha mandato almeno una probe senza nome
    uint32_t lastSeen;
};
static Prober probers[cfg::RADAR_PROBE_MAX_DEVICES];
static int proberCount = 0;

// ---- Coda tra il monitor Wi-Fi e il loop() --------------------
// Il callback gira nel task Wi-Fi, in parallelo al loop(): mette qui
// i dati grezzi, il loop() li elabora. LVGL va usato solo dal loop().
struct ProbeSeen {
    uint8_t mac[6];
    char ssid[33];   // "" = generica (senza nome)
};
static const int QUEUE_LEN = 32;
static ProbeSeen queue[QUEUE_LEN];
static int qHead = 0, qTail = 0;
static portMUX_TYPE qMux = portMUX_INITIALIZER_UNLOCKED;
static volatile bool acceptFrames = false;

// Callback del monitor mode: chiamato per ogni pacchetto "di servizio".
// Tiene solo le probe request e ne estrae MAC e nome di rete cercato.
static void sniffer(void *buf, wifi_promiscuous_pkt_type_t type)
{
    if (!acceptFrames || type != WIFI_PKT_MGMT) return;
    const wifi_promiscuous_pkt_t *pkt = (wifi_promiscuous_pkt_t *)buf;
    const uint8_t *p = pkt->payload;
    const int len = pkt->rx_ctrl.sig_len;
    if (len < 26) return;
    if ((p[0] & 0xFC) != 0x40) return;   // non e' una probe request

    ProbeSeen s;
    memset(&s, 0, sizeof(s));
    memcpy(s.mac, p + 10, 6);            // indirizzo del mittente (Addr2)

    // Parametri "a tag": il primo (id 0) e' il nome di rete (SSID)
    if (p[24] == 0) {
        int sl = p[25];
        if (sl > 32) sl = 32;
        if (26 + sl <= len) memcpy(s.ssid, p + 26, sl);
        s.ssid[sl] = 0;
    }

    portENTER_CRITICAL_ISR(&qMux);
    const int next = (qHead + 1) % QUEUE_LEN;
    if (next != qTail) {
        queue[qHead] = s;
        qHead = next;
    }
    portEXIT_CRITICAL_ISR(&qMux);
}

// ---- Interfaccia ----------------------------------------------
static lv_obj_t *list = nullptr;        // contenitore scorrevole delle righe
static lv_obj_t *rows[cfg::RADAR_PROBE_MAX_DEVICES] = {};
static lv_obj_t *emptyLabel = nullptr;  // "Premi Avvia" / "Nessuno ancora"
static lv_obj_t *card = nullptr;
static lv_obj_t *cardTitle = nullptr;
static lv_obj_t *cardInfo = nullptr;

// ---- Funzioni di supporto -------------------------------------

static void macToStr(const uint8_t *m, char *out)
{
    snprintf(out, 18, "%02X:%02X:%02X:%02X:%02X:%02X", m[0], m[1], m[2], m[3], m[4], m[5]);
}

// Il font non ha accenti e simboli: li sostituisce con '?'
static void cleanSsid(char *s)
{
    for (; *s; s++)
        if ((uint8_t)*s < 32 || (uint8_t)*s > 126) *s = '?';
}

static void onRowClicked(lv_event_t *e)
{
    if (!launcher.clickAllowed()) return;
    wifiProbe.showDetails((int)(intptr_t)lv_event_get_user_data(e));
}

static void onCardClicked(lv_event_t * /*e*/)
{
    if (!launcher.clickAllowed()) return;
    wifiProbe.hideDetails();
}

// ---- Costruzione della schermata ------------------------------

void WifiProbe::build(lv_obj_t *parent)
{
    // Lista scorrevole
    list = lv_obj_create(parent);
    lv_obj_set_size(list, lv_pct(100), lv_pct(100));
    lv_obj_set_pos(list, 0, 0);
    pk::styleClear(list);
    pk::styleScrollbar(list);
    lv_obj_set_style_pad_all(list, 6, 0);
    lv_obj_set_style_pad_row(list, 6, 0);
    lv_obj_set_style_pad_bottom(list, 54, 0);   // spazio sotto per il pulsante d'angolo
    lv_obj_set_flex_flow(list, LV_FLEX_FLOW_COLUMN);

    // Scritta al centro quando non c'e' niente
    emptyLabel = lv_label_create(parent);
    pk::styleLabel(emptyLabel, &pk_font_small, pk::DIM);
    lv_label_set_text(emptyLabel, "Premi Avvia");
    lv_obj_align(emptyLabel, LV_ALIGN_CENTER, 0, -10);

    // Scheda dettagli (sopra la lista, nascosta)
    card = lv_obj_create(parent);
    lv_obj_set_size(card, 224, 170);
    lv_obj_align(card, LV_ALIGN_CENTER, 0, -6);
    pk::styleBox(card, 3);
    lv_obj_set_style_pad_all(card, 10, 0);
    lv_obj_set_style_pad_row(card, 2, 0);
    lv_obj_set_flex_flow(card, LV_FLEX_FLOW_COLUMN);
    lv_obj_add_event_cb(card, onCardClicked, LV_EVENT_CLICKED, nullptr);

    cardTitle = lv_label_create(card);
    lv_obj_set_width(cardTitle, 200);
    lv_label_set_long_mode(cardTitle, LV_LABEL_LONG_DOT);
    pk::styleLabel(cardTitle, &pk_font_small);

    cardInfo = lv_label_create(card);
    lv_obj_set_width(cardInfo, 200);
    pk::styleLabel(cardInfo, &pk_font_small);
    lv_obj_set_style_text_line_space(cardInfo, 4, 0);
    lv_obj_add_flag(card, LV_OBJ_FLAG_HIDDEN);
}

// Ridisegna le righe della lista a partire dai dispositivi sentiti
void WifiProbe::rebuildList()
{
    if (!list) return;
    char mac[18];
    int shown = 0;
    for (int i = 0; i < cfg::RADAR_PROBE_MAX_DEVICES; i++) {
        Prober &pr = probers[i];
        if (!pr.used) {
            if (rows[i]) { lv_obj_delete(rows[i]); rows[i] = nullptr; }
            continue;
        }
        if (!rows[i]) {
            rows[i] = lv_button_create(list);
            lv_obj_set_width(rows[i], lv_pct(100));
            lv_obj_set_height(rows[i], LV_SIZE_CONTENT);
            pk::styleButton(rows[i]);
            lv_obj_set_style_pad_all(rows[i], 8, 0);
            lv_obj_add_event_cb(rows[i], onRowClicked, LV_EVENT_CLICKED, (void *)(intptr_t)i);

            lv_obj_t *l = lv_label_create(rows[i]);
            pk::styleLabel(l, &pk_font_small);
            lv_obj_center(l);
        }
        // Aggiorna il testo della riga
        macToStr(pr.mac, mac);
        const int nets = pr.ssidCount;
        lv_obj_t *l = lv_obj_get_child(rows[i], 0);
        // MAC casuale: riga con lo sfondo piu' scuro
        lv_obj_set_style_bg_color(rows[i], pk::color(pr.randomMac ? pk::SHADE : pk::BG), 0);
        if (nets > 0)
            lv_label_set_text_fmt(l, "%s\n%d reti%s", mac, nets, pr.randomMac ? "  (casuale)" : "");
        else
            lv_label_set_text_fmt(l, "%s\ngenerica%s", mac, pr.randomMac ? "  (casuale)" : "");
        shown++;
    }
    if (emptyLabel) {
        if (shown > 0) lv_obj_add_flag(emptyLabel, LV_OBJ_FLAG_HIDDEN);
        else {
            lv_label_set_text(emptyLabel, scanning ? "Nessuno ancora" : "Premi Avvia");
            lv_obj_remove_flag(emptyLabel, LV_OBJ_FLAG_HIDDEN);
        }
    }
}

// ---- Scansione ------------------------------------------------

void WifiProbe::start()
{
    if (scanning) return;

    // Azzera i dati e la coda
    portENTER_CRITICAL(&qMux);
    qHead = qTail = 0;
    portEXIT_CRITICAL(&qMux);
    for (int i = 0; i < cfg::RADAR_PROBE_MAX_DEVICES; i++) {
        if (rows[i]) { lv_obj_delete(rows[i]); rows[i] = nullptr; }
        probers[i].used = false;
    }
    proberCount = 0;

    // Monitor mode: ascolta i pacchetti "di servizio" (management)
    WiFi.mode(WIFI_MODE_STA);
    WiFi.disconnect(false, true);
    esp_wifi_set_promiscuous(false);
    wifi_promiscuous_filter_t filter = {};
    filter.filter_mask = WIFI_PROMIS_FILTER_MASK_MGMT;
    esp_wifi_set_promiscuous_filter(&filter);
    esp_wifi_set_promiscuous_rx_cb(&sniffer);
    channel = 1;
    esp_wifi_set_channel(channel, WIFI_SECOND_CHAN_NONE);
    acceptFrames = true;
    esp_wifi_set_promiscuous(true);

    scanning = true;
    startedAt = lastHop = millis();
    App::log(name(), "Ascolto probe avviato");
    buzz();
    rebuildList();
    if (screen.isOn()) screen.setTimeout(0);   // resta acceso durante l'ascolto
}

void WifiProbe::stop(bool feedback)
{
    const bool was = scanning;
    acceptFrames = false;
    scanning = false;
    esp_wifi_set_promiscuous(false);
    esp_wifi_set_promiscuous_rx_cb(nullptr);
    WiFi.mode(WIFI_MODE_NULL);

    portENTER_CRITICAL(&qMux);
    qHead = qTail = 0;
    portEXIT_CRITICAL(&qMux);
    for (int i = 0; i < cfg::RADAR_PROBE_MAX_DEVICES; i++) {
        if (rows[i]) { lv_obj_delete(rows[i]); rows[i] = nullptr; }
        probers[i].used = false;
    }
    proberCount = 0;
    hideDetails();
    rebuildList();

    if (was) {
        App::log(name(), "Ascolto probe fermato");
        if (feedback) buzz();
    }
}

void WifiProbe::processQueue()
{
    bool changed = false;
    for (int n = 0; n < 24; n++) {
        ProbeSeen s;
        portENTER_CRITICAL(&qMux);
        const bool empty = (qTail == qHead);
        if (!empty) {
            s = queue[qTail];
            qTail = (qTail + 1) % QUEUE_LEN;
        }
        portEXIT_CRITICAL(&qMux);
        if (empty) break;

        cleanSsid(s.ssid);
        const uint32_t now = millis();

        // Dispositivo gia' sentito?
        int idx = -1;
        for (int i = 0; i < cfg::RADAR_PROBE_MAX_DEVICES; i++)
            if (probers[i].used && memcmp(probers[i].mac, s.mac, 6) == 0) { idx = i; break; }

        if (idx < 0) {
            for (int i = 0; i < cfg::RADAR_PROBE_MAX_DEVICES; i++)
                if (!probers[i].used) { idx = i; break; }
            if (idx < 0) {   // pieno: scarta il piu' vecchio
                idx = 0;
                for (int i = 1; i < cfg::RADAR_PROBE_MAX_DEVICES; i++)
                    if ((int32_t)(probers[i].lastSeen - probers[idx].lastSeen) < 0) idx = i;
                if (rows[idx]) { lv_obj_delete(rows[idx]); rows[idx] = nullptr; }
            } else {
                proberCount++;
            }
            Prober &pr = probers[idx];
            memset(&pr, 0, sizeof(pr));
            pr.used = true;
            memcpy(pr.mac, s.mac, 6);
            pr.randomMac = (s.mac[0] & 0x02) != 0;   // bit "locally administered" = MAC casuale
            changed = true;
        }

        Prober &pr = probers[idx];
        pr.lastSeen = now;
        if (s.ssid[0]) {
            bool have = false;
            for (int k = 0; k < pr.ssidCount; k++)
                if (strcmp(pr.ssids[k], s.ssid) == 0) { have = true; break; }
            if (!have && pr.ssidCount < cfg::RADAR_PROBE_MAX_SSID) {
                strlcpy(pr.ssids[pr.ssidCount], s.ssid, sizeof(pr.ssids[0]));
                pr.ssidCount++;
                changed = true;
            }
        } else {
            pr.hasGeneric = true;
        }
    }
    if (changed) dirty = true;
}

void WifiProbe::update()
{
    if (!scanning) return;
    processQueue();
    const uint32_t now = millis();

    // Salta tra i canali per sentire le probe su tutte le frequenze
    if (now - lastHop >= cfg::RADAR_PROBE_HOP_MS) {
        lastHop = now;
        channel++;
        if (channel > cfg::RADAR_PROBE_CHANNELS) channel = 1;
        esp_wifi_set_channel(channel, WIFI_SECOND_CHAN_NONE);
    }

    // Ridisegna la lista al massimo ~3 volte al secondo
    if (dirty && now - lastRedraw >= 300) {
        lastRedraw = now;
        dirty = false;
        rebuildList();
    }
}

// ---- Scheda dei dettagli --------------------------------------

void WifiProbe::showDetails(int index)
{
    if (!card || index < 0 || index >= cfg::RADAR_PROBE_MAX_DEVICES || !probers[index].used) return;
    const Prober &pr = probers[index];
    char mac[18];
    macToStr(pr.mac, mac);
    lv_label_set_text_fmt(cardTitle, "%s%s", mac, pr.randomMac ? "  (casuale)" : "");

    char buf[400];
    int off = 0;
    off += snprintf(buf + off, sizeof(buf) - off, "Reti cercate:");
    if (pr.ssidCount == 0 && pr.hasGeneric)
        off += snprintf(buf + off, sizeof(buf) - off, "\n  (solo probe generiche)");
    for (int k = 0; k < pr.ssidCount && off < (int)sizeof(buf) - 40; k++)
        off += snprintf(buf + off, sizeof(buf) - off, "\n  %s", pr.ssids[k]);
    if (pr.ssidCount > 0 && pr.hasGeneric)
        off += snprintf(buf + off, sizeof(buf) - off, "\n  (generica)");
    lv_label_set_text(cardInfo, buf);

    lv_obj_remove_flag(card, LV_OBJ_FLAG_HIDDEN);
}

void WifiProbe::hideDetails()
{
    if (card) lv_obj_add_flag(card, LV_OBJ_FLAG_HIDDEN);
}

bool WifiProbe::onBack()
{
    if (card && !lv_obj_has_flag(card, LV_OBJ_FLAG_HIDDEN)) {
        hideDetails();
        return true;
    }
    return false;
}

void WifiProbe::buzz()
{
    if (!userSettings.buttonVibrate) return;
    instance.drv.setWaveform(0, 1);
    instance.drv.setWaveform(1, 0);
    instance.drv.run();
}

bool WifiProbe::onSerialCommand(const char *line)
{
    if (strcmp(line, "probe") != 0) return false;
    if (!scanning) {
        App::log(name(), "Fermo (si avvia dalla scheda Wi-Fi del Radar)");
        return true;
    }
    App::log(name(), "In ascolto da %lu s, %d dispositivi:",
             (unsigned long)((millis() - startedAt) / 1000), proberCount);
    char mac[18];
    for (const Prober &pr : probers) {
        if (!pr.used) continue;
        macToStr(pr.mac, mac);
        App::log(name(), "  %s%s  %d reti", mac, pr.randomMac ? " (casuale)" : "", pr.ssidCount);
        for (int k = 0; k < pr.ssidCount; k++)
            App::log(name(), "      %s", pr.ssids[k]);
    }
    return true;
}

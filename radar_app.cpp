#include <LilyGoLib.h>
#include <NimBLEDevice.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include "radar_app.h"
#include "wifi_probe.h"
#include "launcher.h"
#include "screen.h"
#include "app.h"
#include "config.h"
#include "user_settings.h"
#include "theme.h"

RadarApp radarApp;

// ---- Schede (meta' dell'altezza di quelle dell'app Tempo) ------
static const int TABS_H = 16;              // le schede dell'orologio sono 30
static const int PAGE_H = 200 - TABS_H;    // altezza delle pagine sotto le schede

// ---- Geometria del radar (dentro la pagina Bluetooth: 240xPAGE_H)
static const int SQ = 176;                 // lato del quadrato
static const int CX = 120, CY = 92;        // centro
static const int LEFT = CX - SQ / 2;       // 32
static const int TOP = CY - SQ / 2;        // 4
static const int RIGHT = CX + SQ / 2;      // 208
static const int BOTTOM = CY + SQ / 2;     // 180
static const int DOT = 12;                 // lato dei puntini (bordo compreso)
static const int DOT_BORDER = 2;           // bordo color inchiostro dei puntini
static const int TRAIL = 5;                // righe della scia (la prima e' la riga viva)
static const int TRAIL_STEP = 7;           // distanza in pixel tra le righe della scia

// Colori: quelli del tema (theme.h). I puntini sono quadratini con il
// bordo color inchiostro, riempiti con il colore di sfondo di un'altra
// palette del Pokekron: stesso produttore = stesso colore. Il colore
// della palette attiva viene saltato (si confonderebbe con lo sfondo).
// Produttore sconosciuto = quadratino vuoto (solo il bordo).

// ---- Produttori (codici ufficiali Bluetooth SIG) --------------
struct Company { uint16_t id; const char *name; };
static const Company COMPANIES[] = {
    {0x0006, "Microsoft"}, {0x004C, "Apple"},   {0x0057, "Harman (JBL)"},
    {0x0059, "Nordic"},    {0x005D, "Realtek"}, {0x006B, "Polar"},
    {0x0075, "Samsung"},   {0x0087, "Garmin"},  {0x009E, "Bose"},
    {0x00C4, "LG"},        {0x00E0, "Google"},  {0x012D, "Sony"},
    {0x0157, "Amazfit"},   {0x0171, "Amazon"},  {0x027D, "Huawei"},
    {0x02E5, "Espressif"}, {0x038F, "Xiaomi"},
};
static const uint16_t NO_COMPANY = 0xFFFF;

// ---- Dispositivi Bluetooth trovati ----------------------------
struct Device {
    bool used;
    char addr[18];
    char name[24];
    uint16_t company;
    int16_t rssi;
    uint16_t angle;
    int16_t y;
    uint8_t opa;
    uint32_t lastSeen;
    lv_obj_t *dot;
};
static Device devices[cfg::RADAR_MAX_DEVICES];
static int deviceCount = 0;

// ---- Coda tra il Bluetooth e il loop() ------------------------
struct Seen {
    char addr[18];
    char name[24];
    uint16_t company;
    int8_t rssi;
};
static const int QUEUE_LEN = 24;
static Seen queue[QUEUE_LEN];
static int qHead = 0, qTail = 0;
static portMUX_TYPE qMux = portMUX_INITIALIZER_UNLOCKED;
static volatile bool acceptResults = false;

class RadarScanCallbacks : public NimBLEScanCallbacks {
    void onResult(const NimBLEAdvertisedDevice *d) override
    {
        if (!acceptResults) return;
        Seen s;
        memset(&s, 0, sizeof(s));
        strlcpy(s.addr, d->getAddress().toString().c_str(), sizeof(s.addr));
        if (d->haveName()) strlcpy(s.name, d->getName().c_str(), sizeof(s.name));
        s.company = NO_COMPANY;
        if (d->haveManufacturerData()) {
            const std::string m = d->getManufacturerData();
            if (m.size() >= 2) s.company = (uint8_t)m[0] | ((uint16_t)(uint8_t)m[1] << 8);
        }
        s.rssi = d->getRSSI();

        portENTER_CRITICAL(&qMux);
        const int next = (qHead + 1) % QUEUE_LEN;
        if (next != qTail) { queue[qHead] = s; qHead = next; }
        portEXIT_CRITICAL(&qMux);
    }
};
static RadarScanCallbacks scanCallbacks;

// ---- Interfaccia ----------------------------------------------
static lv_obj_t *tabBtn[2] = {nullptr, nullptr};
static lv_obj_t *tabText[2] = {nullptr, nullptr};
static lv_obj_t *pageBT = nullptr;
static lv_obj_t *pageWifi = nullptr;
static lv_obj_t *dotLayer = nullptr;
static lv_obj_t *sweep[TRAIL] = {};
static lv_point_precise_t sweepPts[TRAIL][2];
static lv_obj_t *countLabel = nullptr;
static lv_obj_t *hintLabel = nullptr;
static lv_obj_t *actionBtn = nullptr;
static lv_obj_t *actionIcon = nullptr;
static lv_obj_t *card = nullptr;
static lv_obj_t *cardName = nullptr;
static lv_obj_t *cardInfo = nullptr;

// ---- Funzioni di supporto -------------------------------------

static const char *companyName(uint16_t id, char *buf, size_t size)
{
    if (id == NO_COMPANY) return "Non trasmesso";
    for (const Company &c : COMPANIES)
        if (c.id == id) return c.name;
    snprintf(buf, size, "Codice 0x%04X", id);
    return buf;
}

// Colore di riempimento per un produttore: lo sfondo di una delle 7
// palette diverse da quella attiva
static uint32_t colorForCompany(uint16_t id)
{
    const int others = pk::PALETTE_COUNT - 1;
    int p = (id ^ (id >> 8)) % others;
    if (p >= pk::palette()) p++;   // salta la palette attiva
    return pk::paletteBg(p);
}

static uint16_t angleFor(const char *addr)
{
    uint32_t h = 2166136261u;
    for (; *addr; addr++) { h ^= (uint8_t)*addr; h *= 16777619u; }
    return h % 360;
}

static int radiusFor(int rssi)
{
    const int nearDb = cfg::RADAR_RSSI_NEAR, farDb = cfg::RADAR_RSSI_FAR;
    if (rssi > nearDb) rssi = nearDb;
    if (rssi < farDb) rssi = farDb;
    const int minR = 12, maxR = SQ / 2 - DOT / 2 - 6;
    return minR + (nearDb - rssi) * (maxR - minR) / (nearDb - farDb);
}

static void cleanName(char *s)
{
    for (; *s; s++)
        if ((uint8_t)*s < 32 || (uint8_t)*s > 126) *s = '?';
}

static void setSonarLine(int i, int y)
{
    sweepPts[i][0].x = LEFT;  sweepPts[i][0].y = y;
    sweepPts[i][1].x = RIGHT; sweepPts[i][1].y = y;
    lv_line_set_points(sweep[i], sweepPts[i], 2);
}

static void placeDot(Device &d)
{
    if (!d.dot) return;
    const float a = d.angle * (float)M_PI / 180.0f;
    const int r = radiusFor(d.rssi);
    const int x = CX + (int)(r * sinf(a));
    d.y = CY - (int)(r * cosf(a));
    lv_obj_set_pos(d.dot, x - DOT / 2, d.y - DOT / 2);
}

// Trasparenza del puntino (si schiarisce lontano dalla riga del sonar).
// Lo sconosciuto resta vuoto: si vede solo il bordo.
static void setDotOpa(Device &d)
{
    if (!d.dot) return;
    lv_obj_set_style_border_opa(d.dot, d.opa, 0);
    lv_obj_set_style_bg_opa(d.dot, d.company == NO_COMPANY ? LV_OPA_TRANSP : d.opa, 0);
}

static void colorDot(Device &d)
{
    if (!d.dot) return;
    lv_obj_set_style_border_color(d.dot, pk::color(pk::INK), 0);
    if (d.company != NO_COMPANY)
        lv_obj_set_style_bg_color(d.dot, pk::color(colorForCompany(d.company)), 0);
    setDotOpa(d);
}

static void onDotClicked(lv_event_t *e)
{
    if (!launcher.clickAllowed()) return;
    radarApp.showDetails((int)(intptr_t)lv_event_get_user_data(e));
}

static void createDot(int index)
{
    Device &d = devices[index];
    if (!dotLayer) return;
    d.dot = lv_obj_create(dotLayer);
    lv_obj_set_size(d.dot, DOT, DOT);
    lv_obj_set_style_radius(d.dot, 0, 0);   // quadratino, come i pixel dello schermo LCD
    lv_obj_set_style_border_width(d.dot, DOT_BORDER, 0);
    lv_obj_set_style_pad_all(d.dot, 0, 0);
    lv_obj_remove_flag(d.dot, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_ext_click_area(d.dot, 8);
    lv_obj_add_event_cb(d.dot, onDotClicked, LV_EVENT_CLICKED, (void *)(intptr_t)index);
    d.opa = 255;
    colorDot(d);
    placeDot(d);
}

static void freeDevice(Device &d)
{
    if (d.dot) lv_obj_delete(d.dot);
    d.dot = nullptr;
    if (d.used) deviceCount--;
    d.used = false;
}

static void onActionClicked(lv_event_t * /*e*/)
{
    if (!launcher.clickAllowed()) return;
    radarApp.toggleScan();
}

static void onCardClicked(lv_event_t * /*e*/)
{
    if (!launcher.clickAllowed()) return;
    radarApp.hideDetails();
}

static void onTab(lv_event_t *e)
{
    if (!launcher.clickAllowed()) return;
    radarApp.showTab((RadarApp::Tab)(intptr_t)lv_event_get_user_data(e));
}

// ---- Icona del menu: un piccolo radar --------------------------

void RadarApp::onPaletteChanged()
{
    // I colori dei produttori dipendono dalla palette attiva
    for (Device &d : devices)
        if (d.used) colorDot(d);
}

bool RadarApp::drawIcon(lv_obj_t *tile)
{
    lv_obj_t *box = lv_obj_create(tile);
    lv_obj_set_size(box, 34, 34);
    lv_obj_set_style_radius(box, 0, 0);
    lv_obj_set_style_bg_opa(box, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_color(box, pk::color(pk::INK), 0);
    lv_obj_set_style_border_width(box, 2, 0);
    lv_obj_set_style_pad_all(box, 0, 0);
    lv_obj_remove_flag(box, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_remove_flag(box, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_align(box, LV_ALIGN_CENTER, 0, -12);

    static lv_point_precise_t line[] = {{2, 17}, {32, 17}};
    lv_obj_t *l = lv_line_create(box);
    lv_line_set_points(l, line, 2);
    lv_obj_set_style_line_width(l, 2, 0);
    lv_obj_set_style_line_color(l, pk::color(pk::INK), 0);
    lv_obj_remove_flag(l, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_pos(l, 0, 0);

    lv_obj_t *blip = lv_obj_create(box);
    lv_obj_set_size(blip, 6, 6);
    lv_obj_set_style_radius(blip, 0, 0);
    lv_obj_set_style_bg_color(blip, pk::color(pk::INK), 0);
    lv_obj_set_style_border_width(blip, 0, 0);
    lv_obj_remove_flag(blip, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_pos(blip, 20, 8);
    return true;
}

// ---- Schede e pagine ------------------------------------------

void RadarApp::buildTabs(lv_obj_t *parent)
{
    const char *labels[2] = {"Bluetooth", "Wi-Fi"};
    for (int i = 0; i < 2; i++) {
        tabBtn[i] = lv_button_create(parent);
        lv_obj_set_size(tabBtn[i], 116, TABS_H);
        lv_obj_set_pos(tabBtn[i], i ? 122 : 2, 0);
        lv_obj_set_style_pad_all(tabBtn[i], 0, 0);
        lv_obj_add_event_cb(tabBtn[i], onTab, LV_EVENT_CLICKED, (void *)(intptr_t)i);
        tabText[i] = lv_label_create(tabBtn[i]);
        lv_obj_set_style_text_font(tabText[i], &pk_font_small, 0);
        lv_label_set_text(tabText[i], labels[i]);
        lv_obj_center(tabText[i]);
    }

    pageBT = lv_obj_create(parent);
    lv_obj_set_size(pageBT, 240, PAGE_H);
    lv_obj_set_pos(pageBT, 0, TABS_H);
    pk::styleClear(pageBT);
    lv_obj_remove_flag(pageBT, LV_OBJ_FLAG_SCROLLABLE);

    pageWifi = lv_obj_create(parent);
    lv_obj_set_size(pageWifi, 240, PAGE_H);
    lv_obj_set_pos(pageWifi, 0, TABS_H);
    pk::styleClear(pageWifi);
    lv_obj_remove_flag(pageWifi, LV_OBJ_FLAG_SCROLLABLE);
}

void RadarApp::buildRadar(lv_obj_t *parent)
{
    lv_obj_t *bg = lv_obj_create(parent);
    lv_obj_set_size(bg, SQ, SQ);
    lv_obj_set_pos(bg, LEFT, TOP);
    lv_obj_set_style_bg_color(bg, pk::color(pk::BG), 0);
    lv_obj_set_style_bg_opa(bg, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(bg, pk::color(pk::INK), 0);
    lv_obj_set_style_border_width(bg, 2, 0);
    lv_obj_set_style_radius(bg, 0, 0);
    lv_obj_set_style_pad_all(bg, 0, 0);
    lv_obj_remove_flag(bg, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_remove_flag(bg, LV_OBJ_FLAG_CLICKABLE);

    for (int k = 1; k <= 3; k++) {
        const int gx = LEFT + SQ * k / 4;
        const int gy = TOP + SQ * k / 4;
        static lv_point_precise_t vp[3][2], hp[3][2];
        vp[k - 1][0] = {(lv_value_precise_t)gx, (lv_value_precise_t)TOP};
        vp[k - 1][1] = {(lv_value_precise_t)gx, (lv_value_precise_t)BOTTOM};
        hp[k - 1][0] = {(lv_value_precise_t)LEFT, (lv_value_precise_t)gy};
        hp[k - 1][1] = {(lv_value_precise_t)RIGHT, (lv_value_precise_t)gy};
        for (lv_point_precise_t *pts : {vp[k - 1], hp[k - 1]}) {
            lv_obj_t *g = lv_line_create(parent);
            lv_line_set_points(g, pts, 2);
            lv_obj_set_style_line_width(g, 1, 0);
            lv_obj_set_style_line_color(g, pk::color(k == 2 ? pk::MID : pk::SHADE), 0);
            lv_obj_remove_flag(g, LV_OBJ_FLAG_CLICKABLE);
            lv_obj_set_pos(g, 0, 0);
        }
    }

    static const lv_opa_t trailOpa[TRAIL] = {LV_OPA_COVER, 150, 95, 55, 25};
    for (int i = TRAIL - 1; i >= 0; i--) {
        sweep[i] = lv_line_create(parent);
        lv_obj_set_style_line_width(sweep[i], i == 0 ? 3 : 2, 0);
        lv_obj_set_style_line_color(sweep[i], pk::color(pk::INK), 0);
        lv_obj_set_style_line_opa(sweep[i], trailOpa[i], 0);
        lv_obj_set_style_line_rounded(sweep[i], false, 0);
        lv_obj_remove_flag(sweep[i], LV_OBJ_FLAG_CLICKABLE);
        lv_obj_set_pos(sweep[i], 0, 0);
        setSonarLine(i, CY);
    }

    dotLayer = lv_obj_create(parent);
    lv_obj_set_size(dotLayer, 240, PAGE_H);
    lv_obj_set_pos(dotLayer, 0, 0);
    pk::styleClear(dotLayer);
    lv_obj_remove_flag(dotLayer, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_remove_flag(dotLayer, LV_OBJ_FLAG_CLICKABLE);

    countLabel = lv_label_create(parent);
    pk::styleLabel(countLabel, &pk_font_small);
    lv_obj_set_pos(countLabel, LEFT + 6, TOP + 4);

    hintLabel = lv_label_create(parent);
    pk::styleLabel(hintLabel, &pk_font_small, pk::DIM);
    lv_label_set_text(hintLabel, "Premi Avvia");
    lv_obj_align(hintLabel, LV_ALIGN_CENTER, 0, -14);
}

void RadarApp::build(lv_obj_t *parent)
{
    buildTabs(parent);
    buildRadar(pageBT);
    wifiProbe.build(pageWifi);

    // Pulsante Avvia/Stop: quadratino in basso a destra, sopra tutto
    actionBtn = lv_button_create(parent);
    lv_obj_set_size(actionBtn, 46, 46);
    lv_obj_align(actionBtn, LV_ALIGN_BOTTOM_RIGHT, -6, -6);
    pk::styleButton(actionBtn);
    lv_obj_set_style_bg_opa(actionBtn, LV_OPA_80, 0);   // semitrasparente
    lv_obj_add_event_cb(actionBtn, onActionClicked, LV_EVENT_CLICKED, nullptr);
    actionIcon = lv_label_create(actionBtn);
    lv_obj_set_style_text_font(actionIcon, &lv_font_montserrat_20, 0);   // colore: quello del pulsante
    lv_obj_center(actionIcon);

    // Scheda dettagli Bluetooth (sopra a tutto, nascosta)
    card = lv_obj_create(parent);
    lv_obj_set_size(card, 216, 130);
    lv_obj_align(card, LV_ALIGN_CENTER, 0, 0);
    pk::styleBox(card, 3);
    lv_obj_set_style_pad_all(card, 12, 0);
    lv_obj_remove_flag(card, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_event_cb(card, onCardClicked, LV_EVENT_CLICKED, nullptr);

    cardName = lv_label_create(card);
    lv_obj_set_width(cardName, 188);
    lv_label_set_long_mode(cardName, LV_LABEL_LONG_DOT);
    pk::styleLabel(cardName, &pk_font_text);
    lv_obj_set_pos(cardName, 0, 0);

    cardInfo = lv_label_create(card);
    lv_obj_set_width(cardInfo, 188);
    pk::styleLabel(cardInfo, &pk_font_small);
    lv_obj_set_style_text_line_space(cardInfo, 4, 0);
    lv_obj_set_pos(cardInfo, 0, 28);
    lv_obj_add_flag(card, LV_OBJ_FLAG_HIDDEN);

    showTab(TAB_BT);
}

void RadarApp::showTab(Tab t)
{
    // C'e' una sola radio: cambiando scheda si ferma la scansione in corso
    if (scanning) stopScan(false);
    if (wifiProbe.isScanning()) wifiProbe.stop(false);
    hideDetails();
    wifiProbe.hideDetails();

    tab = t;
    for (int i = 0; i < 2; i++) {
        const bool on = (i == (int)t);
        pk::styleTab(tabBtn[i], tabText[i], on);
    }
    if (t == TAB_BT) {
        lv_obj_remove_flag(pageBT, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(pageWifi, LV_OBJ_FLAG_HIDDEN);
    } else {
        lv_obj_add_flag(pageBT, LV_OBJ_FLAG_HIDDEN);
        lv_obj_remove_flag(pageWifi, LV_OBJ_FLAG_HIDDEN);
    }
    refreshUi();
    if (isOpen()) screen.setTimeout(screenTimeoutMs());
}

void RadarApp::toggleScan()
{
    if (tab == TAB_BT) {
        if (scanning) stopScan();
        else startScan();
    } else {
        if (wifiProbe.isScanning()) wifiProbe.stop();
        else wifiProbe.start();
    }
    refreshUi();
    if (isOpen()) screen.setTimeout(screenTimeoutMs());
}

bool RadarApp::anyScanning() const
{
    return scanning || wifiProbe.isScanning();
}

void RadarApp::refreshUi()
{
    if (!actionBtn) return;
    const bool active = (tab == TAB_BT) ? scanning : wifiProbe.isScanning();
    pk::styleButton(actionBtn, active);   // in scansione: pieno
    lv_obj_set_style_bg_opa(actionBtn, LV_OPA_80, 0);   // semitrasparente
    lv_label_set_text(actionIcon, active ? LV_SYMBOL_STOP : LV_SYMBOL_PLAY);

    // Elementi della scheda Bluetooth
    if (hintLabel) {
        if (scanning) lv_obj_add_flag(hintLabel, LV_OBJ_FLAG_HIDDEN);
        else lv_obj_remove_flag(hintLabel, LV_OBJ_FLAG_HIDDEN);
    }
    for (int i = 0; i < TRAIL; i++) {
        if (!sweep[i]) continue;
        if (i > 0 && !scanning) lv_obj_add_flag(sweep[i], LV_OBJ_FLAG_HIDDEN);
        else lv_obj_remove_flag(sweep[i], LV_OBJ_FLAG_HIDDEN);
    }
    if (!scanning && sweep[0]) setSonarLine(0, CY);
    if (countLabel) {
        if (scanning) lv_label_set_text_fmt(countLabel, "%d trovati", deviceCount);
        else lv_label_set_text(countLabel, "");
    }
}

// ---- Scansione Bluetooth --------------------------------------

void RadarApp::startScan()
{
    if (scanning) return;
    if (!NimBLEDevice::isInitialized() && !NimBLEDevice::init("")) {
        App::log(name(), "Errore: Bluetooth non avviato");
        return;
    }
    NimBLEScan *scan = NimBLEDevice::getScan();
    scan->setScanCallbacks(&scanCallbacks, true);
    scan->setDuplicateFilter(0);
    scan->setActiveScan(cfg::RADAR_ACTIVE_SCAN);
    scan->setInterval(100);
    scan->setWindow(80);
    scan->setMaxResults(0);

    portENTER_CRITICAL(&qMux);
    qHead = qTail = 0;
    portEXIT_CRITICAL(&qMux);
    acceptResults = true;

    if (!scan->start(0, false, true)) {
        acceptResults = false;
        NimBLEDevice::deinit(true);
        App::log(name(), "Errore: scansione non avviata");
        return;
    }
    scanning = true;
    scanStartedAt = millis();
    lastCleanup = millis();
    App::log(name(), "Scansione Bluetooth avviata");
    buzz();
    refreshUi();
}

void RadarApp::stopScan(bool feedback)
{
    const bool was = scanning;
    acceptResults = false;
    scanning = false;
    if (NimBLEDevice::isInitialized()) {
        NimBLEDevice::getScan()->stop();
        NimBLEDevice::deinit(true);
    }
    portENTER_CRITICAL(&qMux);
    qHead = qTail = 0;
    portEXIT_CRITICAL(&qMux);
    for (Device &d : devices) freeDevice(d);
    deviceCount = 0;
    hideDetails();
    if (was) {
        App::log(name(), "Scansione Bluetooth fermata");
        if (feedback) buzz();
    }
    refreshUi();
}

void RadarApp::processQueue()
{
    const uint32_t now = millis();
    bool changed = false;
    for (int n = 0; n < 16; n++) {
        Seen s;
        portENTER_CRITICAL(&qMux);
        const bool empty = (qTail == qHead);
        if (!empty) { s = queue[qTail]; qTail = (qTail + 1) % QUEUE_LEN; }
        portEXIT_CRITICAL(&qMux);
        if (empty) break;

        cleanName(s.name);

        int idx = -1;
        for (int i = 0; i < cfg::RADAR_MAX_DEVICES; i++)
            if (devices[i].used && strcmp(devices[i].addr, s.addr) == 0) { idx = i; break; }

        if (idx >= 0) {
            Device &d = devices[idx];
            d.rssi = (d.rssi * 3 + s.rssi) / 4;
            d.lastSeen = now;
            if (!d.name[0] && s.name[0]) strlcpy(d.name, s.name, sizeof(d.name));
            if (d.company == NO_COMPANY && s.company != NO_COMPANY) {
                d.company = s.company;
                colorDot(d);
            }
            placeDot(d);
            continue;
        }

        for (int i = 0; i < cfg::RADAR_MAX_DEVICES; i++)
            if (!devices[i].used) { idx = i; break; }
        if (idx < 0) {
            idx = 0;
            for (int i = 1; i < cfg::RADAR_MAX_DEVICES; i++)
                if ((int32_t)(devices[i].lastSeen - devices[idx].lastSeen) < 0) idx = i;
            freeDevice(devices[idx]);
        }
        Device &d = devices[idx];
        memset(&d, 0, sizeof(d));
        d.used = true;
        strlcpy(d.addr, s.addr, sizeof(d.addr));
        strlcpy(d.name, s.name, sizeof(d.name));
        d.company = s.company;
        d.rssi = s.rssi;
        d.angle = angleFor(s.addr);
        d.lastSeen = now;
        deviceCount++;
        createDot(idx);
        changed = true;
    }
    if (changed) refreshUi();
}

void RadarApp::removeOld()
{
    const uint32_t now = millis();
    bool changed = false;
    for (Device &d : devices) {
        if (d.used && now - d.lastSeen > cfg::RADAR_DEVICE_TIMEOUT_S * 1000UL) {
            freeDevice(d);
            changed = true;
        }
    }
    if (changed) refreshUi();
}

void RadarApp::updateSweep()
{
    const uint32_t period = 2 * cfg::RADAR_SWEEP_MS;
    const uint32_t t = (millis() - scanStartedAt) % period;
    const bool goingDown = (t < cfg::RADAR_SWEEP_MS);
    const float p = goingDown ? (float)t / cfg::RADAR_SWEEP_MS
                              : (float)(period - t) / cfg::RADAR_SWEEP_MS;
    const int lineY = TOP + (int)(p * SQ);

    const int dir = goingDown ? -1 : 1;
    for (int i = 0; i < TRAIL; i++) {
        int y = lineY + dir * i * TRAIL_STEP;
        if (y < TOP) y = TOP;
        if (y > BOTTOM) y = BOTTOM;
        setSonarLine(i, y);
    }

    for (Device &d : devices) {
        if (!d.used || !d.dot) continue;
        int dy = abs(d.y - lineY);
        int opa = 255 - dy * 3;
        if (opa < 70) opa = 70;
        if (abs(opa - (int)d.opa) >= 8) {
            d.opa = opa;
            setDotOpa(d);
        }
    }
}

void RadarApp::update()
{
    if (!scanning) return;
    processQueue();
    const uint32_t now = millis();
    if (now - lastCleanup >= 1000) {
        lastCleanup = now;
        removeOld();
    }
    if (isOpen() && tab == TAB_BT && now - lastFrame >= cfg::RADAR_FRAME_MS) {
        lastFrame = now;
        updateSweep();
    }
}

// ---- Scheda dei dettagli Bluetooth ----------------------------

void RadarApp::showDetails(int index)
{
    if (!card || index < 0 || index >= cfg::RADAR_MAX_DEVICES || !devices[index].used) return;
    const Device &d = devices[index];
    char buf[16];
    lv_label_set_text(cardName, d.name[0] ? d.name : "Senza nome");
    lv_label_set_text_fmt(cardInfo, "Produttore: %s\nIndirizzo:\n%s",
                          companyName(d.company, buf, sizeof(buf)), d.addr);
    lv_obj_remove_flag(card, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(actionBtn, LV_OBJ_FLAG_HIDDEN);
}

void RadarApp::hideDetails()
{
    if (!card) return;
    lv_obj_add_flag(card, LV_OBJ_FLAG_HIDDEN);
    if (actionBtn) lv_obj_remove_flag(actionBtn, LV_OBJ_FLAG_HIDDEN);
}

// ---- Navigazione, schermo, comandi ----------------------------

void RadarApp::onClose()
{
    stopScan(false);
    wifiProbe.stop(false);
}

bool RadarApp::onBack()
{
    if (tab == TAB_BT) {
        if (card && !lv_obj_has_flag(card, LV_OBJ_FLAG_HIDDEN)) { hideDetails(); return true; }
    } else {
        if (wifiProbe.onBack()) return true;
    }
    return false;
}

bool RadarApp::canSleep()
{
    return !anyScanning();
}

uint32_t RadarApp::screenTimeoutMs() const
{
    if (scanning || wifiProbe.isScanning()) return 0;
    return userSettings.menuTimeoutMs();
}

void RadarApp::buzz()
{
    if (!userSettings.buttonVibrate) return;
    instance.drv.setWaveform(0, 1);
    instance.drv.setWaveform(1, 0);
    instance.drv.run();
}

bool RadarApp::onSerialCommand(const char *line)
{
    if (strcmp(line, "radar") != 0) return false;
    if (!scanning) {
        App::log(name(), "Bluetooth fermo (si avvia dalla scheda Bluetooth del Radar)");
        return true;
    }
    App::log(name(), "Bluetooth: in scansione da %lu s, %d dispositivi:",
             (unsigned long)((millis() - scanStartedAt) / 1000), deviceCount);
    char buf[16];
    for (const Device &d : devices) {
        if (!d.used) continue;
        App::log(name(), "  %s  %4d dBm  %-13s  %s", d.addr, d.rssi,
                 companyName(d.company, buf, sizeof(buf)), d.name[0] ? d.name : "-");
    }
    return true;
}

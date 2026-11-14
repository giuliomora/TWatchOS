#include <LilyGoLib.h>
#include <Preferences.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include "pokeradar_app.h"
#include "steps_app.h"
#include "time_service.h"
#include "launcher.h"
#include "sound.h"
#include "screen.h"
#include "app.h"
#include "theme.h"

PokeradarApp pokeradarApp;

// ---- Griglia (presa dalla ricerca strumenti del gioco) ---------
// Area dell'app: 240 x 200. Te al centro, 15 x 17 caselle.
static const int CX = 120, CY = 100;      // centro (incrocio degli assi)
static const int TILE_W = 16, TILE_H = 12;// una casella
static const int NX = 7, NY = 8;          // caselle per lato (15 x 17)
static const int AXIS = 4;                // spessore degli assi
static const int TICK = 3;                // spessore delle tacche
static const int TICK_H = 21;             // tacche sull'asse orizzontale (in verticale)
static const int TICK_V = 16;             // tacche sull'asse verticale (in orizzontale)
static const int MARK = 12;               // quadratino dello strumento (bordo compreso)
static const int PING_R = 44;             // raggio del ping (pixel)
static const int NEAR_R = 100;            // entro questa distanza il ping si ripete
static const uint32_t PING_MS = 450;      // durata del cerchio
static const uint32_t REVEAL_MS = 2000;   // quanto lampeggia lo strumento
static const uint32_t BLINK_MS = 200;
static const uint32_t ECHO_MS = 500;      // intervallo tra i ping ripetuti
static const int ECHO_COUNT = 2;          // ping ripetuti quando sei vicino
static const uint32_t MESSAGE_MS = 3500;
static const uint32_t SAVE_EVERY_MS = 120000;

// ---- Strumenti (nomi del gioco) e quanto sono frequenti -------
struct Loot { const char *name; uint8_t weight; };
static const Loot LOOT[] = {
    {"Pozione", 20},       {"Superpozione", 10},  {"Iperpozione", 4},
    {"Antidoto", 10},      {"Antiparalisi", 8},   {"Poke Ball", 18},
    {"Mega Ball", 10},     {"Ultra Ball", 5},     {"Perla", 6},
    {"Pepita", 4},         {"Polvostella", 6},    {"Revitalizzante", 4},
    {"Pietrafocaia", 2},   {"Pietraidrica", 2},   {"Pietrafolgore", 2},
    {"Pietrafoglia", 2},   {"Pietralunare", 2},   {"Rara Caramella", 1},
    {"Master Ball", 1},
};
static const int LOOT_COUNT = sizeof(LOOT) / sizeof(LOOT[0]);

static const char *randomLoot()
{
    int total = 0;
    for (const Loot &l : LOOT) total += l.weight;
    int r = (int)random(total);
    for (const Loot &l : LOOT) {
        if (r < l.weight) return l.name;
        r -= l.weight;
    }
    return LOOT[0].name;
}

static float rnd() { return (float)random(1000000) / 1000000.0f; }   // 0..1

// ---- Interfaccia ----------------------------------------------
struct Ping { lv_obj_t *obj; int cx, cy; bool busy; };
static const int PINGS = 4;
static Ping pings[PINGS] = {};
static lv_obj_t *layer = nullptr;
static lv_obj_t *marks[cfg::POKERADAR_MAX_ITEMS] = {};
static lv_obj_t *msgBox = nullptr;
static lv_obj_t *msgLabel = nullptr;

// Rettangolo pieno color inchiostro (assi, tacche)
static lv_obj_t *rect(lv_obj_t *parent, int x, int y, int w, int h)
{
    lv_obj_t *r = lv_obj_create(parent);
    lv_obj_set_pos(r, x, y);
    lv_obj_set_size(r, w, h);
    lv_obj_set_style_radius(r, 0, 0);
    lv_obj_set_style_border_width(r, 0, 0);
    lv_obj_set_style_pad_all(r, 0, 0);
    lv_obj_set_style_shadow_width(r, 0, 0);
    lv_obj_set_style_bg_color(r, pk::color(pk::INK), 0);
    lv_obj_set_style_bg_opa(r, LV_OPA_COVER, 0);
    lv_obj_remove_flag(r, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_remove_flag(r, LV_OBJ_FLAG_CLICKABLE);   // il tocco va alla griglia
    return r;
}

static void onPress(lv_event_t * /*e*/)
{
    if (!launcher.touchAllowed() || !layer) return;
    lv_indev_t *indev = lv_indev_active();
    if (!indev) return;
    lv_point_t p;
    lv_indev_get_point(indev, &p);
    lv_area_t a;
    lv_obj_get_coords(layer, &a);
    pokeradarApp.pingAt(p.x - a.x1, p.y - a.y1);
}

static void pingExec(void *var, int32_t v)
{
    Ping *p = (Ping *)var;
    lv_obj_set_size(p->obj, v, v);
    lv_obj_set_pos(p->obj, p->cx - v / 2, p->cy - v / 2);
}

static void pingDone(lv_anim_t *a)
{
    Ping *p = (Ping *)lv_anim_get_user_data(a);
    lv_obj_add_flag(p->obj, LV_OBJ_FLAG_HIDDEN);
    p->busy = false;
}

// ---- Icona del menu: assi con le tacche e un cerchio --------
bool PokeradarApp::drawIcon(lv_obj_t *tile)
{
    lv_obj_t *box = lv_obj_create(tile);
    lv_obj_set_size(box, 44, 36);
    pk::styleClear(box);
    lv_obj_remove_flag(box, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_remove_flag(box, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_align(box, LV_ALIGN_CENTER, 0, -12);

    rect(box, 0, 17, 44, 2);    // asse orizzontale
    rect(box, 21, 0, 2, 36);    // asse verticale
    for (int k = 1; k <= 2; k++) {
        rect(box, 21 - k * 8, 14, 2, 8);
        rect(box, 21 + k * 8, 14, 2, 8);
        rect(box, 18, 18 - k * 7, 8, 1);
        rect(box, 18, 18 + k * 7, 8, 1);
    }
    lv_obj_t *ring = lv_obj_create(box);
    lv_obj_set_size(ring, 18, 18);
    lv_obj_set_pos(ring, 26, 1);
    lv_obj_set_style_radius(ring, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_opa(ring, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_color(ring, pk::color(pk::INK), 0);
    lv_obj_set_style_border_width(ring, 2, 0);
    lv_obj_set_style_pad_all(ring, 0, 0);
    lv_obj_remove_flag(ring, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_remove_flag(ring, LV_OBJ_FLAG_CLICKABLE);
    return true;
}

// ---- Costruzione ---------------------------------------------
void PokeradarApp::build(lv_obj_t *parent)
{
    layer = lv_obj_create(parent);
    lv_obj_set_size(layer, 240, 200);
    lv_obj_set_pos(layer, 0, 0);
    pk::styleClear(layer);
    lv_obj_remove_flag(layer, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(layer, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(layer, onPress, LV_EVENT_PRESSED, nullptr);

    // Assi: dividono lo schermo in 4 quadranti
    rect(layer, 2, CY - AXIS / 2, 236, AXIS);
    rect(layer, CX - AXIS / 2, 0, AXIS, 198);
    // Tacche: una per casella
    for (int k = 1; k <= NX; k++) {
        rect(layer, CX + k * TILE_W - TICK / 2, CY - TICK_H / 2, TICK, TICK_H);
        rect(layer, CX - k * TILE_W - TICK / 2, CY - TICK_H / 2, TICK, TICK_H);
    }
    for (int k = 1; k <= NY; k++) {
        rect(layer, CX - TICK_V / 2, CY + k * TILE_H - TICK / 2, TICK_V, TICK);
        rect(layer, CX - TICK_V / 2, CY - k * TILE_H - TICK / 2, TICK_V, TICK);
    }

    // Strumenti (nascosti finche' un ping non li trova). Bordo chiaro:
    // cosi' si vedono anche quando sono sopra un asse.
    for (int i = 0; i < cfg::POKERADAR_MAX_ITEMS; i++) {
        marks[i] = rect(layer, 0, 0, MARK, MARK);
        lv_obj_set_style_border_color(marks[i], pk::color(pk::BG), 0);
        lv_obj_set_style_border_width(marks[i], 2, 0);
        lv_obj_add_flag(marks[i], LV_OBJ_FLAG_HIDDEN);
    }

    // Cerchi dei ping
    for (int i = 0; i < PINGS; i++) {
        lv_obj_t *c = lv_obj_create(layer);
        lv_obj_set_style_radius(c, LV_RADIUS_CIRCLE, 0);
        lv_obj_set_style_bg_opa(c, LV_OPA_TRANSP, 0);
        lv_obj_set_style_border_color(c, pk::color(pk::INK), 0);
        lv_obj_set_style_border_width(c, 3, 0);
        lv_obj_set_style_pad_all(c, 0, 0);
        lv_obj_set_style_shadow_width(c, 0, 0);
        lv_obj_remove_flag(c, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_remove_flag(c, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_add_flag(c, LV_OBJ_FLAG_HIDDEN);
        pings[i] = {c, 0, 0, false};
    }

    // Messaggio "Hai trovato..."
    msgBox = lv_obj_create(layer);
    lv_obj_set_size(msgBox, 210, 64);
    lv_obj_align(msgBox, LV_ALIGN_BOTTOM_MID, 0, -8);
    pk::styleBox(msgBox, 3);
    lv_obj_set_style_pad_all(msgBox, 6, 0);
    lv_obj_remove_flag(msgBox, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_remove_flag(msgBox, LV_OBJ_FLAG_CLICKABLE);
    msgLabel = lv_label_create(msgBox);
    pk::styleLabel(msgLabel, &pk_font_text);
    lv_obj_set_width(msgLabel, lv_pct(100));
    lv_label_set_long_mode(msgLabel, LV_LABEL_LONG_WRAP);
    lv_obj_set_style_text_align(msgLabel, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_center(msgLabel);
    lv_obj_add_flag(msgBox, LV_OBJ_FLAG_HIDDEN);
}

// ---- Strumenti -----------------------------------------------
int PokeradarApp::itemCount() const
{
    int n = 0;
    for (const Item &it : items) if (it.used) n++;
    return n;
}

bool PokeradarApp::itemTile(const Item &it, int &tx, int &ty) const
{
    const float a = it.angle * (float)M_PI / 180.0f;
    const float x = it.dist * sinf(a);
    const float y = -it.dist * cosf(a);
    tx = (int)lroundf(x / TILE_W);
    ty = (int)lroundf(y / TILE_H);
    return abs(tx) <= NX && abs(ty) <= NY;
}

bool PokeradarApp::spawn(int minDist, int maxDist)
{
    for (int i = 0; i < cfg::POKERADAR_MAX_ITEMS; i++) {
        if (items[i].used) continue;
        items[i].used = 1;
        items[i].angle = (uint16_t)random(360);
        items[i].dist = (uint16_t)(minDist + random(maxDist - minDist + 1));
        revealUntil[i] = 0;
        dirty = true;
        App::log(name(), "Nuovo strumento nascosto a %u passi", items[i].dist);
        return true;
    }
    return false;   // gia' pieno
}

// Distanza casuale: di solito lontano, a volte vicino, raramente gia' sul radar
static void spawnRange(int &lo, int &hi)
{
    const float x = rnd();
    const int mn = cfg::POKERADAR_SPAWN_MIN, mx = cfg::POKERADAR_SPAWN_MAX;
    if (x < 0.10f)      { lo = mn;  hi = 90; }
    else if (x < 0.35f) { lo = 90;  hi = 160; }
    else                { lo = 160; hi = mx; }
    if (hi < lo) hi = lo;
}

void PokeradarApp::roll()
{
    // Umore della fortuna: ondate e secche che durano qualche tiro
    if (moodLeft > 0) {
        if (--moodLeft == 0) mood = 0;
    } else {
        const float m = rnd();
        if (m < 0.06f)      { mood = 1; moodLeft = 2 + random(5); }    // ondata
        else if (m < 0.14f) { mood = 2; moodLeft = 4 + random(12); }   // secca
    }
    // La fortuna di base cambia piano piano
    luck += (rnd() - 0.5f) * 0.12f;
    if (luck < 0.03f) luck = 0.03f;
    if (luck > 0.35f) luck = 0.35f;

    const float chance = mood == 1 ? 0.55f : (mood == 2 ? 0.02f : luck);
    int lo, hi;
    if (rnd() < 0.03f) {
        // sciame: si riempie tutto
        int n = 0;
        do { spawnRange(lo, hi); } while (spawn(lo, hi) && ++n < cfg::POKERADAR_MAX_ITEMS);
        if (n) App::log(name(), "Sciame di strumenti!");
    } else if (rnd() < chance) {
        spawnRange(lo, hi);
        spawn(lo, hi);
    }
    dirty = true;
}

void PokeradarApp::addSteps(uint32_t steps)
{
    if (steps == 0) return;
    for (Item &it : items) {
        if (!it.used) continue;
        it.dist = it.dist > steps ? it.dist - steps : 0;
    }
    stepAcc += steps;
    int rolls = 0;
    while (stepAcc >= cfg::POKERADAR_ROLL_STEPS && rolls < 40) {
        stepAcc -= cfg::POKERADAR_ROLL_STEPS;
        roll();
        rolls++;
    }
    if (stepAcc >= cfg::POKERADAR_ROLL_STEPS) stepAcc = 0;   // dopo un salto enorme
    dirty = true;
}

// ---- Ping ----------------------------------------------------
void PokeradarApp::startPing(int x, int y)
{
    for (Ping &p : pings) {
        if (p.busy) continue;
        p.busy = true;
        p.cx = x;
        p.cy = y;
        lv_obj_remove_flag(p.obj, LV_OBJ_FLAG_HIDDEN);
        lv_anim_t a;
        lv_anim_init(&a);
        lv_anim_set_var(&a, &p);
        lv_anim_set_user_data(&a, &p);
        lv_anim_set_values(&a, 6, PING_R * 2);
        lv_anim_set_duration(&a, PING_MS);
        lv_anim_set_exec_cb(&a, pingExec);
        lv_anim_set_completed_cb(&a, pingDone);
        lv_anim_start(&a);
        break;
    }
    sound.ping();
}

void PokeradarApp::reveal(int i)
{
    int tx, ty;
    if (!itemTile(items[i], tx, ty)) return;
    lv_obj_set_pos(marks[i], CX + tx * TILE_W - MARK / 2, CY + ty * TILE_H - MARK / 2);
    revealUntil[i] = millis() + REVEAL_MS;
    if (revealUntil[i] == 0) revealUntil[i] = 1;
    // Nella casella centrale: trovato!
    if (tx == 0 && ty == 0 && foundIndex < 0) {
        foundIndex = i;
        foundAt = millis();
    }
}

void PokeradarApp::pingAt(int x, int y)
{
    if (messageUntil) { hideMessage(); return; }   // il tocco chiude il messaggio
    startPing(x, y);
    echoLeft = 0;

    bool found = false, nearby = false;
    for (int i = 0; i < cfg::POKERADAR_MAX_ITEMS; i++) {
        if (!items[i].used) continue;
        int tx, ty;
        if (!itemTile(items[i], tx, ty)) continue;
        const float d = hypotf((float)(CX + tx * TILE_W - x), (float)(CY + ty * TILE_H - y));
        if (d <= PING_R) { reveal(i); found = true; }
        else if (d <= NEAR_R) nearby = true;
    }
    // Strumento vicino ma fuori dal cerchio: il ping si ripete
    if (!found && nearby) {
        echoX = x;
        echoY = y;
        echoLeft = ECHO_COUNT;
        echoAt = millis() + ECHO_MS;
    }
}

void PokeradarApp::showFound()
{
    const int i = foundIndex;
    foundIndex = -1;
    if (i < 0 || !items[i].used) return;
    items[i].used = 0;
    revealUntil[i] = 0;
    lv_obj_add_flag(marks[i], LV_OBJ_FLAG_HIDDEN);

    const char *loot = randomLoot();
    lv_label_set_text_fmt(msgLabel, "Hai trovato:\n%s!", loot);
    lv_obj_remove_flag(msgBox, LV_OBJ_FLAG_HIDDEN);
    messageUntil = millis() + MESSAGE_MS;
    if (messageUntil == 0) messageUntil = 1;
    App::log(name(), "Trovato: %s", loot);

    // ogni tanto uno strumento tira l'altro
    if (rnd() < 0.2f) {
        int lo, hi;
        spawnRange(lo, hi);
        spawn(lo, hi);
    }
    dirty = true;
    save();
}

void PokeradarApp::hideMessage()
{
    messageUntil = 0;
    if (msgBox) lv_obj_add_flag(msgBox, LV_OBJ_FLAG_HIDDEN);
}

// ---- Ciclo di vita -------------------------------------------
void PokeradarApp::begin()
{
    load();
}

void PokeradarApp::update()
{
    const uint32_t now = millis();

    // Passi: ogni secondo (anche ad app chiusa)
    if (now - lastCheck >= 1000) {
        lastCheck = now;
        const uint32_t today = stepsApp.today();
        const int16_t day = (int16_t)timeService.now().tm_yday;
        uint32_t delta = 0;
        if (lastDay < 0) delta = 0;                              // prima lettura
        else if (day != lastDay) delta = today;                  // dopo mezzanotte
        else if (today >= lastSteps) delta = today - lastSteps;
        lastSteps = today;
        lastDay = day;
        addSteps(delta);
        if (dirty && now - lastSave >= SAVE_EVERY_MS) save();
    }

    if (!isOpen() || !screen.isOn()) return;

    // Ping ripetuti
    if (echoLeft > 0 && (int32_t)(now - echoAt) >= 0) {
        startPing(echoX, echoY);
        echoLeft--;
        echoAt = now + ECHO_MS;
    }

    // Strumenti che lampeggiano
    for (int i = 0; i < cfg::POKERADAR_MAX_ITEMS; i++) {
        if (!marks[i]) continue;
        if (revealUntil[i] && (int32_t)(now - revealUntil[i]) < 0 && items[i].used) {
            const bool on = ((revealUntil[i] - now) / BLINK_MS) % 2 == 0;
            if (on) lv_obj_remove_flag(marks[i], LV_OBJ_FLAG_HIDDEN);
            else lv_obj_add_flag(marks[i], LV_OBJ_FLAG_HIDDEN);
        } else {
            revealUntil[i] = 0;
            lv_obj_add_flag(marks[i], LV_OBJ_FLAG_HIDDEN);
        }
    }

    // Trovato: il messaggio arriva dopo un attimo di lampeggio
    if (foundIndex >= 0 && now - foundAt >= 900) showFound();
    if (messageUntil && (int32_t)(now - messageUntil) >= 0) hideMessage();
}

void PokeradarApp::onOpen()
{
    sound.enable(true);
}

void PokeradarApp::onClose()
{
    sound.enable(false);
    echoLeft = 0;
    if (foundIndex >= 0) showFound();   // non perdere uno strumento gia' trovato
    hideMessage();
    for (int i = 0; i < cfg::POKERADAR_MAX_ITEMS; i++) {
        revealUntil[i] = 0;
        if (marks[i]) lv_obj_add_flag(marks[i], LV_OBJ_FLAG_HIDDEN);
    }
}

void PokeradarApp::onScreenOff()
{
    if (dirty && millis() - lastSave >= 30000) save();
}

// ---- Memoria (NVS) -------------------------------------------
struct Saved {
    uint8_t version;
    uint8_t mood, moodLeft, count;
    float luck;
    uint32_t stepAcc, lastSteps;
    int16_t lastDay;
    uint16_t angle[cfg::POKERADAR_MAX_ITEMS];
    uint16_t dist[cfg::POKERADAR_MAX_ITEMS];
};
static const uint8_t SAVE_VERSION = 1;

void PokeradarApp::load()
{
    Saved s;
    Preferences p;
    p.begin("pokeradar", true);
    const size_t n = p.getBytes("state", &s, sizeof(s));
    p.end();
    if (n != sizeof(s) || s.version != SAVE_VERSION) {
        // Prima volta: uno strumento per cominciare
        spawn(90, 160);
        save();
        return;
    }
    mood = s.mood; moodLeft = s.moodLeft; luck = s.luck;
    stepAcc = s.stepAcc; lastSteps = s.lastSteps; lastDay = s.lastDay;
    for (int i = 0; i < cfg::POKERADAR_MAX_ITEMS; i++) {
        items[i].used = i < s.count ? 1 : 0;
        items[i].angle = s.angle[i];
        items[i].dist = s.dist[i];
    }
    App::log(name(), "%d strumenti nascosti", itemCount());
}

void PokeradarApp::save()
{
    Saved s;
    memset(&s, 0, sizeof(s));
    s.version = SAVE_VERSION;
    s.mood = mood; s.moodLeft = moodLeft; s.luck = luck;
    s.stepAcc = stepAcc; s.lastSteps = lastSteps; s.lastDay = lastDay;
    int n = 0;
    for (const Item &it : items) {
        if (!it.used) continue;
        s.angle[n] = it.angle;
        s.dist[n] = it.dist;
        n++;
    }
    s.count = (uint8_t)n;
    Preferences p;
    p.begin("pokeradar", false);
    p.putBytes("state", &s, sizeof(s));
    p.end();
    dirty = false;
    lastSave = millis();
}

// ---- Comandi seriali -----------------------------------------
void PokeradarApp::print()
{
    static const char *MOOD[] = {"normale", "ondata di fortuna", "secca"};
    App::log(name(), "Fortuna %d%%, umore: %s, prossimo tiro tra %lu passi",
             (int)(luck * 100), MOOD[mood % 3],
             (unsigned long)(cfg::POKERADAR_ROLL_STEPS - stepAcc));
    for (const Item &it : items) {
        if (!it.used) continue;
        int tx, ty;
        const bool on = itemTile(it, tx, ty);
        App::log(name(), "  strumento a %u passi, direzione %u gradi%s",
                 it.dist, it.angle, on ? " (sul radar)" : "");
    }
    if (itemCount() == 0) App::log(name(), "  nessuno strumento nascosto");
}

bool PokeradarApp::onSerialCommand(const char *line)
{
    if (strncasecmp(line, "pokeradar", 9) != 0) return false;
    const char *arg = line + 9;
    while (*arg == ' ') arg++;
    if (*arg == 0) {
        print();
    } else if (strcasecmp(arg, "spawn") == 0) {
        if (!spawn(cfg::POKERADAR_SPAWN_MIN, 90)) App::log(name(), "Gia' %d strumenti", itemCount());
        print();
    } else if (strncasecmp(arg, "passi", 5) == 0) {
        const long n = atol(arg + 5);
        if (n > 0) addSteps((uint32_t)n);
        print();
    } else if (strcasecmp(arg, "reset") == 0) {
        for (Item &it : items) it.used = 0;
        dirty = true;
        save();
        print();
    } else {
        return false;
    }
    return true;
}

// =============================================================
//  launcher.h - Navigazione: quadrante -> menu -> app
//
//    - tocco sul quadrante         -> menu (griglia 2x2, prima pagina)
//    - scorrimento in su / in giu' -> pagina successiva / precedente
//                                     (4 app per pagina; quadratini a destra)
//    - tocco su un'icona           -> app
//    - scorrimento verso destra
//      o freccia in alto a sinistra -> indietro
//    - schermo spento               -> si torna al quadrante
//  Uscendo da un'app si torna alla pagina di quell'app.
//
//  Tempi di spegnimento: quadrante SCREEN_TIMEOUT_MS,
//  menu MENU_TIMEOUT_MS, app quello che chiede l'app.
//  Dopo un risveglio i tocchi sono ignorati per
//  WAKE_TOUCH_IGNORE_MS (cosi' il tocco che accende lo
//  schermo non apre anche il menu).
// =============================================================
#pragma once
#include <lvgl.h>
#include "watch_module.h"
#include "watch_app.h"

class Launcher : public WatchModule {
public:
    const char *name() const override { return "launcher"; }
    void begin() override;
    void onScreenOn() override;
    void onScreenOff() override;

    // Aggiunge un'app al menu (da chiamare nel setup, prima di App::begin).
    // Registra anche l'app come modulo.
    void addApp(WatchApp *app);

    // Navigazione (usabile anche da altri moduli)
    void showWatchface(bool animate = true);
    void showMenu(bool animate = true);
    void openApp(WatchApp *app);
    void back();

    // Cambia il colore dello schermo e ricolora subito tutte le schermate
    // (quadrante, menu, app, batteria). false = era gia' quello.
    bool applyPalette(int index);

    // Chiamata dal menu quando finisce lo scorrimento tra le pagine
    void menuScrolled();

    // false subito dopo un risveglio
    bool touchAllowed() const;

    // Da usare nei pulsanti delle app: false subito dopo un risveglio
    // o se il "click" era in realta' uno scorrimento
    bool clickAllowed() const;

private:
    enum Page { FACE, MENU, APP };

    void buildMenu();
    void buildAppScreen(int index);
    void closeCurrentApp();
    void load(lv_obj_t *scr, bool forward, bool animate);
    void ignoreTouchesNow();

    void showPage(int p, bool animate);   // scorre il menu alla pagina p
    void refreshDots();                    // quadratini delle pagine

    static constexpr int MAX_APPS = 12;          // 3 pagine
    static constexpr int APPS_PER_PAGE = 4;
    WatchApp *apps[MAX_APPS];
    lv_obj_t *appScreens[MAX_APPS];
    int appCount = 0;

    lv_obj_t *faceScreen = nullptr;
    lv_obj_t *menuScreen = nullptr;
    lv_obj_t *pager = nullptr;                   // contenitore delle pagine (scorre in verticale)
    lv_obj_t *dots[MAX_APPS / APPS_PER_PAGE] = {};
    int pageCount = 1;
    int menuPage = 0;                            // pagina mostrata

    Page page = FACE;
    WatchApp *currentApp = nullptr;
    uint32_t ignoreTouchUntil = 0;
};

extern Launcher launcher;

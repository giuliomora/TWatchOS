// =============================================================
//  launcher.h - Navigazione: quadrante -> menu -> app
//
//    - tocco sul quadrante         -> menu (griglia 2x2)
//    - tocco su un'icona           -> app
//    - scorrimento verso destra
//      o freccia in alto a sinistra -> indietro
//    - schermo spento               -> si torna al quadrante
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

    // false subito dopo un risveglio
    bool touchAllowed() const;

private:
    enum Page { FACE, MENU, APP };

    void buildMenu();
    void buildAppScreen(int index);
    void closeCurrentApp();
    void load(lv_obj_t *scr, bool forward, bool animate);
    void ignoreTouchesNow();

    static constexpr int MAX_APPS = 8;
    WatchApp *apps[MAX_APPS];
    lv_obj_t *appScreens[MAX_APPS];
    int appCount = 0;

    lv_obj_t *faceScreen = nullptr;
    lv_obj_t *menuScreen = nullptr;

    Page page = FACE;
    WatchApp *currentApp = nullptr;
    uint32_t ignoreTouchUntil = 0;
};

extern Launcher launcher;

#include <LilyGoLib.h>
#include <LV_Helper.h>
#include "launcher.h"
#include "screen.h"
#include "app.h"
#include "config.h"

Launcher launcher;

// ---- Funzioni di supporto ----------------------------------

// Crea una schermata vuota, nera, che non scorre
static lv_obj_t *newScreen()
{
    lv_obj_t *scr = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(scr, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, 0);
    lv_obj_remove_flag(scr, LV_OBJ_FLAG_SCROLLABLE);
    return scr;
}

// true se il tocco appena finito era in realta' uno scorrimento.
// (LVGL invia il "click" anche alla fine di uno scorrimento.)
static bool wasSwipe()
{
    lv_indev_t *indev = lv_indev_active();
    return indev && lv_indev_get_gesture_dir(indev) != LV_DIR_NONE;
}

// Scorrimento verso destra = indietro (su menu e app)
static void onGesture(lv_event_t * /*e*/)
{
    if (!launcher.touchAllowed()) return;
    lv_indev_t *indev = lv_indev_active();
    if (!indev) return;
    if (lv_indev_get_gesture_dir(indev) == LV_DIR_RIGHT) {
        lv_indev_wait_release(indev);
        launcher.back();
    }
}

// Tocco sul quadrante = apri il menu
static void onFaceClicked(lv_event_t * /*e*/)
{
    if (!launcher.touchAllowed() || wasSwipe()) return;
    launcher.showMenu();
}

// Tocco su un'icona del menu = apri l'app
static void onTileClicked(lv_event_t *e)
{
    if (!launcher.touchAllowed() || wasSwipe()) return;
    WatchApp *app = (WatchApp *)lv_event_get_user_data(e);
    launcher.openApp(app);
}

// Freccia in alto a sinistra = indietro
static void onBackClicked(lv_event_t * /*e*/)
{
    if (!launcher.touchAllowed() || wasSwipe()) return;
    launcher.back();
}

// ---- Launcher ----------------------------------------------

void Launcher::addApp(WatchApp *app)
{
    if (appCount >= MAX_APPS) {
        App::log(name(), "Troppe app! Aumenta MAX_APPS in launcher.h");
        return;
    }
    apps[appCount] = app;
    appScreens[appCount] = nullptr;
    appCount++;
    App::add(app);
}

void Launcher::begin()
{
    // Il quadrante e' gia' stato disegnato (dal modulo watchface)
    // sulla schermata attiva: diventa la schermata "casa".
    faceScreen = lv_screen_active();
    lv_obj_add_flag(faceScreen, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(faceScreen, onFaceClicked, LV_EVENT_CLICKED, nullptr);

    buildMenu();
    for (int i = 0; i < appCount; i++) buildAppScreen(i);

    page = FACE;
    App::log(name(), "Menu pronto (%d app)", appCount);
}

void Launcher::buildMenu()
{
    menuScreen = newScreen();
    lv_obj_add_event_cb(menuScreen, onGesture, LV_EVENT_GESTURE, nullptr);

    // Griglia 2x2 sotto la barra della batteria
    const int TILE_W = 104, TILE_H = 92;
    const int X0 = 12, X1 = 124;
    const int Y0 = 36, Y1 = 140;

    for (int i = 0; i < appCount && i < 4; i++) {
        WatchApp *app = apps[i];
        lv_obj_t *tile = lv_button_create(menuScreen);
        lv_obj_set_size(tile, TILE_W, TILE_H);
        lv_obj_set_pos(tile, (i % 2) ? X1 : X0, (i / 2) ? Y1 : Y0);
        lv_obj_set_style_bg_color(tile, lv_color_hex(app->color()), 0);
        lv_obj_set_style_radius(tile, 18, 0);
        lv_obj_set_style_shadow_width(tile, 0, 0);
        lv_obj_remove_flag(tile, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_add_event_cb(tile, onTileClicked, LV_EVENT_CLICKED, app);

        lv_obj_t *icon = lv_label_create(tile);
        lv_obj_set_style_text_font(icon, &lv_font_montserrat_28, 0);
        lv_obj_set_style_text_color(icon, lv_color_white(), 0);
        lv_label_set_text(icon, app->icon());
        lv_obj_align(icon, LV_ALIGN_CENTER, 0, -12);

        lv_obj_t *label = lv_label_create(tile);
        lv_obj_set_style_text_font(label, &lv_font_montserrat_14, 0);
        lv_obj_set_style_text_color(label, lv_color_white(), 0);
        lv_label_set_text(label, app->title());
        lv_obj_align(label, LV_ALIGN_BOTTOM_MID, 0, 0);
    }
    if (appCount > 4) App::log(name(), "Il menu mostra solo le prime 4 app");
}

void Launcher::buildAppScreen(int index)
{
    WatchApp *app = apps[index];
    lv_obj_t *scr = newScreen();
    lv_obj_add_event_cb(scr, onGesture, LV_EVENT_GESTURE, nullptr);

    // Freccia "indietro" in alto a sinistra
    lv_obj_t *backBtn = lv_button_create(scr);
    lv_obj_set_size(backBtn, 52, 40);
    lv_obj_set_pos(backBtn, 0, 0);
    lv_obj_set_style_bg_opa(backBtn, LV_OPA_TRANSP, 0);
    lv_obj_set_style_shadow_width(backBtn, 0, 0);
    lv_obj_add_event_cb(backBtn, onBackClicked, LV_EVENT_CLICKED, nullptr);
    lv_obj_t *arrow = lv_label_create(backBtn);
    lv_obj_set_style_text_font(arrow, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(arrow, lv_color_white(), 0);
    lv_label_set_text(arrow, LV_SYMBOL_LEFT);
    lv_obj_center(arrow);

    // Titolo, sotto la barra della batteria
    lv_obj_t *title = lv_label_create(scr);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(title, lv_color_hex(app->color()), 0);
    lv_label_set_text(title, app->title());
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 38);

    // Area in cui l'app disegna il suo contenuto
    lv_obj_t *content = lv_obj_create(scr);
    lv_obj_set_size(content, 240, 170);
    lv_obj_set_pos(content, 0, 70);
    lv_obj_set_style_bg_opa(content, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(content, 0, 0);
    lv_obj_set_style_pad_all(content, 0, 0);
    lv_obj_remove_flag(content, LV_OBJ_FLAG_SCROLLABLE);

    app->build(content);
    appScreens[index] = scr;
}

void Launcher::load(lv_obj_t *scr, bool forward, bool animate)
{
    if (!scr) return;
    if (animate && cfg::MENU_ANIM_MS > 0) {
        lv_screen_load_anim(scr, forward ? LV_SCREEN_LOAD_ANIM_MOVE_LEFT
                                         : LV_SCREEN_LOAD_ANIM_MOVE_RIGHT,
                            cfg::MENU_ANIM_MS, 0, false);
    } else {
        lv_screen_load(scr);
    }
}

void Launcher::closeCurrentApp()
{
    if (!currentApp) return;
    currentApp->open = false;
    currentApp->onClose();
    currentApp = nullptr;
}

void Launcher::showWatchface(bool animate)
{
    closeCurrentApp();
    load(faceScreen, false, animate);
    page = FACE;
    screen.setTimeout(cfg::SCREEN_TIMEOUT_MS);
}

void Launcher::showMenu(bool animate)
{
    const bool forward = (page == FACE);
    closeCurrentApp();
    load(menuScreen, forward, animate);
    page = MENU;
    screen.setTimeout(cfg::MENU_TIMEOUT_MS);
    App::log(name(), "Menu");
}

void Launcher::openApp(WatchApp *app)
{
    for (int i = 0; i < appCount; i++) {
        if (apps[i] != app) continue;
        closeCurrentApp();
        load(appScreens[i], true, true);
        page = APP;
        currentApp = app;
        app->open = true;
        app->onOpen();
        screen.setTimeout(app->screenTimeoutMs());
        App::log(name(), "Aperta app: %s", app->title());
        return;
    }
}

void Launcher::back()
{
    if (page == APP) showMenu();
    else if (page == MENU) showWatchface();
}

void Launcher::ignoreTouchesNow()
{
    ignoreTouchUntil = millis() + cfg::WAKE_TOUCH_IGNORE_MS;
    if (ignoreTouchUntil == 0) ignoreTouchUntil = 1;
    // Ignora anche il dito ancora appoggiato, finche' non si stacca
    for (lv_indev_t *i = lv_indev_get_next(NULL); i; i = lv_indev_get_next(i)) {
        if (lv_indev_get_type(i) == LV_INDEV_TYPE_POINTER) lv_indev_wait_release(i);
    }
}

bool Launcher::touchAllowed() const
{
    return (int32_t)(millis() - ignoreTouchUntil) >= 0;
}

void Launcher::onScreenOn()
{
    // Al risveglio si e' sempre sul quadrante (vedi onScreenOff)
    ignoreTouchesNow();
    screen.setTimeout(cfg::SCREEN_TIMEOUT_MS);
}

void Launcher::onScreenOff()
{
    // Torna al quadrante subito, senza animazione:
    // al prossimo risveglio sara' gia' pronto.
    if (page != FACE) showWatchface(false);
}

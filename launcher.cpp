#include <LilyGoLib.h>
#include <LV_Helper.h>
#include "launcher.h"
#include "screen.h"
#include "app.h"
#include "config.h"
#include "user_settings.h"
#include "theme.h"

Launcher launcher;

// ---- Funzioni di supporto ----------------------------------

// Crea una schermata vuota (schermo LCD verde) che non scorre
static lv_obj_t *newScreen()
{
    lv_obj_t *scr = lv_obj_create(NULL);
    pk::styleScreen(scr);
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

// Fine dello scorrimento tra le pagine del menu
static void onMenuScrollEnd(lv_event_t * /*e*/)
{
    launcher.menuScrolled();
}

// Geometria del menu
static const int PAGER_Y = 34;        // sotto la batteria
static const int PAGE_H = 204;        // altezza di una pagina (fino al bordo in basso)
static const int TILE_W = 100, TILE_H = 92;
static const int TILE_X0 = 10, TILE_X1 = 116;
static const int TILE_Y0 = 2, TILE_Y1 = 106;   // dentro la pagina
static const int DOT = 8, DOT_GAP = 6, DOT_X = 222;

void Launcher::buildMenu()
{
    menuScreen = newScreen();
    lv_obj_add_event_cb(menuScreen, onGesture, LV_EVENT_GESTURE, nullptr);

    pageCount = (appCount + APPS_PER_PAGE - 1) / APPS_PER_PAGE;
    if (pageCount < 1) pageCount = 1;

    // Contenitore delle pagine: scorre in verticale e si aggancia
    // sempre a una pagina intera (una alla volta). Agli estremi
    // si ferma con un piccolo rimbalzo.
    pager = lv_obj_create(menuScreen);
    lv_obj_set_size(pager, DOT_X - 4, PAGE_H);
    lv_obj_set_pos(pager, 0, PAGER_Y);
    pk::styleClear(pager);
    lv_obj_set_scroll_dir(pager, LV_DIR_VER);
    lv_obj_set_scroll_snap_y(pager, LV_SCROLL_SNAP_START);
    lv_obj_add_flag(pager, LV_OBJ_FLAG_SCROLL_ONE);
    lv_obj_set_scrollbar_mode(pager, LV_SCROLLBAR_MODE_OFF);
    lv_obj_add_event_cb(pager, onMenuScrollEnd, LV_EVENT_SCROLL_END, nullptr);

    for (int p = 0; p < pageCount; p++) {
        lv_obj_t *page = lv_obj_create(pager);
        lv_obj_set_size(page, DOT_X - 4, PAGE_H);
        lv_obj_set_pos(page, 0, p * PAGE_H);
        pk::styleClear(page);
        lv_obj_remove_flag(page, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_remove_flag(page, LV_OBJ_FLAG_CLICKABLE);   // il dito fa scorrere il contenitore

        for (int k = 0; k < APPS_PER_PAGE; k++) {
            const int i = p * APPS_PER_PAGE + k;
            if (i >= appCount) break;   // posti vuoti: si riempiono con le app nuove
            WatchApp *app = apps[i];
            lv_obj_t *tile = lv_button_create(page);
            lv_obj_set_size(tile, TILE_W, TILE_H);
            lv_obj_set_pos(tile, (k % 2) ? TILE_X1 : TILE_X0, (k / 2) ? TILE_Y1 : TILE_Y0);
            pk::styleButton(tile);
            lv_obj_set_style_pad_all(tile, 0, 0);
            lv_obj_remove_flag(tile, LV_OBJ_FLAG_SCROLLABLE);
            lv_obj_add_event_cb(tile, onTileClicked, LV_EVENT_CLICKED, app);

            if (!app->drawIcon(tile)) {
                lv_obj_t *icon = lv_label_create(tile);
                pk::styleLabel(icon, &lv_font_montserrat_28);
                lv_label_set_text(icon, app->icon());
                lv_obj_align(icon, LV_ALIGN_CENTER, 0, -12);
            }

            lv_obj_t *label = lv_label_create(tile);
            pk::styleLabel(label, &pk_font_small);
            lv_label_set_text(label, app->title());
            lv_obj_align(label, LV_ALIGN_BOTTOM_MID, 0, -4);
        }
    }

    // Quadratini delle pagine, in colonna sul lato destro (solo se le pagine sono piu' di una)
    if (pageCount > 1) {
        const int total = pageCount * DOT + (pageCount - 1) * DOT_GAP;
        const int y0 = PAGER_Y + (PAGE_H - total) / 2;
        for (int p = 0; p < pageCount; p++) {
            dots[p] = lv_obj_create(menuScreen);
            lv_obj_set_size(dots[p], DOT, DOT);
            lv_obj_set_pos(dots[p], DOT_X + 4, y0 + p * (DOT + DOT_GAP));
            lv_obj_set_style_radius(dots[p], 0, 0);
            lv_obj_set_style_pad_all(dots[p], 0, 0);
            lv_obj_set_style_border_color(dots[p], pk::color(pk::INK), 0);
            lv_obj_set_style_border_width(dots[p], 2, 0);
            lv_obj_set_style_bg_color(dots[p], pk::color(pk::INK), 0);
            lv_obj_remove_flag(dots[p], LV_OBJ_FLAG_SCROLLABLE);
            lv_obj_remove_flag(dots[p], LV_OBJ_FLAG_CLICKABLE);
        }
    }
    refreshDots();
}

void Launcher::refreshDots()
{
    for (int p = 0; p < pageCount; p++) {
        if (!dots[p]) continue;
        // pagina attuale: pieno; le altre: solo il bordo
        lv_obj_set_style_bg_opa(dots[p], p == menuPage ? LV_OPA_COVER : LV_OPA_TRANSP, 0);
    }
}

void Launcher::showPage(int p, bool animate)
{
    if (p < 0) p = 0;
    if (p >= pageCount) p = pageCount - 1;
    menuPage = p;
    if (pager) lv_obj_scroll_to_y(pager, p * PAGE_H, animate ? LV_ANIM_ON : LV_ANIM_OFF);
    refreshDots();
}

void Launcher::menuScrolled()
{
    if (!pager) return;
    const int y = lv_obj_get_scroll_y(pager);
    int p = (y + PAGE_H / 2) / PAGE_H;
    if (p < 0) p = 0;
    if (p >= pageCount) p = pageCount - 1;
    if (p != menuPage) {
        menuPage = p;
        refreshDots();
        App::log(name(), "Menu: pagina %d", p + 1);
    }
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
    pk::styleClear(backBtn);
    lv_obj_set_style_color_filter_opa(backBtn, LV_OPA_TRANSP, LV_STATE_PRESSED);
    lv_obj_add_event_cb(backBtn, onBackClicked, LV_EVENT_CLICKED, nullptr);
    lv_obj_t *arrow = lv_label_create(backBtn);
    pk::styleLabel(arrow, &lv_font_montserrat_20);
    lv_label_set_text(arrow, LV_SYMBOL_LEFT);
    lv_obj_center(arrow);

    // Titolo, sotto la barra della batteria (se l'app lo vuole)
    if (!app->hideTitle()) {
        lv_obj_t *title = lv_label_create(scr);
        pk::styleLabel(title, &pk_font_text);
        lv_label_set_text(title, app->title());
        lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 40);
    }

    // Area in cui l'app disegna il suo contenuto
    const int contentY = app->hideTitle() ? 40 : 70;
    lv_obj_t *content = lv_obj_create(scr);
    lv_obj_set_size(content, 240, 240 - contentY);
    lv_obj_set_pos(content, 0, contentY);
    pk::styleClear(content);
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
    screen.setTimeout(userSettings.faceTimeoutMs());
}

void Launcher::showMenu(bool animate)
{
    const bool forward = (page == FACE);
    // Dal quadrante: sempre la prima pagina. Da un'app: la pagina di quell'app.
    int p = 0;
    if (page == APP && currentApp) {
        for (int i = 0; i < appCount; i++)
            if (apps[i] == currentApp) p = i / APPS_PER_PAGE;
    }
    showPage(p, false);
    closeCurrentApp();
    load(menuScreen, forward, animate);
    page = MENU;
    screen.setTimeout(userSettings.menuTimeoutMs());
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
    if (page == APP && currentApp && currentApp->onBack()) return;  // l'app torna indietro da sola
    if (page == APP) showMenu();
    else if (page == MENU) showWatchface();
}

bool Launcher::applyPalette(int index)
{
    const int from = pk::palette();
    if (!pk::setPalette(index)) return false;
    // La schermata attiva e le altre gia' disegnate
    pk::recolor(faceScreen, from);
    pk::recolor(menuScreen, from);
    for (int i = 0; i < appCount; i++) pk::recolor(appScreens[i], from);
    pk::recolor(lv_layer_top(), from);   // batteria in alto a destra
    for (int i = 0; i < appCount; i++) apps[i]->onPaletteChanged();
    App::log(name(), "Colore dello schermo: %s", pk::paletteName(index));
    return true;
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

bool Launcher::clickAllowed() const
{
    return touchAllowed() && !wasSwipe();
}

void Launcher::onScreenOn()
{
    // Al risveglio si e' sempre sul quadrante (vedi onScreenOff)
    ignoreTouchesNow();
    screen.setTimeout(userSettings.faceTimeoutMs());
}

void Launcher::onScreenOff()
{
    // Torna al quadrante subito, senza animazione:
    // al prossimo risveglio sara' gia' pronto.
    if (page != FACE) showWatchface(false);
}

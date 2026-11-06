// =============================================================
//  empty_app.h - App "vuota" (segnaposto)
//
//  Serve per provare la navigazione: mostra solo "In arrivo".
//  Ogni app vera prendera' il posto di uno di questi segnaposto
//  con un suo file (es. steps_app.h / steps_app.cpp).
// =============================================================
#pragma once
#include "watch_app.h"

class EmptyApp : public WatchApp {
public:
    EmptyApp(const char *title, const char *icon, uint32_t color)
        : appTitle(title), appIcon(icon), appColor(color) {}

    const char *title() const override { return appTitle; }
    const char *icon() const override { return appIcon; }
    uint32_t color() const override { return appColor; }
    void build(lv_obj_t *parent) override;

private:
    const char *appTitle;
    const char *appIcon;
    uint32_t appColor;
};

// App del menu ancora vuote (Passi e Cronometro hanno il loro file)
extern EmptyApp batteryApp;
extern EmptyApp settingsApp;

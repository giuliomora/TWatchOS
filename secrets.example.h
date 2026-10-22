// =============================================================
//  secrets.example.h - Modello per secrets.h
//
//  1. Copia questo file e rinomina la copia in "secrets.h"
//  2. Scrivi nome e password della tua rete Wi-Fi (solo 2.4 GHz)
//
//  secrets.h e' escluso da Git (.gitignore): la password
//  resta solo sul tuo computer.
// =============================================================
#pragma once

namespace secrets {
constexpr const char *WIFI_SSID     = "";   // nome della rete, es. "CasaMia"
constexpr const char *WIFI_PASSWORD = "";   // password della rete
}

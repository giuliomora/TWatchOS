#include <Arduino.h>
#include <stdarg.h>
#include <string.h>
#include "app.h"
#include "config.h"

namespace {
constexpr int MAX_MODULES = 16;
WatchModule *modules[MAX_MODULES];
int moduleCount = 0;

// Buffer per i comandi in arrivo dal Serial Monitor
char serialLine[96];
size_t serialLen = 0;

void handleSerialLine(char *line)
{
    // Togli spazi e caratteri invisibili all'inizio e alla fine
    while (*line && (unsigned char)*line <= ' ') line++;
    size_t n = strlen(line);
    while (n > 0 && (unsigned char)line[n - 1] <= ' ') line[--n] = '\0';
    if (line[0] == '\0') return;
    App::log("app", "Ricevuto: \"%s\"", line);
    for (int i = 0; i < moduleCount; i++) {
        if (modules[i]->onSerialCommand(line)) return;
    }
    App::log("app", "Comando non riconosciuto: %s", line);
}

void readSerial()
{
    while (Serial.available()) {
        char c = (char)Serial.read();
        if (c == '\r') continue;
        if (c == '\n') {
            serialLine[serialLen] = '\0';
            handleSerialLine(serialLine);
            serialLen = 0;
        } else if (serialLen < sizeof(serialLine) - 1) {
            serialLine[serialLen++] = c;
        }
    }
}
}  // namespace

namespace App {

void add(WatchModule *module)
{
    if (moduleCount >= MAX_MODULES) {
        log("app", "Troppi moduli! Aumenta MAX_MODULES in app.cpp");
        return;
    }
    modules[moduleCount++] = module;
}

void begin()
{
    for (int i = 0; i < moduleCount; i++) {
        log("app", "Avvio modulo: %s", modules[i]->name());
        modules[i]->begin();
    }
    log("app", "Pronto (%d moduli)", moduleCount);
}

void update()
{
    readSerial();
    for (int i = 0; i < moduleCount; i++) {
        modules[i]->update();
    }
}

void notifyScreenOn()
{
    for (int i = 0; i < moduleCount; i++) modules[i]->onScreenOn();
}

void notifyScreenOff()
{
    for (int i = 0; i < moduleCount; i++) modules[i]->onScreenOff();
}

bool canSleep()
{
    if (Serial.available()) return false;  // c'e' un comando da leggere
    for (int i = 0; i < moduleCount; i++) {
        if (!modules[i]->canSleep()) return false;
    }
    return true;
}

uint32_t msUntilNextWork()
{
    uint32_t best = UINT32_MAX;
    for (int i = 0; i < moduleCount; i++) {
        uint32_t ms = modules[i]->msUntilNextWork();
        if (ms < best) best = ms;
    }
    return best;
}

void log(const char *tag, const char *fmt, ...)
{
    if (!cfg::DEBUG_LOG) return;
    char buf[160];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);
    Serial.printf("[%8lu][%s] %s\n", (unsigned long)millis(), tag, buf);
}

}  // namespace App

#include <LilyGoLib.h>
#include <math.h>
#include <strings.h>
#include "inputs.h"
#include "screen.h"
#include "app.h"
#include "config.h"

Inputs inputs;

// Stampa l'inclinazione attuale dell'orologio (per il comando "polso")
static void logWristPosition(const char *what)
{
    AccelerometerData d;
    if (!instance.bma423.readData(d)) {
        App::log("inputs", "%s (accelerometro non leggibile)", what);
        return;
    }
    const float g = 9.80665f;
    const float x = d.mps2.x / g, y = d.mps2.y / g, z = d.mps2.z / g;
    const float n = sqrtf(x * x + y * y + z * z);
    const float angle = n > 0.1f ? acosf(fmaxf(-1.0f, fminf(1.0f, z / n))) * 57.2958f : -1;
    App::log("inputs", "%s  x=%+.2f y=%+.2f z=%+.2f g  inclinazione %.0f gradi", what, x, y, z, angle);
}

// Riceve gli eventi hardware dalla libreria LilyGoLib
// (sensore di movimento, pulsante della corona, batteria, ...)
static void onDeviceEvent(const DeviceEvent &event, void * /*user_data*/)
{
    if (event.type == SENSOR_EVENT) {
        if (instance.getSensorEventType(event) == SENSOR_TILT_DETECTED &&
            cfg::WAKE_ON_WRIST_TILT) {
            if (inputs.wristDebug) logWristPosition("Gesto del polso");
            screen.wake("polso");
        }
    }

    if (event.type == POWER_EVENT) {
        if (instance.getPMUEventType(event) == PMU_EVENT_KEY_CLICKED &&
            cfg::WAKE_ON_CROWN_CLICK) {
            screen.wake("corona");
        }
    }
}

void Inputs::begin()
{
    instance.onEvent(onDeviceEvent);

    // LilyGoLib imposta il gesto del polso in modalita' "smartphone":
    // su un orologio funziona meglio quella "braccialetto".
    if (cfg::WRIST_MODE_WRISTBAND) {
        bool ok = instance.bma423.selectPlatform(SensorBMA423::Platform::WRISTBAND);
        App::log(name(), "Gesto del polso: modalita' braccialetto %s", ok ? "attiva" : "NON riuscita");
    }
}

void Inputs::update()
{
    if (cfg::WAKE_ON_TOUCH) pollTouch();
}

void Inputs::pollTouch()
{
    int16_t x, y;
    if (instance.getPoint(&x, &y, 1) > 0) {
        // Ogni tocco accende lo schermo o lo tiene acceso
        screen.wake("tocco");
    }
}

bool Inputs::onSerialCommand(const char *line)
{
    if (strcasecmp(line, "polso") != 0) return false;
    wristDebug = !wristDebug;
    App::log(name(), "Messaggi del gesto del polso: %s", wristDebug ? "ACCESI" : "spenti");
    if (wristDebug) logWristPosition("Posizione attuale");
    return true;
}

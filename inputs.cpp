#include <LilyGoLib.h>
#include "inputs.h"
#include "screen.h"
#include "app.h"
#include "config.h"

Inputs inputs;

// Riceve gli eventi hardware dalla libreria LilyGoLib
// (sensore di movimento, pulsante della corona, batteria, ...)
static void onDeviceEvent(const DeviceEvent &event, void * /*user_data*/)
{
    if (event.type == SENSOR_EVENT) {
        if (instance.getSensorEventType(event) == SENSOR_TILT_DETECTED &&
            cfg::WAKE_ON_WRIST_TILT) {
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

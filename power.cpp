#include <LilyGoLib.h>
#include <driver/gpio.h>
#include <esp_sleep.h>
#include <string.h>
#include <strings.h>
#include "power.h"
#include "screen.h"
#include "app.h"
#include "config.h"

Power power;

// Somma di tutti gli eventi registrati dal sensore di movimento:
// se cambia dopo il risveglio, a svegliarci e' stato il sensore.
static uint32_t sensorEventTotal()
{
    BMASensorRuntimeStatus_t s = instance.getRuntimeStatus();
    return s.stepEvents + s.singleTaps + s.doubleTaps + s.tripleTaps +
           s.activityEvents + s.tiltEvents + s.motionEvents + s.noMotionEvents;
}

static uint32_t tiltEventTotal()
{
    return instance.getRuntimeStatus().tiltEvents;
}

// Interrupt "normali" dei pin, da ripristinare dopo la sospensione
// (sono quelli impostati da LilyGoLib in LilyGoWatchS3.cpp)
static void restorePinInterrupt(int pin, gpio_int_type_t type)
{
    gpio_wakeup_disable((gpio_num_t)pin);
    gpio_set_intr_type((gpio_num_t)pin, type);
    gpio_intr_enable((gpio_num_t)pin);
}

void Power::begin()
{
    setCpuFrequencyMhz(cfg::CPU_MHZ);

    // Spegni le periferiche che non usiamo
    instance.powerControl(POWER_GPS, false);
    instance.powerControl(POWER_SPEAK, false);
#if defined(LILYGO_RADIO_CLASS) && !defined(ARDUINO_LILYGO_LORA_SX1280)
    radio.sleep();  // la radio LoRa per ora non serve
#endif

    resetStats();
    App::log(name(), "CPU a %u MHz, sospensione %s", (unsigned)getCpuFrequencyMhz(),
             cfg::SLEEP_ENABLED ? "attiva" : "disattivata");
}

void Power::resetStats()
{
    statsSince = millis();
    sleptMs = 0;
    sleepCount = 0;
    wakeTouch = wakeTilt = wakeCrown = wakeSensorOther = wakeTimer = 0;
    batteryAtStart = instance.getBatteryPercent();
}

bool Power::usbConnected()
{
    if (millis() - usbCheckedAt > 1000 || usbCheckedAt == 0) {
        usbCached = instance.isUsbIn();
        usbCheckedAt = millis();
    }
    return usbCached;
}

void Power::update()
{
    if (!cfg::SLEEP_ENABLED) return;
    if (!App::canSleep()) return;
    if (!cfg::SLEEP_ON_USB && usbConnected()) return;

    uint32_t maxMs = App::msUntilNextWork();
    if (maxMs < 50) return;  // c'e' da lavorare tra pochissimo: non vale la pena
    lightSleep(maxMs);
}

void Power::lightSleep(uint32_t maxMs)
{
    const uint32_t sensorBefore = sensorEventTotal();
    const uint32_t tiltBefore = tiltEventTotal();

    // Il chip di alimentazione deve "svegliarci" solo per la corona
    PmicIrqBase *irq = instance.pmic.getIrq();
    irq->disable(AXP2101Irq::IRQ_ALL_MASK);
    if (cfg::WAKE_ON_CROWN_CLICK) irq->enable(AXP2101Irq::IRQ_PEKEY_SHORT_PRESS);
    irq->clearStatus();

    // Sorgenti di risveglio
    esp_sleep_disable_wakeup_source(ESP_SLEEP_WAKEUP_ALL);
    gpio_intr_disable((gpio_num_t)TP_INT);
    gpio_intr_disable((gpio_num_t)SENSOR_INT);
    gpio_intr_disable((gpio_num_t)PMU_INT);
    if (cfg::WAKE_ON_TOUCH) gpio_wakeup_enable((gpio_num_t)TP_INT, GPIO_INTR_LOW_LEVEL);
    gpio_wakeup_enable((gpio_num_t)SENSOR_INT, GPIO_INTR_HIGH_LEVEL);
    if (cfg::WAKE_ON_CROWN_CLICK) gpio_wakeup_enable((gpio_num_t)PMU_INT, GPIO_INTR_LOW_LEVEL);
    esp_sleep_enable_gpio_wakeup();
    if (maxMs != UINT32_MAX) esp_sleep_enable_timer_wakeup((uint64_t)maxMs * 1000ULL);

    Serial.flush();
    const uint32_t t0 = millis();
    esp_light_sleep_start();                    // ---- qui il processore dorme ----
    const uint32_t slept = millis() - t0;
    const esp_sleep_wakeup_cause_t cause = esp_sleep_get_wakeup_cause();

    // Leggi subito il pin della corona, prima che il segnale sparisca
    const bool crownPin = digitalRead(PMU_INT) == LOW;

    // Ripristina tutto com'era
    esp_sleep_disable_wakeup_source(ESP_SLEEP_WAKEUP_ALL);
    restorePinInterrupt(TP_INT, GPIO_INTR_NEGEDGE);
    restorePinInterrupt(SENSOR_INT, GPIO_INTR_POSEDGE);
    restorePinInterrupt(PMU_INT, GPIO_INTR_NEGEDGE);
    irq->disable(AXP2101Irq::IRQ_ALL_MASK);
    irq->enable(AXP2101Irq::IRQ_VBUS_INSERT | AXP2101Irq::IRQ_VBUS_REMOVE |
                AXP2101Irq::IRQ_BAT_CHG_START | AXP2101Irq::IRQ_BAT_CHG_DONE |
                AXP2101Irq::IRQ_PEKEY_SHORT_PRESS | AXP2101Irq::IRQ_PEKEY_LONG_PRESS);
    irq->clearStatus();

    // Fai leggere al sensore cosa e' successo: se era una rotazione
    // del polso, il modulo inputs accende lo schermo.
    instance.loopSensor();
    const bool sensorFired = sensorEventTotal() != sensorBefore;
    const bool tiltFired = tiltEventTotal() != tiltBefore;

    sleepCount++;
    sleptMs += slept;

    // Capisci chi ci ha svegliato
    if (cause == ESP_SLEEP_WAKEUP_TIMER) {
        wakeTimer++;
    } else if (crownPin) {
        wakeCrown++;
        screen.wake("corona");
    } else if (tiltFired) {
        wakeTilt++;  // lo schermo lo ha gia' acceso inputs
    } else if (sensorFired) {
        wakeSensorOther++;  // es. un passo: si torna a dormire
    } else if (cfg::WAKE_ON_TOUCH) {
        // nessun'altra causa: e' stato il tocco
        wakeTouch++;
        screen.wake("tocco");
    }
}

void Power::printStatus()
{
    const float pct = instance.getBatteryPercent();
    float volt = instance.getBattVoltage();
    if (volt > 100) volt /= 1000.0f;  // alcune versioni restituiscono mV
    const uint32_t elapsed = millis() - statsSince;
    const float hours = elapsed / 3600000.0f;
    const float sleepPct = elapsed ? (100.0f * (float)sleptMs / (float)elapsed) : 0;

    App::log(name(), "Batteria: %.0f%%  (%.2f V)  USB: %s", pct, volt,
             instance.isUsbIn() ? "collegato" : "scollegato");
    App::log(name(), "CPU: %u MHz  Sospensione: %s%s", (unsigned)getCpuFrequencyMhz(),
             cfg::SLEEP_ENABLED ? "attiva" : "disattivata",
             (!cfg::SLEEP_ON_USB && instance.isUsbIn()) ? " (in pausa: USB collegato)" : "");
    App::log(name(), "Statistiche delle ultime %.2f ore:", hours);
    App::log(name(), "  in sospensione il %.1f%% del tempo (%lu volte)",
             sleepPct, (unsigned long)sleepCount);
    App::log(name(), "  risvegli: polso %lu, tocco %lu, corona %lu, altri sensore %lu, timer %lu",
             (unsigned long)wakeTilt, (unsigned long)wakeTouch, (unsigned long)wakeCrown,
             (unsigned long)wakeSensorOther, (unsigned long)wakeTimer);
    if (batteryAtStart >= 0 && hours > 0.25f) {
        const float used = batteryAtStart - pct;
        App::log(name(), "  batteria usata: %.0f%% (circa %.1f%% all'ora)", used, used / hours);
    }
}

bool Power::onSerialCommand(const char *line)
{
    if (strcasecmp(line, "power") == 0) {
        printStatus();
        return true;
    }
    if (strcasecmp(line, "power reset") == 0) {
        resetStats();
        App::log(name(), "Statistiche azzerate");
        return true;
    }
    return false;
}

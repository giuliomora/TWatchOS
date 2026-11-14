#include <LilyGoLib.h>
#include <math.h>
#include <strings.h>
#include "sound.h"
#include "user_settings.h"
#include "config.h"
#include "app.h"

Sound sound;

static const uint32_t RATE = 16000;      // campioni al secondo
static const int CHUNK = 256;            // campioni scritti a ogni update (16 ms)

void Sound::enable(bool on)
{
    AudioOutputIf *out = instance.getAudioOutput();
    if (!out) return;
    if (on && userSettings.sound && !opened) {
        out->open(16, 1, RATE);   // accende l'amplificatore (LilyGoLib)
        out->setVolume(cfg::SOUND_VOLUME);
        opened = true;
    } else if (!on && opened) {
        qCount = 0;
        playing = false;
        out->close();             // spegne l'amplificatore
        opened = false;
    }
}

void Sound::note(uint16_t f0, uint16_t f1, uint16_t ms)
{
    if (!opened || qCount >= QUEUE) return;
    queue[(qHead + qCount) % QUEUE] = {f0, f1, ms};
    qCount++;
    playing = true;
}

void Sound::ping()
{
    note(1900, 1150, 140);   // scende come un sonar
}

void Sound::update()
{
    if (!opened || !playing) return;
    AudioOutputIf *out = instance.getAudioOutput();
    if (!out) return;

    static int16_t buf[CHUNK];
    int n = 0;
    while (n < CHUNK) {
        if (len == 0) {
            // nota successiva
            if (qCount == 0) {
                // fine: un po' di silenzio, cosi' l'altoparlante non ronza
                if (tail <= 0) tail = 512;
                while (n < CHUNK && tail > 0) { buf[n++] = 0; tail--; }
                if (tail == 0) playing = false;
                break;
            }
            tail = 0;
            pos = 0;
            len = (uint32_t)queue[qHead].ms * RATE / 1000;
            if (len == 0) len = 1;
        }
        const Note &nt = queue[qHead];
        int16_t s = 0;
        if (nt.f0 > 0) {
            const float t = (float)pos / len;
            const float f = nt.f0 + (nt.f1 - nt.f0) * t;
            phase += 2.0f * (float)M_PI * f / RATE;
            if (phase > 2.0f * (float)M_PI) phase -= 2.0f * (float)M_PI;
            // sfumatura: attacco rapido, coda che si spegne
            float env = 1.0f;
            if (pos < 48) env = pos / 48.0f;
            if (t > 0.6f) env *= (1.0f - t) / 0.4f;
            s = (int16_t)(sinf(phase) * env * 9000.0f);
        }
        buf[n++] = s;
        if (++pos >= len) {
            len = 0;
            qHead = (qHead + 1) % QUEUE;
            qCount--;
        }
    }
    if (n > 0) out->write((const uint8_t *)buf, n * sizeof(int16_t));
}

bool Sound::onSerialCommand(const char *line)
{
    if (strcasecmp(line, "suono") != 0) return false;
    const bool wasOpen = opened;
    enable(true);
    if (!opened) {
        App::log(name(), "Suono spento nelle Impostazioni");
        return true;
    }
    ping();
    // suona subito (dal Serial Monitor non c'e' un'app aperta)
    while (playing) update();
    if (!wasOpen) enable(false);
    return true;
}

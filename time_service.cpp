#include <LilyGoLib.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/time.h>
#include "time_service.h"
#include "app.h"
#include "config.h"

TimeService timeService;

// Giorni dal 1/1/1970 per una data (algoritmo di H. Hinnant).
// Serve per convertire l'ora UTC dell'RTC in secondi senza
// dipendere dal fuso orario.
static int64_t daysFromCivil(int y, unsigned m, unsigned d)
{
    y -= m <= 2;
    const int era = (y >= 0 ? y : y - 399) / 400;
    const unsigned yoe = (unsigned)(y - era * 400);
    const unsigned doy = (153 * (m + (m > 2 ? -3 : 9)) + 2) / 5 + d - 1;
    const unsigned doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
    return (int64_t)era * 146097 + (int64_t)doe - 719468;
}

static void setSystemTime(time_t t)
{
    struct timeval tv = {t, 0};
    settimeofday(&tv, nullptr);
}

void TimeService::begin()
{
    // Fuso orario con regole ora solare/legale
    setenv("TZ", cfg::TIMEZONE, 1);
    tzset();

    struct tm r;
    memset(&r, 0, sizeof(r));
    instance.rtc.getDateTime(&r);  // UTC
    int year = r.tm_year + 1900;
    bool valid = instance.rtc.isClockIntegrityGuaranteed() && year >= 2025 && year < 2100;

    if (valid) {
        time_t t = (time_t)(daysFromCivil(year, r.tm_mon + 1, r.tm_mday) * 86400LL +
                            r.tm_hour * 3600 + r.tm_min * 60 + r.tm_sec);
        setSystemTime(t);
    } else {
        // Nessuna ora valida: si parte dall'ora di compilazione.
        // Verra' corretta dal Wi-Fi o dal comando "ora".
        RTC_DateTime b(__DATE__, __TIME__);
        set(b.getYear(), b.getMonth(), b.getDay(), b.getHour(), b.getMinute(), b.getSecond());
        App::log(name(), "Ora non valida: impostata all'ora di compilazione");
    }

    struct tm t = now();
    App::log(name(), "Ora locale: %04d-%02d-%02d %02d:%02d:%02d",
             t.tm_year + 1900, t.tm_mon + 1, t.tm_mday, t.tm_hour, t.tm_min, t.tm_sec);
}

struct tm TimeService::now()
{
    time_t t = time(nullptr);
    struct tm lt;
    localtime_r(&t, &lt);
    return lt;
}

void TimeService::saveSystemTimeToRtc()
{
    time_t t = time(nullptr);
    struct tm u;
    gmtime_r(&t, &u);
    instance.rtc.setDateTime(RTC_DateTime(u.tm_year + 1900, u.tm_mon + 1, u.tm_mday,
                                          u.tm_hour, u.tm_min, u.tm_sec));
}

void TimeService::set(int year, int month, int day, int hour, int minute, int second)
{
    struct tm lt;
    memset(&lt, 0, sizeof(lt));
    lt.tm_year = year - 1900;
    lt.tm_mon = month - 1;
    lt.tm_mday = day;
    lt.tm_hour = hour;
    lt.tm_min = minute;
    lt.tm_sec = second;
    lt.tm_isdst = -1;  // lascia decidere se e' ora legale
    setSystemTime(mktime(&lt));
    saveSystemTimeToRtc();
    App::log(name(), "Ora impostata: %04d-%02d-%02d %02d:%02d:%02d",
             year, month, day, hour, minute, second);
}

bool TimeService::onSerialCommand(const char *line)
{
    // Accetta "ora 2026-10-22 16:35:00", "ORA ...", "time ..."
    // oppure direttamente "2026-10-22 16:35:00"
    const char *p = line;
    if (strncasecmp(p, "ora", 3) == 0) p += 3;
    else if (strncasecmp(p, "time", 4) == 0) p += 4;
    else if (!(p[0] >= '0' && p[0] <= '9')) return false;

    int Y, M, D, h, m, s = 0;
    int n = sscanf(p, " %d-%d-%d %d:%d:%d", &Y, &M, &D, &h, &m, &s);
    if (n >= 5 && Y >= 2000 && M >= 1 && M <= 12 && D >= 1 && D <= 31 &&
        h >= 0 && h < 24 && m >= 0 && m < 60 && s >= 0 && s < 60) {
        set(Y, M, D, h, m, s);
    } else {
        App::log(name(), "Formato non valido. Esempio: ora 2026-10-22 15:30:00");
    }
    return true;
}

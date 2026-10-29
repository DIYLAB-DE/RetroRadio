#include "I18n.h"
#include "Strings_retroradio.h"
#include "Strings_sdplayer.h"
#include "Strings_dayview.h"
#include "Strings_calendar.h"
#include "Strings_todo.h"
#include "Strings_contacts.h"
#include "Strings_pomodoro.h"
#include "Strings_breather.h"
#include "Strings_workshop.h"

#include <pgmspace.h>
#include <stdio.h>
#include <string.h>

#include "AppState.h"
#include "BaseModule.h"

// ===================================================================
// Externe Lookup-Funktionen aus den Strings-Modulen
// ===================================================================

extern const __FlashStringHelper *i18n_getCommon(CommonKey key, LangIndex lang);
extern const __FlashStringHelper *i18n_getWeather(WeatherKey key, LangIndex lang);
extern const __FlashStringHelper *i18n_getRetroRadio(RetroRadioKey key, LangIndex lang);
extern const __FlashStringHelper *i18n_getSDPlayer(SDPlayerKey key, LangIndex lang);
extern const __FlashStringHelper *i18n_getDayView(DayViewKey key, LangIndex lang);
extern const __FlashStringHelper *i18n_getCalendar(CalendarKey key, LangIndex lang);
extern const __FlashStringHelper *i18n_getToDo(ToDoKey key, LangIndex lang);
extern const __FlashStringHelper *i18n_getContacts(ContactsKey key, LangIndex lang);
extern const __FlashStringHelper *i18n_getPomodoro(PomodoroKey key, LangIndex lang);
extern const __FlashStringHelper *i18n_getBreather(BreatherKey key, LangIndex lang);
extern const __FlashStringHelper *i18n_getWorkshop(WorkshopKey key, LangIndex lang);
extern const __FlashStringHelper *i18n_getSettings(SettingsKey key, LangIndex lang);
extern const __FlashStringHelper *i18n_getOff(OffKey key, LangIndex lang);

// ===================================================================
// Sprach-Index
// ===================================================================

/**
 * @brief Liefert die aktuelle Sprache als Index
 */
LangIndex I18n::currentLang()
{
    const char *l = APP_STATE.getUiLanguage();
    if (l[0] == 'e' && l[1] == 'n')
        return LangIndex::EN;
    return LangIndex::DE;
}

// ===================================================================
// Direkte Lookups
// ===================================================================

/**
 * @brief Liefert den lokalisierten Text für den angegebenen Schlüssel.
 */
const __FlashStringHelper *I18n::str(CommonKey key)
{
    return i18n_getCommon(key, currentLang());
}

/**
 * @brief Liefert den lokalisierten Text für den angegebenen Schlüssel.
 */
const __FlashStringHelper *I18n::str(WeatherKey key)
{
    return i18n_getWeather(key, currentLang());
}

/**
 * @brief Liefert den lokalisierten Text für den angegebenen Schlüssel.
 */
const __FlashStringHelper *I18n::str(RetroRadioKey key)
{
    return i18n_getRetroRadio(key, currentLang());
}

/**
 * @brief Liefert den lokalisierten Text für den angegebenen Schlüssel.
 */
const __FlashStringHelper *I18n::str(SDPlayerKey key)
{
    return i18n_getSDPlayer(key, currentLang());
}

/**
 * @brief Liefert den lokalisierten Text für den angegebenen Schlüssel.
 */
const __FlashStringHelper *I18n::str(DayViewKey key)
{
    return i18n_getDayView(key, currentLang());
}

/**
 * @brief Liefert den lokalisierten Text für den angegebenen Schlüssel.
 */
const __FlashStringHelper *I18n::str(CalendarKey key)
{
    return i18n_getCalendar(key, currentLang());
}

/**
 * @brief Liefert den lokalisierten Text für den angegebenen Schlüssel.
 */
const __FlashStringHelper *I18n::str(ToDoKey key)
{
    return i18n_getToDo(key, currentLang());
}

/**
 * @brief Liefert den lokalisierten Text für den angegebenen Schlüssel.
 */
const __FlashStringHelper *I18n::str(ContactsKey key)
{
    return i18n_getContacts(key, currentLang());
}

/**
 * @brief Liefert den lokalisierten Text für den angegebenen Schlüssel.
 */
const __FlashStringHelper *I18n::str(PomodoroKey key)
{
    return i18n_getPomodoro(key, currentLang());
}

/**
 * @brief Liefert den lokalisierten Text für den angegebenen Schlüssel.
 */
const __FlashStringHelper *I18n::str(BreatherKey key)
{
    return i18n_getBreather(key, currentLang());
}

/**
 * @brief Liefert den lokalisierten Text für den angegebenen Schlüssel.
 */
const __FlashStringHelper *I18n::str(WorkshopKey key)
{
    return i18n_getWorkshop(key, currentLang());
}

/**
 * @brief Liefert den lokalisierten Text für den angegebenen Schlüssel.
 */
const __FlashStringHelper *I18n::str(SettingsKey key)
{
    return i18n_getSettings(key, currentLang());
}

/**
 * @brief Liefert den lokalisierten Text für den angegebenen Schlüssel.
 */
const __FlashStringHelper *I18n::str(OffKey key)
{
    return i18n_getOff(key, currentLang());
}

// ===================================================================
// Format-Parser
// ===================================================================

namespace
{
    constexpr size_t MAX_PLACEHOLDERS = 9;
    constexpr size_t MAX_FORMAT_SPEC = 16;

    /**
     * @brief Schreibt ein Argument als printf-formatierten String in den Puffer
     * @return Anzahl tatsächlich geschriebener Bytes ohne Nullterminierung
     */
    size_t writeArg(char *dst, size_t cap, const char *spec, va_list *args)
    {
        if (cap == 0)
            return 0;
        const int n = vsnprintf(dst, cap, spec, *args);
        if (n < 0)
            return 0;
        return (static_cast<size_t>(n) < cap) ? static_cast<size_t>(n) : cap - 1;
    }

    /**
     * @brief Liest ein Argument vom passenden Typ aus va_list und expandiert spec
     *
     * Die Argument-Typen werden anhand des Format-Specs erkannt (`%d`, `%s`, `%f`, ...). Wir müssen das Argument korrekt aus va_list pop'pen, sonst gerät der Stack durcheinander.
     */
    size_t expandPlaceholder(char *dst, size_t cap, const char *spec, va_list *args)
    {
        // Letztes alphabetisches Zeichen ist der Konvertierungstyp
        const size_t specLen = strlen(spec);
        if (specLen == 0)
            return 0;
        const char conv = spec[specLen - 1];

        // Lokale va_list-Kopie für den vsnprintf-Aufruf, weil va_arg verschoben hat
        va_list localCopy;
        va_copy(localCopy, *args);

        size_t written = 0;
        switch (conv)
        {
        case 'd':
        case 'i':
        case 'u':
        case 'x':
        case 'X':
        case 'o':
        case 'c':
            va_arg(*args, int);
            written = vsnprintf(dst, cap, spec, localCopy);
            break;
        case 'l':
            // Sollte eigentlich nicht als letztes Zeichen stehen, aber abfangen
            va_arg(*args, long);
            written = vsnprintf(dst, cap, spec, localCopy);
            break;
        case 'f':
        case 'e':
        case 'g':
        case 'F':
        case 'E':
        case 'G':
            va_arg(*args, double);
            written = vsnprintf(dst, cap, spec, localCopy);
            break;
        case 's':
            va_arg(*args, const char *);
            written = vsnprintf(dst, cap, spec, localCopy);
            break;
        default:
            // Unbekannter Conversion - als String behandeln
            va_arg(*args, const char *);
            written = vsnprintf(dst, cap, spec, localCopy);
            break;
        }

        va_end(localCopy);
        if (written == static_cast<size_t>(-1))
            return 0;
        return (written < cap) ? written : cap - 1;
    }

    /**
     * @brief Generischer Format-Helper, der den PROGMEM-Template-String iteriert
     */
    size_t formatImpl(char *dst, size_t dstSize, const __FlashStringHelper *tpl, va_list args)
    {
        if (!dst || dstSize == 0)
            return 0;
        dst[0] = '\0';
        if (!tpl)
            return 0;

        // Argument-Spec pro Index sammeln, damit Argumente in Reihenfolge der Indizes aus va_list konsumiert werden, nicht in Reihenfolge des Auftretens im Template.
        char specs[MAX_PLACEHOLDERS][MAX_FORMAT_SPEC] = {{0}};
        bool seen[MAX_PLACEHOLDERS] = {false};
        uint8_t placeholderOrder[MAX_PLACEHOLDERS]; // Reihenfolge des Auftretens
        size_t placeholderCount = 0;

        PGM_P p = reinterpret_cast<PGM_P>(tpl);

        // Erster Durchgang: Specs einsammeln, Auftretens-Reihenfolge speichern
        size_t i = 0;
        while (true)
        {
            const char c = static_cast<char>(pgm_read_byte(p + i));
            if (c == '\0')
                break;
            if (c == '{')
            {
                const char next = static_cast<char>(pgm_read_byte(p + i + 1));
                if (next == '{')
                {
                    i += 2;
                    continue;
                }
                // Index lesen
                size_t j = i + 1;
                int idx = 0;
                bool gotDigit = false;
                while (true)
                {
                    const char d = static_cast<char>(pgm_read_byte(p + j));
                    if (d >= '0' && d <= '9')
                    {
                        idx = idx * 10 + (d - '0');
                        gotDigit = true;
                        j++;
                    }
                    else
                        break;
                }
                if (!gotDigit || idx >= static_cast<int>(MAX_PLACEHOLDERS))
                {
                    i++;
                    continue;
                }
                char specBuf[MAX_FORMAT_SPEC];
                specBuf[0] = '%';
                size_t specLen = 1;
                const char afterIdx = static_cast<char>(pgm_read_byte(p + j));
                if (afterIdx == ':')
                {
                    // Spec übernehmen
                    j++; // Doppelpunkt überspringen
                    const char pct = static_cast<char>(pgm_read_byte(p + j));
                    if (pct == '%')
                        j++;
                    while (true)
                    {
                        const char d = static_cast<char>(pgm_read_byte(p + j));
                        if (d == '}' || d == '\0')
                            break;
                        if (specLen + 1 < MAX_FORMAT_SPEC)
                            specBuf[specLen++] = d;
                        j++;
                    }
                }
                else
                {
                    // Default: %s
                    specBuf[specLen++] = 's';
                }
                specBuf[specLen] = '\0';

                // Bei wiederholtem Auftreten desselben Index: spec überschreiben darf nicht passieren
                if (!seen[idx])
                {
                    memcpy(specs[idx], specBuf, specLen + 1);
                    seen[idx] = true;
                    if (placeholderCount < MAX_PLACEHOLDERS)
                        placeholderOrder[placeholderCount++] = static_cast<uint8_t>(idx);
                }
                if (static_cast<char>(pgm_read_byte(p + j)) == '}')
                    j++;
                i = j;
                continue;
            }
            i++;
        }

        // Argumente aus va_list konsumieren - in Reihenfolge der Indizes (0, 1, 2, ...) Das ist die Standard-Annahme: Caller übergibt Args in numerischer Reihenfolge.
        //
        // Pro-Argument-Puffer: 256 Bytes. Deckt typische Anwender-Inhalte ab (Schlagzeilen-Buffer ist 256 Bytes, Wochentag-/Monatsnamen passen mit Reserve). Bei längeren Argument-Strings wird hier abgeschnitten; in dem Fall lieber safeCopy() ohne STRF() verwenden oder den Wert vorab kürzen.
        constexpr size_t ARG_BUF_SIZE = 256;
        char argStr[MAX_PLACEHOLDERS][ARG_BUF_SIZE];
        for (size_t idx = 0; idx < MAX_PLACEHOLDERS; ++idx)
            argStr[idx][0] = '\0';

        for (size_t idx = 0; idx < MAX_PLACEHOLDERS; ++idx)
        {
            if (!seen[idx])
                continue;
            expandPlaceholder(argStr[idx], sizeof(argStr[idx]), specs[idx], &args);
        }

        // Zweiter Durchgang: Template in dst auszugeben, Platzhalter durch argStr ersetzen
        size_t written = 0;
        i = 0;
        while (written + 1 < dstSize)
        {
            const char c = static_cast<char>(pgm_read_byte(p + i));
            if (c == '\0')
                break;
            if (c == '{')
            {
                const char next = static_cast<char>(pgm_read_byte(p + i + 1));
                if (next == '{')
                {
                    dst[written++] = '{';
                    i += 2;
                    continue;
                }
                size_t j = i + 1;
                int idx = 0;
                bool gotDigit = false;
                while (true)
                {
                    const char d = static_cast<char>(pgm_read_byte(p + j));
                    if (d >= '0' && d <= '9')
                    {
                        idx = idx * 10 + (d - '0');
                        gotDigit = true;
                        j++;
                    }
                    else
                        break;
                }
                if (!gotDigit || idx >= static_cast<int>(MAX_PLACEHOLDERS))
                {
                    dst[written++] = c;
                    i++;
                    continue;
                }
                // Bis zur schließenden Klammer überspringen
                while (true)
                {
                    const char d = static_cast<char>(pgm_read_byte(p + j));
                    if (d == '\0' || d == '}')
                        break;
                    j++;
                }
                if (static_cast<char>(pgm_read_byte(p + j)) == '}')
                    j++;

                // argStr einfügen
                const char *src = argStr[idx];
                while (*src && written + 1 < dstSize)
                    dst[written++] = *src++;
                i = j;
                continue;
            }
            if (c == '}')
            {
                const char next = static_cast<char>(pgm_read_byte(p + i + 1));
                if (next == '}')
                {
                    dst[written++] = '}';
                    i += 2;
                    continue;
                }
            }
            dst[written++] = c;
            i++;
        }

        dst[written] = '\0';
        return written;
    }
}

/**
 * @brief Formatiert den lokalisierten Text für den angegebenen Schlüssel.
 */
size_t I18n::format(char *dst, size_t dstSize, CommonKey key, ...)
{
    va_list args;
    va_start(args, key);
    const size_t n = formatImpl(dst, dstSize, str(key), args);
    va_end(args);
    return n;
}

/**
 * @brief Formatiert den lokalisierten Text für den angegebenen Schlüssel.
 */
size_t I18n::format(char *dst, size_t dstSize, WeatherKey key, ...)
{
    va_list args;
    va_start(args, key);
    const size_t n = formatImpl(dst, dstSize, str(key), args);
    va_end(args);
    return n;
}

/**
 * @brief Formatiert den lokalisierten Text für den angegebenen Schlüssel.
 */
size_t I18n::format(char *dst, size_t dstSize, RetroRadioKey key, ...)
{
    va_list args;
    va_start(args, key);
    const size_t n = formatImpl(dst, dstSize, str(key), args);
    va_end(args);
    return n;
}

/**
 * @brief Formatiert den lokalisierten Text für den angegebenen Schlüssel.
 */
size_t I18n::format(char *dst, size_t dstSize, SDPlayerKey key, ...)
{
    va_list args;
    va_start(args, key);
    const size_t n = formatImpl(dst, dstSize, str(key), args);
    va_end(args);
    return n;
}

/**
 * @brief Formatiert den lokalisierten Text für den angegebenen Schlüssel.
 */
size_t I18n::format(char *dst, size_t dstSize, DayViewKey key, ...)
{
    va_list args;
    va_start(args, key);
    const size_t n = formatImpl(dst, dstSize, str(key), args);
    va_end(args);
    return n;
}

/**
 * @brief Formatiert den lokalisierten Text für den angegebenen Schlüssel.
 */
size_t I18n::format(char *dst, size_t dstSize, CalendarKey key, ...)
{
    va_list args;
    va_start(args, key);
    const size_t n = formatImpl(dst, dstSize, str(key), args);
    va_end(args);
    return n;
}

/**
 * @brief Formatiert den lokalisierten Text für den angegebenen Schlüssel.
 */
size_t I18n::format(char *dst, size_t dstSize, ToDoKey key, ...)
{
    va_list args;
    va_start(args, key);
    const size_t n = formatImpl(dst, dstSize, str(key), args);
    va_end(args);
    return n;
}

/**
 * @brief Formatiert den lokalisierten Text für den angegebenen Schlüssel.
 */
size_t I18n::format(char *dst, size_t dstSize, ContactsKey key, ...)
{
    va_list args;
    va_start(args, key);
    const size_t n = formatImpl(dst, dstSize, str(key), args);
    va_end(args);
    return n;
}

/**
 * @brief Formatiert den lokalisierten Text für den angegebenen Schlüssel.
 */
size_t I18n::format(char *dst, size_t dstSize, PomodoroKey key, ...)
{
    va_list args;
    va_start(args, key);
    const size_t n = formatImpl(dst, dstSize, str(key), args);
    va_end(args);
    return n;
}

/**
 * @brief Formatiert den lokalisierten Text für den angegebenen Schlüssel.
 */
size_t I18n::format(char *dst, size_t dstSize, BreatherKey key, ...)
{
    va_list args;
    va_start(args, key);
    const size_t n = formatImpl(dst, dstSize, str(key), args);
    va_end(args);
    return n;
}

/**
 * @brief Formatiert den lokalisierten Text für den angegebenen Schlüssel.
 */
size_t I18n::format(char *dst, size_t dstSize, WorkshopKey key, ...)
{
    va_list args;
    va_start(args, key);
    const size_t n = formatImpl(dst, dstSize, str(key), args);
    va_end(args);
    return n;
}

/**
 * @brief Formatiert den lokalisierten Text für den angegebenen Schlüssel.
 */
size_t I18n::format(char *dst, size_t dstSize, SettingsKey key, ...)
{
    va_list args;
    va_start(args, key);
    const size_t n = formatImpl(dst, dstSize, str(key), args);
    va_end(args);
    return n;
}

/**
 * @brief Formatiert den lokalisierten Text für den angegebenen Schlüssel.
 */
size_t I18n::format(char *dst, size_t dstSize, OffKey key, ...)
{
    va_list args;
    va_start(args, key);
    const size_t n = formatImpl(dst, dstSize, str(key), args);
    va_end(args);
    return n;
}

// ===================================================================
// Wochentag-/Monatsnamen aus Laufzeit-Index
// ===================================================================

/**
 * @brief Kopiert den lokalisierten Wochentagsnamen in einen Stack-Puffer
 */
size_t I18n::copyWeekday(char *dst, size_t dstSize, int wdayIndex, bool longForm)
{
    if (!dst || dstSize == 0)
        return 0;
    dst[0] = '\0';
    if (wdayIndex < 0 || wdayIndex > 6)
        return 0;

    const auto baseKey = longForm ? CommonKey::COMMON_WEEKDAY_LONG_SUN
                                  : CommonKey::COMMON_WEEKDAY_SUN;
    const auto key = static_cast<CommonKey>(static_cast<size_t>(baseKey) + wdayIndex);

    strncpy_P(dst, reinterpret_cast<PGM_P>(str(key)), dstSize - 1);
    dst[dstSize - 1] = '\0';
    return strlen(dst);
}

/**
 * @brief Kopiert den lokalisierten Monatsnamen in einen Stack-Puffer
 */
size_t I18n::copyMonth(char *dst, size_t dstSize, int monthIndex, bool longForm)
{
    if (!dst || dstSize == 0)
        return 0;
    dst[0] = '\0';
    if (monthIndex < 0 || monthIndex > 11)
        return 0;

    const auto baseKey = longForm ? CommonKey::COMMON_MONTH_LONG_JAN
                                  : CommonKey::COMMON_MONTH_JAN;
    const auto key = static_cast<CommonKey>(static_cast<size_t>(baseKey) + monthIndex);

    strncpy_P(dst, reinterpret_cast<PGM_P>(str(key)), dstSize - 1);
    dst[dstSize - 1] = '\0';
    return strlen(dst);
}

// ===================================================================
// Sprachwechsel-Mechanik (Dirty-Bitmaske pro AppModule)
// ===================================================================

namespace
{
    /**
     * @brief Ein Bit pro AppModule-Wert
     *
     * Bei `onLanguageChanged()` werden alle Bits gesetzt. Jedes Modul konsumiert sein Bit beim ersten `consumeLanguageChanged()`-Aufruf in seinem `update()`. `AppModule` hat aktuell 13 Werte, also passt das mit Reserve in einen uint32_t.
     */
    volatile uint32_t s_dirtyMask = 0;
}

/**
 * @brief Signalisiert allen Modulen, dass die Sprache gewechselt wurde
 */
void I18n::onLanguageChanged()
{
    s_dirtyMask = 0xFFFFFFFFu;
}

/**
 * @brief Prüft und konsumiert das Dirty-Flag für das aktuell aktive Modul
 */
bool I18n::consumeLanguageChanged()
{
    const auto mod = static_cast<uint8_t>(APP_STATE.getActiveModule());
    if (mod >= 32)
        return false;
    const uint32_t bit = 1u << mod;
    const bool dirty = (s_dirtyMask & bit) != 0;
    s_dirtyMask &= ~bit;
    return dirty;
}

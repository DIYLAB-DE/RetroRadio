#ifndef I18N_H
#define I18N_H

#include <Arduino.h>
#include <stdarg.h>
#include <stddef.h>

#include "Strings_common.h"
#include "Strings_weather.h"

// Forward-Declarations für Modul-Strings, die nicht über I18n.h einbezogen werden
enum class RetroRadioKey : uint16_t;
enum class SDPlayerKey : uint16_t;
enum class DayViewKey : uint16_t;
enum class CalendarKey : uint16_t;
enum class ToDoKey : uint16_t;
enum class ContactsKey : uint16_t;
enum class PomodoroKey : uint16_t;
enum class BreatherKey : uint16_t;
enum class WorkshopKey : uint16_t;
enum class SettingsKey : uint16_t;
enum class OffKey : uint16_t;

/**
 * @brief Sprach-Index für das I18n-System
 *
 * Reihenfolge muss zu den Spalten der Pointer-Tabellen in den Strings-Modulen passen.
 */
enum class LangIndex : uint8_t
{
    DE = 0,
    EN = 1,
    COUNT
};

/**
 * @brief Zentrale Internationalisierungs-API
 *
 * Liefert Texte als PROGMEM-Pointer (`__FlashStringHelper *`), die LovyanGFX direkt an `drawString()` weiterreichen kann. Für Strings mit Platzhaltern gibt es eine `format()`-Variante, die in einen Stack-Puffer schreibt.
 *
 * Die aktuelle Sprache wird ausschließlich über `APP_STATE.getUiLanguage()` bestimmt. `setUiLanguage()` ruft am Ende `I18n::onLanguageChanged()` auf und informiert alle Module über eine Dirty-Bitmaske. Jedes Modul prüft in seinem `update()`-Frame über `consumeLanguageChanged()`, ob es vollständig neu gezeichnet werden muss.
 */
namespace I18n
{
    /**
     * @brief Liefert die aktuelle Sprache als Index
     * @return DE (0) oder EN (1), Fallback DE bei unbekannter Sprache
     *
     * Liest `APP_STATE.getUiLanguage()` und mappt auf den Index.
     */
    LangIndex currentLang();

    /**
     * @brief Liefert den übersetzten Text als PROGMEM-Pointer (Common-Keys)
     * @param key Schlüssel aus Strings_common.h
     * @return Niemals nullptr, im Notfall leerer String
     */
    const __FlashStringHelper *str(CommonKey key);

    /**
     * @brief Liefert den übersetzten Text als PROGMEM-Pointer (Weather-Keys)
     * @param key Schlüssel aus Strings_weather.h
     * @return Niemals nullptr, im Notfall leerer String
     */
    const __FlashStringHelper *str(WeatherKey key);

    /**
     * @brief Liefert den übersetzten Text als PROGMEM-Pointer (RetroRadio-Keys)
     * @param key Schlüssel aus Strings_retroradio.h
     * @return Niemals nullptr, im Notfall leerer String
     *
     * Der Aufrufer muss `Strings_retroradio.h` zusätzlich includieren, da diese Datei nicht von I18n.h mit eingezogen wird.
     */
    const __FlashStringHelper *str(RetroRadioKey key);

    /**
     * @brief Liefert den übersetzten Text als PROGMEM-Pointer (SDPlayer-Keys)
     * @param key Schlüssel aus Strings_sdplayer.h
     * @return Niemals nullptr, im Notfall leerer String
     *
     * Der Aufrufer muss `Strings_sdplayer.h` zusätzlich includieren, da diese Datei nicht von I18n.h mit eingezogen wird.
     */
    const __FlashStringHelper *str(SDPlayerKey key);

    /**
     * @brief Liefert den übersetzten Text als PROGMEM-Pointer (DayView-Keys)
     * @param key Schlüssel aus Strings_dayview.h
     * @return Niemals nullptr, im Notfall leerer String
     *
     * Der Aufrufer muss `Strings_dayview.h` zusätzlich includieren, da diese Datei nicht von I18n.h mit eingezogen wird.
     */
    const __FlashStringHelper *str(DayViewKey key);

    /**
     * @brief Liefert den übersetzten Text als PROGMEM-Pointer (Calendar-Keys)
     * @param key Schlüssel aus Strings_calendar.h
     * @return Niemals nullptr, im Notfall leerer String
     *
     * Der Aufrufer muss `Strings_calendar.h` zusätzlich includieren, da diese Datei nicht von I18n.h mit eingezogen wird.
     */
    const __FlashStringHelper *str(CalendarKey key);

    /**
     * @brief Liefert den übersetzten Text als PROGMEM-Pointer (ToDo-Keys)
     * @param key Schlüssel aus Strings_todo.h
     * @return Niemals nullptr, im Notfall leerer String
     *
     * Der Aufrufer muss `Strings_todo.h` zusätzlich includieren, da diese Datei nicht von I18n.h mit eingezogen wird.
     */
    const __FlashStringHelper *str(ToDoKey key);

    /**
     * @brief Liefert den übersetzten Text als PROGMEM-Pointer (Contacts-Keys)
     * @param key Schlüssel aus Strings_contacts.h
     * @return Niemals nullptr, im Notfall leerer String
     *
     * Der Aufrufer muss `Strings_contacts.h` zusätzlich includieren, da diese Datei nicht von I18n.h mit eingezogen wird.
     */
    const __FlashStringHelper *str(ContactsKey key);

    /**
     * @brief Liefert den übersetzten Text als PROGMEM-Pointer (Pomodoro-Keys)
     * @param key Schlüssel aus Strings_pomodoro.h
     * @return Niemals nullptr, im Notfall leerer String
     *
     * Der Aufrufer muss `Strings_pomodoro.h` zusätzlich includieren, da diese Datei nicht von I18n.h mit eingezogen wird.
     */
    const __FlashStringHelper *str(PomodoroKey key);

    /**
     * @brief Liefert den übersetzten Text als PROGMEM-Pointer (Breather-Keys)
     * @param key Schlüssel aus Strings_breather.h
     * @return Niemals nullptr, im Notfall leerer String
     *
     * Der Aufrufer muss `Strings_breather.h` zusätzlich includieren, da diese Datei nicht von I18n.h mit eingezogen wird.
     */
    const __FlashStringHelper *str(BreatherKey key);

    /**
     * @brief Liefert den übersetzten Text als PROGMEM-Pointer (Workshop-Keys)
     * @param key Schlüssel aus Strings_workshop.h
     * @return Niemals nullptr, im Notfall leerer String
     *
     * Der Aufrufer muss `Strings_workshop.h` zusätzlich includieren, da diese Datei nicht von I18n.h mit eingezogen wird.
     */
    const __FlashStringHelper *str(WorkshopKey key);

    /**
     * @brief Liefert den übersetzten Text als PROGMEM-Pointer (Settings-Keys)
     * @param key Schlüssel aus Strings_settings.h
     * @return Niemals nullptr, im Notfall leerer String
     *
     * Der Aufrufer muss `Strings_settings.h` zusätzlich includieren, da diese Datei nicht von I18n.h mit eingezogen wird.
     */
    const __FlashStringHelper *str(SettingsKey key);

    /**
     * @brief Liefert den übersetzten Text als PROGMEM-Pointer (Off-Keys)
     * @param key Schlüssel aus Strings_off.h
     * @return Niemals nullptr, im Notfall leerer String
     *
     * Der Aufrufer muss `Strings_off.h` zusätzlich includieren, da diese Datei nicht von I18n.h mit eingezogen wird.
     */
    const __FlashStringHelper *str(OffKey key);

    /**
     * @brief Kopiert den übersetzten Text in einen Stack-Puffer und expandiert Platzhalter
     * @param dst Ziel-Puffer
     * @param dstSize Größe des Puffers in Bytes
     * @param key Schlüssel
     * @param ... Variadic-Argumente für {0}, {1}, ...
     * @return Anzahl geschriebener Bytes ohne Null-Terminator
     *
     * Platzhalter-Syntax: `{N}` für `%s`, `{N:%d}` für int, `{N:%.1f}` für float etc. Maximal 9 Platzhalter (`{0}` bis `{8}`). Argument-Reihenfolge wird über den Index N gesteuert, sodass Übersetzungen die Wort-Reihenfolge ändern dürfen.
     *
     * `{{` und `}}` werden als wortwörtliche Klammern ausgegeben.
     */
    size_t format(char *dst, size_t dstSize, CommonKey key, ...);
    size_t format(char *dst, size_t dstSize, WeatherKey key, ...);
    size_t format(char *dst, size_t dstSize, RetroRadioKey key, ...);
    size_t format(char *dst, size_t dstSize, SDPlayerKey key, ...);
    size_t format(char *dst, size_t dstSize, DayViewKey key, ...);
    size_t format(char *dst, size_t dstSize, CalendarKey key, ...);
    size_t format(char *dst, size_t dstSize, ToDoKey key, ...);
    size_t format(char *dst, size_t dstSize, ContactsKey key, ...);
    size_t format(char *dst, size_t dstSize, PomodoroKey key, ...);
    size_t format(char *dst, size_t dstSize, BreatherKey key, ...);
    size_t format(char *dst, size_t dstSize, WorkshopKey key, ...);
    size_t format(char *dst, size_t dstSize, SettingsKey key, ...);
    size_t format(char *dst, size_t dstSize, OffKey key, ...);

    /**
     * @brief Kopiert den lokalisierten Wochentagsnamen in einen Stack-Puffer
     * @param dst Ziel-Puffer
     * @param dstSize Größe des Puffers in Bytes
     * @param wdayIndex Wochentag in tm_wday-Konvention (0=Sonntag, 6=Samstag)
     * @param longForm true für Langform ("Sonntag"/"Sunday"), false für Kurzform ("So"/"Sun")
     * @return Anzahl geschriebener Bytes ohne Null-Terminator
     *
     * Wird typischerweise verwendet, um Wochentagsnamen aus einem Laufzeit-Index (z.B. `tm.tm_wday` aus `localtime_r`) in einen lokalen Puffer zu kopieren, der dann an einen klassischen `%s`-Platzhalter übergeben werden kann. Bei ungültigem Index bleibt der Puffer leer und es wird 0 zurückgegeben.
     */
    size_t copyWeekday(char *dst, size_t dstSize, int wdayIndex, bool longForm);

    /**
     * @brief Kopiert den lokalisierten Monatsnamen in einen Stack-Puffer
     * @param dst Ziel-Puffer
     * @param dstSize Größe des Puffers in Bytes
     * @param monthIndex Monat in tm_mon-Konvention (0=Januar, 11=Dezember)
     * @param longForm true für Langform ("Januar"/"January"), false für Kurzform ("Jan"/"Jan")
     * @return Anzahl geschriebener Bytes ohne Null-Terminator
     *
     * Wird typischerweise verwendet, um Monatsnamen aus einem Laufzeit-Index (z.B. `tm.tm_mon` aus `localtime_r`) in einen lokalen Puffer zu kopieren, der dann an einen klassischen `%s`-Platzhalter übergeben werden kann. Bei ungültigem Index bleibt der Puffer leer und es wird 0 zurückgegeben.
     */
    size_t copyMonth(char *dst, size_t dstSize, int monthIndex, bool longForm);

    /**
     * @brief Signalisiert allen Modulen, dass die Sprache gewechselt wurde
     *
     * Wird von `AppState::setUiLanguage()` aufgerufen. Setzt für alle Modul-Slots in `AppModule` ein Dirty-Bit. Jedes Modul konsumiert sein Bit einmal in `update()` und macht dann einen Vollredraw.
     */
    void onLanguageChanged();

    /**
     * @brief Prüft und konsumiert das Dirty-Flag für das aktuell aktive Modul
     * @return true beim ersten Aufruf nach einem Sprachwechsel, danach false
     *
     * Liest den aktuellen Modul-Slot über `APP_STATE.getActiveModule()`. Jedes Modul bekommt sein Bit unabhängig von der Aufrufreihenfolge genau einmal.
     */
    bool consumeLanguageChanged();
}

// ============================================================================
// KURZFORM-MAKROS
// ============================================================================

/**
 * @brief Kurzform für `I18n::str(KEY)` - Rückgabe ist `const __FlashStringHelper *`
 * @param key Enum-Wert aus einem der Strings_*.h-Header
 *
 * Direkt verwendbar in `m_tft->drawString(STR(KEY), x, y)`.
 */
#define STR(key) I18n::str(key)

/**
 * @brief Kurzform für `I18n::format(buf, sizeof(buf), KEY, ...)`
 * @param buf Stack-Puffer (sizeof wird automatisch ermittelt)
 * @param key Enum-Wert aus einem der Strings_*.h-Header
 * @param ... Variadic-Argumente für die Platzhalter
 *
 * Beispiel: `char buf[32]; STRF(buf, WEATHER_FEELS_LIKE, "21.3");`
 */
#define STRF(buf, key, ...) I18n::format((buf), sizeof(buf), (key), ##__VA_ARGS__)

#endif // I18N_H

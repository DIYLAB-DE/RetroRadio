#ifndef LOGGER_H
#define LOGGER_H

#include <Arduino.h>

#if defined(ARDUINO_ARCH_ESP32)
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#endif

#ifndef LOGGER_ENABLED
#define LOGGER_ENABLED 1
#endif

#if LOGGER_ENABLED
/**
 * @brief Allokationsfreier, farbiger Singleton-Logger für ESP32/Arduino-Projekte.
 *
 * Diese Klasse implementiert das Singleton-Pattern und ermöglicht performantes, farbiges Logging im Terminal von überall im Projekt, ohne dynamische String-Allokationen zur Laufzeit. Die Ausgabe erfolgt über die serielle Schnittstelle mit ANSI-Farbcodes.
 *
 * @note Es werden ANSI-Farbcodes verwendet. Die Darstellung im Terminal hängt vom verwendeten Programm ab.
 *
 * @note Auf dem ESP32 schützt ein FreeRTOS-Mutex die atomaren Log-Ausgaben.
 *
 * @example
 * // Einmalige Initialisierung in main.cpp
 * Logger::init(true, true, 1024);
 *
 * // Verwendung mit getInstance()
 * Logger::getInstance().println(Logger::INFO, "Systemstart erfolgreich!");
 * Logger::getInstance().printf(Logger::SUCCESS, "Wert: %d", 1234);
 *
 * // Oder mit den praktischen Makros für kürzere Syntax
 * LOG_INFO("Systemstart erfolgreich!");
 * LOG_SUCCESS_F("Wert: %d", 1234);
 * LOG_LINE_LABEL("SYSTEM BEREIT");
 * LOG_ERROR("Fehler aufgetreten!");
 */
class Logger
{
public:
    // ===================================================================
    // Öffentliche Typen und Enums
    // ===================================================================

    /**
     * @enum Level
     * @brief Vordefinierte Logging-Level zur strukturierten Log-Ausgabe.
     *
     * Jeder Level hat eine eigene Farbe und ein eigenes Tag-Label.
     */
    enum Level
    {
        INFO,     ///< Allgemeine Information (blau)
        WARNING,  ///< Warnung (gelb)
        ERROR,    ///< Fehler (rot)
        SUCCESS,  ///< Erfolgsmeldung (grün)
        CUSTOM,   ///< Benutzerdefiniertes Tag (cyan)
        BITRATE,  ///< Bitraten-Informationen (magenta)
        TITEL,    ///< Titel-Informationen (cyan)
        SENDER,   ///< Sender-Informationen (hell-magenta)
        AUDIO,    ///< Audio-bezogene Meldungen (hell-blau)
        RADIO,    ///< Radio-bezogene Meldungen (hell-grün)
        CALENDAR, ///< Kalender-bezogene Meldungen (hell-gelb)
        WEATHER,  ///< Wetter-bezogene Meldungen (hell-cyan)
        SETTINGS  ///< Einstellungs-bezogene Meldungen (weiß)
    };

    /**
     * @struct Tag
     * @brief Definition für ein einzelnes Log-Tag mit Name und Farbe.
     *
     * Wird verwendet, um die Zuordnung von Level zu Farbe und Label zu speichern.
     */
    struct Tag
    {
        const char *name;  ///< Anzeigename des Tags (z.B. "info", "error")
        const char *color; ///< ANSI-Farbe als Escape-String
    };

    // ===================================================================
    // ANSI-Farbcodes (16-Farben-Palette)
    // ===================================================================

    // Normale Farben
    static constexpr const char *BLACK = "\033[30m";   ///< Schwarz
    static constexpr const char *RED = "\033[31m";     ///< Rot
    static constexpr const char *GREEN = "\033[32m";   ///< Grün
    static constexpr const char *YELLOW = "\033[33m";  ///< Gelb
    static constexpr const char *BLUE = "\033[34m";    ///< Blau
    static constexpr const char *MAGENTA = "\033[35m"; ///< Magenta
    static constexpr const char *CYAN = "\033[36m";    ///< Cyan
    static constexpr const char *WHITE = "\033[37m";   ///< Weiß

    // Helle Farben
    static constexpr const char *BRIGHT_BLACK = "\033[90m";   ///< Hell-Schwarz / Grau
    static constexpr const char *BRIGHT_RED = "\033[91m";     ///< Hellrot
    static constexpr const char *BRIGHT_GREEN = "\033[92m";   ///< Hellgrün
    static constexpr const char *BRIGHT_YELLOW = "\033[93m";  ///< Hellgelb
    static constexpr const char *BRIGHT_BLUE = "\033[94m";    ///< Hellblau
    static constexpr const char *BRIGHT_MAGENTA = "\033[95m"; ///< Hellmagenta
    static constexpr const char *BRIGHT_CYAN = "\033[96m";    ///< Hellcyan
    static constexpr const char *BRIGHT_WHITE = "\033[97m";   ///< Hellweiß

    static constexpr const char *RESET = "\033[0m"; ///< Setzt die Textfarbe zurück

    /**
     * @brief Vordefinierte Tags für die Logger-Ausgabe.
     *
     * Array mit allen definierten Log-Levels und ihren zugehörigen Namen und Farben. Die Reihenfolge entspricht dem Level-Enum.
     */
    static const Tag TAGS[13];

    // ===================================================================
    // Singleton-Verwaltung
    // ===================================================================

    /**
     * @brief Initialisiert den Singleton-Logger (nur einmal in main.cpp aufrufen).
     * @param enabled      Logging global aktivieren/deaktivieren
     * @param withTime     Zeitangabe bei jeder Log-Ausgabe einblenden
     * @param bufferSize   Größe des internen Puffers für Ausgaben (empfohlen: ≥512)
     * @return Referenz auf die Logger-Instanz
     *
     * @note Diese Funktion sollte nur einmal beim Programmstart aufgerufen werden. Weitere Aufrufe haben keine Wirkung.
     *
     * @warning Ein zu kleiner Puffer kann Ausgaben abschneiden. Die empfohlene Mindestgröße beträgt 512 Byte.
     */
    static Logger &init(bool enabled = true, bool withTime = false, size_t bufferSize = 512);

    /**
     * @brief Gibt die Singleton-Instanz zurück.
     * @return Referenz auf die Logger-Instanz
     *
     * @note Löst eine Assertion aus, wenn init() noch nicht aufgerufen wurde. Im Release-Build kann dies zu undefiniertem Verhalten führen.
     */
    static Logger &getInstance();

    /**
     * @brief Prüft, ob der Logger bereits initialisiert wurde.
     * @return true wenn Logger initialisiert ist, sonst false
     */
    static bool isInitialized();

    // Kopierkonstruktor und Zuweisungsoperator verhindern (Singleton)
    Logger(const Logger &) = delete;
    Logger &operator=(const Logger &) = delete;

    /**
     * @brief Destruktor: Gibt den Buffer und den Mutex wieder frei.
     */
    ~Logger();

    // ===================================================================
    // Konfiguration
    // ===================================================================

    /**
     * @brief Logging ein- oder ausschalten.
     * @param enabled true = Logging aktiv, false = alle Ausgaben unterdrückt
     *
     * Wenn Logging deaktiviert ist, werden alle Log-Aufrufe ignoriert und erzeugen keine Ausgabe auf Serial.
     */
    void setEnabled(bool enabled);

    /**
     * @brief Zeitstempel-Ausgabe aktivieren/deaktivieren.
     * @param enabled true = Zeitangabe anzeigen, false = ohne Zeitangabe
     *
     * Die Zeit wird relativ zum Start bzw. zum letzten resetTime() berechnet und im Format MM:SS.mmm angezeigt.
     */
    void setTimeOutput(bool enabled);

    /**
     * @brief Setzt den Startzeitpunkt zurück (z.B. für neue Messperioden).
     *
     * Nach dem Aufruf beginnt die Zeitmessung von neuem bei 00:00.000.
     */
    void resetTime();

    // ===================================================================
    // Trennlinien
    // ===================================================================

    /**
     * @brief Gibt eine optische Trennlinie im Log aus.
     * @param color ANSI-Farbe der Linie (optional, Standard: grau)
     * @param width Anzahl der Zeichen (optional, Standard: 80, Maximum: 80)
     *
     * Erzeugt eine durchgehende Linie aus Bindestrichen zur visuellen Strukturierung der Log-Ausgabe.
     */
    void line(const char *color = BRIGHT_BLACK, int width = 80);

    /**
     * @brief Gibt eine Trennlinie mit Text-Label im Log aus.
     * @param label  Text, der in der Mitte stehen soll
     * @param color  ANSI-Farbe der Linie (optional, Standard: grau)
     * @param width  Gesamtlänge der Trennlinie (optional, Standard: 80, Maximum: 80)
     *
     * Erzeugt eine Trennlinie mit zentriertem Text, z.B.: "--------- SYSTEMSTART ---------"
     */
    void line(const char *label, const char *color = BRIGHT_BLACK, int width = 80);

    // ===================================================================
    // Standard-Logging mit vordefinierten Tags
    // ===================================================================

    /**
     * @brief Loggt eine Nachricht ohne Zeilenumbruch.
     * @param level Log-Level (INFO, ERROR, SUCCESS, ...)
     * @param msg   Nachricht (const char*)
     *
     * Gibt die Nachricht mit dem entsprechenden Tag aus, ohne automatischen Zeilenumbruch am Ende.
     */
    void print(Level level, const char *msg);

    /**
     * @brief Loggt eine Nachricht mit Zeilenumbruch.
     * @param level Log-Level (INFO, ERROR, SUCCESS, ...)
     * @param msg   Nachricht (const char*)
     *
     * Gibt die Nachricht mit dem entsprechenden Tag aus und fügt automatisch einen Zeilenumbruch am Ende hinzu.
     */
    void println(Level level, const char *msg);

    /**
     * @brief Formatiertes Logging im printf-Stil.
     * @param level  Log-Level
     * @param format Format-String (wie printf)
     * @param ...    Variablen-Argumente
     *
     * Ermöglicht formatierte Ausgaben mit Variablen, z.B.:
     * logger.printf(Logger::INFO, "Wert: %d, Name: %s", 42, "Test");
     *
     * @note Bei einem Pufferüberlauf wird ein Fehler gemeldet und die Ausgabe abgeschnitten.
     */
    void printf(Level level, const char *format, ...);

    // ===================================================================
    // Custom-Tag-Logging
    // ===================================================================

    /**
     * @brief Loggt eine Nachricht mit benutzerdefiniertem Tag (mit Zeilenumbruch).
     * @param tag   Name des Custom-Tags
     * @param msg   Nachricht
     * @param color ANSI-Farbe für das Tag (optional, Standard: Cyan)
     *
     * Ermöglicht die Ausgabe mit beliebigen Tag-Namen, die nicht in den vordefinierten Levels enthalten sind.
     */
    void printlnCustom(const char *tag, const char *msg, const char *color = CYAN);

    /**
     * @brief Loggt eine Nachricht mit benutzerdefiniertem Tag (ohne Zeilenumbruch).
     * @param tag   Name des Custom-Tags
     * @param msg   Nachricht
     * @param color ANSI-Farbe für das Tag (optional, Standard: Cyan)
     *
     * Wie printlnCustom(), jedoch ohne automatischen Zeilenumbruch.
     */
    void printCustom(const char *tag, const char *msg, const char *color = CYAN);

private:
    // ===================================================================
    // Private Konstruktoren und Member
    // ===================================================================

    /**
     * @brief Privater Konstruktor für Singleton-Pattern.
     * @param enabled      Logging global aktivieren/deaktivieren
     * @param withTime     Zeitangabe bei jeder Log-Ausgabe einblenden
     * @param bufferSize   Größe des internen Puffers für Ausgaben
     */
    Logger(bool enabled, bool withTime, size_t bufferSize);

    static Logger *_instance; ///< Singleton-Instanz

    bool _enabled;                    ///< Gibt an, ob Logging aktiv ist
    bool _withTime;                   ///< Gibt an, ob Zeitangaben ausgegeben werden
    uint32_t _start;                  ///< Startzeit für Zeitberechnungen (Millisekunden)
    char *_buffer;                    ///< Interner Buffer für Log-Ausgaben
    size_t _bufferSize;               ///< Größe des Puffers in Bytes
    SemaphoreHandle_t _mtx = nullptr; ///< FreeRTOS-Mutex für atomare Ausgaben

    // ===================================================================
    // Private Hilfsfunktionen
    // ===================================================================

    /**
     * @brief Baut das Prefix ([Tag][Zeit]) für eine Logzeile im Buffer auf.
     * @param level  Log-Level
     * @return Anzahl der belegten Bytes im Buffer nach dem Prefix
     */
    size_t buildPrefix(Level level);

    /**
     * @brief Baut das Prefix für benutzerdefinierte Tags im Buffer auf.
     * @param tag   Name des Tags
     * @param color ANSI-Farbe
     * @return Anzahl der belegten Bytes nach Prefix
     */
    size_t buildCustomPrefix(const char *tag, const char *color);

    /**
     * @brief Hauptfunktion für das Logging ohne String-Allokationen (Standard-Tags).
     * @param level   Log-Level
     * @param msg     Nachricht
     * @param newline true = mit Zeilenumbruch, false = ohne
     */
    void logDirect(Level level, const char *msg, bool newline);

    /**
     * @brief Logging mit benutzerdefinierten Tags (ohne String-Allokationen).
     * @param tag     Name des Custom-Tags
     * @param msg     Nachricht
     * @param color   ANSI-Farbe
     * @param newline true = mit Zeilenumbruch, false = ohne
     */
    void logCustomDirect(const char *tag, const char *msg, const char *color, bool newline);

    /**
     * @brief Formatiert Millisekunden direkt in ein char-Array.
     * @param ms      Millisekunden seit Start
     * @param buf     Zielpuffer für die Zeit
     * @param bufSize Größe des Puffers
     *
     * Format: MM:SS.mmm (z.B. "02:34.567")
     */
    void formatMillisDirect(uint32_t ms, char *buf, size_t bufSize) const;

    /**
     * @brief Sperrt den Mutex für Thread-Safe-Zugriff.
     */
    void lock();

    /**
     * @brief Gibt den Mutex wieder frei.
     */
    void unlock();
};

// ===================================================================
// MAKROS FÜR KÜRZERE SYNTAX
// ===================================================================

/**
 * @brief Makros für vereinfachten Logger-Zugriff ohne getInstance() schreiben zu müssen.
 * @note Diese Makros sind nur verfügbar, wenn der Logger bereits initialisiert wurde.
 */

#define LOG_DIRECT_F(format, ...)                                          \
    do                                                                     \
    {                                                                      \
        Serial.printf("[%saudio%s] ", Logger::BRIGHT_BLUE, Logger::RESET); \
        Serial.printf(format, ##__VA_ARGS__);                              \
        Serial.println();                                                  \
    } while (0)

/// @brief Loggt eine Info-Nachricht mit Zeilenumbruch
#define LOG_INFO(msg) Logger::getInstance().println(Logger::INFO, msg)

/// @brief Loggt eine Warn-Nachricht mit Zeilenumbruch
#define LOG_WARNING(msg) Logger::getInstance().println(Logger::WARNING, msg)

/// @brief Loggt eine Fehler-Nachricht mit Zeilenumbruch
#define LOG_ERROR(msg) Logger::getInstance().println(Logger::ERROR, msg)

/// @brief Loggt eine Erfolgs-Nachricht mit Zeilenumbruch
#define LOG_SUCCESS(msg) Logger::getInstance().println(Logger::SUCCESS, msg)

/// @brief Loggt eine Audio-Nachricht mit Zeilenumbruch
#define LOG_AUDIO(msg) Logger::getInstance().println(Logger::AUDIO, msg)

/// @brief Loggt eine Radio-Nachricht mit Zeilenumbruch
#define LOG_RADIO(msg) Logger::getInstance().println(Logger::RADIO, msg)

/// @brief Loggt eine Kalender-Nachricht mit Zeilenumbruch
#define LOG_CALENDAR(msg) Logger::getInstance().println(Logger::CALENDAR, msg)

/// @brief Loggt eine Wetter-Nachricht mit Zeilenumbruch
#define LOG_WEATHER(msg) Logger::getInstance().println(Logger::WEATHER, msg)

/// @brief Loggt eine Einstellungs-Nachricht mit Zeilenumbruch
#define LOG_SETTINGS(msg) Logger::getInstance().println(Logger::SETTINGS, msg)

/// @brief Loggt eine Info-Nachricht ohne Zeilenumbruch
#define LOG_INFO_PRINT(msg) Logger::getInstance().print(Logger::INFO, msg)

/// @brief Loggt eine Warn-Nachricht ohne Zeilenumbruch
#define LOG_WARNING_PRINT(msg) Logger::getInstance().print(Logger::WARNING, msg)

/// @brief Loggt eine Fehler-Nachricht ohne Zeilenumbruch
#define LOG_ERROR_PRINT(msg) Logger::getInstance().print(Logger::ERROR, msg)

/// @brief Loggt eine Erfolgs-Nachricht ohne Zeilenumbruch
#define LOG_SUCCESS_PRINT(msg) Logger::getInstance().print(Logger::SUCCESS, msg)

/// @brief Printf-Style Logging mit beliebigem Level
#define LOG_PRINTF(level, format, ...) Logger::getInstance().printf(level, format, ##__VA_ARGS__)

/// @brief Printf-Style Info-Logging
#define LOG_INFO_F(format, ...) Logger::getInstance().printf(Logger::INFO, format, ##__VA_ARGS__)

/// @brief Printf-Style Warn-Logging
#define LOG_WARNING_F(format, ...) Logger::getInstance().printf(Logger::WARNING, format, ##__VA_ARGS__)

/// @brief Printf-Style Fehler-Logging
#define LOG_ERROR_F(format, ...) Logger::getInstance().printf(Logger::ERROR, format, ##__VA_ARGS__)

/// @brief Printf-Style Erfolgs-Logging
#define LOG_SUCCESS_F(format, ...) Logger::getInstance().printf(Logger::SUCCESS, format, ##__VA_ARGS__)

/// @brief Printf-Style Audio-Logging
#define LOG_AUDIO_F(format, ...) Logger::getInstance().printf(Logger::AUDIO, format, ##__VA_ARGS__)

/// @brief Printf-Style Radio-Logging
#define LOG_RADIO_F(format, ...) Logger::getInstance().printf(Logger::RADIO, format, ##__VA_ARGS__)

/// @brief Printf-Style Kalender-Logging
#define LOG_CALENDAR_F(format, ...) Logger::getInstance().printf(Logger::CALENDAR, format, ##__VA_ARGS__)

/// @brief Printf-Style Wetter-Logging
#define LOG_WEATHER_F(format, ...) Logger::getInstance().printf(Logger::WEATHER, format, ##__VA_ARGS__)

/// @brief Printf-Style Einstellungs-Logging
#define LOG_SETTINGS_F(format, ...) Logger::getInstance().printf(Logger::SETTINGS, format, ##__VA_ARGS__)

/// @brief Custom-Tag mit Zeilenumbruch (Standardfarbe: Cyan)
#define LOG_CUSTOM(tag, msg) Logger::getInstance().printlnCustom(tag, msg)

/// @brief Custom-Tag mit Zeilenumbruch und eigener Farbe
#define LOG_CUSTOM_COLOR(tag, msg, color) Logger::getInstance().printlnCustom(tag, msg, color)

/// @brief Trennlinie (Standard: grau, 80 Zeichen)
#define LOG_LINE() Logger::getInstance().line()

/// @brief Trennlinie mit eigener Farbe und Breite
#define LOG_LINE_COLOR(color, width) Logger::getInstance().line(color, width)

/// @brief Trennlinie mit Label (zentriert)
#define LOG_LINE_LABEL(label) Logger::getInstance().line(label)

/// @brief Trennlinie mit Label, eigener Farbe und Breite
#define LOG_LINE_LABEL_COLOR(label, color, width) Logger::getInstance().line(label, color, width)

#else

/**
 * @brief Compile-Time-Stub für deaktiviertes Logging.
 *
 * Diese Variante hält die öffentliche API des Loggers verfügbar, erzeugt aber keine Ausgabe und benötigt keine Logger-Implementierung im aktiven Codepfad.
 */
class Logger
{
public:
    /**
     * @enum Level
     * @brief Vordefinierte Logging-Level zur strukturierten Log-Ausgabe.
     */
    enum Level
    {
        INFO,
        WARNING,
        ERROR,
        SUCCESS,
        CUSTOM,
        BITRATE,
        TITEL,
        SENDER,
        AUDIO,
        RADIO,
        CALENDAR,
        WEATHER,
        SETTINGS
    };

    /**
     * @struct Tag
     * @brief Definition für ein einzelnes Log-Tag mit Name und Farbe.
     */
    struct Tag
    {
        const char *name;  ///< Anzeigename des Tags
        const char *color; ///< ANSI-Farbe als Escape-String
    };

    static constexpr const char *BLACK = "";           ///< Schwarz
    static constexpr const char *RED = "";             ///< Rot
    static constexpr const char *GREEN = "";           ///< Grün
    static constexpr const char *YELLOW = "";          ///< Gelb
    static constexpr const char *BLUE = "";            ///< Blau
    static constexpr const char *MAGENTA = "";         ///< Magenta
    static constexpr const char *CYAN = "";            ///< Cyan
    static constexpr const char *WHITE = "";           ///< Weiß
    static constexpr const char *BRIGHT_BLACK = "";    ///< Hell-Schwarz / Grau
    static constexpr const char *BRIGHT_RED = "";      ///< Hellrot
    static constexpr const char *BRIGHT_GREEN = "";    ///< Hellgrün
    static constexpr const char *BRIGHT_YELLOW = "";   ///< Hellgelb
    static constexpr const char *BRIGHT_BLUE = "";     ///< Hellblau
    static constexpr const char *BRIGHT_MAGENTA = "";  ///< Hellmagenta
    static constexpr const char *BRIGHT_CYAN = "";     ///< Hellcyan
    static constexpr const char *BRIGHT_WHITE = "";    ///< Hellweiß
    static constexpr const char *RESET = "";           ///< Setzt die Textfarbe zurück

    static const Tag TAGS[13];

    static Logger &init(bool enabled = true, bool withTime = false, size_t bufferSize = 512)
    {
        (void)enabled;
        (void)withTime;
        (void)bufferSize;
        initializedFlag() = true;
        return instance();
    }

    static Logger &getInstance()
    {
        return instance();
    }

    static bool isInitialized()
    {
        return initializedFlag();
    }

    Logger(const Logger &) = delete;
    Logger &operator=(const Logger &) = delete;

    ~Logger() = default;

    void setEnabled(bool enabled)
    {
        (void)enabled;
    }

    void setTimeOutput(bool enabled)
    {
        (void)enabled;
    }

    void resetTime()
    {
    }

    void line(const char *color = BRIGHT_BLACK, int width = 80)
    {
        (void)color;
        (void)width;
    }

    void line(const char *label, const char *color = BRIGHT_BLACK, int width = 80)
    {
        (void)label;
        (void)color;
        (void)width;
    }

    void print(Level level, const char *msg)
    {
        (void)level;
        (void)msg;
    }

    void println(Level level, const char *msg)
    {
        (void)level;
        (void)msg;
    }

    void printf(Level level, const char *format, ...)
    {
        (void)level;
        (void)format;
    }

    void printlnCustom(const char *tag, const char *msg, const char *color = CYAN)
    {
        (void)tag;
        (void)msg;
        (void)color;
    }

    void printCustom(const char *tag, const char *msg, const char *color = CYAN)
    {
        (void)tag;
        (void)msg;
        (void)color;
    }

private:
    Logger() = default;

    static Logger &instance()
    {
        static Logger logger;
        return logger;
    }

    static bool &initializedFlag()
    {
        static bool initialized = false;
        return initialized;
    }
};

#define LOG_DIRECT_F(...) ((void)0)
#define LOG_INFO(...) ((void)0)
#define LOG_WARNING(...) ((void)0)
#define LOG_ERROR(...) ((void)0)
#define LOG_SUCCESS(...) ((void)0)
#define LOG_AUDIO(...) ((void)0)
#define LOG_RADIO(...) ((void)0)
#define LOG_CALENDAR(...) ((void)0)
#define LOG_WEATHER(...) ((void)0)
#define LOG_SETTINGS(...) ((void)0)
#define LOG_INFO_PRINT(...) ((void)0)
#define LOG_WARNING_PRINT(...) ((void)0)
#define LOG_ERROR_PRINT(...) ((void)0)
#define LOG_SUCCESS_PRINT(...) ((void)0)
#define LOG_PRINTF(...) ((void)0)
#define LOG_INFO_F(...) ((void)0)
#define LOG_WARNING_F(...) ((void)0)
#define LOG_ERROR_F(...) ((void)0)
#define LOG_SUCCESS_F(...) ((void)0)
#define LOG_AUDIO_F(...) ((void)0)
#define LOG_RADIO_F(...) ((void)0)
#define LOG_CALENDAR_F(...) ((void)0)
#define LOG_WEATHER_F(...) ((void)0)
#define LOG_SETTINGS_F(...) ((void)0)
#define LOG_CUSTOM(...) ((void)0)
#define LOG_CUSTOM_COLOR(...) ((void)0)
#define LOG_LINE() ((void)0)
#define LOG_LINE_COLOR(...) ((void)0)
#define LOG_LINE_LABEL(...) ((void)0)
#define LOG_LINE_LABEL_COLOR(...) ((void)0)

#endif

#endif

#include "Logger.h"

#if LOGGER_ENABLED
#include <cassert>
#include <cstdarg>
#include <cstdio>
#include <cstring>

// ===================================================================
// Statische Member-Initialisierung
// ===================================================================

Logger *Logger::_instance = nullptr;

/**
 * @brief Definition der festen Tags für die Logausgabe
 */
const Logger::Tag Logger::TAGS[13] = {
    {"info", Logger::BLUE},
    {"warning", Logger::YELLOW},
    {"error", Logger::RED},
    {"success", Logger::GREEN},
    {"custom", Logger::CYAN},
    {"bitrate", Logger::MAGENTA},
    {"titel", Logger::CYAN},
    {"sender", Logger::BRIGHT_MAGENTA},
    {"audio", Logger::BRIGHT_BLUE},
    {"radio", Logger::BRIGHT_GREEN},
    {"calendar", Logger::BRIGHT_YELLOW},
    {"weather", Logger::BRIGHT_CYAN},
    {"settings", Logger::WHITE}};

// ===================================================================
// Singleton-Verwaltung
// ===================================================================

/**
 * @brief Initialisiert den Singleton-Logger
 */
Logger &Logger::init(bool enabled, bool withTime, size_t bufferSize)
{
    if (_instance == nullptr)
        _instance = new Logger(enabled, withTime, bufferSize);

    return *_instance;
}

/**
 * @brief Gibt die Singleton-Instanz zurück
 */
Logger &Logger::getInstance()
{
    assert(_instance != nullptr && "Logger not initialized! Call Logger::init() first.");

    return *_instance;
}

/**
 * @brief Prüft ob der Logger initialisiert wurde
 */
bool Logger::isInitialized()
{
    return _instance != nullptr;
}

// ===================================================================
// Konstruktor / Destruktor
// ===================================================================

/**
 * @brief Privater Konstruktor
 */
Logger::Logger(bool enabled, bool withTime, size_t bufferSize)
    : _enabled(enabled), _withTime(withTime), _start(millis()), _bufferSize(bufferSize)
{
    _mtx = xSemaphoreCreateMutex();
    _buffer = new char[_bufferSize];
}

/**
 * @brief Destruktor
 */
Logger::~Logger()
{
    if (_mtx)
        vSemaphoreDelete(_mtx);

    delete[] _buffer;
}

// ===================================================================
// Konfiguration
// ===================================================================

/**
 * @brief Aktiviert oder deaktiviert das Logging
 */
void Logger::setEnabled(bool enabled)
{
    _enabled = enabled;
}

/**
 * @brief Aktiviert oder deaktiviert die Zeitausgabe
 */
void Logger::setTimeOutput(bool enabled)
{
    _withTime = enabled;
}

/**
 * @brief Setzt den Startzeitpunkt zurück
 */
void Logger::resetTime()
{
    _start = millis();
}

// ===================================================================
// Standard-Logging-Funktionen
// ===================================================================

/**
 * @brief Loggt eine Nachricht ohne Zeilenumbruch
 */
void Logger::print(Level level, const char *msg)
{
    lock();
    logDirect(level, msg, false);
    unlock();
}

/**
 * @brief Loggt eine Nachricht mit Zeilenumbruch
 */
void Logger::println(Level level, const char *msg)
{
    lock();
    logDirect(level, msg, true);
    unlock();
}

/**
 * @brief Formatiertes Logging im printf-Stil
 */
void Logger::printf(Level level, const char *format, ...)
{
    lock();

    if (!_enabled)
    {
        unlock();
        return;
    }

    // Prefix aufbauen
    size_t pos = buildPrefix(level);

    // Formatierte Nachricht anhängen
    va_list args;
    va_start(args, format);
    int written = vsnprintf(_buffer + pos, _bufferSize - pos, format, args);
    va_end(args);

    // Ausgabe oder Fehler
    if (written > 0 && pos + written < _bufferSize)
        Serial.println(_buffer);
    else
        Serial.println("[LOG ERROR: Buffer overflow]");

    unlock();
}

// ===================================================================
// Custom-Tag-Logging
// ===================================================================

/**
 * @brief Loggt eine Nachricht mit Custom-Tag und Zeilenumbruch
 */
void Logger::printlnCustom(const char *tag, const char *msg, const char *color)
{
    lock();
    logCustomDirect(tag, msg, color, true);
    unlock();
}

/**
 * @brief Loggt eine Nachricht mit Custom-Tag ohne Zeilenumbruch
 */
void Logger::printCustom(const char *tag, const char *msg, const char *color)
{
    lock();
    logCustomDirect(tag, msg, color, false);
    unlock();
}

// ===================================================================
// Trennlinien
// ===================================================================

/**
 * @brief Gibt eine einfache Trennlinie aus
 */
void Logger::line(const char *color, int width)
{
    if (!_enabled)
        return;

    // Buffer für Trennlinie
    char buf[81];
    int count = (width < 80) ? width : 80;

    // Trennzeichen einfügen
    for (int i = 0; i < count; ++i)
        buf[i] = '-';

    buf[count] = '\0';

    // Ausgabe mit Farbe
    Serial.print(color);
    Serial.print(buf);
    Serial.println(RESET);
}

/**
 * @brief Gibt eine Trennlinie mit zentriertem Label aus
 */
void Logger::line(const char *label, const char *color, int width)
{
    if (!_enabled)
        return;

    char buf[81];
    int labelLen = strlen(label);
    int total = (width < 80) ? width : 80;
    int dashes = total - labelLen - 2; // 2 Leerzeichen um Label
    int left = dashes / 2;
    int right = dashes - left;
    int pos = 0;

    // Linke Bindestriche
    for (int i = 0; i < left; ++i)
        buf[pos++] = '-';

    // Label mit Leerzeichen
    buf[pos++] = ' ';
    strncpy(buf + pos, label, sizeof(buf) - pos - 1);
    pos += labelLen;
    buf[pos++] = ' ';

    // Rechte Bindestriche
    for (int i = 0; i < right; ++i)
        buf[pos++] = '-';

    buf[pos] = '\0';

    // Ausgabe mit Farbe
    Serial.print(color);
    Serial.print(buf);
    Serial.println(RESET);
}

// ===================================================================
// Private Hilfsfunktionen
// ===================================================================

/**
 * @brief Baut das Prefix mit Standard-Tag auf
 */
size_t Logger::buildPrefix(Level level)
{
    const Tag &t = TAGS[level];
    size_t pos = 0;

    // Tag mit Farbe
    pos += snprintf(_buffer + pos, _bufferSize - pos, "[%s%s%s] ", t.color, t.name, RESET);

    // Optional: Zeitstempel
    if (_withTime && pos < _bufferSize)
    {
        char timeStr[16];
        formatMillisDirect(millis() - _start, timeStr, sizeof(timeStr));
        pos += snprintf(_buffer + pos, _bufferSize - pos, "%s[%s]%s ", BRIGHT_BLACK, timeStr, RESET);
    }

    return pos;
}

/**
 * @brief Baut das Prefix mit Custom-Tag auf
 */
size_t Logger::buildCustomPrefix(const char *tag, const char *color)
{
    size_t pos = 0;

    // Tag mit Farbe
    pos += snprintf(_buffer + pos, _bufferSize - pos, "[%s%s%s] ", color, tag, RESET);

    // Optional: Zeitstempel
    if (_withTime && pos < _bufferSize)
    {
        char timeStr[16];
        formatMillisDirect(millis() - _start, timeStr, sizeof(timeStr));
        pos += snprintf(_buffer + pos, _bufferSize - pos, "%s[%s]%s ", BRIGHT_BLACK, timeStr, RESET);
    }

    return pos;
}

/**
 * @brief Direktes Logging mit Standard-Tag
 */
void Logger::logDirect(Level level, const char *msg, bool newline)
{
    if (!_enabled)
        return;

    // Prefix aufbauen
    size_t pos = buildPrefix(level);

    // Nachricht anhängen
    snprintf(_buffer + pos, _bufferSize - pos, "%s", msg);

    // Ausgabe mit/ohne Zeilenumbruch
    if (newline)
        Serial.println(_buffer);
    else
        Serial.print(_buffer);
}

/**
 * @brief Direktes Logging mit Custom-Tag
 */
void Logger::logCustomDirect(const char *tag, const char *msg, const char *color, bool newline)
{
    if (!_enabled)
        return;

    // Prefix aufbauen
    size_t pos = buildCustomPrefix(tag, color);

    // Nachricht anhängen
    snprintf(_buffer + pos, _bufferSize - pos, "%s", msg);

    // Ausgabe mit/ohne Zeilenumbruch
    if (newline)
        Serial.println(_buffer);
    else
        Serial.print(_buffer);
}

/**
 * @brief Formatiert Millisekunden in einen String (MM:SS.mmm)
 */
void Logger::formatMillisDirect(uint32_t ms, char *buf, size_t bufSize) const
{
    uint16_t minutes = ms / 60000;
    uint8_t seconds = (ms / 1000) % 60;
    uint16_t millisRest = ms % 1000;
    snprintf(buf, bufSize, "%02u:%02u.%03u", minutes, seconds, millisRest);
}

// ===================================================================
// FreeRTOS Mutex für die Thread-Sicherheit
// ===================================================================

/**
 * @brief Sperrt den Mutex
 */
void Logger::lock()
{
    if (_mtx == nullptr)
        _mtx = xSemaphoreCreateMutex();

    if (_mtx)
        xSemaphoreTake(_mtx, portMAX_DELAY);
}

/**
 * @brief Gibt den Mutex frei
 */
void Logger::unlock()
{
    if (_mtx)
        xSemaphoreGive(_mtx);
}

#else

const Logger::Tag Logger::TAGS[13] = {
    {"", ""},
    {"", ""},
    {"", ""},
    {"", ""},
    {"", ""},
    {"", ""},
    {"", ""},
    {"", ""},
    {"", ""},
    {"", ""},
    {"", ""},
    {"", ""},
    {"", ""}
};

#endif

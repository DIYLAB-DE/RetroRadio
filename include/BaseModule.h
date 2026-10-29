#ifndef BASE_MODULE_H
#define BASE_MODULE_H

#include <stdint.h>

/**
 * @brief Module im Projekt
 */
enum class AppModule
{
    NONE,
    RETRO_RADIO,
    SD_PLAYER,
    WORKSHOP,
    CALENDAR,
    TODO,
    CONTACTS,
    BREATHER,
    WEATHER,
    POMODORO,
    SETTINGS,
    DAYVIEW,
    OFF
};

/**
 * @brief Abstrakte Basisklasse für alle bildschirm-gebundenen Module.
 */
class BaseModule
{
public:
    virtual ~BaseModule() = default;
    virtual void onEnter() {}                                     ///< Aufruf beim Aktivieren
    virtual void onExit() {}                                      ///< Aufruf beim Deaktivieren
    virtual void draw() = 0;                                      ///< Frame zeichnen
    virtual void handleTouch(int16_t, int16_t, bool) {}           ///< Optionaler Touch-Handler
    virtual uint16_t preferredFrameInterval() const { return 0; } ///< 0 = schnellstmöglich
    virtual AppModule type() const = 0;                           ///< Modultyp ohne RTTI
};

#endif // BASE_MODULE_H

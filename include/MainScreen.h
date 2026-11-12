#ifndef MAINSCREEN_H
#define MAINSCREEN_H

#include <stdint.h>

#include <array>
#include <memory>

#include "BaseModule.h"
#include "Button.h"
#include "Gfx.h"
#include "LockScreen.h"

class DS1307_RTC;
class StationManager;
class Audio;

/**
 * @brief MainScreen-Klasse für den Hauptbildschirm und Modul-Management
 *
 * Layout: Linke Seite mit analoger Uhr, Status-Markern und Datum. Rechte Seite mit 4x3 Button-Raster für die Module.
 */
class MainScreen
{
public:
    /**
     * @brief Enum für Zeichnungsstatus
     */
    enum DrawState
    {
        IDLE,         ///< Nichts zu tun
        DRAWING,      ///< Zeichnet gerade
        READY_TO_SHOW ///< Fertig gezeichnet, bereit zum Anzeigen
    };

private:
    /**
     * @brief Zeigerwinkel in Grad im Uhrzeigersinn ab 12 Uhr
     */
    struct ClockHands
    {
        float hour = 0.0f;   ///< Winkel des Stundenzeigers
        float minute = 0.0f; ///< Winkel des Minutenzeigers
        float second = 0.0f; ///< Winkel des Sekundenzeigers einschließlich Sekundenbruchteil
    };

    /**
     * @brief Zu erneuernder Bereich innerhalb des Uhrsprites
     */
    struct ClockRect
    {
        int x; ///< Linke Kante
        int y; ///< Obere Kante
        int w; ///< Breite in Pixeln
        int h; ///< Höhe in Pixeln
    };

    /**
     * @brief Zeichenansicht auf den vorhandenen Uhrpuffer mit Teilpixelkoordinaten
     */
    class ClockSprite : public Gfx_Sprite
    {
    public:
        /**
         * @brief Zeichnet eine geglättete Keillinie ohne Rundung der Endpunkte auf ganze Pixel
         * @param x0 X-Koordinate des Anfangs
         * @param y0 Y-Koordinate des Anfangs
         * @param x1 X-Koordinate des Endes
         * @param y1 Y-Koordinate des Endes
         * @param r0 Halbbreite am Anfang
         * @param r1 Halbbreite am Ende
         * @param color Farbe in RGB565
         */
        void drawHand(float x0, float y0, float x1, float y1, float r0, float r1, uint16_t color)
        {
            draw_wedgeline(x0, y0, x1, y1, r0, r1, lgfx::convert_to_rgb888(color));
        }
    };

    // Display und Hardware
    LGFX *m_tft = nullptr;                      ///< Zeiger auf TFT-Display
    DS1307_RTC *m_rtc = nullptr;                ///< Zeiger auf Echtzeituhr
    StationManager *m_stationManager = nullptr; ///< Zeiger auf Station Manager
    Audio *m_audio = nullptr;                   ///< Zeiger auf Audio-Instanz


    // Sprites
    Gfx_Sprite *m_sprMainScreen = nullptr; ///< Sprite für das Hauptdisplay
    Gfx_Sprite *m_sprClockDial = nullptr;  ///< Zifferblatt samt Datum ohne Zeiger, nullptr aktiviert den Sekundentakt
    ClockSprite m_clockCanvas;            ///< Nutzt den Speicher von m_sprMainScreen ohne eigene Pixelallokation
    ClockHands m_drawnClockHands;          ///< Zuletzt gezeichnete Zeigerstellung
    bool m_clockTimeValid = false;        ///< Zeitgültigkeit der aktuellen Uhrdarstellung
    bool m_clockSweep = true;             ///< Zuletzt übernommene Gangart, true ist schleichend
    bool m_clockBarsDirty = false;        ///< WLAN-Balken im Zifferblatt müssen aktualisiert werden
    uint32_t m_clockIpv4 = 0;             ///< Zuletzt geprüfte IP-Adresse
    char m_clockSsid[33] = {};            ///< Zuletzt geprüfter WLAN-Name
    Gfx_Sprite *m_sprDateCache = nullptr;                ///< Cache-Sprite für Datumszeile (nur bei Tageswechsel neu gerendert)
    Gfx_Sprite *m_sprLabelNtp = nullptr;                 ///< Cache-Sprite für "NTP"-Label (einmalig gerendert, rotiert geblittet)
    Gfx_Sprite *m_sprLabelDst = nullptr;                 ///< Cache-Sprite für "DST"-Label (einmalig gerendert, rotiert geblittet)

    // Double Buffer Management
    uint8_t m_drawStep = 0;           ///< Aktueller Zeichenschritt für inkrementelles Zeichnen
    DrawState m_drawState = IDLE;     ///< Aktueller Zeichnungsstatus
    bool m_needsRedraw = false;       ///< Flag ob Neuzeichnen erforderlich

    // Timing für Non-Blocking
    unsigned long m_lastDrawTime = 0;  ///< Zeitstempel der letzten Zeichenoperation
    unsigned long m_drawInterval = 16; ///< Mindestabstand der Zeichenschritte, schleichende Uhr zusätzlich auf 30 Bilder/s begrenzt
    int m_lastDrawnSecond = -1;        ///< Zuletzt gezeichnete Sekunde zur Optimierung

    // Initialisierungs-Flag
    bool m_initialized = false; ///< Flag ob erfolgreich initialisiert

    // Button-Management (4 Zeilen x 3 Spalten)
    static constexpr int BUTTON_COUNT = 12; ///< Gesamtzahl Buttons
    static constexpr int ICON_COUNT = 12;   ///< Anzahl tatsächlicher Icons

    static constexpr int BUTTON_INDEX_RADIO = 0;        ///< Index des Radio-Buttons
    static constexpr int BUTTON_INDEX_SD_PLAYER = 1;    ///< Index des SD-Player-Buttons
    static constexpr int BUTTON_INDEX_DAYVIEW = 2;      ///< Index des DayView-Buttons
    static constexpr int BUTTON_INDEX_CALENDAR = 3;     ///< Index des Kalender-Buttons
    static constexpr int BUTTON_INDEX_TODO = 4;         ///< Index des ToDo-Buttons
    static constexpr int BUTTON_INDEX_CONTACTS = 5;     ///< Index des Kontakte-Buttons
    static constexpr int BUTTON_INDEX_WEATHER = 6;      ///< Index des Wetter-Buttons
    static constexpr int BUTTON_INDEX_POMODORO = 7;     ///< Index des Pomodoro-Buttons
    static constexpr int BUTTON_INDEX_BREATHER = 8;     ///< Index des Auszeit-Buttons
    static constexpr int BUTTON_INDEX_SETTINGS = 9;     ///< Index des Settings-Buttons
    static constexpr int BUTTON_INDEX_WORKSHOP = 10;    ///< Index des Werkstatt-Buttons
    static constexpr int BUTTON_INDEX_OFF = 11;         ///< Index des Off-Buttons

    std::array<std::unique_ptr<Button>, BUTTON_COUNT> m_buttons; ///< Button-Array
    uint16_t *m_tintedIcons[ICON_COUNT] = {};                    ///< PSRAM-Puffer für umgefärbte Icons (free() im Destruktor)

    int8_t m_lastPressedButton = -1; ///< Index des zuletzt gedrückten Buttons
    bool m_touchProcessed = false;   ///< Wurde Touch bereits verarbeitet

    // Off-Button: langer Druck schaltet sofort aus, kurzer Tipp öffnet den Off-Bildschirm
    static constexpr unsigned long OFF_LONG_PRESS_MS = 3000; ///< Haltedauer bis zum sofortigen Ausschalten
    bool m_offButtonHeld = false;        ///< Off-Button wird aktuell gehalten
    bool m_offLongPressFired = false;    ///< Langer Druck in diesem Halten bereits ausgelöst
    unsigned long m_offPressStartMs = 0; ///< Zeitstempel des Halte-Beginns
    bool m_ignoreWakeTouch = false;      ///< Wartet nach dem Aufwecken ohne PIN auf das Loslassen
    bool m_wakeRequiresRelease = false;  ///< Nach dem Einschlafen mit liegendem Finger erst Loslassen abwarten

    // Zeitfenster-Schlaf
    static constexpr uint8_t TIME_WINDOW_SLOT_INVALID = 0xFF; ///< Ungültiger Zeitfenster-Slot
    uint8_t m_timeWindowLastDay = TIME_WINDOW_SLOT_INVALID;   ///< Zuletzt geprüfter Wochentag
    uint8_t m_timeWindowLastHour = TIME_WINDOW_SLOT_INVALID;  ///< Zuletzt geprüfte Stunde
    uint8_t m_timeWindowWakeDay = TIME_WINDOW_SLOT_INVALID;   ///< Wochentag des manuellen Aufweckens
    uint8_t m_timeWindowWakeHour = TIME_WINDOW_SLOT_INVALID;  ///< Stunde des manuellen Aufweckens
    bool m_timeWindowSleepActive = false;                     ///< Display ist wegen Zeitfenster aus

    // Modul-Management
    std::unique_ptr<BaseModule> m_activeModule;     ///< Aktuell aktives Sub-Modul (automatische Speicherverwaltung)
    AppModule m_activeModuleType = AppModule::NONE; ///< Typ des aktiven Moduls
    AppModule m_requestedModule = AppModule::NONE;  ///< Angefordertes Modul für Wechsel
    bool m_moduleCloseRequested = false;            ///< Flag für Modul-Schließen

    // Statusanzeige - Cache für Change-Detection
    bool m_lastNtpStatus = false;     ///< Letzter NTP-Sync-Status
    bool m_lastDstStatus = false;     ///< Letzter DST-Status
    bool m_lastWifiConnected = false; ///< Letzter WiFi-Verbindungsstatus
    uint8_t m_lastWifiPercent = 255;  ///< Letzte WiFi-Signalstärke in Prozent (255 = ungültig/initial)
    int m_lastUpdatedDay = 0;         ///< Zuletzt aktualisierter Tag für Kalender-Button
    bool m_dateCacheValid = false;    ///< Datum-Cache-Sprite ist gültig
    bool m_labelCachesValid = false;  ///< NTP/DST-Label-Cache-Sprites sind gültig

    // Active Corner (Sperrbildschirm-Geste über die linken Ecken)
    bool m_cornerTouchActive = false;   ///< Geste läuft gerade
    int32_t m_cornerStartX = 0;         ///< Touch-Startposition X
    int32_t m_cornerStartY = 0;         ///< Touch-Startposition Y
    bool m_cornerIsUpperLeft = false;   ///< true = obere linke Ecke, false = untere linke Ecke
    bool m_cornerStartedInCorner = false; ///< Touch begann direkt in der kleinen Trigger-Ecke

    // Sperrbildschirm
    LockScreen m_lockScreen;            ///< Non-blocking PIN-Eingabe
    bool m_lockScreenActive = false;    ///< true während der PIN-Eingabe

    // Touch-Zustand pro Frame (einmalig gelesen, von allen Methoden verwendet)
    bool m_frameTouched = false; ///< Touch aktiv im aktuellen Frame
    int32_t m_frameTouchX = 0;   ///< Touch X-Koordinate im aktuellen Frame
    int32_t m_frameTouchY = 0;   ///< Touch Y-Koordinate im aktuellen Frame

    // Private Hilfsfunktionen
    bool spritesReady() const; ///< Prüft ob Sprites für Zeichenoperationen bereit sind

    // Private Zeichenfunktionen für verschiedene UI-Bereiche
    void drawBackground(Gfx_Sprite *buffer);
    void drawAnalogClock(); ///< Setzt Zifferblatt und Zeiger im Uhrpuffer zusammen
    void drawClockDial(Gfx_Sprite *buffer, int centerX, int centerY, float radius); ///< Zeichnet Zifferblatt, Status und Datum ohne Zeiger
    void drawClockSignalBars(Gfx_Sprite *buffer); ///< Zeichnet nur die WLAN-Balken an ihrer festen Position
    void updateClockSweep(); ///< Aktualisiert geänderte Zeigerbereiche aus dem zwischengespeicherten Zifferblatt
    void drawClockHands(const ClockHands &hands); ///< Zeichnet alle Zeiger und die Achse in die begrenzte Zeichenansicht
    void redrawClockRect(const ClockRect &rect, const ClockHands &hands); ///< Stellt einen Bereich wieder her und überträgt ihn mit den aktuellen Zeigern
    static ClockHands currentClockHands(bool sweep); ///< Liest die Systemzeit, bei sweep mit Sekundenbruchteilen
    static ClockRect clockHandRect(float angle, float back, float length, float width); ///< Ermittelt die Zeigergrenzen einschließlich Kantenglättung
    void renderDateCache();   ///< Rendert das Datum-Cache-Sprite neu (nur bei Tageswechsel)
    void renderLabelCaches(); ///< Rendert NTP/DST-Label-Cache-Sprites (einmalig)
    void drawActiveCornerDebugRegions(); ///< Zeichnet Debug-Rechtecke für die Sliding-Bereiche

    // Buttons
    void initButtons();
    void drawButtons();
    void updateButtonStates(bool redraw = true);
    int8_t getPressedButton();

    // Modul-Management
    void handleMainScreenTouch();
    bool handleOffButton(); ///< Sonderbehandlung des Off-Buttons (langer Druck), true wenn der Button den Frame belegt
    void updateMainScreen();
    bool updateTimeWindowSleep(); ///< Prüft Zeitfenster und schaltet bei Bedarf in den Schlafmodus
    bool getCurrentTimeWindowSlot(uint8_t &day, uint8_t &hour) const;
    bool isManualTimeWindowWake(uint8_t day, uint8_t hour) const;
    void rememberManualTimeWindowWake(uint8_t day, uint8_t hour);
    void clearManualTimeWindowWake();
    void enterTimeWindowSleep();
    void wakeFromTimeWindowSleep();
    void switchToModule(AppModule module);
    void closeActiveModule();

    /**
     * @brief Prüft auf Active-Corner-Geste über die linken Ecken
     * @return true wenn die Geste den Sperrbildschirm ausgelöst hat
     */
    bool checkActiveCorner();

    /**
     * @brief Aktiviert den Sperrbildschirm
     */
    void triggerLockScreen();
    void showUnlockedMainScreen(); ///< Zeichnet das Hauptmenü nach dem Start oder Aufwecken ohne aktive Sperre

    /**
     * @brief Mapping: Button-Index -> AppModule
     * @param buttonIndex Index des gedrückten Buttons (0-11)
     * @return Zugehöriges AppModule oder AppModule::NONE bei ungültigem Index
     */
    AppModule buttonIndexToModule(int8_t buttonIndex) const;

public:
    /**
     * @brief Konstruktor für das MainScreen
     */
    MainScreen();

    MainScreen(const MainScreen &) = delete;
    MainScreen &operator=(const MainScreen &) = delete;
    MainScreen(MainScreen &&) = delete;
    MainScreen &operator=(MainScreen &&) = delete;

    /**
     * @brief Destruktor für das MainScreen
     */
    ~MainScreen();

    /**
     * @brief Initialisiert das MainScreen-Interface
     * @param tft Referenz auf das TFT-Display
     * @param rtc Referenz auf die Echtzeituhr
     * @param stationManager Referenz auf den Station Manager
     * @param audio Referenz auf die Audio-Instanz
     * @return true wenn erfolgreich initialisiert, false bei Fehler
     */
    bool init(LGFX &tft, DS1307_RTC &rtc, StationManager &stationManager, Audio &audio);

    /**
     * @brief Gibt den Sprite-Puffer des Hauptbildschirms zurück
     * @return Zeiger auf den Puffer, oder nullptr wenn er nicht bereitsteht
     */
    Gfx_Sprite *getBuffer();

    /**
     * @brief Zeigt den aktuellen Buffer auf dem Display an
     */
    void display();

    /**
     * @brief Haupt-Update-Funktion mit Modul-Management
     * Sollte regelmäßig aus der Hauptschleife aufgerufen werden
     */
    void update();

    /**
     * @brief Zeichnet den Hauptbildschirm (blocking - nur für Initialisierung)
     */
    void drawImmediate();

    /**
     * @brief Zeichnet den Hauptbildschirm inkrementell (non-blocking)
     * @return true wenn Zeichnen abgeschlossen, false wenn noch am Zeichnen
     */
    bool drawIncremental();

    /**
     * @brief Fordert ein Neuzeichnen an
     * @param immediate Wenn true, wird sofort komplett neu gezeichnet (blocking)
     */
    void requestRedraw(bool immediate = false);

    /**
     * @brief Callback für Module um Schließen anzufordern
     */
    void requestModuleClose()
    {
        m_moduleCloseRequested = true;
    }

    /**
     * @brief Schaltet das Gerät aus
     *
     * Aktiviert den Energiesparmodus und sorgt dafür, dass beim nächsten Aufwachen der Sperrbildschirm erscheint. Wird vom Off-Modul und vom langen Druck auf den Off-Button aufgerufen.
     */
    void requestPowerOff();

    /**
     * @brief Prüft ob MainScreen initialisiert ist
     * @return true wenn initialisiert, sonst false
     */
    bool isInitialized() const
    {
        return m_initialized;
    }

    /**
     * @brief Gibt den aktuellen Zeichnungsstatus zurück
     * @return Aktueller DrawState
     */
    DrawState getDrawState() const
    {
        return m_drawState;
    }

    /**
     * @brief Setzt das Zeichenintervall für Non-Blocking-Updates
     * @param ms Millisekunden zwischen Zeichenschritten
     */
    void setDrawInterval(unsigned long ms)
    {
        m_drawInterval = ms;
    }
};

#endif // MAINSCREEN_H

#ifndef RETRO_RADIO_H
#define RETRO_RADIO_H

// Standard-Bibliotheken
#include <memory>
#include <stdint.h>
#include <vector>

// Basis-Klassen
#include "BaseModule.h"

// Member-Typen
#include "AppState.h"       // Für AppModule enum
#include "AudioManager.h"   // Für AudioManagerEventType
#include "Button.h"         // Button Member Array
#include "DS1307_RTC.h"     // RTC Pointer (Methoden werden aufgerufen!)
#include "Gfx.h"            // Sprite-Pointer
#include "MessageBox.h"     // unique_ptr<MessageBox>
#include "PresetManager.h"  // unique_ptr<PresetManager>
#include "SleepTimer.h"     // unique_ptr<SleepTimer>
#include "StationManager.h" // Pointer (Methoden werden aufgerufen!)
#include "Visualizer.h"     // Visualizer Member

constexpr float PI_F = 3.1415927f; ///< Definition von Pi als float

// ===================================================================
// Konfigurationskonstanten für RetroRadio
// ===================================================================
namespace RetroRadioConfig
{
    /**
     * @brief Display-Dimensionen
     */
    namespace Display
    {
        constexpr int WIDTH = 800;  ///< Breite des Displays
        constexpr int HEIGHT = 480; ///< Höhe des Displays
    }

    /**
     * @brief Farbdefinitionen (RGB565)
     */
    namespace Colors
    {
        constexpr uint16_t BACKGROUND = 0x1082;     ///< Dunkler Hintergrund
        constexpr uint16_t BORDER = 0x39E8;         ///< Rahmen/Linien
        constexpr uint16_t ACCENT = 0x5370;         ///< Akzentfarbe (Marker, etc.)
        constexpr uint16_t DIMMED = 0x6b4d;         ///< Gedimmte Farbe (Mono-Symbol)
        constexpr uint16_t PRESET_MARKER = 0x6c64;  ///< Preset-Sender Markierung
        constexpr uint16_t STATION_MARKER = 0xE300; ///< Normale Sender Markierung
        constexpr uint16_t KNOB_ACTIVE = 0x867d;    ///< Aktiver Bereich Lautstärkeregler
        constexpr uint16_t MAGIC_EYE_RING = 0x1924; ///< Magisches Auge Außenring
    }

    /**
     * @brief Layout-Konstanten für UI-Elemente
     */
    namespace Layout
    {
        /**
         * @brief Skala und Nadel
         */
        namespace Scale
        {
            constexpr int WIDTH = 800;        ///< Breite des Skala-Sprites
            constexpr int HEIGHT = 300;       ///< Höhe des Skala-Sprites
            constexpr int NEEDLE_WIDTH = 6;   ///< Breite des Skalenzeigers
            constexpr int DIVIDER_HEIGHT = 3; ///< Höhe der horizontalen Trennlinie
        }

        /**
         * @brief Footer-Bereich (Metadaten)
         */
        namespace Footer
        {
            constexpr int Y = 316;                  ///< Y-Position des Footer-Bereichs
            constexpr int WIDTH = 800;              ///< Breite des Footer-Sprites
            constexpr int HEIGHT = 32;              ///< Höhe des Footer-Sprites
            constexpr int STEREO_INDICATOR_Y = 319; ///< Y-Position des Stereo-Indikators auf dem Display
            constexpr int STEREO_INDICATOR_W = 41;  ///< Breite des Stereo-Indikator-Sprites
            constexpr int STEREO_INDICATOR_H = 25;  ///< Höhe des Stereo-Indikator-Sprites
        }

        /**
         * @brief Lautstärke-Regler
         */
        namespace Knob
        {
            constexpr int X = 0;             ///< X-Position des Lautstärkereglers
            constexpr int Y = 373;           ///< Y-Position des Lautstärkereglers
            constexpr int SIZE = 107;        ///< Breite und Höhe des Reglers (quadratisch)
            constexpr int LABEL_HEIGHT = 13; ///< Höhe des Beschriftungsbereichs über dem Regler
        }

        /**
         * @brief Equalizer-Regler (Balance, Tiefen, Mitten, Höhen)
         */
        namespace Tracks
        {
            constexpr int X = 130;      ///< X-Position des Equalizer-Bereichs
            constexpr int Y = 360;      ///< Y-Position des Equalizer-Bereichs
            constexpr int WIDTH = 190;  ///< Breite des Equalizer-Sprites
            constexpr int HEIGHT = 120; ///< Höhe des Equalizer-Sprites
        }

        /**
         * @brief Button-Leiste
         */
        namespace Buttons
        {
            constexpr int Y = 435;           ///< Y-Position der Button-Leiste
            constexpr int HEIGHT = 45;       ///< Höhe der Buttons
            constexpr int WIDTH_NORMAL = 45; ///< Breite normaler Buttons
            constexpr int WIDTH_RESET = 67;  ///< Breite des Reset-Buttons
            constexpr int RADIUS = 4;        ///< Eckenradius der Buttons
        }

        /**
         * @brief Uhren-Anzeigen
         */
        namespace Clock
        {
            // Analog (im Gonio-Modus)
            constexpr int ANALOG_X = 721;   ///< X-Position der analogen Uhr
            constexpr int ANALOG_Y = 354;   ///< Y-Position der analogen Uhr
            constexpr int ANALOG_SIZE = 70; ///< Breite und Höhe des analogen Uhr-Sprites

            // Digital (Standard-Modus)
            constexpr int DIGITAL_X = 355;     ///< X-Position der digitalen Uhr
            constexpr int DIGITAL_Y = 435;     ///< Y-Position der digitalen Uhr
            constexpr int DIGITAL_WIDTH = 150; ///< Breite des digitalen Uhr-Sprites
            constexpr int DIGITAL_HEIGHT = 45; ///< Höhe des digitalen Uhr-Sprites
        }

        /**
         * @brief Magisches Auge
         */
        namespace MagicEye
        {
            constexpr int WIDTH = 70;       ///< Breite des Magic-Eye-Sprites
            constexpr int HEIGHT = 70;      ///< Höhe des Magic-Eye-Sprites
            constexpr int EM80_WIDTH = 44;  ///< Breite des Rundbogens der EM80, sie steht kleiner und mittig im Sprite
            constexpr int EM80_HEIGHT = 58; ///< Höhe des Rundbogens der EM80
            constexpr int EM84_WIDTH = 25;  ///< Breite des Leuchtbands der EM84, es steht hochkant und mittig im Sprite
            constexpr int EM84_HEIGHT = 68; ///< Höhe des Leuchtbands der EM84, das Verhältnis folgt der Schattenbild-Tafel
            constexpr int X = 710;          ///< X-Position des Magic-Eye-Sprites auf dem Display
            constexpr int Y = 20;           ///< Y-Position des Magic-Eye-Sprites auf dem Display
        }

        /**
         * @brief Dialog-Dimensionen
         */
        namespace Dialog
        {
            constexpr int MESSAGE_WIDTH = 300;  ///< Breite des Bestätigungsdialogs
            constexpr int MESSAGE_HEIGHT = 200; ///< Höhe des Bestätigungsdialogs
            constexpr int SLEEP_SIZE = 300;     ///< Breite und Höhe des Sleep-Timer-Dialogs
        }
    }

    /**
     * @brief Stations-Konfiguration
     */
    namespace Stations
    {
        constexpr int MAX_COUNT = 70; ///< Maximale Anzahl von Sendern
    }
}

// Namespace-Alias für kürzere Schreibweise
namespace RRConfig = RetroRadioConfig;
namespace RRColors = RetroRadioConfig::Colors;
namespace RRLayout = RetroRadioConfig::Layout;

/**
 * @brief Darstellungsvarianten des magischen Auges
 *
 * Wer hier eine Bauform ergänzt, muss drei weitere Stellen nachziehen, die diese Aufzählung mangels Abhängigkeit nicht sehen können: MAX_MAGIC_EYE_STYLE in src/AppState.cpp begrenzt den gespeicherten Wert, APPEARANCE_EYE_MAX samt Beschriftungstabelle in src/Settings.cpp bedient den Schrittschalter, und in src/Strings_settings.cpp gehört der Name der Röhre in beide Sprachen.
 */
enum class MagicEyeStyle
{
    Em11, ///< Nachbildung der Abstimmröhre EM11 mit vier Leuchtkeilen
    Em34, ///< Nachbildung der Abstimmröhre EM34 mit zwei großen Schattenkeilen
    Em80, ///< Nachbildung der Abstimmröhre EM80, ein hochkant stehender Rundbogen mit drei Leuchtfächern
    Em84, ///< Nachbildung der Abstimmröhre EM84, ein hochkant stehendes Leuchtband mit zwei Balken
    Count ///< Anzahl der Darstellungen, dient dem Weiterschalten und der Begrenzung
};

/**
 * @brief Struktur für Button-Definitionen
 */
struct Buttons
{
    LovyanGFX *gfx;        ///< Zeiger auf das Grafik-Objekt
    int16_t x;             ///< X-Koordinate des Buttons
    int16_t y;             ///< Y-Koordinate des Buttons
    uint16_t w;            ///< Breite des Buttons
    uint16_t h;            ///< Höhe des Buttons
    uint16_t r;            ///< Radius für abgerundete Ecken
    uint16_t outlinecolor; ///< Farbe der Button-Umrandung
    uint16_t fillcolor;    ///< Füllfarbe des Buttons
    uint16_t textcolor;    ///< Textfarbe des Buttons
    const char *label;     ///< Text-Label des Buttons
    const uint16_t *icon;  ///< Icon-Daten für den Button
};

/**
 * @brief Callback-Struktur für alle Radio-Events
 */
struct RetroRadioCallbacks
{
    void (*onButtonPressed)(uint16_t btnIdx, bool buttonState) = nullptr; ///< Callback für Button-Druck
    void (*onStationPressed)(uint16_t staIdx) = nullptr;                  ///< Callback für Sender-Auswahl
    void (*onVolumeChanged)(uint8_t volume) = nullptr;                    ///< Callback für Lautstärke-Änderung
    void (*onVolumeBeforeMute)(uint8_t volume) = nullptr;                 ///< Callback für Lautstärke vor Stummschaltung
    void (*onMuteChanged)(bool mute) = nullptr;                           ///< Callback für Stummschaltung
    void (*onBalanceChanged)(int8_t balance) = nullptr;                   ///< Callback für Balance-Änderung
    void (*onGainLowPassChanged)(int8_t gain) = nullptr;                  ///< Callback für Tiefpass-Verstärkung
    void (*onGainBandPassChanged)(int8_t gain) = nullptr;                 ///< Callback für Bandpass-Verstärkung
    void (*onGainHighPassChanged)(int8_t gain) = nullptr;                 ///< Callback für Hochpass-Verstärkung
    void (*onVUModeChanged)(VisMode mode) = nullptr;                      ///< Callback für VU-Modus-Änderung
    void (*onExitRequested)() = nullptr;                                  ///< Callback für RetroRadio-Modul beendet
    void (*requestPlayStation)(uint8_t id) = nullptr;                     ///< Station starten, wenn Nadel am Ziel
    void (*requestStopPlayback)() = nullptr;                              ///< Stopp beim Loslaufen der Nadel
};

/**
 * @brief Hauptklasse für das RetroRadio-Interface
 *
 * Diese Klasse stellt ein Retro-Radio-Interface mit Touch-Bedienung bereit. Sie verwaltet die Benutzeroberfläche, Button-Interaktionen, Sender-Auswahl und delegiert die Visualisierungen an die Visualizer-Klasse.
 */
class RetroRadio : public BaseModule
{
public:
    /**
     * @brief Gibt den Modul-Typ zurück
     * @return AppModule::RETRO_RADIO
     */
    AppModule type() const override;

    // ===================================================================
    // Konstruktor, Destruktor, Init, Draw
    // ===================================================================

    /**
     * @brief Konstruktor
     */
    RetroRadio();

    /**
     * @brief Destruktor
     *
     * Räumt alle allokierten Ressourcen auf.
     */
    ~RetroRadio();

    /**
     * @brief Initialisiert das RetroRadio-Interface
     * @param tft Referenz auf das TFT-Display
     * @param rtc Referenz auf die Echtzeituhr
     * @param stationManager Referenz auf den Sender-Manager
     * @param callbacks Struktur mit Callback-Funktionen
     * @return true wenn erfolgreich initialisiert, false bei Fehler
     *
     * Diese Methode muss vor der ersten Verwendung des RetroRadio-Interfaces aufgerufen werden. Sie initialisiert alle UI-Komponenten und lädt die Sender-Liste.
     */
    bool init(LGFX &tft, DS1307_RTC &rtc, StationManager &stationManager, const RetroRadioCallbacks &callbacks);

    /**
     * @brief Zeichnet die komplette Benutzeroberfläche
     *
     * Diese Methode wird regelmäßig aufgerufen und aktualisiert alle UI-Elemente inklusive Visualisierungen, Buttons und Statusanzeigen.
     */
    void draw() override;

    // ===================================================================
    // Skala
    // ===================================================================

    /**
     * @brief Setzt den aktiven Radiosender
     * @param id ID des zu aktivierenden Senders
     * @return true wenn erfolgreich, false bei Fehler
     *
     * Bewegt die Skalennadel zur Position des Senders und triggert den Playback-Start über Callbacks, wenn die Zielposition erreicht ist.
     */
    bool setStation(uint8_t id);

    /**
     * @brief Schaltet die Stummschaltung ein/aus
     * @param mute true für Stummschaltung, false für normal
     */
    void setMute(bool mute);

    /**
     * @brief Prüft ob das Radio initialisiert ist
     * @return true wenn initialisiert, false wenn nicht
     */
    bool isInitialized() const { return m_initialized; }

private:
    /**
     * @brief Touch-Status für den Lautstärke-Regler
     */
    enum class KnobTouchState
    {
        IDLE,    ///< Kein Touch aktiv
        MUTE,    ///< Touch im Mute-Bereich (Mitte)
        ROTATING ///< Touch im Rotations-Bereich (außen)
    };

    // Initialisierungsstatus
    bool m_initialized = false; ///< Flag für Initialisierungsstatus

    // Touch-Koordinaten
    int m_touchX = 0;            ///< X-Koordinate der Touch-Position
    int m_touchY = 0;            ///< Y-Koordinate der Touch-Position
    bool m_touched = false;      ///< Flag für Touch-Status
    bool m_wasTouching = false;  ///< Touch-Zustand des vorherigen Frames

    // Draw-Status
    bool m_resetRequested = false;     ///< Fordere ein Reset des Anzeigemodus und der Styles an
    char m_finalStation[64];           ///< Finaler Sendername für Anzeige
    char m_finalBitrate[16];           ///< Finale Bitrate für Anzeige
    char m_showstation[64] = {0};      ///< Angezeigter Sendername
    char m_showstreamtitle[256] = {0}; ///< Angezeigter Stream-Titel

    // Nadel-Animation
    uint16_t m_col0[300], m_col5[300]; ///< Gesicherte Skalen-Pixelspalten für die Spalten 0 und 5 des Zeiger-Sprites
    int16_t m_handCounter = -1;        ///< Zähler für Zeiger-Animation
    unsigned long m_lastMoveTime = 0;  ///< Zeitstempel der letzten Zeiger-Bewegung

    // Footer
    char m_truncatedTitle[256]; ///< Gekürzter Titel für Footer-Anzeige

    // Lautstärke-Regler
    KnobTouchState m_knobTouchState = KnobTouchState::IDLE; ///< Touch-Status des Lautstärke-Reglers
    bool m_muteTriggered = false;                           ///< Flag: Mute wurde bereits getriggert (verhindert Mehrfach-Trigger)
    float m_totalDegrees = 0.0f;                            ///< Gesamtdrehung des Reglers
    float m_lastAngle = 0.0f;                               ///< Letzter Winkel des Reglers
    uint8_t m_prevVol = 0;                                  ///< Vorherige Lautstärke

    // Equalizer-Regler
    int8_t m_balance_prev = -128;      ///< Vorherige Balance-Einstellung
    int8_t m_gainLowPass_prev = -128;  ///< Vorherige Tiefpass-Verstärkung
    int8_t m_gainBandPass_prev = -128; ///< Vorherige Bandpass-Verstärkung
    int8_t m_gainHighPass_prev = -128; ///< Vorherige Hochpass-Verstärkung

    // Uhr
    int m_lastDrawnSecond = -1; ///< Zuletzt gezeichnete Sekunde

    // Button-Verwaltung
    bool m_wasPressed[5] = {false};     ///< Array für Button-Druckzustände
    bool m_longPressed[5] = {false};    ///< Array für lange Button-Drücke
    unsigned long m_pressTime[5] = {0}; ///< Array für Button-Druckzeiten

    // Sender-Auswahl
    bool m_isTouched = false; ///< Flag für Sender-Berührung

    // Display und Hardware
    LGFX *m_tft = nullptr;                      ///< Zeiger auf TFT-Display
    DS1307_RTC *m_rtc = nullptr;                ///< Zeiger auf Echtzeituhr
    StationManager *m_stationManager = nullptr; ///< Zeiger auf Sender-Manager
    RetroRadioCallbacks m_callbacks;            ///< Callback-Funktionen
    bool m_animating = false;                   ///< true: Nadel bewegt sich
    bool m_stopSent = false;                    ///< Stop nur 1x zu Beginn der Fahrt
    uint8_t m_lastStartedStation = 0xFF;        ///< Doppelstarts vermeiden
    uint8_t m_lastProcessedStation = 0xFF;      ///< Zuletzt in der Event-Verarbeitung gesehener Sender, erkennt den Senderwechsel

    // Dialoge
    std::unique_ptr<SleepTimer> m_sleepDlg;     ///< Sleep-Timer-Dialog
    std::unique_ptr<MessageBox> m_msg;          ///< Nachrichten-Box
    std::unique_ptr<PresetManager> m_presetDlg; ///< Preset-Manager-Dialog

    // Senderliste
    Station *m_stationList = nullptr; ///< Zeiger auf Senderliste
    uint16_t m_stationCounter = 0;    ///< Anzahl der verfügbaren Sender

    // Buttons
    Button m_btn[5]; ///< Array der Bedienelemente (Favoriten, Zurück, Vor, Reset, Sleep Timer)

    // Sprites für die Anzeige
    Gfx_Sprite *m_sprRetroDiv = nullptr;          ///< Sprite für Retro-Teilung
    Gfx_Sprite *m_sprRetroScale = nullptr;        ///< Sprite für Retro-Skala
    Gfx_Sprite *m_sprRetroNeedle = nullptr;       ///< Sprite für Retro-Zeiger
    Gfx_Sprite *m_sprRetroStereo = nullptr;       ///< Sprite für Stereo-Anzeige (persistent)
    Gfx_Sprite *m_sprRetroFooter = nullptr;       ///< Sprite für Footer-Bereich
    Gfx_Sprite *m_sprRetroKnob = nullptr;         ///< Sprite für Lautstärke-Regler
    Gfx_Sprite *m_sprRetroTracks = nullptr;       ///< Sprite für Equalizer-Regler
    Gfx_Sprite *m_sprMagicEye = nullptr;          ///< Sprite für das magische Auge
    Gfx_Sprite *m_sprRetroAnalogClock = nullptr;  ///< Sprite für analoge Uhr mit Timer
    Gfx_Sprite *m_sprRetroDigitalClock = nullptr; ///< Sprite für digitale Uhr mit Timer

    // Magisches Auge
    int32_t m_lastMagicEyeValue = -1;        ///< Zuletzt gezeichneter Augenwert, nur eine Änderung löst einen Aufbau aus
    int32_t m_lastGoodPosition = -1;         ///< Letzte erfolgreiche Zeigerposition (für graduelles Schließen)
    unsigned long m_magicEyeLastDraw = 0;    ///< Zeitpunkt des letzten Aufbaus, begrenzt die Bildrate der EM34-Darstellung
    int16_t m_lastMagicEyeStyle = -1;        ///< Zuletzt gezeichnete Darstellung, -1 erzwingt den ersten Aufbau
    unsigned long m_magicEyeTouchStart = 0;  ///< Zeitpunkt, an dem die Berührung des Auges begann, 0 bedeutet keine Berührung
    bool m_magicEyeLongPressDone = false;    ///< true, solange die aktuelle Berührung bereits umgeschaltet hat

    // Visualisierung
    Visualizer m_visualizer; ///< Visualisierungs-Engine

    // Event-Queue-Verarbeitung
    char m_eventText[AudioManager::MAX_EVENT_STRING_LENGTH] = {0}; ///< Wiederverwendbarer Buffer

    // Statische Definitionen
    static const Buttons m_buttonDefs[5]; ///< Statische Button-Definitionen (5 Buttons)
    static const char *m_sleepLabels[];   ///< Labels für Sleep-Timer
    static int m_sleepValues[];           ///< Werte für Sleep-Timer

    // ===================================================================
    // Buttons & Dialoge
    // ===================================================================

    /**
     * @brief Behandelt Berührungen der Buttons
     *
     * Erkennt Touch-Ereignisse auf den Buttons und ruft entsprechende Callbacks auf.
     */
    void getPressedRetroButton();

    /**
     * @brief Callback für MessageBox-Ergebnis
     * @param confirmed true wenn bestätigt, false wenn abgebrochen
     */
    void onMessageBoxResult(bool confirmed);

    /**
     * @brief Stoppt das Radio sofort
     *
     * Wird verwendet um das Radio bei Bestätigung des Exit-Dialogs oder beim Sleep-Timer zu stoppen.
     */
    void stopRadioImmediately();

    /**
     * @brief Zeichnet die komplette UI inklusive Visualizer neu
     *
     * Wird bei einem Sprachwechsel aufgerufen, damit alle lokalisierten Labels (Knob, Equalizer, Visualizer-Info, Footer-Datum) neu gerendert werden. Setzt internen State nicht zurück und legt vorhandene Sprites weiter, sondern zeichnet nur deren Inhalt erneut.
     */
    void redrawAll();

    // ===================================================================
    // Skala
    // ===================================================================

    /**
     * @brief Zeichnet die Sender-Skala
     *
     * Rendert die Frequenz-Skala mit allen verfügbaren Sendern.
     */
    void drawScale();

    /**
     * @brief Lädt die Senderliste aus dem StationManager neu und zeichnet die Skala neu
     *
     * Wird aufgerufen wenn die Senderliste per WebUI geändert wurde. Aktualisiert m_stationList, m_stationCounter und zeichnet die Skala neu.
     */
    void reloadStations();

    /**
     * @brief Behandelt Berührungen der Sender-Auswahl auf der Skala
     *
     * Erkennt Touch-Ereignisse auf der Skala und wählt den entsprechenden Sender aus.
     */
    void getPressedStation();

    /**
     * @brief Aktualisiert das Stereo/Mono-Symbol basierend auf dem Audiosignal
     * @param isMono true für Mono, false für Stereo
     *
     * Zeichnet das Stereo/Mono-Symbol in der Metadaten-Zeile rechts. Die Symbolfarbe wechselt automatisch basierend auf dem aktuellen Audiosignal.
     */
    void updateStereoIndicator(bool isMono);

    // ===================================================================
    // Footer
    // ===================================================================

    /**
     * @brief Zeichnet den Footer-Text mit Sender-Informationen
     * @param init true für Initialisierung, false für Update
     * @param sender Name des Senders
     * @param title Titel des aktuellen Titels
     * @param bitrate Bitrate des Streams
     * @param senderColor Farbe für Sender-Text
     * @param titleColor Farbe für Titel-Text
     * @param bitrateColor Farbe für Bitrate-Text
     * @param separatorColor Farbe für Trennzeichen
     * @param separator Trennzeichen zwischen Informationen
     * @param reservedPixels Reservierte Pixel für andere Elemente
     * @param backgroundColor Hintergrundfarbe
     *
     * Der Footer zeigt Sender-Name, aktuellen Titel und Bitrate an. Bei zu langem Text wird automatisch gekürzt.
     */
    void drawFooterText(
        bool init,
        const char *sender = "",
        const char *title = "",
        const char *bitrate = "",
        uint16_t senderColor = RRColors::ACCENT,
        uint16_t titleColor = TFT_SILVER,
        uint16_t bitrateColor = RRColors::PRESET_MARKER,
        uint16_t separatorColor = TFT_DARKGRAY,
        const char *separator = " | ",
        int reservedPixels = 90,
        uint16_t backgroundColor = RRColors::BACKGROUND);

    // ===================================================================
    // Controls
    // ===================================================================

    /**
     * @brief Hilfsfunktion für den Lautstärkeregler
     * @param angleDeg Winkel in Grad
     * @param center Mittelpunkt
     * @param radiusOffset Radius-Versatz
     * @param outX Ausgabe X-Koordinate
     * @param outY Ausgabe Y-Koordinate
     *
     * Berechnet die Position eines Markers auf einem Kreisbogen.
     */
    void calcMarkerPos(float angleDeg, int center, int radiusOffset, int &outX, int &outY);

    /**
     * @brief Zeichnet den Lautstärke-Regler
     * @param init true für Initialisierung, false für Update
     * @param vol Lautstärke-Wert (0-100)
     * @param skipCallbacks true unterdrückt den onVolumeChanged-Callback (Neuzeichnen nach externer Änderung)
     *
     * Zeigt einen kreisförmigen Lautstärke-Regler mit Touch-Steuerung. Unterstützt auch Mute-Funktion bei Touch in der Mitte.
     */
    void drawKnob(bool init, uint8_t vol = 0, bool skipCallbacks = false);

    /**
     * @brief Zeichnet die Equalizer-Regler
     * @param init true für Initialisierung, false für Update
     * @param balance Balance-Wert (-16 bis +16)
     * @param gainLowPass Tiefpass-Verstärkung (-12 bis +12 dB)
     * @param gainBandPass Bandpass-Verstärkung (-12 bis +12 dB)
     * @param gainHighPass Hochpass-Verstärkung (-12 bis +12 dB)
     * @param skipCallbacks true unterdrückt die Änderungs-Callbacks (Neuzeichnen nach externer Änderung)
     *
     * Zeigt vier Schieberegler für Balance und 3-Band-Equalizer.
     */
    void drawTracks(bool init, int8_t balance = 0, int8_t gainLowPass = 0, int8_t gainBandPass = 0, int8_t gainHighPass = 0, bool skipCallbacks = false);

    // ===================================================================
    // Magisches Auge
    // ===================================================================

    /**
     * @brief Simulation eines magischen Auges
     * @param init true legt das Sprite an und zeichnet noch nichts, false baut die Anzeige auf
     * @param x X-Position des Sprites auf dem Display
     * @param y Y-Position des Sprites auf dem Display
     * @param value Abstimmwert aus calculateMagicEyeValue im Bereich 35 bis 100
     *
     * Zeichnet die in AppState hinterlegte Bauform: EM11, EM34, EM80 oder EM84. Die beiden runden füllen das quadratische Sprite aus, der Rundbogen der EM80 und das Leuchtband der EM84 stehen kleiner und mittig darin.
     */
    void drawMagicEye(bool init, int32_t x, int32_t y, int32_t value = 0);

    /**
     * @brief Schaltet die Darstellung des magischen Auges bei langem Druck um
     *
     * Wird je Bild einmal mit dem aktuellen Berührungszustand gefüttert. Der Wechsel löst genau einmal je Berührung aus und wird sofort dauerhaft gespeichert.
     */
    void handleMagicEyeTouch();

    /**
     * @brief Berechnet den Wert für das magische Auge basierend auf Nadelposition
     * @return Wert zwischen 35 (kein Signal) und 100 (perfekte Abstimmung)
     *
     * Die Berechnung berücksichtigt:
     * - Keine gültige Nadel- oder Zielposition -> 35
     * - Annäherung an das Ziel -> linear interpoliert über 30 Bildpunkte
     * - Wegfahren von der letzten guten Position -> schneller Abfall über 6 Bildpunkte
     * - Nadel am Ziel -> 100
     */
    int32_t calculateMagicEyeValue();

    // ===================================================================
    // Uhren
    // ===================================================================

    /**
     * @brief Zeichnet die analoge Uhr
     * @param init true für Initialisierung, false für Update
     * @param x X-Position der Uhr
     * @param y Y-Position der Uhr
     * @param remainingMinutes Verbleibende Minuten (für Sleep-Timer)
     * @param remainingSeconds Verbleibende Sekunden (für Sleep-Timer)
     */
    void drawAnalogClock(bool init, int32_t x = 0, int32_t y = 0, int remainingMinutes = 0, int remainingSeconds = 0);

    /**
     * @brief Zeichnet die digitale Uhr
     * @param init true für Initialisierung, false für Update
     * @param x X-Position der Uhr
     * @param y Y-Position der Uhr
     * @param remainingMinutes Verbleibende Minuten (für Sleep-Timer)
     * @param remainingSeconds Verbleibende Sekunden (für Sleep-Timer)
     */
    void drawDigitalClock(bool init, int32_t x = 0, int32_t y = 0, int remainingMinutes = 0, int remainingSeconds = 0);
};

#endif // RETRO_RADIO_H

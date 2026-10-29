#ifndef SD_PLAYER_H
#define SD_PLAYER_H

#include <JPEGDEC.h>

#include <memory>
#include <vector>

#include "AppState.h"
#include "AudioManager.h"
#include "BaseModule.h"
#include "Button.h"
#include "FileExplorer.h"
#include "Gfx.h"
#include "Logger.h"
#include "MessageBox.h"
#include "SleepTimer.h"
#include "Visualizer.h"

class DS1307_RTC;

/**
 * @brief Audio-Status Struktur für Playback-Information
 *
 * Enthält alle relevanten Informationen über den aktuellen Wiedergabestatus eines Audio-Tracks.
 */
struct AudioStatus
{
    uint32_t fileSize = 0;    ///< Gesamtgröße der Audiodatei in Bytes
    uint32_t filePos = 0;     ///< Aktuelle Position in der Datei in Bytes
    uint32_t duration = 0;    ///< Gesamtdauer des Tracks in Sekunden
    uint32_t currentTime = 0; ///< Aktuelle Wiedergabezeit in Sekunden
    uint8_t progress = 0;     ///< Fortschritt in Prozent (0-100%)
    bool isPlaying = false;   ///< Wiedergabe aktiv
};

/**
 * @brief Callback-Struktur für das SD-Player Modul
 *
 * Enthält alle Callback-Funktionen, die vom SD-Player aufgerufen werden, um mit dem Hauptprogramm zu kommunizieren.
 */
struct SdPlayerCallbacks
{
    void (*onExitRequested)() = nullptr;                     ///< Wird aufgerufen, wenn das Modul verlassen werden soll
    void (*onVolumeChanged)(uint8_t volume) = nullptr;       ///< Wird aufgerufen, wenn die Lautstärke geändert wurde
    void (*onBalanceChanged)(int8_t balance) = nullptr;      ///< Wird aufgerufen, wenn die Balance geändert wurde
    void (*onGainLowPassChanged)(int8_t gain) = nullptr;     ///< Wird aufgerufen, wenn die Tiefen geändert wurden
    void (*onGainBandPassChanged)(int8_t gain) = nullptr;    ///< Wird aufgerufen, wenn die Mitten geändert wurden
    void (*onGainHighPassChanged)(int8_t gain) = nullptr;    ///< Wird aufgerufen, wenn die Höhen geändert wurden
    void (*onVUModeChanged)(VisMode mode) = nullptr;         ///< Wird aufgerufen, wenn der VU-Mode geändert wurde
    void (*onPlayFileRequested)(const char *path) = nullptr; ///< Wird aufgerufen, wenn eine Datei abgespielt werden soll
    void (*onStopRequested)() = nullptr;                     ///< Wird aufgerufen, wenn die Wiedergabe gestoppt werden soll
    void (*onPauseResumeRequested)() = nullptr;              ///< Wird aufgerufen, wenn Pause/Resume getriggert wurde
    void (*onSeekOffsetRequested)(int sec) = nullptr;        ///< Wird aufgerufen, wenn im Titel gespult werden soll, negativ spult zurück
};

/**
 * @brief Struktur für Button-Definitionen
 *
 * Definiert Position, Größe, Farben und Label eines Buttons.
 */
struct SdPlayerButton
{
    int16_t x;             ///< X-Koordinate der linken oberen Ecke
    int16_t y;             ///< Y-Koordinate der linken oberen Ecke
    uint16_t w;            ///< Breite des Buttons in Pixeln
    uint16_t h;            ///< Höhe des Buttons in Pixeln
    uint16_t r;            ///< Radius für abgerundete Ecken
    uint16_t outlinecolor; ///< Umrandungsfarbe (RGB565)
    uint16_t fillcolor;    ///< Füllfarbe (RGB565)
    uint16_t textcolor;    ///< Textfarbe (RGB565)
    const char *label;     ///< Button-Text oder Symbol-Code
};

/**
 * @brief Parametersatz für ein Kassetten-Design auf Farbtabellen-Basis
 *
 * Grundlage ist eine 285x184-Farbtabelle aus einer fotorealistischen Vorlage. Animiert werden nur das Bandfenster (Wickelbögen) und die beiden Naben, über die der in buildWrapSprite() gezeichnete Einheitsdorn rotiert gelegt wird. Alle Koordinaten beziehen sich auf das Kassetten-Sprite.
 */
struct CassetteDesign
{
    const unsigned short *bitmap; ///< 285x184-Farbtabelle (RGB565) der Kassette
    uint16_t labelText;           ///< Schriftfarbe des Interpreten auf dem Beschriftungsstreifen
    int16_t labelX;               ///< Interpreten-Zone, linke Kante
    int16_t labelY;               ///< Interpreten-Zone, obere Kante
    int16_t labelW;               ///< Interpreten-Zone, Breite; 0 = die Vorlage hat keinen freien Streifen
    uint16_t tape;                ///< dunklere Lagenringe im Bandwickel
    uint16_t tapeEdge;            ///< Grundfarbe des Bandwickels
    int16_t winX;                 ///< Bandfenster, linke Kante
    int16_t winY;                 ///< Bandfenster, obere Kante
    int16_t winW;                 ///< Bandfenster, Breite
    int16_t winH;                 ///< Bandfenster, Höhe
    int16_t hubY;                 ///< Höhe der Nabenzentren
    int16_t hubXLeft;             ///< Nabenzentrum links; voll gekachelte Fenster: 82/202 (echter C-Kassetten-Abstand 42,5/100,4 der Breite)
    int16_t hubXRight;            ///< Nabenzentrum rechts; Bullaugen-Designs folgen stattdessen den sichtbaren Vorlagen-Tellern
    int16_t hubCoverR;            ///< Deckradius des Einheitsdorns: groß genug für den Vorlagen-Teller, klein genug für die Nabenöffnung
    int16_t reelRMin;             ///< Wickelradius leer, an die Proportionen der Vorlage angepasst; mindestens 25, damit der Einheitsdorn immer im Band sitzt
    int16_t reelRMax;             ///< Wickelradius voll: 24,5/42,5 des Nabenabstands, wie bei einer echten C90
};

/**
 * @brief Hauptklasse für den SD-Karten Music-Player
 *
 * Diese Klasse implementiert einen vollständigen Music-Player mit folgenden Features:
 * - Playlist-Verwaltung mit Scroll-Funktion, ein Tipp auf einen Titel spielt ihn direkt
 * - Metadaten-Anzeige (Artist, Title, Bitrate)
 * - Cover-Image-Anzeige (JPEG/PNG)
 * - Animierte Kassetten-Ansicht mit wechselbaren Designs (Tipp auf die Kassette)
 * - Lautstärkeregler und 3-Band-Equalizer
 * - Player-Controls (Play, Pause, Stop, Previous, Next), Halten der Sprungtasten spult
 * - Fortschrittsanzeige mit Zeitangabe, Tippen springt, Ziehen scrubbt
 * - Sleep-Timer mit Uhr-Anzeige
 * - Integration mit AudioManager für Echtzeit-Audiodaten
 * - FileExplorer-Integration für Dateiauswahl
 */
class SdPlayer : public BaseModule
{
public:
    /**
     * @brief Konstruktor
     * @param tft Referenz auf das LGFX Display-Objekt
     * @param audio Referenz auf die Audio-Instanz für Playback-Steuerung
     *
     * Übernimmt die Referenzen auf Display und Audio-Instanz; sämtliche Allokationen (Sprites, Puffer) erfolgen erst in init() bzw. bei Bedarf.
     */
    explicit SdPlayer(LGFX &tft, Audio &audio);

    /**
     * @brief Destruktor
     */
    ~SdPlayer();

    /**
     * @brief Initialisiert das SD-Player Modul
     * @param rtc Referenz auf die Echtzeituhr
     * @param callbacks Struktur mit allen Callback-Funktionen
     * @return true bei erfolgreicher Initialisierung, false bei Fehler
     *
     * Erstellt alle UI-Elemente, initialisiert Sprites, zeichnet den initialen Zustand und bereitet das Modul für die Verwendung vor.
     */
    bool init(DS1307_RTC &rtc, const SdPlayerCallbacks &callbacks);

    /**
     * @brief Hauptzeichnungsschleife - muss zyklisch aufgerufen werden
     *
     * Verarbeitet:
     * - Touch-Input für alle interaktiven Elemente
     * - AudioManager-Events (EOF, ID3-Tags, Bitrate, Cover-Images)
     * - UI-Updates (Playlist, Metadaten, Fortschritt, Cover)
     * - Scroll-Physik für Playlist
     * - Button-Zustände und -Feedback
     * - Dialoge (Datei-Explorer, Meldungsbox, Sleep-Timer); Titelenden werden auch bei offenem Dialog verarbeitet
     */
    void draw() override;

    /**
     * @brief Gibt den Modul-Typ zurück
     * @return AppModule::SD_PLAYER
     *
     * Wird vom Modul-System verwendet, um den Typ zu identifizieren.
     */
    AppModule type() const override;

    /**
     * @brief Aktualisiert die Kassetten-Anzeige
     * @param force Erzwingt das Neuzeichnen von Wickeln und Zahnkränzen, etwa nach einem Designwechsel
     *
     * - Bandwickel: Radien r_L / r_R aus Flächenkonstanz (r^2 linear mit Bandlänge), neu gezeichnet nur bei sichtbarer Radiusänderung
     * - Zahnkränze: ω skaliert mit 1/r (über Referenzradius), transparent über die Wickel gelegt
     */
    void updateCassette(bool force = false);

    /**
     * @brief Kürzt Text UTF-8-sicher auf maxWidth (px) und hängt "..." an, gemessen mit der geladenen Schrift des Sprites
     * @param spr Sprite, dessen Schriftmetrik für die Breitenmessung gilt
     * @param in Eingabetext
     * @param out Zielpuffer für den gekürzten Text
     * @param outSize Größe des Zielpuffers in Bytes
     * @param maxWidth Maximale Textbreite in Pixeln
     */
    static void truncateWithEllipsis(Gfx_Sprite *spr, const char *in, char *out, int outSize, int maxWidth);

    /**
     * @brief Zeichnet den Interpreten zentriert auf den Beschriftungsstreifen des aktiven Designs (ComfortaaBold14)
     * @param force Erzwingt das Neuzeichnen auch bei unverändertem Text
     *
     * Designs ohne Beschriftungsstreifen (labelW = 0) bleiben unbeschriftet; gezeichnet wird nur, wenn sich der sichtbare gekürzte Text geändert hat oder force gesetzt ist.
     */
    void drawCassetteLabel(bool force = false);

    /**
     * @brief Callback für ein reguläres Track-Ende
     *
     * Wird vom AudioManager aufgerufen, wenn ein Track sauber beendet ist. Markiert den Track als abgespielt und startet den nächsten Track.
     */
    void onAudioTrackFinished();

    /**
     * @brief Aktualisiert Metadaten aus der Audio-Bibliothek
     * @param artist Interpret/Künstler (oder "" für keine Änderung)
     * @param title Titel des Songs (oder "" für keine Änderung)
     * @param bitrate Bitrate als String (oder "" für keine Änderung)
     *
     * Triggert ein UI-Update wenn sich Metadaten ändern.
     */
    void updateMetadata(const char *artist, const char *title, const char *bitrate);

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

    // ===================================================================
    // private Hilfsfunktionen
    // ===================================================================

    /**
     * @brief Art der Track-Beendigung
     */
    enum class TrackFinishType
    {
        COMPLETED, ///< Track wurde regulär bis zum Ende abgespielt
        ABORTED    ///< Track wurde abgebrochen oder Start ist fehlgeschlagen
    };

    /**
     * @brief Fordert das Backend zum Stoppen der Wiedergabe auf
     */
    void requestBackendStop();

    /**
     * @brief Bricht wartende Start-/Cover-Zustände ab
     */
    void cancelPendingPlaybackWaits();

    /**
     * @brief Setzt laufzeitbezogene Playback-Daten zurück
     */
    void resetRuntimePlaybackState();

    /**
     * @brief Leert die angezeigten Metadaten
     * @param clearCurrentFile Wenn true, wird auch der aktuelle Dateipfad gelöscht
     */
    void clearDisplayedMetadata(bool clearCurrentFile = false);

    /**
     * @brief Behandelt einen regulär beendeten oder abgebrochenen Track konsistent
     * @param finishType Art der Beendigung
     * @param advanceToNext Wenn true, wird anschließend der nächste Track gestartet
     */
    void finalizeCurrentTrack(TrackFinishType finishType, bool advanceToNext);

    /**
     * @brief Rundet eine Bitrate auf die nächste Standard-MP3-Bitrate
     */
    uint32_t snapToStandardBitrate(uint32_t bitrate);

    /**
     * @brief Aktualisiert den Audio-Status durch Polling der Audio-Instanz
     *
     * Diese Methode wird periodisch (alle 1 Sekunde) aus draw() aufgerufen und aktualisiert alle relevanten Playback-Informationen:
     * - Wiedergabestatus (läuft/gestoppt); der Pausenzustand wird über m_playbackState geführt
     * - Zeitinformationen (currentTime/duration)
     * - Fortschritt in Prozent
     * - Dateigröße und Position
     */
    void updateAudioStatus();

    /**
     * @brief Entfernt Track-Nummerierungen vom Anfang des Dateinamens
     * @param filename Der Dateiname (ohne Pfad, mit Extension)
     * @param output Buffer für den bereinigten Namen
     * @param maxLen Maximale Länge des Output-Buffers
     *
     * Erkennt und entfernt verschiedene Nummerierungsmuster:
     * - "01 - Titel.mp3" -> "Titel"
     * - "01 Titel.mp3" -> "Titel"
     * - "2-01 Titel.mp3" -> "Titel"
     * - "01. Titel.mp3" -> "Titel"
     * - "Track 01 - Titel.mp3" -> "Titel"
     * - etc.
     *
     * Die Funktion ist flexibel und erkennt:
     * - 1-3 stellige Zahlen
     * - Optional: Disc-Nummer (z.B. "2-01")
     * - Verschiedene Trennzeichen: Leerzeichen, Minus, Punkt, Unterstrich
     * - Optional: "Track" oder "CD" Präfixe
     */
    void cleanupFilename(const char *filename, char *output, size_t maxLen);

    // ===================================================================
    // Cover-Image-Verwaltung
    // ===================================================================

    /**
     * @brief Lädt und zeichnet das eingebettete Cover-Bild (JPEG/PNG) in m_sprCover.
     *
     * Ablauf:
     * 1) Cover-Block aus der Datei in PSRAM lesen.
     * 2) Container-Parse:
     *      - ID3v2 APIC (MP3)
     *      - FLAC PICTURE (FLAC)
     *    Falls nicht erkannt: Fallback auf JPEG/PNG-Magic-Suche.
     * 3) Dekodierung / Skalierung:
     *      - JPEG:
     *          * JPEGDEC nur bei "großen" Bildern über DOWNSCALE_THRESHOLD mit 1/2, 1/4 oder 1/8 verwenden, so dass die Decoder-Ausgabe COVER_W x COVER_H in beiden Richtungen noch abdeckt. Danach isotrop ausfüllend in m_sprCover skalieren, der Überstand wird mittig beschnitten.
     *          * Kleinere Bilder bis zum Schwellwert werden 1:1 decodiert und mit LGFX isotrop eingepasst.
     *      - PNG:
     *          * Quellgröße aus IHDR (breit kompatibel).
     *          * isotrop "Contain"-Zoom via LGFX::drawPng und zentrierte Position.
     * 4) m_sprCover (COVER_W x COVER_H = 220x220) aufs Panel pushen.
     *
     * @return true  bei Erfolg (Cover angezeigt)
     * @return false bei Fehler (es wird ein Platzhalter gezeichnet)
     */
    bool loadAndDrawCover();

    /**
     * @brief Setzt das Cover zurück (Platzhalter)
     */
    void resetCover();

    /**
     * @brief Zeichnet einen dynamischen Platzhalter in die Cover-Fläche,
     *        wenn kein eingebettetes Cover vorhanden ist. Nutzt Dateiname/Metadaten
     *        zur Akzentfarbe, Initialen und Unterzeile.
     * @param preferredText Optionaler Titel/Name; wenn leer, wird der Dateiname bereinigt.
     */
    void drawCoverFallback(const char *preferredText = nullptr);

    /**
     * @brief Details unter dem Cover zeichnen (Album, Format, Bitrate, Track xx/xx etc.); ohne geladenen Titel zeigt die Box SD-Kapazität und Playlist-Kurzfassung
     */
    void drawDetails(bool init);

    /**
     * @brief Infobox ohne geladenen Titel: SD-Kapazität oben, darunter Playlist-Kurzfassung oder Hinweis auf die Durchsuchen-Taste
     */
    void drawIdleDetails();

    // ===================================================================
    // Interne Strukturen
    // ===================================================================

    /**
     * @brief Scroll-State für die Playlist
     */
    struct ScrollState
    {
        int scrollOffset = 0;            ///< Aktueller Scroll-Offset in Pixeln
        int contentHeight = 0;           ///< Gesamthöhe des Playlist-Inhalts
        int viewportHeight = 0;          ///< Sichtbare Höhe des Viewports
        int maxScroll = 0;               ///< Maximaler Scroll-Offset
        float scrollVelocity = 0;        ///< Scroll-Geschwindigkeit für Trägheit
        float inertiaRemainder = 0;      ///< gesammelte Bruchteil-Bildpunkte der Trägheitsschritte; die Ganzzahl-Position schluckte sie sonst
        bool isDragging = false;         ///< Nutzer zieht gerade
        int dragStartX = 0;              ///< X-Position beim Start des Drags, entlarvt seitliche Wischgesten
        int dragStartY = 0;              ///< Y-Position beim Start des Drags
        int dragStartOffset = 0;         ///< Scroll-Offset beim Start des Drags
        unsigned long lastTouchTime = 0; ///< Zeitpunkt des letzten Touch-Events
        int lastTouchY = 0;              ///< Y-Position des letzten Touch-Events
        bool tapCandidate = false;       ///< Geste begann in ruhender Liste und kann noch ein Tipp werden
        unsigned long touchStartMs = 0;  ///< Zeitpunkt des Touch-Beginns für die Tipp-Haltezeit
        bool startedInPlaylist = false;  ///< aktuelle Geste begann im Playlist-Bereich; nur dann sind die Drag-Anker gültig
    };

    /**
     * @brief Playback-Status für Button-Visualisierung
     */
    enum class PlaybackState
    {
        STOPPED, ///< Keine Wiedergabe
        PLAYING, ///< Aktive Wiedergabe
        PAUSED   ///< Pausiert
    };

    // ===================================================================
    // Layout-Konstanten
    // ===================================================================

    static constexpr int PADDING = 15;                                     ///< Standard-Abstand zwischen Elementen
    static constexpr int SCROLLBAR_W = 5;                                  ///< Breite der Scrollbar
    static constexpr int PLAYLIST_Y = 5;                                   ///< Y-Position der Playlist
    static constexpr int PLAYLIST_W = 245;                                 ///< Breite der Playlist
    static constexpr int PLAYLIST_X = 800 - PLAYLIST_W - SCROLLBAR_W;      ///< X-Position der Playlist
    static constexpr int PLAYLIST_H = 300;                                 ///< Höhe der Playlist
    static constexpr int PLAYLIST_ITEM_H = 27;                             ///< Höhe eines Playlist-Eintrags
    static constexpr int STATUS_H = 32;                                    ///< Höhe der Statuszeile
    static constexpr int STATUS_Y = 316;                                   ///< Y-Position der Statuszeile
    static constexpr int BUTTONS_Y = STATUS_Y + STATUS_H + PADDING;        ///< Y-Position der Control-Buttons
    static constexpr int PLAYER_CONTROLS_Y = 250;                          ///< Y-Position der Player-Controls
    static constexpr int PROGRESS_W = 5 * 45 + 4 * PADDING;                ///< Breite der Progress-Bar, bündig mit der Reihe der fünf Player-Buttons
    static constexpr int PROGRESS_H = 24;                                  ///< Höhe Progress-Bar
    static constexpr int PROGRESS_X = 10;                                  ///< X-Position Progress-Bar
    static constexpr int PROGRESS_Y = PLAYER_CONTROLS_Y - 15 - PROGRESS_H; ///< Y-Position Progress-Bar
    static constexpr int COVER_W = 230;                                    ///< Breite Cover
    static constexpr int COVER_H = 220;                                    ///< Höhe Cover
    static constexpr int COVER_X = PLAYLIST_X - COVER_W - 10;              ///< X-Position Cover
    static constexpr int COVER_Y = 15;                                     ///< Y-Position Cover
    // Audio-Kassette
    static constexpr int CASSETTE_W = 285; ///< Breite des Kassetten-Sprites
    static constexpr int CASSETTE_H = 184; ///< Höhe des Kassetten-Sprites
    static constexpr int CASSETTE_X = 10;  ///< X-Position der Kassette auf dem Panel
    static constexpr int CASSETTE_Y = 15;  ///< Y-Position der Kassette auf dem Panel
    // Bandwickel und Zahnkränze
    static constexpr int WRAP_SPR_SIZE = 48;           ///< Kantenlänge des Naben-Sprites, fasst auch die großen Kupfer- und Goldteller
    static constexpr uint32_t WRAP_UPDATE_MS = 25;     ///< kürzester Abstand zweier Naben-Bilder, deckelt die Zeichenlast
    static constexpr uint16_t WRAP_TRANSP = 0x0020;    ///< Transparenzfarbe des Naben-Sprites, dunkel gewählt, damit Mischsäume der Rotation unsichtbar bleiben
    static constexpr int CASSETTE_DESIGN_COUNT = 8;    ///< Anzahl der Kassetten-Designs
    // Beschriftungsfeld auf der Kassette
    static constexpr int TAPE_LABEL_W = 215;             ///< Breite des Kassetten-Beschriftungsfelds
    static constexpr int TAPE_LABEL_H = 20;              ///< Höhe des Kassetten-Beschriftungsfelds

    // ===================================================================
    // Kapazitäts-Konstanten (Anti-Fragmentierung)
    // ===================================================================
    // Hinweis: shrink_to_fit() auf ESP32/PSRAM triggert free+realloc und verursacht Heap-Fragmentierung. Stattdessen einmalig reserve() beim Start setzen und bei clear() NICHT shrink_to_fit() aufrufen.
    static constexpr size_t PLAYLIST_RESERVE_CAPACITY = 200;      ///< Initiale Kapazität für Playlist
    static constexpr size_t PLAYED_TRACKS_RESERVE_CAPACITY = 200; ///< Initiale Kapazität für PlayedTracks
    static constexpr size_t COVER_EVENT_RESERVE_CAPACITY = 8;     ///< Initiale Kapazität für Cover-Events

    // ===================================================================
    // Kernkomponenten
    // ===================================================================

    LGFX *m_tft = nullptr;         ///< Zeiger auf Display-Objekt
    DS1307_RTC *m_rtc = nullptr;   ///< Zeiger auf Echtzeituhr
    Audio *m_audio = nullptr;      ///< Zeiger auf Audio-Instanz für Playback-Status
    SdPlayerCallbacks m_callbacks; ///< Callback-Funktionen
    bool m_initialized = false;    ///< Initialisierungsstatus

    // RAII mit unique_ptr
    std::unique_ptr<FileExplorer> m_explorer; ///< FileExplorer für Dateiauswahl
    std::unique_ptr<MessageBox> m_messageBox; ///< MessageBox für Bestätigungsdialoge
    std::unique_ptr<SleepTimer> m_sleepDlg;   ///< SleepTimer-Dialog

    Visualizer m_visualizer; ///< Visualizer-Instanz für VU-Meter, Spektrum-Analyse und Wellenform-Darstellung

    // ===================================================================
    // Buttons
    // ===================================================================

    Button m_controlButtons[4];                        ///< Control-Buttons: "durchs.", "leeren", "reset", Power/Sleep-Timer
    Button m_playerButtons[5];                         ///< Player-Buttons: Previous, Play, Pause, Stop, Next
    static const SdPlayerButton m_buttonDefs[4];       ///< Statische Definitionen für Control-Buttons
    static const SdPlayerButton m_playerButtonDefs[5]; ///< Statische Definitionen für Player-Buttons
    bool m_wasPressed[4] = {false};                    ///< Zustand der Control-Buttons
    bool m_wasPlayerPressed[5] = {false};              ///< Zustand der Player-Buttons
    unsigned long m_pressTime[4] = {0};                ///< Press-Zeit der Control-Buttons
    unsigned long m_playerPressTime[5] = {0};          ///< Press-Zeit der Player-Buttons
    bool m_seekHoldTriggered = false;                  ///< Halten einer Sprungtaste hat das Spulen ausgelöst, das Loslassen springt dann keinen Titel
    uint32_t m_lastSeekStepMs = 0;                     ///< Zeitpunkt des letzten Spulschritts
    bool m_progressScrubbing = false;                  ///< Finger zieht am Fortschrittsbalken, die Berührung gehört dem Balken
    bool m_progressTouchSeen = false;                  ///< Berührungszustand des Vorframes, erkennt frische Berührungen
    uint32_t m_scrubTargetSec = 0;                     ///< Zielzeit des Ziehens in Sekunden

    // Statische Definitionen für SleepTimer
    static const char *m_sleepLabels[]; ///< Beschriftungen für Sleep-Timer-Optionen
    static int m_sleepValues[];         ///< Minutenwerte für Sleep-Timer-Optionen

    // ===================================================================
    // Sprites
    // ===================================================================

    Gfx_Sprite *m_sprDiv = nullptr;               ///< Sprite für Trennlinien
    Gfx_Sprite *m_sprKnob = nullptr;              ///< Sprite für Lautstärkeregler
    Gfx_Sprite *m_sprKnobText = nullptr;          ///< Sprite für Lautstärkeregler-Text
    Gfx_Sprite *m_sprTracks = nullptr;            ///< Sprite für Equalizer-Regler
    Gfx_Sprite *m_sprPlaylist = nullptr;          ///< Sprite für Playlist-Anzeige
    uint16_t *m_emptyFolderIcon = nullptr;        ///< Ordner-Symbol der leeren Playlist, vergrößert gezeichnet, im PSRAM
    Gfx_Sprite *m_sprScrollbar = nullptr;         ///< Sprite für Scrollbar
    Gfx_Sprite *m_sprMeta = nullptr;              ///< Sprite für Metadaten-Anzeige
    Gfx_Sprite *m_sprProgress = nullptr;          ///< Sprite für Fortschrittsanzeige
    Gfx_Sprite *m_sprCover = nullptr;             ///< Sprite für Cover-Anzeige
    Gfx_Sprite *m_sprDetails = nullptr;           ///< Sprite für Details unter dem Cover
    Gfx_Sprite *m_sprCassette = nullptr;          ///< Sprite für Audio-Kassette
    Gfx_Sprite *m_sprTapeLabel = nullptr;         ///< Sprite für den Beschriftungsstreifen der Kassette, zeigt den Interpreten
    Gfx_Sprite *m_sprWinGround = nullptr;         ///< Fenstergrund im PSRAM, einmal je Design gefüllt: erspart dem Wickel-Neuaufbau den Flash-Reload (20-ms-Lastspitze störte das Panel)
    Gfx_Sprite *m_sprDorn = nullptr;              ///< Kompositionspuffer für den rotierten Dorn: pushRotateZoomWithAA direkt auf den Bildspeicher kam dort nur häppchenweise an (Panel-Pfad, gleiche Familie wie der setClipRect-Fehler)
    Gfx_Sprite *m_sprWin = nullptr;               ///< Sprite in Bandfenstergröße; der ungeclippte Push ersetzt setClipRect, das beim Sprite-Push auf dem RGB-Panel wirkungslos blieb
    Gfx_Sprite *m_sprWrap = nullptr;              ///< Sprite für die Zahnkränze der Kassette
    Gfx_Sprite *m_sprRetroAnalogClock = nullptr;  ///< Sprite für analoge Uhr mit Timer
    Gfx_Sprite *m_sprRetroDigitalClock = nullptr; ///< Sprite für digitale Uhr mit Timer

    // ===================================================================
    // Cover-Image-Verwaltung
    // ===================================================================

    bool m_hasCover = false;              ///< Flag: Cover verfügbar
    std::vector<uint32_t> m_coverOffsets; ///< Cover-Offsets aus AudioManager [pos, len, pos, len, ...]
    char m_currentAudioFile[256] = {0};   ///< Aktueller Audio-Dateipfad für Cover-Zugriff
    bool m_needsCoverRedraw = false;      ///< Flag: Cover muss neu gezeichnet werden

    // Cover-Tracking für intelligentes Caching
    char m_lastDisplayedCoverFile[256] = {0};          ///< Pfad der Datei, deren Cover aktuell angezeigt wird
    std::vector<uint32_t> m_lastDisplayedCoverOffsets; ///< Offsets des aktuell angezeigten Covers

    // ===================================================================
    // Cover-Buffer (Anti-Fragmentierung)
    // ===================================================================
    // Der Puffer wächst bei Bedarf und wird nicht verkleinert, sodass keine wechselnden Blockgrößen im PSRAM freigegeben und neu allokiert werden. Das temporäre Sprite für die JPEG-Dekodierung bleibt dynamisch, da pushRotateZoom den gesamten Inhalt transformiert und die genaue Decoder-Ausgabegröße benötigt.
    uint8_t *m_coverBuffer = nullptr; ///< Wiederverwendbarer PSRAM-Buffer für Cover-Daten
    size_t m_coverBufferCapacity = 0; ///< Aktuelle Kapazität des Cover-Buffers

    // Cover-Timeout für verzögertes Fallback (verhindert Flackern bei Alben)
    bool m_waitingForCoverEvent = false;                     ///< Flag: Warten auf Cover-Event nach Track-Start
    uint32_t m_coverEventWaitStartMs = 0;                    ///< Zeitpunkt des Track-Starts für Timeout
    static constexpr uint32_t COVER_EVENT_TIMEOUT_MS = 4000; ///< Timeout (4 Sek.) bis Fallback gezeigt wird

    // Playback-Start-Timeout (verhindert "hängende" UI bei Wiedergabe-Fehlern)
    bool m_waitingForPlaybackStart = false;                     ///< Flag: Warten auf Playback-Start nach onPlayFileRequested
    uint32_t m_playbackStartWaitMs = 0;                         ///< Zeitpunkt des Playback-Requests
    static constexpr uint32_t PLAYBACK_START_TIMEOUT_MS = 3000; ///< Timeout (3 Sek.) bis Fehler angenommen wird

    // ===================================================================
    // Audio-Status
    // ===================================================================

    AudioStatus m_audioStatus;          ///< Aktueller Audio-Status
    bool m_needsProgressRedraw = false; ///< Flag für Progress-Bar-Neuzeichnung
    uint8_t m_lastDrawnProgress = 0;    ///< Letzter gezeichneter Progress (für Delta-Check)
    uint32_t m_lastDrawnTimeSec = 0;    ///< Zuletzt gezeichnete Laufzeit in Sekunden (für Delta-Check)
    bool m_uiSuspended = false;         ///< Dialog oder Grafik-Sperre aktiv: Zeichenaufrufe der Event-/EOF-Kette werden auf die Redraw-Flags umgelenkt
    uint32_t m_cachedDuration = 0;      ///< Gecachte Track-Dauer

    // Duration-Management
    uint32_t m_lastDurationUpdateMs = 0; ///< Zeitstempel letztes Duration-Update
    bool m_isVbrTrack = false;           ///< MP3-Titel, dessen Dauer aus der Library-Schätzung stammt und während der Wiedergabe nachgeführt wird

    // EOF-Verarbeitung: Verhindert doppelte Aufrufe (Event + Fallback)
    bool m_trackFinishedProcessing = false; ///< Flag: Track-Ende wird bereits verarbeitet

    // ===================================================================
    // Playlist-Daten
    // ===================================================================

    std::vector<FileResult> m_playlist; ///< Liste aller Tracks
    std::vector<bool> m_playedTracks;   ///< je Eintrag true, sobald der Titel bis zum Ende gespielt wurde; Grundlage der Grau-Markierung, Abbrüche und Sprünge zählen nicht
    int m_currentTrackIndex = 0;        ///< Index des aktuellen Tracks
    bool m_playlistFinished = false;    ///< Flag: Playlist komplett abgespielt

    // ===================================================================
    // Metadaten
    // ===================================================================

    char m_currentArtist[128] = {0};         ///< Aktueller Artist
    char m_currentTitle[128] = {0};          ///< Aktueller Title
    char m_currentAlbum[128] = {0};          ///< Aktuelles Album
    char m_currentBitrate[32] = {0};         ///< Aktuelle Bitrate
    bool m_metadataUpdated = false;          ///< Flag für Metadaten-Update
    char m_displayBitrate[32] = {0};         ///< Anzuzeigende Bitrate in kbps
    char m_lastArtistDrawn[128] = {0};       ///< zuletzt gerenderter Artist (für "no-op")
    char m_lastAlbumDrawn[128] = {0};        ///< zuletzt gerendertes Album (für "no-op")
    char m_lastFileTypeDrawn[8] = {0};       ///< zuletzt gerenderter Dateityp (für "no-op")
    char m_lastBitrateDrawn[32] = {0};       ///< zuletzt gerenderte Bitrate (für "no-op")
    int m_lastTrackIndexDrawn = -1;          ///< zuletzt gerenderter Track-Index (für "no-op")
    int m_lastPlaylistSizeDrawn = -1;        ///< zuletzt gerenderte Playlist-Größe (für "no-op")
    uint64_t m_sdTotalBytes = 0;             ///< Gesamtkapazität der SD-Karte in Bytes, gelesen beim Initialisieren der Infobox
    uint64_t m_sdUsedBytes = 0;              ///< belegter Platz der SD-Karte in Bytes
    bool m_needsCassetteLabelRedraw = false; ///< Flag: Label neu zeichnen?

    // ===================================================================
    // Audio-Kassette
    // ===================================================================

    uint32_t m_lastWrapTime = 0; ///< Für zeitbasierte Wickeldorn-Rotation

    // Performance-Optimierung: Wickel und Zahnkränze nur bei Wiedergabe oder State-Änderung rendern
    bool m_lastCassettePlaying = false; ///< Letzter Wiedergabe-Status für Change-Detection
    float m_lastWrapAngleL = -999.0f;   ///< Letzter gerenderter Winkel links
    float m_lastWrapAngleR = -999.0f;   ///< Letzter gerenderter Winkel rechts
    int m_lastReelRadiusL = -1;         ///< Letzter gerenderter Wickelradius links in Bildpunkten
    int m_lastReelRadiusR = -1;         ///< Letzter gerenderter Wickelradius rechts in Bildpunkten
    uint32_t m_lastWrapPushMs = 0;      ///< Zeitpunkt des letzten Zahnkranz-Bildes für die Zeichenlast-Drossel

    // Wickel-Physik. Die Radien kommen aus dem Design, die Drehgeschwindigkeit skaliert um deren Mittelwert
    static constexpr float WRAP_DPS_AT_REF = 140.0f; ///< Deg/s beim mittleren Wickelradius, zusammen mit WRAP_UPDATE_MS klein genug für ruckelfreie Drehschritte

    float m_wrapAngleL = 0.0f; ///< Winkel linker Dorn (deg)
    float m_wrapAngleR = 0.0f; ///< Winkel rechter Dorn (deg)

    // Kassetten-Design
    static const CassetteDesign m_cassetteDesigns[CASSETTE_DESIGN_COUNT]; ///< Parametersätze der Designs
    uint8_t m_cassetteDesignIdx = 0;                                      ///< Index des aktiven Designs
    bool m_cassetteTouchSeen = false;                                     ///< Berührungszustand des Vorframes für den Tipp-Wechsel
    uint32_t m_lastCassetteSwitchMs = 0;                                  ///< Zeitpunkt des letzten Designwechsels (Entprellung des Tipp-Wechsels)
    bool m_pendingCassetteApply = false;                                  ///< Designwechsel gewählt, Aufbau folgt im nächsten dialogfreien Frame

    // ===================================================================
    // Uhr
    // ===================================================================
    int m_lastDrawnSecond = -1; ///< Zuletzt gezeichnete Sekunde

    // ===================================================================
    // Album-Cover
    // ===================================================================

    static JPEGDEC s_jpeg; ///< Statischer JPEG-Decoder (spart RAM)

    // ===================================================================
    // Playback-Status
    // ===================================================================

    PlaybackState m_playbackState = PlaybackState::STOPPED; ///< Aktueller Playback-Zustand
    bool m_resetRequested = false;                          ///< Flag: Reset des Players angefordert

    // ===================================================================
    // Lautstärkeregler-Zustand
    // ===================================================================

    KnobTouchState m_knobTouchState = KnobTouchState::IDLE; ///< Touch-Status des Lautstärke-Reglers
    bool m_muteTriggered = false;                           ///< Flag: Mute wurde bereits getriggert (verhindert Mehrfach-Trigger)
    float m_totalDegrees = 0.0f;                            ///< Gesamtwinkel des Lautstärkereglers
    float m_lastAngle = 0.0f;                               ///< Letzter Winkel für Delta-Berechnung
    uint8_t m_prevVol = 0;                                  ///< Vorherige Lautstärke

    // ===================================================================
    // Equalizer-Zustand
    // ===================================================================

    int8_t m_balance_prev = -128;      ///< Vorherige Balance-Position
    int8_t m_gainLowPass_prev = -128;  ///< Vorherige Tiefen-Position
    int8_t m_gainBandPass_prev = -128; ///< Vorherige Mitten-Position
    int8_t m_gainHighPass_prev = -128; ///< Vorherige Höhen-Position

    // ===================================================================
    // Touch-Handling
    // ===================================================================

    int m_touchX = 0;       ///< Aktuelle Touch X-Position
    int m_touchY = 0;       ///< Aktuelle Touch Y-Position
    bool m_touched = false; ///< Touch aktiv

    // ===================================================================
    // Scroll-State
    // ===================================================================

    ScrollState m_scrollState;          ///< Scroll-Zustand der Playlist
    bool m_needsPlaylistRedraw = true;  ///< Flag für Playlist-Neuzeichnung
    bool m_needsDetailsRedraw = true;   ///< Flag für Details-Neuzeichnung
    int m_lastScrollOffset = -1;        ///< Letzter Scroll-Offset
    bool m_wasTouching = false;         ///< Touch-Zustand des vorherigen Frames
    bool m_autoScrollRequested = false; ///< Flag für Auto-Scroll zum aktuellen Track

    // ===================================================================
    // Private Hilfsfunktionen
    // ===================================================================

    // Playlist-Helfer
    bool isFileInPlaylist(const char *path) const;

    // Zeichenfunktionen
    void drawProgressBar();
    void drawMetaText(bool init, const char *first = "", const char *second = "", uint16_t firstColor = 0x5370, uint16_t secondColor = TFT_SILVER, uint16_t separatorColor = TFT_DARKGRAY, const char *separator = " | ", int reservedPixels = 10, uint16_t backgroundColor = 0x1082);
    void calcMarkerPos(float angleDeg, int center, int radiusOffset, int &outX, int &outY);
    void drawKnob(bool init, uint8_t vol = 0, bool skipCallbacks = false);
    void drawTracks(bool init, int8_t balance = 0, int8_t gainLowPass = 0, int8_t gainBandPass = 0, int8_t gainHighPass = 0, bool skipCallbacks = false);
    void drawPlaylist();

    /**
     * @brief Leere Playlist: vergrößertes Ordner-Symbol, darunter "Keine Titel" und zwei Hinweiszeilen zur Durchsuchen-Taste
     */
    void drawEmptyPlaylist();
    void drawPlaylistEntry(int index, int yPos);
    void drawScrollbar();
    void drawStatusLine();

    /**
     * @brief Zeichnet die analoge Uhr
     * @param init true für Initialisierung, false für Update
     * @param x X-Position der Uhr
     * @param y Y-Position der Uhr
     * @param remainingMinutes Verbleibende Minuten (für Sleep-Timer)
     * @param remainingSeconds Verbleibende Sekunden (für Sleep-Timer)
     *
     * Zeigt eine analoge Uhr mit optional integriertem Sleep-Timer-Countdown.
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

    // Scroll-Physik
    void updateScrollPhysics();
    void handlePlaylistTouch(int32_t x, int32_t y, bool is_touched);
    void handlePlaylistTap();
    void scrollToCurrentTrack();
    void resetPlayedStatus();

    // Event-Queue-Verarbeitung (ausgelagert für bessere Performance-Kontrolle)
    void processEventQueue();

    // Button-Handling
    void updatePlayerButtonStates();
    void handleButtons();
    void handlePlayerButtons();
    void handleProgressTouch();
    void handleCassetteTouch();

    // Kassetten-Darstellung
    void drawCassetteBody();
    void buildWrapSprite();
    void pushDorn(int hubX, int hubY, float angle);
    void drawReels(int radiusLeftPx, int radiusRightPx);
    void applyCassetteDesign();
    void selectRandomCassetteDesign();

    // Playback-Steuerung
    void playNextTrack();
    void playPreviousTrack();
    void playCurrentTrack();
    void clearPlaylist();
    void stopPlayerImmediately();

    // Callbacks
    void onMessageBoxResult(bool confirmed);

    // Hilfsfunktionen
    bool isDialogOpen() const;
    void zeroVisualizer();
    void refineDurationForVbr();

    /**
     * @brief Zeichnet die komplette UI nach einem Sprachwechsel neu
     *
     * Wird aus `draw()` aufgerufen, sobald `I18n::consumeLanguageChanged()` true liefert. Löscht das Display, initialisiert alle Sprites und Buttons mit den aktuellen Sprach-Strings neu und triggert eine Materialisierung der dynamischen Inhalte. Audio-Callbacks (Lautstärke, Equalizer, VU-Mode) werden dabei nicht gefeuert.
     */
    void redrawAll();

    // Event-Queue-Verarbeitung
    char m_eventText[256] = {0};            ///< Buffer für Event-Text
    std::vector<uint32_t> m_eventCoverData; ///< Buffer für Cover-Offsets
};

#endif // SD_PLAYER_H

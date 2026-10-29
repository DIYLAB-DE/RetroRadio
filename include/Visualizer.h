#ifndef VISUALIZER_H
#define VISUALIZER_H

#include <array>
#include <atomic>
#include <cstdint>
#include <memory>

// ESP32 FreeRTOS
#include "freertos/FreeRTOS.h"
#include "freertos/portmacro.h"

#include "psram_unique_ptr.hpp"

// Projekt-Header
#include "Logger.h"
#include "AppState.h"
#include "BaseModule.h"
#include "DS1307_RTC.h"
#include "Gfx.h"
#include "kiss_fftr.h"

template <typename T>
using PsramUniquePtr = std::unique_ptr<T[], PsramDeleter>;

/**
 * @brief Enumeration für die Visualisierungsmodi
 */
enum class VisMode
{
    VIS_ANALOG = 0,   ///< Analoge VU-Meter Anzeige
    VIS_DIGITAL = 1,  ///< Digitale VU-Meter Anzeige
    VIS_SPECTRUM = 2, ///< Spektrum-Analysator Anzeige
    VIS_WAVE = 3,     ///< Wellenform Anzeige
    VIS_GONIO = 4,    ///< Goniometer Anzeige
    VIS_SPECTRUM_CENTERED = 5, ///< Spektrum mit 36 aus der Mitte wachsenden Balken
    VIS_WATERFALL = 6, ///< Wasserfall-Spektrum mit zeitlichem Verlauf
    VIS_AURORA = 7 ///< Ruhige Nordlicht-Farbschleier
};

/// Anzahl der Visualisierungsmodi
static constexpr int VIS_MODE_COUNT = 8;

/**
 * @brief Enumeration für die Stile der digitalen VU-Anzeige
 */
enum class DigitalVUStyle
{
    CLASSIC, ///< Klassischer Stil
    RAINBOW  ///< Regenbogen-Stil
};

/**
 * @brief Enumeration für die Stile der Spektrum-Anzeige
 */
enum class SpectrumStyle
{
    CLASSIC, ///< Klassischer Stil
    RAINBOW  ///< Regenbogen-Stil
};

/**
 * @brief Farbwelten des Nordlichts
 */
enum class AuroraStyle : uint8_t
{
    COOL, ///< Petrol, Türkis und Violett
    WARM  ///< Bernstein, Kupfer und warmes Rot
};

/**
 * @brief Enumeration für die Wellenform Anzeige
 */
enum class WaveformStyle
{
    OVERLAY, ///< Überlagerter Stil
    STACKED  ///< Gestapelter Stil
};

/**
 * @brief Enumeration für die Stile der Goniometer-Anzeige
 */
enum class GonioStyle
{
    CLASSIC, ///< Klassischer Stil
    RAINBOW  ///< Regenbogen-Stil
};

/**
 * @brief Farbfassung des analogen VU-Meters
 *
 * Beide Fassungen zeigen dasselbe Blatt in derselben Geometrie; nur die Farben unterscheiden sich, einschließlich des Zeigers. Umgeschaltet wird mit langem Druck auf die Anzeige, wie bei den anderen Visualisierungen auch.
 */
enum class AnalogVUStyle : uint8_t
{
    CLASSIC, ///< Blau, Weiß und Rot wie auf einem Studiogerät
    GREEN,   ///< Grünes Blatt mit bernsteinfarbenem Zeiger, wie ein hinterleuchtetes Instrument
    COUNT    ///< Anzahl der Fassungen
};

/**
 * @brief Trägheit des Zeigers im analogen VU-Meter
 *
 * Ein VU-Instrument nach Norm braucht 300 ms, bis der Zeiger einem Sprung zu 99 Prozent gefolgt ist, und überschwingt dabei um 1 bis 1,5 Prozent. Die Stufen sind nach genau dieser Zeit benannt, damit die Einstellung nachprüfbar bleibt. Ein Drehspulwerk läuft in beide Richtungen gleich, deshalb gibt es nur eine Zeit je Stufe.
 */
enum class VuBallistics : uint8_t
{
    FAST,     ///< 150 ms: lebhafter als die Norm
    STANDARD, ///< 300 ms: so verlangt es die Norm
    SLOW,     ///< 600 ms: ruhiges Bild
    COUNT     ///< Anzahl der Einstellungen
};

/**
 * @brief Konfigurationsstruktur für das Goniometer
 *
 * Definiert Position und Größe des Goniometers für verschiedene Module. Das Goniometer ist immer quadratisch.
 */
struct GonioConfig
{
    int16_t posX;       ///< X-Position des Goniometers
    int16_t posY;       ///< Y-Position des Goniometers
    uint8_t size;       ///< Größe des Goniometers (immer quadratisch)
    uint8_t center;     ///< Mittelpunkt
    uint8_t scale;      ///< Skalierungsfaktor für Koordinatenberechnungen
    uint8_t corrBarY;   ///< Y-Position des Korrelationsbalkens (relativ zum Sprite)
    uint8_t corrBarW;   ///< Breite des Korrelationsbalkens
    uint8_t corrCenter; ///< Mittelpunkt des Korrelationsbalkens
    uint8_t corrRange;  ///< Halbe Breite des Korrelationsbereichs
};

/**
 * @brief Struktur für Spektrumbänder
 */
struct SpectrumBand
{
    float smoothedMagnitude; ///< Geglättete Magnitude des Spektrumbands
    float peakPosFloat;      ///< Gleitkomma-Position des Peaks
    uint16_t currentPos;     ///< Aktuelle Position der Anzeige
    uint16_t peakPos;        ///< Position des Peak-Werts
    uint32_t peakTime;       ///< Zeitstempel des letzten Peaks
};

/**
 * @brief Struktur für Kanal-Daten (RMS/PPM)
 */
struct ChannelData
{
    float smoothedDB;    ///< Geglätteter dB-Wert
    uint16_t currentPos; ///< Aktuelle Position der Anzeige
    uint16_t peakPos;    ///< Position des Peak-Werts
    uint32_t peakTime;   ///< Zeitstempel des letzten Peaks
};

/**
 * @brief Klasse für Audio-Visualisierungen
 *
 * Diese Klasse bietet verschiedene Visualisierungsmodi für Audio-Signale:
 * - Analoge VU-Meter mit animierten Zeigern
 * - Digitale VU-Meter mit RMS und Peak-Anzeige
 * - Spektrum-Analysator mit FFT
 * - Wellenform-Anzeige
 */
class Visualizer
{
public:
    /**
     * @brief Konstruktor für den Visualizer
     */
    Visualizer();

    /**
     * @brief Destruktor für den Visualizer
     */
    ~Visualizer();

    /**
     * @brief Räumt alle allokierten Ressourcen auf
     */
    void cleanup();

    /**
     * @brief Initialisiert den Visualizer
     * @param tft Referenz auf das TFT-Display
     * @param rtc Referenz auf die Echtzeituhr
     * @return true wenn erfolgreich initialisiert, false bei Fehler
     */
    bool init(LGFX &tft, DS1307_RTC &rtc);

    /**
     * @brief Setzt die Audio-Werte für die Visualisierung
     * @param rmsLeft RMS-Wert für linken Kanal
     * @param rmsRight RMS-Wert für rechten Kanal
     * @param peakLeft Peak-Wert für linken Kanal
     * @param peakRight Peak-Wert für rechten Kanal
     */
    void setAudioValues(float rmsLeft, float rmsRight, float peakLeft, float peakRight);

    /**
     * @brief Setzt die Audio-Sample-Daten (thread-safe via Double-Buffer)
     * @param samplesL Zeiger auf Sample-Daten für linken Kanal
     * @param samplesR Zeiger auf Sample-Daten für rechten Kanal
     * @param count Anzahl der Samples
     * @param sampleRate Abtastrate dieses Sample-Blocks in Hz, 0 bei unbekannter Rate oder Stille
     *
     * Schreibt in den inaktiven Puffer und führt dann einen atomaren Swap durch. Kann sicher aus einem anderen Task aufgerufen werden während draw...() läuft.
     */
    void setSampleData(const int16_t *samplesL, const int16_t *samplesR, uint16_t count, uint32_t sampleRate);

    /**
     * @brief Behandelt Touch-Eingaben im Visualisierungsbereich
     * @param touchX X-Koordinate der Touch-Position
     * @param touchY Y-Koordinate der Touch-Position
     * @param touched true wenn Bildschirm berührt wird
     */
    void handleTouch(int touchX, int touchY, bool touched);

    /**
     * @brief Zeichnet die aktuelle Visualisierung
     * @param init true für Initialisierung, false für Update
     */
    void draw(bool init);

    /**
     * @brief Gibt den aktuellen Visualisierungs-Modus zurück
     * @return Aktueller VisMode
     */
    VisMode getMode() const { return m_visMode; }

    /**
     * @brief Setzt den Visualisierungs-Modus
     * @param mode Der zu setzende Modus
     */
    void setMode(VisMode mode);

    /**
     * @brief Gibt den aktuellen Spektrum-Style zurück
     * @return Aktueller SpectrumStyle
     */
    SpectrumStyle getSpectrumStyle() const { return m_spectrumStyle; }

    /**
     * @brief Gibt den aktuellen DigitalVU-Style zurück
     * @return Aktueller DigitalVUStyle
     */
    DigitalVUStyle getDigitalVUStyle() const { return m_digitalVUStyle; }

    /**
     * @brief Gibt den aktuellen Waveform-Style zurück
     * @return Aktueller WaveformStyle
     */
    WaveformStyle getWaveformStyle() const { return m_waveformStyle; }

    /**
     * @brief Gibt den aktuellen Goniometer-Style zurück
     * @return Aktueller GonioStyle
     */
    GonioStyle getGonioStyle() const { return m_gonioStyle; }

    /**
     * @brief Setzt das aktive Modul für modulspezifische Konfiguration
     * @param module Das aktive Modul (SD_PLAYER, RETRO_RADIO, etc.)
     *
     * Das Goniometer passt seine Größe und Position automatisch an das aktive Modul an.
     */
    void setActiveModule(AppModule module) { m_activeModule = module; }

private:
    // Konfigurationen für verschiedene Module
    static constexpr GonioConfig CONFIG_SDPLAYER = {
        341, // posX
        354, // posY
        126, // size
        63,  // center
        90,  // scale
        116, // corrBarY
        116, // corrBarW
        63,  // corrCenter
        55   // corrRange
    };

    static constexpr GonioConfig CONFIG_RETRORADIO = {
        341, // posX
        354, // posY
        126, // size
        63,  // center
        90,  // scale
        116, // corrBarY
        116, // corrBarW
        63,  // corrCenter
        55   // corrRange
    };

    // Grenzen für die Visualisierungen
    static constexpr float VU_DB_LOW = -60.0f; ///< Untere dB-Grenze der VU-Meter (Analog, Digital und Gonio)
    static constexpr float VU_DB_HIGH = 0.0f;  ///< Obere dB-Grenze der VU-Meter: Vollaussteuerung, denn höher geht ein Digitalsignal nicht
    static constexpr float VU_CALIBRATION_DB = 0.0f; ///< Aufschlag auf den gemessenen Pegel; null heißt, dass amplitudeToDb() echte dBFS liefert

    // Kennlinie des analogen VU-Meters, abgestimmt auf das Skalenblatt in images/retro_vu_meter.h
    static constexpr float VU_ZERO_SHARE = 0.71f;   ///< Anteil des Skalenweges, auf dem 0 VU liegt; so schreibt es die Norm vor
    static constexpr float VU_TAN_MAX = 0.831169f;  ///< Steigung des äußersten Strahls des Skalenblatts
    static constexpr float VU_MAX_ANGLE = 39.7323f; ///< Halber Schwenk des Zeigers in Grad, folgt aus VU_TAN_MAX


    // Bereich für Visualisierungen
    static constexpr int VIS_AREA_X = 355; ///< X-Koordinate des Visualisierungsbereichs
    static constexpr int VIS_AREA_Y = 354; ///< Y-Koordinate des Visualisierungsbereichs
    static constexpr int VIS_AREA_W = 445; ///< Breite des Visualisierungsbereichs
    static constexpr int VIS_AREA_H = 71;  ///< Höhe des Visualisierungsbereichs

    static constexpr int SPECTRUM_LABEL_H = 12; ///< Höhe der Frequenzzeile über Balken oder Wasserfall
    static constexpr uint32_t WATERFALL_FRAME_MS = 50; ///< Eine Wasserfallzeile je 50 ms

    static constexpr int AURORA_LAYERS = 3; ///< Anzahl der überlagerten Nordlicht-Schleier
    static constexpr int AURORA_GLOW_STEPS = 1024; ///< Auflösung des weichen Leuchtprofils

    // FFT-Parameter
    static constexpr int CENTERED_BAR_GAP = 4; ///< Abstand zwischen den mittigen Spektrumbalken
    static constexpr int CENTERED_BAND_COUNT = 36; ///< Anzahl der mittig wachsenden Spektrumbalken
    static constexpr int FFT_N = 1024;       ///< FFT-Größe (Anzahl Samples)
    static constexpr int BIN_WIDTH = 5;      ///< Breite eines Spektrum-Bins
    static constexpr int BIN_GAP = 2;        ///< Abstand zwischen Spektrum-Bins
    static constexpr int MAX_SAMPLES = 2048; ///< Maximale Anzahl von Audio-Samples

    LGFX *m_tft = nullptr;       ///< Zeiger auf das TFT-Display
    DS1307_RTC *m_rtc = nullptr; ///< Zeiger auf Echtzeituhr

    // Touch-Status für Visualisierungs-Bereich
    enum class VisTouchState
    {
        IDLE,           ///< Kein Touch aktiv
        TOUCH_STARTED,  ///< Touch begonnen, warte auf Geste
        LONG_PRESS_DONE ///< Langer Druck wurde ausgeführt
    };

    // Touch-Handling
    VisTouchState m_visTouchState = VisTouchState::IDLE; ///< Touch-Status
    bool m_visTouchSeen = false;                         ///< Berührungszustand des Vorframes, erkennt frische Berührungen
    int m_visTouchStartX = 0;                            ///< Start-X-Position der Visualisierungsbereich-Berührung
    int m_visTouchLastX = 0;                             ///< Letzte gültige X-Position (für Release-Erkennung)
    unsigned long m_visTouchStartTime = 0;               ///< Startzeit der Visualisierungsbereich-Berührung

    // Visualisierungs-Modi und Styles
    VisMode m_visMode = VisMode::VIS_ANALOG;                   ///< Aktueller Visualisierungsmodus
    SpectrumStyle m_spectrumStyle = SpectrumStyle::CLASSIC;    ///< Aktueller Style für die Spektrum-Anzeige
    SpectrumStyle m_centeredSpectrumStyle = SpectrumStyle::CLASSIC; ///< Unabhängige Farbwahl des mittigen Spektrums
    SpectrumStyle m_waterfallStyle = SpectrumStyle::RAINBOW; ///< Unabhängige Farbwahl des Wasserfalls
    AuroraStyle m_auroraStyle = AuroraStyle::COOL; ///< Gespeicherte Ziel-Farbwelt des Nordlichts
    DigitalVUStyle m_digitalVUStyle = DigitalVUStyle::CLASSIC; ///< Aktueller Style für die digitale VU-Anzeige
    WaveformStyle m_waveformStyle = WaveformStyle::OVERLAY;    ///< Aktueller Style für die Wellenform Anzeige
    GonioStyle m_gonioStyle = GonioStyle::CLASSIC;             ///< Aktueller Style für die Goniometer-Anzeige
    AppModule m_activeModule = AppModule::NONE;                ///< Aktuelles Modul für modulspezifische Konfiguration

    // Analoge VU-Meter
    bool m_needleInit = false;                                ///< Flag für Zeiger-Initialisierung
    float m_pastValL = -VU_MAX_ANGLE, m_pastValR = -VU_MAX_ANGLE;           ///< Aktueller Zeigerwinkel je Kanal
    float m_needleVelL = 0.0f, m_needleVelR = 0.0f;                         ///< Winkelgeschwindigkeit des Zeigers in Grad je Sekunde
    float m_lastRenderedL = -VU_MAX_ANGLE, m_lastRenderedR = -VU_MAX_ANGLE; ///< Zuletzt gerenderte Werte
    VuBallistics m_vuBallistics = VuBallistics::STANDARD;                   ///< Trägheit des Zeigers
    AnalogVUStyle m_analogVUStyle = AnalogVUStyle::CLASSIC;                 ///< Farbfassung des Skalenblatts
    uint8_t m_vuSensitivity = 2;                                            ///< Stufe der Empfindlichkeit, 0 bis 4
    unsigned long m_lastUpdate_analog = 0;                    ///< Zeitstempel des letzten analogen Updates

    // Digitale VU-Meter
    ChannelData m_rmsChannels[2] = {{VU_DB_LOW, 0, 0, 0}, {VU_DB_LOW, 0, 0, 0}}; ///< RMS-Kanaldaten
    ChannelData m_ppmChannels[2] = {{VU_DB_LOW, 0, 0, 0}, {VU_DB_LOW, 0, 0, 0}}; ///< PPM-Kanaldaten
    unsigned long m_lastUpdate_digital = 0;                              ///< Zeitstempel des letzten digitalen Updates

    // ===================================================================
    // Sample-Daten (Double-Buffer für Thread-Safety)
    // ===================================================================
    // Writer schreibt in Puffer [m_writeBufferIdx], dann atomarer Swap. Reader liest immer aus Puffer [m_writeBufferIdx ^ 1].
    static constexpr uint8_t SAMPLE_BUFFER_COUNT = 2; ///< Anzahl der Double-Buffer

    PsramUniquePtr<int16_t> m_samplesL[SAMPLE_BUFFER_COUNT]; ///< Double-Buffer für linke Samples [0] und [1]
    PsramUniquePtr<int16_t> m_samplesR[SAMPLE_BUFFER_COUNT]; ///< Double-Buffer für rechte Samples [0] und [1]
    uint16_t m_sampleCount[SAMPLE_BUFFER_COUNT] = {0, 0};    ///< Sample-Count pro Puffer (für jeden Puffer separat)
    uint32_t m_sampleRate[SAMPLE_BUFFER_COUNT] = {0, 0};    ///< Abtastrate in Hz, gemeinsam mit dem jeweiligen Sample-Puffer veröffentlicht

    std::atomic<uint8_t> m_writeBufferIdx{0};  ///< Aktueller Schreib-Puffer-Index (0 oder 1). Nach dem Schreiben wird geswappt.
    std::atomic<bool> m_readerActive{false};   ///< Flag: Reader ist aktiv (verhindert Swap während des Lesens)

    // Zaehlt jeden Pufferwechsel. Das Goniometer zeichnet seine Figur nur, wenn die Marke sich
    // seit dem letzten Bild geaendert hat: Der Zeichenweg kostet gut 7 ms, die Abtastwerte kommen
    // aber nur im Takt der Rahmen des Kodierers - bei FLAC rund 11-mal, bei MP3 rund 38-mal je
    // Sekunde. Ohne die Marke wuerde vier von fuenf Bildern dasselbe noch einmal gezeichnet.
    std::atomic<uint32_t> m_sampleSeq{0};      ///< Fortlaufende Marke der Abtastwert-Uebergaben
    uint32_t m_gonioDrawnSeq = 0;              ///< Marke, die das Goniometer zuletzt gezeichnet hat

    // Spektrum-Analysator
    PsramUniquePtr<SpectrumBand> m_spectrum_bands;          ///< Array der Spektrum-Bänder
    kiss_fftr_cfg m_fft_cfg = nullptr;                      ///< FFT-Konfiguration
    std::array<kiss_fft_scalar, FFT_N> m_fft_input{};       ///< FFT-Eingangsdaten (statisch, kein Heap)
    std::array<kiss_fft_cpx, FFT_N / 2 + 1> m_fft_output{}; ///< FFT-Ausgangsdaten (statisch, kein Heap)
    std::array<float, FFT_N> m_fft_window_lut{};            ///< Look-Up-Table für Hann-Fenster
    std::array<int, 64> m_band_bin_indices_lut{};           ///< Look-Up-Table für Band-Bin-Indizes
    std::array<float, CENTERED_BAND_COUNT + 1> m_centeredBandEdges{}; ///< Bandgrenzen innerhalb der FFT-Bins für anteilige Leistungszuordnung
    bool m_fftWindowInitialized = false;                   ///< Hann-Fenster wurde berechnet
    uint32_t m_spectrumSampleRate = 0;                     ///< Abtastrate der aktuellen Bandzuordnung in Hz
    uint32_t m_lastSpectrumUpdate = 0;                      ///< Zeitstempel des letzten Spektrum-Updates
    std::array<float, 64> m_spectrumTargets{};              ///< Normierte Zielhöhen der zuletzt analysierten Frequenzbänder
    std::array<uint16_t, CENTERED_BAND_COUNT> m_centeredSpectrumColors{}; ///< Vorberechnete Regenbogenfarben
    uint32_t m_spectrumAnalyzedSeq = 0;                     ///< Zuletzt analysierte Sample-Marke
    bool m_spectrumHadSamples = false;                     ///< Die letzte Analyse enthielt freigegebene Audiodaten
    bool m_centeredSpectrumDirty = true;                   ///< Erzwingt Neuzeichnen nach Initialisierung oder Farbwechsel

    /**
     * @brief Wiederverwendete Frequenzzuordnung, Farbskala und aktuelle Zeile des Wasserfalls
     */
    struct WaterfallData
    {
        std::array<float, VIS_AREA_W + 1> binEdges; ///< Logarithmische Pixelgrenzen innerhalb der FFT-Bins
        std::array<uint16_t, 256> colors; ///< Farbskala in der Byte-Reihenfolge des Zeichenpuffers
        std::array<uint8_t, VIS_AREA_W> targetLevels; ///< Pegelstufen des zuletzt ausgewerteten Spektrums
        std::array<uint8_t, VIS_AREA_W> displayLevels; ///< Zeitlich geglättete Pegelstufen der neuesten Zeile
        uint32_t sampleRate; ///< Abtastrate der Pixelzuordnung
        uint16_t activeRows; ///< Verbleibende Zeilen bis zum vollständig leeren Verlauf
    };
    std::unique_ptr<WaterfallData, PsramDeleter> m_waterfall; ///< Beim ersten Aufruf angelegte Wasserfalldaten im PSRAM

    /**
     * @brief Vorberechnete Leuchtprofile und langsame Bewegungszustände des Nordlichts
     */
    struct AuroraData
    {
        std::array<uint16_t, AURORA_GLOW_STEPS + 1> glow; ///< Gaußförmiges Leuchtprofil mit 15 Nachkommabits
        std::array<uint16_t, VIS_AREA_H> edgeFade; ///< Vertikaler Randverlauf von 0 bis 256
        float phase[AURORA_LAYERS][2]; ///< Unabhängige Phasen der beiden Wellen je Schleier
        float audioLevel; ///< Über mehrere Sekunden geglätteter Musikpegel von 0 bis 1
        float warmMix; ///< Überblendung von der kühlen zur warmen Farbwelt
        float fadeIn; ///< Sanftes Einblenden nach einem Moduswechsel
        uint32_t lastFrame; ///< Zeitstempel des zuletzt begonnenen Bildes
        float waveSin[AURORA_LAYERS][2]; ///< Sinuswerte an der nächsten zu berechnenden Spalte
        float waveCos[AURORA_LAYERS][2]; ///< Kosinuswerte an der nächsten zu berechnenden Spalte
        float stepSin[AURORA_LAYERS][2]; ///< Sinuswerte der räumlichen Phasenschritte
        float stepCos[AURORA_LAYERS][2]; ///< Kosinuswerte der räumlichen Phasenschritte
        float colors[AURORA_LAYERS][3]; ///< RGB-Koeffizienten des begonnenen Bildes
        int profileScale[AURORA_LAYERS]; ///< Skalierung der Leuchtprofile des begonnenen Bildes
        uint16_t nextColumn; ///< Nächste Spalte; VIS_AREA_W bedeutet ein vollständig berechnetes Bild
        bool rendering; ///< Ein begonnenes Bild wird berechnet oder wartet auf seine Ausgabe
    };
    std::unique_ptr<AuroraData, PsramDeleter> m_aurora; ///< Einmalig angelegte Nordlicht-Daten im PSRAM

    // Wellenform-Anzeige
    uint32_t m_lastWaveUpdateMs = 0;                          ///< Zeitstempel des letzten Wave-Updates in ms
    float m_waveAmp = 1.0f;                                   ///< Auto-Gain Verstärkungsfaktor
    static constexpr uint8_t WAVE_PEAK_HISTORY_SIZE = 90;     ///< Größe des Peak-History-Buffers (~3 Sekunden bei 30fps)
    float m_wavePeakHistory[WAVE_PEAK_HISTORY_SIZE] = {0.0f}; ///< Ringpuffer für Peak-Werte
    uint8_t m_wavePeakHistoryIdx = 0;                         ///< Aktueller Index im Ringpuffer
    float m_waveSlidingMax = 0.0f;                            ///< Gleitendes Maximum über Peak-History
    uint32_t m_waveLastPeakUpdate = 0;                        ///< Zeitstempel des letzten Peak-Updates

    // Goniometer-Anzeige
    float m_gonioAmp = 1.0f;                                    ///< Auto-Gain Verstärkungsfaktor
    float m_gonioCorrPrev = 0.0f;                               ///< Vorheriger Korrelationswert (für Ballistik)
    static constexpr uint8_t GONIO_PEAK_HISTORY_SIZE = 90;      ///< Größe des Peak-History-Buffers (~3 Sekunden bei 30fps)
    float m_gonioPeakHistory[GONIO_PEAK_HISTORY_SIZE] = {0.0f}; ///< Ringpuffer für Peak-Werte
    uint8_t m_gonioPeakHistoryIdx = 0;                          ///< Aktueller Index im Ringpuffer
    float m_gonioSlidingMax = 0.0f;                             ///< Gleitendes Maximum über Peak-History
    uint32_t m_gonioLastPeakUpdate = 0;                         ///< Zeitstempel des letzten Peak-Updates
    uint32_t m_gonioLastDynamicsUpdate = 0;                     ///< Zeitstempel der letzten Dynamik-Aktualisierung

    // Goniometer VU-Meter (zeitbasierte Ballistik)
    static constexpr float GONIO_VU_FALL_RATE = 50.0f;    ///< VU-Abfallgeschwindigkeit in Pixel/Sekunde
    static constexpr float GONIO_VU_RISE_RATE = 2000.0f;  ///< VU-Anstiegsgeschwindigkeit in Pixel/Sekunde
    static constexpr uint32_t GONIO_PEAK_HOLD_TIME = 800; ///< Peak-Haltezeit in Millisekunden
    static constexpr float GONIO_PEAK_FALL_RATE = 80.0f;  ///< Peak-Abfallgeschwindigkeit in Pixel/Sekunde

    uint32_t m_lastGonioVUUpdate = 0; ///< Zeitstempel des letzten GonioVU-Updates
    float m_gonioVUValL = 0.0f;       ///< Aktueller VU-Wert links (float für smooth interpolation)
    float m_gonioVUValR = 0.0f;       ///< Aktueller VU-Wert rechts (float für smooth interpolation)
    float m_gonioVUPeakL = 0.0f;      ///< Aktueller Peak-Wert links
    float m_gonioVUPeakR = 0.0f;      ///< Aktueller Peak-Wert rechts
    uint32_t m_gonioVUPeakTimeL = 0;  ///< Zeitstempel des letzten Peak-Updates links
    uint32_t m_gonioVUPeakTimeR = 0;  ///< Zeitstempel des letzten Peak-Updates rechts
    uint8_t m_lastDrawnValL = 0;      ///< Zuletzt gezeichneter VU-Wert links (Dirty-Flag)
    uint8_t m_lastDrawnValR = 0;      ///< Zuletzt gezeichneter VU-Wert rechts (Dirty-Flag)
    uint8_t m_lastDrawnPeakL = 0;     ///< Zuletzt gezeichneter Peak-Wert links (Dirty-Flag)
    uint8_t m_lastDrawnPeakR = 0;     ///< Zuletzt gezeichneter Peak-Wert rechts (Dirty-Flag)

    // Sprites für die Visualisierungen
    Gfx_Sprite *m_sprVU_Analog_Needle = nullptr;          ///< Sprite für analoge VU-Zeiger
    // Je ein Sprite genügt. Ein zweiter Puffer lohnt nur, solange eine Übertragung noch läuft;
    // beim RGB-Panel liegt der Bildspeicher aber im PSRAM, und pushSprite ist ein Kopiervorgang.
    Gfx_Sprite *m_sprVU_Analog_L = nullptr; ///< Sprite für das analoge VU links
    Gfx_Sprite *m_sprVU_Analog_R = nullptr; ///< Sprite für das analoge VU rechts
    Gfx_Sprite *m_sprVU_Digital_RMS_L = nullptr;          ///< Sprite für digitale RMS links
    Gfx_Sprite *m_sprVU_Digital_RMS_R = nullptr;          ///< Sprite für digitale RMS rechts
    Gfx_Sprite *m_sprVU_Digital_PPM_L = nullptr;          ///< Sprite für digitale PPM links
    Gfx_Sprite *m_sprVU_Digital_PPM_R = nullptr;          ///< Sprite für digitale PPM rechts
    Gfx_Sprite *m_sprVU_Spectrum = nullptr; ///< Sprite für die Spektrum-Anzeige
    Gfx_Sprite *m_sprAurora = nullptr; ///< Wiederverwendeter Zeichenpuffer für das Nordlicht
    Gfx_Sprite *m_sprVU_Wave = nullptr;     ///< Sprite für die Wellenform-Anzeige
    Gfx_Sprite *m_sprGonio = nullptr;       ///< Sprite für die Goniometer-Anzeige

    // Kreuz, Teilbalken und Beschriftungen des Goniometers ändern sich nie. Gemessen kostet
    // ihr Neuaufbau je Bild rund 5 ms, das Kopieren des fertigen Untergrunds nur 2,5 ms.
    // Der Kopiervorgang muss dafuer ein rohes memcpy sein: pushSprite von Sprite zu Sprite
    // geht durch die Farbumsetzung von LovyanGFX und war in der Messung sogar teurer als
    // das Neuzeichnen. Der Untergrund belegt 126*126*2 Byte, also gut 31 KB im PSRAM.
    Gfx_Sprite *m_sprGonioBg = nullptr;     ///< Fertiger Untergrund des Goniometers
    Gfx_Sprite *m_sprGonioVULeft = nullptr;               ///< Sprite für Goniometer VU links
    Gfx_Sprite *m_sprGonioVUMiddle = nullptr;             ///< Sprite für Goniometer VU Mitte
    Gfx_Sprite *m_sprGonioVURight = nullptr;              ///< Sprite für Goniometer VU rechts
    Gfx_Sprite *m_sprGonioInfo = nullptr;                 ///< Sprite für Goniometer Info-Anzeige

    // Schwellen der beiden Spitzenwert-Lampen, linear gerechnet. Gemessen wird der Programmpegel,
    // und Rundfunkstroeme sind bis dicht unter die Vollaussteuerung begrenzt; bei den frueheren
    // -12 und -6 dBFS leuchteten beide Lampen praktisch dauernd und sagten damit nichts mehr.
    static constexpr float GONIO_INFO_THR1_LINEAR = 0.70794578f; ///< Schwelle 1, entsprechend -3 dBFS
    static constexpr float GONIO_INFO_THR2_LINEAR = 0.94406088f; ///< Schwelle 2, entsprechend -0,5 dBFS

    // Farben
    static constexpr uint16_t GONIO_INFO_COLOR_STANDBY = 0x2104;     ///< Standby-Farbe der Quadrate
    static constexpr uint16_t GONIO_INFO_COLOR_THR1_ON = TFT_ORANGE; ///< Farbe bei Schwelle 1
    static constexpr uint16_t GONIO_INFO_COLOR_THR2_ON = TFT_RED;    ///< Farbe bei Schwelle 2

    // Layout
    static constexpr int SQW = 30;    ///< Breite eines Peak-Quadrats in Pixeln
    static constexpr int SQH = 15;    ///< Höhe eines Peak-Quadrats in Pixeln
    static constexpr int GAP = 10;    ///< Abstand zwischen den Quadraten in Pixeln
    static constexpr int baseY = 50;  ///< Y-Startposition der Quadrate (relativ zum Sprite)
    static constexpr int baseXL = 11; ///< X-Startposition der linken Quadratgruppe (relativ zum Sprite)
    static constexpr int baseXR = 121; ///< X-Startposition der rechten Quadratgruppe (relativ zum Sprite)

    static constexpr uint32_t GONIO_INFO_HOLD_MS = 1200; ///< Haltezeit eines getriggerten Quadrats in Millisekunden

    // Trigger-Haltezeiten je Quadrat (L1/L2/R1/R2)
    uint32_t m_infoPeakHoldL1 = 0; ///< Zeitstempel des letzten L1-Triggers (millis)
    uint32_t m_infoPeakHoldL2 = 0; ///< Zeitstempel des letzten L2-Triggers (millis)
    uint32_t m_infoPeakHoldR1 = 0; ///< Zeitstempel des letzten R1-Triggers (millis)
    uint32_t m_infoPeakHoldR2 = 0; ///< Zeitstempel des letzten R2-Triggers (millis)

    // Goniometer Info Cache
    float m_lastDrawnGain = -999.0f; ///< Zuletzt gezeichneter Gain-Faktor

    // Cache für Quadrat-Zustände
    bool m_lastSquareStateL1 = false; ///< Letzter Zustand des L1-Quadrats
    bool m_lastSquareStateL2 = false; ///< Letzter Zustand des L2-Quadrats
    bool m_lastSquareStateR1 = false; ///< Letzter Zustand des R1-Quadrats
    bool m_lastSquareStateR2 = false; ///< Letzter Zustand des R2-Quadrats
    int16_t m_gainValueX = 120;       ///< X-Position des Gain-Werts im Info-Sprite, nach drawGonio()-Init dynamisch an Label-Breite angepasst

    // Audio-Werte
    float m_currentRMSLeft = 0.0f;                                  ///< Aktueller RMS-Wert links
    float m_currentRMSRight = 0.0f;                                 ///< Aktueller RMS-Wert rechts
    float m_currentPeakLeft = 0.0f;                                 ///< Aktueller Peak-Wert links
    float m_currentPeakRight = 0.0f;                                ///< Aktueller Peak-Wert rechts
    mutable portMUX_TYPE m_audioMux = portMUX_INITIALIZER_UNLOCKED; ///< Mutex für Audio-Werte (Dual-Core-Safe)

    /**
     * @brief Wechselt den Visualisierungs-Modus
     * @param direction Richtung des Wechsels (1 = vorwärts, -1 = rückwärts)
     */
    void switchVisMode(int direction);

    /**
     * @brief Schaltet den Style der aktuellen Visualisierung um
     */
    void toggleVisSubMode();

    /**
     * @brief Initialisiert den gewählten Visualisierungs-Modus
     * @param mode Der zu initialisierende Visualisierungsmodus
     */
    void initVisMode(VisMode mode);

    /**
     * @brief Löscht den gesamten Visualisierungsbereich
     *
     * Löscht den größtmöglichen Bereich, den ein Visualisierungsmodul belegen kann. Dies umfasst:
     * - Den Hauptvisualisierungsbereich (VIS_AREA)
     * - Den Goniometer-Bereich inkl. VU-Meter
     *
     * Diese Methode wird beim Wechsel zwischen Visualisierungsmodi aufgerufen, um Reste des vorherigen Modus zu entfernen.
     */
    void clearVisualizationArea();

    /**
     * @brief Zeichnet die Visualisierung basierend auf aktuellem Modus
     * @param init true für Initialisierung, false für Update
     * @param rmsLeft RMS-Wert für linken Kanal
     * @param rmsRight RMS-Wert für rechten Kanal
     * @param peakLeft Peak-Wert für linken Kanal
     * @param peakRight Peak-Wert für rechten Kanal
     */
    void drawVis(bool init, float rmsLeft, float rmsRight, float peakLeft, float peakRight);

    /**
     * @brief Räumt die Sprites des aktuellen Visualisierungs-Modus auf
     */
    void cleanupCurrentVisSprites();

    /**
     * @brief Rechnet einen linearen Amplitudenwert in einen kalibrierten VU-Pegel um
     * @param amplitude Linearer Pegel, 1,0 entspricht Vollausschlag (0 dBFS)
     * @return dB-Wert, begrenzt auf [VU_DB_LOW, VU_DB_HIGH]
     *
     * Zentrale Umrechnung für Analog-VU, Digital-VU und Gonio-VU. Begrenzt das Ergebnis auf den sichtbaren VU-Bereich. Bei Stille (amplitude nahe 0) wird VU_DB_LOW geliefert.
     */
    float amplitudeToDb(float amplitude) const;

    /**
     * @brief Liefert die eingestellte Trägheit des analogen VU-Zeigers
     * @return Aktuelle VuBallistics
     */
    VuBallistics getVuBallistics() const { return m_vuBallistics; }

    /**
     * @brief Rechnet einen VU-Pegel in den Zeigerwinkel des analogen Instruments um
     * @param db Pegel in dB, 0 dB entspricht 0 VU
     * @return Winkel in Grad, 0 zeigt senkrecht nach oben
     *
     * Ein VU-Instrument schlägt proportional zur Spannung aus, nicht zum Pegel in Dezibel. Der Weg von links ist deshalb VU_ZERO_SHARE mal zehn hoch dB durch zwanzig; daraus folgt der Winkel über den Arkustangens. Die frühere Rechnung war linear in Dezibel und ließ den Zeiger an keiner Stelle auf seinen Teilstrich zeigen.
     *
     * Welcher Pegel 0 VU entspricht, hängt von der eingestellten Empfindlichkeit ab. Eine VU-Skala umfasst nur 23 dB, der Lautstärkeregler dagegen 60 dB; die Stufe verschiebt das Fenster dorthin, wo tatsächlich gehört wird.
     */
    float dbToNeedleAngle(float db) const;

    /**
     * @brief Liefert den eingestellten Bezugspegel in dBFS
     * @return Pegel, bei dem der Zeiger 0 VU erreicht
     *
     * Denselben Wert benutzt die Spektrumanzeige, damit ihr Fenster zum selben Programm passt.
     */
    float vuAlignmentDb() const;

    /**
     * @brief Liefert das Skalenblatt der eingestellten Farbfassung
     * @return Zeiger auf die RGB565-Tabelle mit 218 mal 70 Bildpunkten
     */
    const uint16_t *analogVUPlate() const;

    /**
     * @brief Zeichnet das analoge VU-Meter
     * @param init true für Initialisierung, false für Update
     * @param rmsLeft RMS-Wert für linken Kanal
     * @param rmsRight RMS-Wert für rechten Kanal
     */
    void drawAnalogVU(bool init, float rmsLeft, float rmsRight);

    /**
     * @brief Zeichnet das digitale VU-Meter
     * @param init true für Initialisierung, false für Update
     * @param rmsLeft RMS-Wert für linken Kanal
     * @param rmsRight RMS-Wert für rechten Kanal
     * @param peakLeft Peak-Wert für linken Kanal
     * @param peakRight Peak-Wert für rechten Kanal
     */
    void drawDigitalVU(bool init, float rmsLeft, float rmsRight, float peakLeft, float peakRight);

    /**
     * @brief Zeichnet je nach Modus das klassische Spektrum, mittige Balken oder den Wasserfall
     * @param init true für Initialisierung, false für Update
     */
    void drawSpectrum(bool init);

    /**
     * @brief Beschriftet die Frequenzachse des aktiven Spektrums
     * @param cutoffFreqs Bandgrenzen in Hz, mit bandCount + 1 Einträgen
     * @param bandCount Anzahl der Bänder
     */
    void drawSpectrumLabels(const float *cutoffFreqs, int bandCount);

    /**
     * @brief Aktualisiert den Wasserfall im gemeinsamen Spektrum-Zeichenpuffer
     * @param init true zum Zurücksetzen des Verlaufs und Vorbereiten der Farbskala
     * @param analyze true wenn neue FFT-Werte oder ein geänderter Wiedergabezustand vorliegen
     * @param hasSamples true wenn die FFT gültige Audiodaten enthält
     * @param elapsed Vergangene Zeit seit der letzten Wasserfallzeile in Millisekunden
     */
    void drawWaterfall(bool init, bool analyze, bool hasSamples, uint32_t elapsed);

    /**
     * @brief Zeichnet langsam bewegte Nordlicht-Schleier mit sanfter Audioreaktion
     * @param init true zum Vorbereiten des Zeichenpuffers und Neustarten der Einblendung
     * @param rmsLeft Linearer Effektivwert des linken Audiokanals
     * @param rmsRight Linearer Effektivwert des rechten Audiokanals
     */
    void drawAurora(bool init, float rmsLeft, float rmsRight);

    /**
     * @brief Zeichnet die Wellenform Anzeige
     * @param init true für Initialisierung, false für Update
     */
    void drawWave(bool init);

    /**
     * @brief Zeichnet die Goniometer Anzeige
     * @param init true für Initialisierung, false für Update
     * @param peakLeft Peak-Wert für linken Kanal (0..1)
     * @param peakRight Peak-Wert für rechten Kanal (0..1)
     */
    void drawGonio(bool init, float peakLeft, float peakRight);

    /**
     * @brief Zeichnet die Goniometer VU-Anzeige
     * @param init true für Initialisierung, false für Update
     * @param peakLeft Peak-Wert für linken Kanal (0..1)
     * @param peakRight Peak-Wert für rechten Kanal (0..1)
     */
    void drawGonioVU(bool init, float rmsLeft, float rmsRight);

    /**
     * @brief Goniometer Info-Anzeige
     *  - vier Peak-Quadrate (L: [Thr1][Thr2], R: [Thr1][Thr2]) mit Hold
     *  - aktuellen Auto-Gain-Faktor
     *
     * @param gainFactor  aktueller Auto-Gain-Faktor
     * @param peakL       Peak linear (0..1) links
     * @param peakR       Peak linear (0..1) rechts
     */
    void drawGonioInfo(float gainFactor, float peakL, float peakR);

    /**
     * @brief Zeichnet ein Quadrat für die Goniometer Info-Anzeige
     * @param x X-Position des Quadrats
     * @param y Y-Position des Quadrats
     * @param width Breite des Quadrats
     * @param height Höhe des Quadrats
     * @param colorFill Füllfarbe des Quadrats
     */
    void drawGonioSquare(int x, int y, int width, int height, uint16_t colorFill);

    /**
     * @brief Zeichnet die statischen Elemente des Goniometers
     * @param spr Zeiger auf das Sprite, in das gezeichnet werden soll
     * @param cfg Konfiguration des Goniometers (Position, Größe, etc.)
     *
     * Zeichnet alle statischen Elemente des Goniometers wie Kreuz, Balken, Beschriftungen und Korrelationsanzeige. Wird sowohl bei der Initialisierung als auch bei jedem Update aufgerufen.
     */
    void drawGonioStaticElements(Gfx_Sprite *spr, const GonioConfig &cfg);
};

#endif // VISUALIZER_H

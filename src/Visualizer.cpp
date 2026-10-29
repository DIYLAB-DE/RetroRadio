#include "Visualizer.h"

#include <cmath>
#include <cstring>

#include "StringUtils.h"
#include "Logger.h"
#include "Theme.h"
#include "Tools.h"

#include "I18n.h"
#include "Strings_retroradio.h"

// Fonts
#include "fonts/ComfortaaBold10.h"

// Icons & Bilder
#include "images/retro_vu_meter.h"
#include "images/retro_vu_meter_green.h"

// ===================================================================
// Hilfsfunktionen
// ===================================================================

namespace
{
    constexpr float VU_RAD_TO_DEG = 57.29578f; ///< Umrechnung von Radiant in Grad

    // Lage und Länge der Nadel, wie sie das Sprite und sein Drehpunkt vorgeben. Daraus folgt,
    // welchen Bereich sie in einer Bildzeile bedeckt.
    constexpr int VU_PLATE_W = 218;               ///< Breite des Skalenblatts
    constexpr int VU_PLATE_H = 70;                ///< Höhe des Skalenblatts
    constexpr float VU_PIVOT_X = 109.0f;          ///< Drehpunkt der Nadel im Blatt
    constexpr float VU_PIVOT_Y = 120.0f;          ///< Der Drehpunkt liegt unter dem Blatt
    constexpr float VU_NEEDLE_INNER_R = 10.0f;    ///< Innerer Radius der Nadel
    constexpr float VU_NEEDLE_OUTER_R = 130.0f;   ///< Äußerer Radius der Nadel
    constexpr float VU_NEEDLE_HALF_W = 3.5f;      ///< Halbe Breite samt weicher Kante, großzügig

    // Der Radius wird an der Mittellinie gemessen; die Ecken des Balkens reichen darueber hinaus.
    // Ohne diesen Zuschlag blieben an Spitze und Fuss ein bis zwei Zeilen unaufgeraeumt stehen.
    constexpr float VU_NEEDLE_R_MARGIN = 5.0f;

    // Die Spannen liegen fest im Speicher statt auf dem Stapel: sie werden nur nacheinander
    // gebraucht, und der Zeichenweg läuft ausschließlich im Task der Anzeige.
    int16_t g_vuSpanAltVon[VU_PLATE_H], g_vuSpanAltBis[VU_PLATE_H];
    int16_t g_vuSpanNeuVon[VU_PLATE_H], g_vuSpanNeuBis[VU_PLATE_H];

    /**
     * @brief Ermittelt je Bildzeile, welchen Bereich die Nadel bei diesem Winkel bedeckt.
     *
     * Liegt in einer Zeile keine Nadel, weil sie dort ihren Radius verlassen hat, wird die
     * Spanne leer gemeldet: bis liegt dann vor von.
     */
    void vuNeedleSpans(float angleDeg, int16_t *von, int16_t *bis)
    {
        const float rad = angleDeg / VU_RAD_TO_DEG;
        const float sinus = sinf(rad);
        const float kosinus = cosf(rad);

        for (int y = 0; y < VU_PLATE_H; ++y)
        {
            const float dy = VU_PIVOT_Y - static_cast<float>(y);
            const float radius = dy / kosinus;

            if (radius < VU_NEEDLE_INNER_R - VU_NEEDLE_R_MARGIN ||
                radius > VU_NEEDLE_OUTER_R + VU_NEEDLE_R_MARGIN)
            {
                von[y] = 1;
                bis[y] = 0;
                continue;
            }

            const float mitte = VU_PIVOT_X + dy * (sinus / kosinus);
            const float halb = VU_NEEDLE_HALF_W / kosinus + 1.0f;

            int a = static_cast<int>(floorf(mitte - halb));
            int b = static_cast<int>(ceilf(mitte + halb));

            if (a < 0)
                a = 0;

            if (b > VU_PLATE_W - 1)
                b = VU_PLATE_W - 1;

            von[y] = static_cast<int16_t>(a);
            bis[y] = static_cast<int16_t>(b);
        }
    }

    // Zeigerfarben der beiden Fassungen, passend zum jeweiligen Skalenblatt. Auf dem grünen
    // Blatt steht der Zeiger in Bernstein: Ein Zeiger im Farbton seiner Skala tritt nicht hervor,
    // sondern konkurriert mit ihr. Warm gegen kühl trennt Zeiger und Skala am deutlichsten.
    constexpr uint16_t VU_NEEDLE_CLASSIC = 0xF800; ///< Rot, zum klassischen Blatt
    constexpr uint16_t VU_NEEDLE_GREEN = 0xFD87;   ///< Bernstein, zum grünen Blatt

    // Fenster der Spektrumanzeige, gerechnet und nicht geraten: Bei Hann-Fenster und 1024
    // Punkten liegt die Summe aller Bänder rund 53 dB über dem Effektivwert in dBFS, und das
    // stärkste Einzelband einige Dezibel darunter. Die Oberkante liegt deshalb 50 dB über dem
    // Bezugspegel des VU-Meters; damit passen Spektrum und Zeiger zum selben Programm.
    constexpr float SPECTRUM_TOP_OVER_REFERENCE = 50.0f;
    constexpr float SPECTRUM_SPAN_DB = 36.0f;
    constexpr float SPECTRUM_MIN_HZ = 86.0f; ///< Untere Frequenzgrenze aller Spektren
    constexpr float SPECTRUM_MAX_HZ = 12987.0f; ///< Obere Frequenzgrenze aller Spektren
    constexpr float SPECTRUM_SCALE_FACTOR = 32.66f / 32767.0f; ///< Pegelskalierung der FFT-Eingangswerte

    // Pegel in dBFS, der 0 VU entspricht, je Empfindlichkeitsstufe. Kleinere Werte lassen den
    // Zeiger früher ausschlagen; die Zahlen stehen so auch auf der Einstellseite.
    //
    // Der Bezugspegel der EBU-Empfehlung R68 liegt bei -18 dBFS und steht hier als unterste
    // Stufe. Er gilt für Studiomaterial. Rundfunkströme sind durchweg kräftig bearbeitet und
    // liefern eher -12 bis -6 dBFS; deshalb reicht die Reihe bis -2 hinauf.
    constexpr float VU_ALIGNMENT_DB[] = {-2.0f, -6.0f, -10.0f, -14.0f, -18.0f};

    // Zeit, in der der Zeiger 99 Prozent eines Sprungs zurückgelegt hat, in Sekunden.
    // Die Norm verlangt 300 ms; die Reihenfolge folgt der Aufzählung VuBallistics.
    constexpr float VU_RESPONSE_TIME[] = {0.15f, 0.30f, 0.60f};
    static_assert(sizeof(VU_RESPONSE_TIME) / sizeof(VU_RESPONSE_TIME[0]) == static_cast<size_t>(VuBallistics::COUNT),
                  "Zu jeder Trägheit gehört genau eine Einschwingzeit");

    // Ein Drehspulinstrument ist eine Masse an einer Feder mit Dämpfung, kein Tiefpass erster
    // Ordnung. Der Unterschied ist deutlich sichtbar: Ein Tiefpass springt im ersten Augenblick
    // mit voller Geschwindigkeit los, ein Zeiger muss erst beschleunigen. Beide erreichen 99
    // Prozent nach derselben Zeit, aber der Tiefpass wirkt dabei viel flinker.
    //
    // Die Norm verlangt 1 bis 1,5 Prozent Überschwingen. Daraus folgt der Dämpfungsgrad 0,81,
    // und aus der Sprungantwort folgt, dass die Kreisfrequenz mal der Einschwingzeit 4,03 ergibt.
    constexpr float VU_DAMPING = 0.81f;
    constexpr float VU_OMEGA_TIMES_TIME = 4.03f;

    // Die Bewegungsgleichung wird schrittweise integriert. Zu große Schritte machen die Rechnung
    // unruhig, deshalb wird ein langes Bildintervall in mehrere kleine zerlegt.
    constexpr float VU_MAX_STEP = 0.005f;
    constexpr int VU_MAX_STEPS = 32;

    constexpr float GONIO_RAINBOW_DISTANCE_SPLIT_SQ = 0.0625f;
    constexpr float GONIO_RAINBOW_DISTANCE_MAX_SQ = 1.44f;
    constexpr uint16_t GONIO_RAINBOW_NEAR_LAST_INDEX = 127;
    constexpr uint16_t GONIO_RAINBOW_FAR_FIRST_INDEX = 128;
    constexpr float GONIO_RAINBOW_NEAR_INDEX_SCALE =
        static_cast<float>(GONIO_RAINBOW_NEAR_LAST_INDEX) / GONIO_RAINBOW_DISTANCE_SPLIT_SQ;
    constexpr float GONIO_RAINBOW_FAR_INDEX_SCALE =
        static_cast<float>(GONIO_RAINBOW_NEAR_LAST_INDEX) / (GONIO_RAINBOW_DISTANCE_MAX_SQ - GONIO_RAINBOW_DISTANCE_SPLIT_SQ);
    constexpr uint8_t GONIO_RAINBOW_COLOR_INDEX_LUT[256] = {
        96, 94, 93, 92, 92, 92, 91, 91, 90, 90, 90, 90, 89, 89, 89, 89,
        88, 88, 88, 88, 88, 87, 87, 87, 87, 87, 86, 86, 86, 86, 86, 86,
        85, 85, 85, 85, 85, 85, 85, 84, 84, 84, 84, 84, 84, 84, 83, 83,
        83, 83, 83, 83, 83, 83, 82, 82, 82, 82, 82, 82, 82, 82, 82, 81,
        81, 81, 81, 81, 81, 81, 81, 81, 80, 80, 80, 80, 80, 80, 80, 80,
        80, 80, 79, 79, 79, 79, 79, 79, 79, 79, 79, 79, 78, 78, 78, 78,
        78, 78, 78, 78, 78, 78, 78, 77, 77, 77, 77, 77, 77, 77, 77, 77,
        77, 77, 77, 76, 76, 76, 76, 76, 76, 76, 76, 76, 76, 76, 76, 76,
        76, 74, 72, 71, 69, 68, 67, 66, 65, 63, 62, 61, 60, 59, 58, 58,
        57, 56, 55, 54, 53, 52, 52, 51, 50, 49, 49, 48, 47, 46, 46, 45,
        44, 44, 43, 42, 42, 41, 40, 40, 39, 39, 38, 37, 37, 36, 36, 35,
        34, 34, 33, 33, 32, 32, 31, 31, 30, 29, 29, 28, 28, 27, 27, 26,
        26, 25, 25, 24, 24, 23, 23, 23, 22, 22, 21, 21, 20, 20, 19, 19,
        18, 18, 17, 17, 17, 16, 16, 15, 15, 14, 14, 14, 13, 13, 12, 12,
        11, 11, 11, 10, 10, 9, 9, 9, 8, 8, 7, 7, 7, 6, 6, 5,
        5, 5, 4, 4, 4, 3, 3, 2, 2, 2, 1, 1, 1, 0, 0, 0};

    /**
     * @brief Ermittelt den Rainbow-Farbindex aus dem quadrierten Goniometer-Abstand.
     */
    uint8_t gonioRainbowColorIndex(float distanceSquared)
    {
        if (distanceSquared >= GONIO_RAINBOW_DISTANCE_MAX_SQ)
            return 0;

        uint16_t lutIndex;
        if (distanceSquared < GONIO_RAINBOW_DISTANCE_SPLIT_SQ)
        {
            lutIndex = static_cast<uint16_t>(distanceSquared * GONIO_RAINBOW_NEAR_INDEX_SCALE + 0.5f);
        }
        else
        {
            lutIndex = GONIO_RAINBOW_FAR_FIRST_INDEX +
                       static_cast<uint16_t>((distanceSquared - GONIO_RAINBOW_DISTANCE_SPLIT_SQ) * GONIO_RAINBOW_FAR_INDEX_SCALE + 0.5f);
        }

        return GONIO_RAINBOW_COLOR_INDEX_LUT[lutIndex];
    }
}

/**
 * @brief Byte-Swap für 16-Bit Farben (RGB565 Big-Endian <-> Little-Endian)
 */
inline uint16_t bswap16(uint16_t color)
{
    return (color >> 8) | (color << 8);
}

/**
 * @brief RAII-Guard für Reader-Flag (Race-Protection Double-Buffer)
 *
 * Setzt m_readerActive auf true bei Konstruktion und automatisch auf false bei Destruktion (auch bei frühem return).
 *
 * Verwendet memory_order_release um sicherzustellen, dass der Writer auf dem anderen Core den Status zuverlässig sieht (ESP32 Dual-Core).
 */
class ReaderGuard
{
public:
    /**
     * @brief Markiert den zugehörigen Lesepfad für die Lebensdauer des Guards als aktiv.
     */
    explicit ReaderGuard(std::atomic<bool> &flag) : m_flag(flag)
    {
        m_flag.store(true, std::memory_order_release);
    }
    /**
     * @brief Hebt die Markierung des Lesepfads wieder auf.
     */
    ~ReaderGuard()
    {
        m_flag.store(false, std::memory_order_release);
    }
    // Nicht kopierbar
    ReaderGuard(const ReaderGuard &) = delete;
    ReaderGuard &operator=(const ReaderGuard &) = delete;

private:
    std::atomic<bool> &m_flag;
};

// ===================================================================
// Konstruktor, Destruktor, Init
// ===================================================================

/**
 * @brief Konstruktor - allokiert Double-Buffer und Spektrum-Bänder in PSRAM
 */
Visualizer::Visualizer()
{
    // Double-Buffer für Sample-Arrays in PSRAM allokieren
    for (uint8_t i = 0; i < SAMPLE_BUFFER_COUNT; ++i)
    {
        m_samplesL[i].reset(static_cast<int16_t *>(heap_caps_malloc(MAX_SAMPLES * sizeof(int16_t), MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT)));
        m_samplesR[i].reset(static_cast<int16_t *>(heap_caps_malloc(MAX_SAMPLES * sizeof(int16_t), MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT)));

        if (m_samplesL[i] && m_samplesR[i])
        {
            memset(m_samplesL[i].get(), 0, MAX_SAMPLES * sizeof(int16_t));
            memset(m_samplesR[i].get(), 0, MAX_SAMPLES * sizeof(int16_t));
        }

        m_sampleCount[i] = 0;
    }

    // Spektrum-Bänder in PSRAM allokieren
    m_spectrum_bands.reset(static_cast<SpectrumBand *>(heap_caps_malloc(64 * sizeof(SpectrumBand), MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT)));

    if (m_spectrum_bands)
    {
        for (int i = 0; i < 64; i++)
        {
            m_spectrum_bands[i] = {0.0f, 0.0f, 0, 0, 0};
        }
    }
}

/**
 * @brief Destruktor
 */
Visualizer::~Visualizer()
{
    // Visualisierungs-Sprites aufräumen
    cleanupCurrentVisSprites();

    // FFT-Konfiguration freigeben
    if (m_fft_cfg)
    {
        kiss_fftr_free(m_fft_cfg);
        m_fft_cfg = nullptr;
    }
}

/**
 * @brief Räumt alle allokierten Ressourcen auf
 */
void Visualizer::cleanup()
{
    LOG_INFO_F("Visualizer::cleanup() - Modus war: %d", static_cast<int>(m_visMode));
    cleanupCurrentVisSprites();

    if (m_fft_cfg)
    {
        kiss_fftr_free(m_fft_cfg);
        m_fft_cfg = nullptr;
    }

    m_fftWindowInitialized = false;
    m_spectrumSampleRate = 0;
    m_waterfall.reset();
    m_aurora.reset();
}

/**
 * @brief Initialisierung
 */
bool Visualizer::init(LGFX &tft, DS1307_RTC &rtc)
{
    m_tft = &tft;
    m_rtc = &rtc;

    // Touch-Handling initialisieren
    m_visTouchState = VisTouchState::IDLE;
    m_visTouchStartX = 0;
    m_visTouchLastX = 0;
    m_visTouchStartTime = 0;

    // Visualisierungsmodus aus AppState laden
    m_visMode = static_cast<VisMode>(APP_STATE.getVisMode());
    m_spectrumStyle = static_cast<SpectrumStyle>(APP_STATE.getSpectrumStyle());
    m_centeredSpectrumStyle = static_cast<SpectrumStyle>(APP_STATE.getCenteredSpectrumStyle());
    m_waterfallStyle = static_cast<SpectrumStyle>(APP_STATE.getWaterfallStyle());
    m_auroraStyle = static_cast<AuroraStyle>(APP_STATE.getAuroraStyle());
    m_digitalVUStyle = static_cast<DigitalVUStyle>(APP_STATE.getDigitalVUStyle());
    m_waveformStyle = static_cast<WaveformStyle>(APP_STATE.getWaveformStyle());
    m_gonioStyle = static_cast<GonioStyle>(APP_STATE.getGonioStyle());
    m_vuBallistics = static_cast<VuBallistics>(APP_STATE.getVuBallistics());
    m_vuSensitivity = APP_STATE.getVuSensitivity();
    m_analogVUStyle = static_cast<AnalogVUStyle>(APP_STATE.getAnalogVUStyle());

    // Audio-Werte initialisieren
    m_currentRMSLeft = 0.0f;
    m_currentRMSRight = 0.0f;
    m_currentPeakLeft = 0.0f;
    m_currentPeakRight = 0.0f;

    // Analoge VU-Meter initialisieren
    m_needleInit = false;
    m_pastValL = -VU_MAX_ANGLE;
    m_pastValR = -VU_MAX_ANGLE;
    m_needleVelL = 0.0f;
    m_needleVelR = 0.0f;
    m_lastRenderedL = -VU_MAX_ANGLE;
    m_lastRenderedR = -VU_MAX_ANGLE;
    m_lastUpdate_analog = 0;

    // Digitale VU-Meter initialisieren
    for (int i = 0; i < 2; ++i)
    {
        m_rmsChannels[i] = {VU_DB_LOW, 0, 0, 0};
        m_ppmChannels[i] = {VU_DB_LOW, 0, 0, 0};
    }

    m_lastUpdate_digital = 0;

    // Sample-Daten initialisieren (Double-Buffer)
    m_writeBufferIdx.store(0);
    m_readerActive.store(false);
    for (uint8_t i = 0; i < SAMPLE_BUFFER_COUNT; ++i)
    {
        m_sampleCount[i] = 0;
        m_sampleRate[i] = 0;
    }
    m_lastSpectrumUpdate = 0;

    // Wellenform Auto-Gain initialisieren

    for (uint8_t i = 0; i < WAVE_PEAK_HISTORY_SIZE; i++)
    {
        m_wavePeakHistory[i] = 0.0f;
    }

    m_wavePeakHistoryIdx = 0;
    m_waveSlidingMax = 0.0f;
    m_waveLastPeakUpdate = 0;
    m_waveAmp = 1.0f;

    // Goniometer Auto-Verstärkung initialisieren
    for (uint8_t i = 0; i < GONIO_PEAK_HISTORY_SIZE; i++)
    {
        m_gonioPeakHistory[i] = 0.0f;
    }

    m_gonioPeakHistoryIdx = 0;
    m_gonioSlidingMax = 0.0f;
    m_gonioLastPeakUpdate = 0;
    m_gonioLastDynamicsUpdate = 0;
    m_gonioAmp = 1.0f;      // Auto-Verstärkung zurücksetzen
    m_gonioCorrPrev = 0.0f; // Korrelations-Ballistik zurücksetzen

    LOG_INFO("Visualizer erfolgreich initialisiert");
    return true;
}

// ===================================================================
// Öffentliche Methoden
// ===================================================================

/**
 * @brief Setzt Audio-Werte (thread-safe)
 *
 * Verwendet Critical Section für sichere Aktualisierung auf Dual-Core ESP32. Kann sicher aus dem Audio-Task aufgerufen werden.
 */
void Visualizer::setAudioValues(float rmsLeft, float rmsRight, float peakLeft, float peakRight)
{
    portENTER_CRITICAL(&m_audioMux);
    m_currentRMSLeft = rmsLeft;
    m_currentRMSRight = rmsRight;
    m_currentPeakLeft = peakLeft;
    m_currentPeakRight = peakRight;
    portEXIT_CRITICAL(&m_audioMux);
}

/**
 * @brief Setzt Sample-Daten
 *
 * Writer schreibt in den inaktiven Puffer, dann atomarer Swap. Reader liest immer aus dem aktiven Puffer.
 */
void Visualizer::setSampleData(const int16_t *samplesL, const int16_t *samplesR, uint16_t count, uint32_t sampleRate)
{
    if (!samplesL || !samplesR || count == 0)
        return;

    // Schreib-Puffer ermitteln
    const uint8_t writeIdx = m_writeBufferIdx.load(std::memory_order_relaxed);

    // Puffer-Zeiger prüfen
    if (!m_samplesL[writeIdx] || !m_samplesR[writeIdx])
        return;

    // Original-Anzahl merken für korrekten Start-Offset
    const uint32_t srcCount = count;

    // Anzahl der zu kopierenden Samples begrenzen
    const uint16_t samplesToCopy = (srcCount > MAX_SAMPLES) ? MAX_SAMPLES : static_cast<uint16_t>(srcCount);

    // Start-Offset: Bei mehr als MAX_SAMPLES die NEUESTEN Samples nehmen
    const uint32_t start = (srcCount > MAX_SAMPLES) ? (srcCount - MAX_SAMPLES) : 0;

    // Samples in den Schreib-Puffer kopieren
    memcpy(m_samplesL[writeIdx].get(), samplesL + start, samplesToCopy * sizeof(int16_t));
    memcpy(m_samplesR[writeIdx].get(), samplesR + start, samplesToCopy * sizeof(int16_t));
    m_sampleCount[writeIdx] = samplesToCopy;
    m_sampleRate[writeIdx] = sampleRate;

    // Race-Protection: Kein Swap wenn Reader noch aktiv ist (dieses Update wird verworfen, nächstes kommt bald) memory_order_acquire garantiert Sichtbarkeit des Reader-Status auf Dual-Core ESP32
    if (m_readerActive.load(std::memory_order_acquire))
        return;

    // Atomarer Puffer-Swap: Schreib-Puffer wird zum Lese-Puffer. memory_order_release garantiert, dass alle vorherigen Schreiboperationen abgeschlossen sind, bevor der Swap sichtbar wird.
    m_writeBufferIdx.store(writeIdx ^ 1, std::memory_order_release);

    // Frischemarke hochzaehlen, damit der Zeichenweg neue von schon gezeichneten Werten
    // unterscheiden kann. Eine Verschiebung um ein Bild gegenueber dem Wechsel oben ist
    // harmlos: schlimmstenfalls wird ein Datensatz ein Bild spaeter dargestellt.
    m_sampleSeq.fetch_add(1, std::memory_order_release);
}

/**
 * @brief Behandelt Touch-Eingaben mit State-Machine
 */
void Visualizer::handleTouch(int touchX, int touchY, bool touched)
{
    static constexpr int SWIPE_THRESHOLD = 50;
    static constexpr unsigned long SWIPE_MAX_TIME = 500;
    static constexpr unsigned long LONG_PRESS_TIME = 800;

    // Nur ein frisches Aufsetzen darf eine Geste beginnen; eine laufende Berührung, die aus Playlist-Drag oder Scrubbing hereinwandert, nicht
    const bool freshTouch = touched && !m_visTouchSeen;
    m_visTouchSeen = touched;

    // Prüfe ob Touch im Visualisierungs-Bereich ist
    // Bereich: [X, X+W) und [Y, Y+H) - exklusiv obere Grenze
    const bool inVisArea = touched &&
                           touchX >= VIS_AREA_X &&
                           touchX < VIS_AREA_X + VIS_AREA_W &&
                           touchY >= VIS_AREA_Y &&
                           touchY < VIS_AREA_Y + VIS_AREA_H;

    // State-Machine für Touch-Handling
    switch (m_visTouchState)
    {
    case VisTouchState::IDLE:
        if (inVisArea && freshTouch)
        {
            // Touch beginnt im Visualisierungs-Bereich
            m_visTouchState = VisTouchState::TOUCH_STARTED;
            m_visTouchStartX = touchX;
            m_visTouchLastX = touchX; // Letzte gültige Position initialisieren
            m_visTouchStartTime = millis();
        }
        break;

    case VisTouchState::TOUCH_STARTED:
        if (!touched)
        {
            // Touch endet - prüfe auf Wischgeste
            const unsigned long touchDuration = millis() - m_visTouchStartTime;
            const int swipeDistance = m_visTouchLastX - m_visTouchStartX;

            // Wischgeste erkannt
            if (touchDuration < SWIPE_MAX_TIME && abs(swipeDistance) > SWIPE_THRESHOLD)
                switchVisMode(swipeDistance > 0 ? 1 : -1);

            m_visTouchState = VisTouchState::IDLE;
        }
        else if (inVisArea && millis() - m_visTouchStartTime > LONG_PRESS_TIME)
        {
            // Langer Druck erkannt
            toggleVisSubMode();
            m_visTouchState = VisTouchState::LONG_PRESS_DONE;
        }
        else if (inVisArea)
        {
            // Touch noch aktiv im Bereich - letzte gültige Position speichern
            m_visTouchLastX = touchX;
        }
        // Wenn !inVisArea && touched: Position nicht aktualisieren, Swipe kann später noch erkannt werden
        break;

    case VisTouchState::LONG_PRESS_DONE:
        // Touch endet nach langem Druck
        if (!touched)
            m_visTouchState = VisTouchState::IDLE;

        break;
    }
}

/**
 * @brief Zeichnet die Visualisierung
 *
 * Erstellt eine lokale Kopie der Audio-Werte unter Critical Section, um Thread-Safety auf Dual-Core ESP32 zu gewährleisten.
 */
void Visualizer::draw(bool init)
{
    // Ohne Initialisierung gibt es weder Display noch Sprites. Der SD-Player zeichnet beim
    // Aufbau seiner Oberflaeche einmal, bevor er init() gerufen hat.
    if (!m_tft)
        return;

    // Lokale Kopie der Audio-Werte (thread-safe)
    float rmsLeft, rmsRight, peakLeft, peakRight;
    portENTER_CRITICAL(&m_audioMux);
    rmsLeft = m_currentRMSLeft;
    rmsRight = m_currentRMSRight;
    peakLeft = m_currentPeakLeft;
    peakRight = m_currentPeakRight;
    portEXIT_CRITICAL(&m_audioMux);

    drawVis(init, rmsLeft, rmsRight, peakLeft, peakRight);
}

/**
 * @brief Setzt den Visualisierungs-Modus
 */
void Visualizer::setMode(VisMode mode)
{
    if (m_visMode == mode)
        return;

    // Sprites werden wiederverwendet
    m_visMode = mode;
    APP_STATE.setVisMode(static_cast<uint8_t>(m_visMode));
    initVisMode(m_visMode);
}

// ===================================================================
// Touch-Handling und Modus-Umschaltung
// ===================================================================

/**
 * @brief Wechselt Visualisierungs-Modus
 */
void Visualizer::switchVisMode(int direction)
{
    // Neuen Modus berechnen
    int newMode = (static_cast<int>(m_visMode) + direction) % VIS_MODE_COUNT;

    // Bei negativem Ergebnis zum letzten Modus springen
    if (newMode < 0)
        newMode += VIS_MODE_COUNT;

    // Modus setzen
    setMode(static_cast<VisMode>(newMode));

    LOG_INFO_F("VU-Modus gewechselt zu: %d", static_cast<int>(m_visMode));
}

/**
 * @brief Schaltet den Style der aktuellen Visualisierung um
 */
void Visualizer::toggleVisSubMode()
{
    switch (m_visMode)
    {
    case VisMode::VIS_GONIO:
        m_gonioStyle = (m_gonioStyle == GonioStyle::CLASSIC)
                           ? GonioStyle::RAINBOW
                           : GonioStyle::CLASSIC;

        APP_STATE.setGonioStyle(static_cast<uint8_t>(m_gonioStyle));
        LOG_INFO_F("Goniometer-Stil gewechselt zu: %d", static_cast<int>(m_gonioStyle));
        break;

    case VisMode::VIS_WAVE:
        m_waveformStyle = (m_waveformStyle == WaveformStyle::OVERLAY)
                              ? WaveformStyle::STACKED
                              : WaveformStyle::OVERLAY;

        APP_STATE.setWaveformStyle(static_cast<uint8_t>(m_waveformStyle));
        LOG_INFO_F("Wellenform-Stil gewechselt zu: %d", static_cast<int>(m_waveformStyle));
        break;

    case VisMode::VIS_SPECTRUM:
        m_spectrumStyle = (m_spectrumStyle == SpectrumStyle::CLASSIC)
                              ? SpectrumStyle::RAINBOW
                              : SpectrumStyle::CLASSIC;

        APP_STATE.setSpectrumStyle(static_cast<uint8_t>(m_spectrumStyle));
        LOG_INFO_F("Spektrum-Stil gewechselt zu: %d", static_cast<int>(m_spectrumStyle));
        break;

    case VisMode::VIS_SPECTRUM_CENTERED:
        m_centeredSpectrumStyle = (m_centeredSpectrumStyle == SpectrumStyle::CLASSIC)
                                     ? SpectrumStyle::RAINBOW
                                     : SpectrumStyle::CLASSIC;
        APP_STATE.setCenteredSpectrumStyle(static_cast<uint8_t>(m_centeredSpectrumStyle));
        m_centeredSpectrumDirty = true;
        break;

    case VisMode::VIS_WATERFALL:
        m_waterfallStyle = (m_waterfallStyle == SpectrumStyle::CLASSIC)
                               ? SpectrumStyle::RAINBOW
                               : SpectrumStyle::CLASSIC;
        APP_STATE.setWaterfallStyle(static_cast<uint8_t>(m_waterfallStyle));
        // Die Farbskala gilt für den gesamten Verlauf; alte Zeilen werden beim Wechsel gelöscht.
        m_spectrumHadSamples = false;
        m_lastSpectrumUpdate = millis();
        drawWaterfall(true, false, false, 0);
        break;

    case VisMode::VIS_AURORA:
        m_auroraStyle = m_auroraStyle == AuroraStyle::COOL ? AuroraStyle::WARM : AuroraStyle::COOL;
        APP_STATE.setAuroraStyle(static_cast<uint8_t>(m_auroraStyle));
        break;

    case VisMode::VIS_DIGITAL:
        m_digitalVUStyle = (m_digitalVUStyle == DigitalVUStyle::RAINBOW)
                               ? DigitalVUStyle::CLASSIC
                               : DigitalVUStyle::RAINBOW;

        APP_STATE.setDigitalVUStyle(static_cast<uint8_t>(m_digitalVUStyle));
        LOG_INFO_F("Digital-VU-Stil gewechselt zu: %d", static_cast<int>(m_digitalVUStyle));
        break;

    case VisMode::VIS_ANALOG:
        m_analogVUStyle = (m_analogVUStyle == AnalogVUStyle::CLASSIC) ? AnalogVUStyle::GREEN
                                                                     : AnalogVUStyle::CLASSIC;

        APP_STATE.setAnalogVUStyle(static_cast<uint8_t>(m_analogVUStyle));
        LOG_INFO_F("Analog-VU-Fassung gewechselt zu: %d", static_cast<int>(m_analogVUStyle));

        // Anders als bei den übrigen Stilen hängt hier auch die Farbe des Zeiger-Sprites daran;
        // der wird nur beim Init gefüllt, deshalb den Modus neu aufsetzen.
        initVisMode(m_visMode);
        break;
    }
}

/**
 * @brief Initialisiert Visualisierungs-Modus
 */
void Visualizer::initVisMode(VisMode mode)
{
    switch (mode)
    {
    case VisMode::VIS_ANALOG:
        drawAnalogVU(true, 0.0f, 0.0f);
        break;
    case VisMode::VIS_DIGITAL:
        drawDigitalVU(true, 0.0f, 0.0f, 0.0f, 0.0f);
        break;
    case VisMode::VIS_SPECTRUM:
    case VisMode::VIS_SPECTRUM_CENTERED:
    case VisMode::VIS_WATERFALL:
        drawSpectrum(true);
        break;
    case VisMode::VIS_AURORA:
        drawAurora(true, 0.0f, 0.0f);
        break;
    case VisMode::VIS_WAVE:
        drawWave(true);
        break;
    case VisMode::VIS_GONIO:
        drawGonio(true, 0.0f, 0.0f);
        drawGonioVU(true, 0.0f, 0.0f);
        break;
    }
}

/**
 * @brief Löscht den gesamten Visualisierungsbereich
 */
void Visualizer::clearVisualizationArea()
{
    if (!m_tft)
        return;

    // Zwei separate Bereiche löschen:
    // 1. Hauptvisualisierungsbereich (Analog/Digital/Spectrum/Wave)
    // 2. Goniometer-Bereich inkl. VU-Meter

    // Hauptvisualisierungsbereich
    m_tft->fillRect(VIS_AREA_X - 10, VIS_AREA_Y, VIS_AREA_W + 10, VIS_AREA_H, Theme::ColorMainBackGround);

    // Goniometer-Bereich (größter möglicher Bereich für Gonio + VU-Meter)
    m_tft->fillRect(CONFIG_SDPLAYER.posX, CONFIG_SDPLAYER.posY,
                    CONFIG_SDPLAYER.size + 38, CONFIG_SDPLAYER.size, Theme::ColorMainBackGround);
}

/**
 * @brief Visualisierungs-Router
 */
void Visualizer::drawVis(bool init, float rmsLeft, float rmsRight, float peakLeft, float peakRight)
{
    switch (m_visMode)
    {
    case VisMode::VIS_ANALOG:
        drawAnalogVU(init, rmsLeft, rmsRight);
        break;
    case VisMode::VIS_DIGITAL:
        drawDigitalVU(init, rmsLeft, rmsRight, peakLeft, peakRight);
        break;
    case VisMode::VIS_SPECTRUM:
    case VisMode::VIS_SPECTRUM_CENTERED:
    case VisMode::VIS_WATERFALL:
        drawSpectrum(init);
        break;
    case VisMode::VIS_AURORA:
        drawAurora(init, rmsLeft, rmsRight);
        break;
    case VisMode::VIS_WAVE:
        drawWave(init);
        break;
    case VisMode::VIS_GONIO:
        drawGonio(init, peakLeft, peakRight);
        // Der Balken zeigt den Effektivwert, wie das analoge und das digitale Instrument auch.
        // Die Spitzenwerte bleiben den beiden Lampen unter dem Verstaerkungsfaktor vorbehalten.
        drawGonioVU(init, rmsLeft, rmsRight);
        break;
    }
}

/**
 * @brief Visualisierungs-Sprites aufräumen
 */
void Visualizer::cleanupCurrentVisSprites()
{
    if (!m_tft)
        return;

    // analoges VU-Meter
    m_tft->safeDeleteSprite(m_sprVU_Analog_Needle);
    m_tft->safeDeleteSprite(m_sprVU_Analog_L);
    m_tft->safeDeleteSprite(m_sprVU_Analog_R);

    // digitales VU-Meter
    m_tft->safeDeleteSprite(m_sprVU_Digital_RMS_L);
    m_tft->safeDeleteSprite(m_sprVU_Digital_RMS_R);
    m_tft->safeDeleteSprite(m_sprVU_Digital_PPM_L);
    m_tft->safeDeleteSprite(m_sprVU_Digital_PPM_R);

    // Spektrum-Analyzer
    m_tft->safeDeleteSprite(m_sprVU_Spectrum);

    // Nordlicht
    m_tft->safeDeleteSprite(m_sprAurora);

    // Wellenform-Anzeige
    m_tft->safeDeleteSprite(m_sprVU_Wave);

    // Goniometer-Anzeige
    m_tft->safeDeleteSprite(m_sprGonio);
    m_tft->safeDeleteSprite(m_sprGonioBg);

    // Goniometer VU-Anzeige
    m_tft->safeDeleteSprite(m_sprGonioVULeft);
    m_tft->safeDeleteSprite(m_sprGonioVUMiddle);
    m_tft->safeDeleteSprite(m_sprGonioVURight);

    // Goniometer Info-Anzeige
    if (m_sprGonioInfo)
        m_sprGonioInfo->unloadFont();

    m_tft->safeDeleteSprite(m_sprGonioInfo);
}

// ===================================================================
// Analoge VU-Anzeige
// ===================================================================

/**
 * @brief Rechnet einen linearen Amplitudenwert in einen kalibrierten VU-Pegel um
 */
float Visualizer::amplitudeToDb(float amplitude) const
{
    if (amplitude <= 1e-5f)
        return VU_DB_LOW;

    float db = 20.0f * log10f(amplitude) + VU_CALIBRATION_DB;

    if (db < VU_DB_LOW)
        db = VU_DB_LOW;

    if (db > VU_DB_HIGH)
        db = VU_DB_HIGH;

    return db;
}

/**
 * @brief Rechnet einen VU-Pegel in den Zeigerwinkel des analogen Instruments um
 */
float Visualizer::vuAlignmentDb() const
{
    constexpr size_t anzahl = sizeof(VU_ALIGNMENT_DB) / sizeof(VU_ALIGNMENT_DB[0]);
    return VU_ALIGNMENT_DB[(m_vuSensitivity < anzahl) ? m_vuSensitivity : anzahl / 2];
}

/**
 * @brief Liefert das Skalenblatt der eingestellten Farbfassung
 */
const uint16_t *Visualizer::analogVUPlate() const
{
    return (m_analogVUStyle == AnalogVUStyle::GREEN) ? retro_vu_meter_green : retro_vu_meter;
}

/**
 * @brief Rechnet einen VU-Pegel in den Zeigerwinkel des analogen Instruments um
 */
float Visualizer::dbToNeedleAngle(float db) const
{
    float share = VU_ZERO_SHARE * powf(10.0f, (db - vuAlignmentDb()) / 20.0f);

    if (share > 1.0f)
        share = 1.0f;

    if (share < 0.0f)
        share = 0.0f;

    return atanf((2.0f * share - 1.0f) * VU_TAN_MAX) * VU_RAD_TO_DEG;
}

/**
 * @brief Analoges VU-Meter mit Doublebuffering
 */
void Visualizer::drawAnalogVU(bool init, float rmsLeft, float rmsRight)
{
    if (APP_STATE.getBlockGFX())
        return;

    if (!APP_STATE.getStationSetDone())
        rmsLeft = rmsRight = 0.0f;

    constexpr int vuX1 = 355, vuY1 = 355, vuX2 = 582, vuY2 = 355;
    constexpr float redrawThreshold = 0.5f;

    unsigned long currentTime = millis();
    float deltaTime = (currentTime - m_lastUpdate_analog) / 1000.0f;

    // Initialisierung
    if (init)
    {
        // Die Einstellungen frisch holen: sie können in den Einstellungen geändert worden sein.
        m_vuBallistics = static_cast<VuBallistics>(APP_STATE.getVuBallistics());
        m_vuSensitivity = APP_STATE.getVuSensitivity();
        m_analogVUStyle = static_cast<AnalogVUStyle>(APP_STATE.getAnalogVUStyle());

        m_needleInit = true;
        m_pastValL = m_pastValR = m_lastRenderedL = m_lastRenderedR = -VU_MAX_ANGLE;
        m_needleVelL = m_needleVelR = 0.0f;
        m_lastUpdate_analog = currentTime;

        // Sprites nur erstellen, wenn nicht vorhanden
        if (!m_sprVU_Analog_Needle)
        {
            if (!m_tft->safeCreateSprite(m_sprVU_Analog_Needle, 3, 120, false))
            {
                LOG_ERROR("Konnte m_sprVU_Analog_Needle Sprite nicht erstellen!");
                return;
            }
        }

        m_sprVU_Analog_Needle->clear((m_analogVUStyle == AnalogVUStyle::GREEN) ? VU_NEEDLE_GREEN
                                                                              : VU_NEEDLE_CLASSIC);
        m_sprVU_Analog_Needle->setPivot(2, 130);

        if (!m_sprVU_Analog_L)
        {
            if (!m_tft->safeCreateSprite(m_sprVU_Analog_L, VU_PLATE_W, VU_PLATE_H))
            {
                LOG_ERROR("Konnte m_sprVU_Analog_L Sprite nicht erstellen!");
                return;
            }
        }
        m_sprVU_Analog_L->setPivot(VU_PIVOT_X, VU_PIVOT_Y);

        if (!m_sprVU_Analog_R)
        {
            if (!m_tft->safeCreateSprite(m_sprVU_Analog_R, VU_PLATE_W, VU_PLATE_H))
            {
                LOG_ERROR("Konnte m_sprVU_Analog_R Sprite nicht erstellen!");
                return;
            }
        }
        m_sprVU_Analog_R->setPivot(VU_PIVOT_X, VU_PIVOT_Y);

        // Visualisierungshintergrund löschen
        clearVisualizationArea();

        // Beide Kanäle initial anzeigen
        m_sprVU_Analog_L->pushImage(0, 0, VU_PLATE_W, VU_PLATE_H, analogVUPlate());
        m_sprVU_Analog_Needle->pushRotatedWithAA(m_sprVU_Analog_L, -VU_MAX_ANGLE);
        m_sprVU_Analog_L->pushSprite(vuX1, vuY1);

        m_sprVU_Analog_R->pushImage(0, 0, VU_PLATE_W, VU_PLATE_H, analogVUPlate());
        m_sprVU_Analog_Needle->pushRotatedWithAA(m_sprVU_Analog_R, -VU_MAX_ANGLE);
        m_sprVU_Analog_R->pushSprite(vuX2, vuY2);

        return;
    }

    if (deltaTime <= 0)
        return;

    m_lastUpdate_analog = currentTime;

    // Die Bewegungsgleichung wird in festen kleinen Schritten gerechnet, und deren Zahl ist
    // begrenzt. Ein langes Bildintervall wuerde die Schritte deshalb strecken und die Rechnung
    // aufschaukeln lassen. Beim allerersten Aufruf steht der Zeitstempel noch bei null, das
    // Intervall betruege dann die gesamte Laufzeit seit dem Start.
    if (deltaTime > VU_MAX_STEPS * VU_MAX_STEP)
        deltaTime = VU_MAX_STEPS * VU_MAX_STEP;

    const uint16_t *plate = analogVUPlate();

    // Kennwerte des nachgebildeten Drehspulwerks, je nach Einstellung
    const size_t ballistics = static_cast<size_t>(m_vuBallistics);
    const float omega = VU_OMEGA_TIMES_TIME / VU_RESPONSE_TIME[ballistics];
    const float federKraft = omega * omega;
    const float daempfung = 2.0f * VU_DAMPING * omega;

    int schritte = static_cast<int>(deltaTime / VU_MAX_STEP) + 1;
    if (schritte > VU_MAX_STEPS)
        schritte = VU_MAX_STEPS;

    const float schrittZeit = deltaTime / static_cast<float>(schritte);

    // Kanalverarbeitung: nur der Streifen wird aufgefrischt, den die Nadel verlassen oder neu
    // eingenommen hat. Ein volles Blatt sind gut 15 000 Bildpunkte, der Streifen wenige hundert.
    auto processChannel = [&](float rms, float &pastVal, float &velocity, float &lastRendered, Gfx_Sprite *spr, int x, int y)
    {
        if (!spr)
            return;

        float dBValue = amplitudeToDb(rms);
        float targetVal = dbToNeedleAngle(dBValue);

        // Feder zieht zum Zielwert, Dämpfung bremst; erst die Geschwindigkeit, dann der Weg.
        for (int i = 0; i < schritte; ++i)
        {
            velocity += (federKraft * (targetVal - pastVal) - daempfung * velocity) * schrittZeit;
            pastVal += velocity * schrittZeit;
        }

        if (fabs(pastVal - lastRendered) < redrawThreshold)
            return;

        vuNeedleSpans(lastRendered, g_vuSpanAltVon, g_vuSpanAltBis);
        vuNeedleSpans(pastVal, g_vuSpanNeuVon, g_vuSpanNeuBis);

        // Das Blatt nur dort zurückholen, wo die alte Nadel stand
        for (int row = 0; row < VU_PLATE_H; ++row)
        {
            if (g_vuSpanAltBis[row] < g_vuSpanAltVon[row])
                continue;

            const int breite = g_vuSpanAltBis[row] - g_vuSpanAltVon[row] + 1;
            spr->pushImage(g_vuSpanAltVon[row], row, breite, 1,
                           plate + row * VU_PLATE_W + g_vuSpanAltVon[row]);
        }

        m_sprVU_Analog_Needle->pushRotatedWithAA(spr, pastVal);

        // Auf den Bildschirm kommt das umschließende Rechteck beider Nadelstellungen
        int links = VU_PLATE_W, rechts = -1, oben = VU_PLATE_H, unten = -1;
        for (int row = 0; row < VU_PLATE_H; ++row)
        {
            const bool alt = g_vuSpanAltBis[row] >= g_vuSpanAltVon[row];
            const bool neu = g_vuSpanNeuBis[row] >= g_vuSpanNeuVon[row];

            if (!alt && !neu)
                continue;

            if (alt)
            {
                links = min(links, static_cast<int>(g_vuSpanAltVon[row]));
                rechts = max(rechts, static_cast<int>(g_vuSpanAltBis[row]));
            }

            if (neu)
            {
                links = min(links, static_cast<int>(g_vuSpanNeuVon[row]));
                rechts = max(rechts, static_cast<int>(g_vuSpanNeuBis[row]));
            }

            oben = min(oben, row);
            unten = max(unten, row);
        }

        if (rechts >= links && unten >= oben)
        {
            m_tft->setClipRect(x + links, y + oben, rechts - links + 1, unten - oben + 1);
            spr->pushSprite(x, y);
            m_tft->clearClipRect();
        }

        lastRendered = pastVal;
    };

    processChannel(rmsLeft, m_pastValL, m_needleVelL, m_lastRenderedL, m_sprVU_Analog_L, vuX1, vuY1);
    processChannel(rmsRight, m_pastValR, m_needleVelR, m_lastRenderedR, m_sprVU_Analog_R, vuX2, vuY2);
}

// ===================================================================
// Digitale VU-Anzeige
// ===================================================================

/**
 * @brief Digitale Aussteuerungsanzeige mit RMS- und PPM-Balkenanzeige
 */
void Visualizer::drawDigitalVU(bool init, float rmsLeft, float rmsRight, float peakLeft, float peakRight)
{
    if (APP_STATE.getBlockGFX())
        return;

    if (!APP_STATE.getStationSetDone())
        rmsLeft = rmsRight = peakLeft = peakRight = 0.0f;

    static constexpr int VU_W = 440;
    static constexpr int RMS_HEIGHT = 18;
    static constexpr int RMS_PEAK_WIDTH = 1;
    static constexpr int PPM_PEAK_WIDTH = 8;
    static constexpr uint32_t PEAK_HOLD_MS = 1000;

    static constexpr uint16_t RMS_COLOR_GREEN = 0x6c64;
    static constexpr uint16_t RMS_COLOR_ORANGE = TFT_ORANGE;
    static constexpr uint16_t RMS_COLOR_RED = TFT_RED;
    static constexpr uint16_t RMS_COLOR_BG = 0x2104;
    static constexpr uint16_t RMS_COLOR_PEAK = TFT_WHITE;

    static constexpr uint16_t PPM_COLOR_GREEN = 0x5370;
    static constexpr uint16_t PPM_COLOR_YELLOW = 0x5370;
    static constexpr uint16_t PPM_COLOR_RED = TFT_RED;
    static constexpr uint16_t PPM_COLOR_BG = 0x2104;
    static constexpr uint16_t PPM_COLOR_PEAK = TFT_WHITE;

    const uint32_t now = millis();

    // Initialisierung
    if (init)
    {
        m_rmsChannels[0] = m_rmsChannels[1] = {VU_DB_LOW, 0, 0, 0};
        m_ppmChannels[0] = m_ppmChannels[1] = {VU_DB_LOW, 0, 0, 0};
        m_lastUpdate_digital = now;

        // Sprites nur erstellen, wenn nicht vorhanden
        if (!m_sprVU_Digital_RMS_L)
        {
            if (!m_tft->safeCreateSprite(m_sprVU_Digital_RMS_L, VU_W, RMS_HEIGHT))
            {
                LOG_ERROR("Konnte Digital-VU RMS_L Sprite nicht erstellen!");
                return;
            }
        }
        if (!m_sprVU_Digital_RMS_R)
        {
            if (!m_tft->safeCreateSprite(m_sprVU_Digital_RMS_R, VU_W, RMS_HEIGHT))
            {
                LOG_ERROR("Konnte Digital-VU RMS_R Sprite nicht erstellen!");
                return;
            }
        }
        if (!m_sprVU_Digital_PPM_L)
        {
            if (!m_tft->safeCreateSprite(m_sprVU_Digital_PPM_L, VU_W, 1))
            {
                LOG_ERROR("Konnte Digital-VU PPM_L Sprite nicht erstellen!");
                return;
            }
        }
        if (!m_sprVU_Digital_PPM_R)
        {
            if (!m_tft->safeCreateSprite(m_sprVU_Digital_PPM_R, VU_W, 1))
            {
                LOG_ERROR("Konnte Digital-VU PPM_R Sprite nicht erstellen!");
                return;
            }
        }

        // Visualisierungshintergrund löschen
        clearVisualizationArea();

        // Skalenmarkierungen zeichnen
        constexpr int MARK_Y = VIS_AREA_Y + 25;
        constexpr int MARK_HEIGHT_MAJOR = 20;
        constexpr int MARK_HEIGHT_MINOR = 10;
        constexpr int MARK_HEIGHT_ZERO = 20;
        constexpr uint16_t MARK_COLOR_MAJOR = TFT_SILVER;
        constexpr uint16_t MARK_COLOR_MINOR = 0x4208;
        constexpr uint16_t MARK_COLOR_ZERO = TFT_RED;
        constexpr int TEXT_OFFSET_Y = 4;

        m_tft->loadFont(ComfortaaBold10);

        auto drawMark = [&](float dB, bool isMajor, bool showText)
        {
            if (dB < VU_DB_LOW || dB > VU_DB_HIGH)
                return;

            uint16_t xPos = VIS_AREA_X + (uint16_t)((dB - VU_DB_LOW) / (VU_DB_HIGH - VU_DB_LOW) * VU_W);
            bool isZero = (fabs(dB) < 0.001f);

            uint16_t markColor = isZero ? MARK_COLOR_ZERO : (isMajor ? MARK_COLOR_MAJOR : MARK_COLOR_MINOR);
            int markHeight = isZero ? MARK_HEIGHT_ZERO : (isMajor ? MARK_HEIGHT_MAJOR : MARK_HEIGHT_MINOR);
            int markYOffset = isMajor ? 0 : (MARK_HEIGHT_MAJOR - MARK_HEIGHT_MINOR) / 2;

            m_tft->drawFastVLine(xPos, MARK_Y + markYOffset, markHeight, markColor);

            if (showText && (isMajor || isZero))
            {
                char dbText[8];
                if (isZero)
                {
                    stringutils::safeCopy(dbText, sizeof(dbText), "0");
                    m_tft->setTextColor(MARK_COLOR_ZERO, TFT_BLACK);
                }
                else
                {
                    stringutils::safeFormat(dbText, "%.0f", dB);
                    m_tft->setTextColor(TFT_WHITE, TFT_BLACK);
                }

                int textXPos = xPos - (m_tft->textWidth(dbText) / 2);

                if (dB >= 0.0f)
                    textXPos += 1;

                m_tft->drawString(dbText, textXPos, MARK_Y + TEXT_OFFSET_Y);
            }
        };

        const float majorMarks[] = {-100, -90, -80, -70, -60, -50, -40, -30, -20, -12, -6, -3, 0, 3, 6, 12};
        for (float dB : majorMarks)
            drawMark(dB, true, true);

        bool dBHighInMajors = false;

        for (float mark : majorMarks)
        {
            if (fabs(mark - VU_DB_HIGH) < 0.001f)
            {
                dBHighInMajors = true;
                break;
            }
        }

        if (!dBHighInMajors && VU_DB_HIGH > 3.0f)
            drawMark(VU_DB_HIGH, true, true);

        float currentDB = ceilf(VU_DB_LOW);
        while (currentDB < VU_DB_HIGH)
        {
            bool isMajorMark = false;
            for (float majorDB : majorMarks)
            {
                if (fabs(currentDB - majorDB) < 0.001f)
                {
                    isMajorMark = true;
                    break;
                }
            }

            if (!isMajorMark)
                drawMark(currentDB, false, false);

            if (currentDB < -20.0f)
                currentDB += 2.0f;
            else
                currentDB += 1.0f;
        }

        m_tft->unloadFont();

        return;
    }

    // Hauptteil
    const float dt = (now - m_lastUpdate_digital) * 0.001f;
    if (dt <= 0)
        return;

    m_lastUpdate_digital = now;

    const uint16_t threshOrange = (uint16_t)((-12.0f - VU_DB_LOW) / (VU_DB_HIGH - VU_DB_LOW) * VU_W);
    const uint16_t threshRed = (uint16_t)((-3.0f - VU_DB_LOW) / (VU_DB_HIGH - VU_DB_LOW) * VU_W);
    const uint16_t ppmThreshYellow = (uint16_t)((-18.0f - VU_DB_LOW) / (VU_DB_HIGH - VU_DB_LOW) * VU_W);
    const uint16_t ppmThreshRed = (uint16_t)((-6.0f - VU_DB_LOW) / (VU_DB_HIGH - VU_DB_LOW) * VU_W);

    auto updateRMSChannel = [&](float rms, ChannelData &ch)
    {
        float targetDB = amplitudeToDb(rms);

        float alpha = (targetDB > ch.smoothedDB) ? (1.0f - expf(-dt / 0.05f)) : (1.0f - expf(-dt / 0.3f));

        ch.smoothedDB += (targetDB - ch.smoothedDB) * alpha;
        ch.currentPos = (ch.smoothedDB <= VU_DB_LOW) ? 0 : (ch.smoothedDB >= VU_DB_HIGH) ? VU_W
                                                                                         : (uint16_t)((ch.smoothedDB - VU_DB_LOW) / (VU_DB_HIGH - VU_DB_LOW) * VU_W);

        if (ch.currentPos > ch.peakPos)
        {
            ch.peakPos = ch.currentPos;
            ch.peakTime = now;
        }
        else if (now - ch.peakTime > PEAK_HOLD_MS && ch.peakPos > 0)
        {
            ch.peakPos--;
        }
    };

    auto updatePPMChannel = [&](float peak, ChannelData &ch)
    {
        float targetDB = amplitudeToDb(peak);

        float alpha = (targetDB > ch.smoothedDB) ? (1.0f - expf(-dt / 0.003f)) : (1.0f - expf(-dt / 0.15f));

        ch.smoothedDB += (targetDB - ch.smoothedDB) * alpha;
        ch.currentPos = (ch.smoothedDB <= VU_DB_LOW) ? 0 : (ch.smoothedDB >= VU_DB_HIGH) ? VU_W
                                                                                         : (uint16_t)((ch.smoothedDB - VU_DB_LOW) / (VU_DB_HIGH - VU_DB_LOW) * VU_W);

        if (ch.currentPos > ch.peakPos)
        {
            ch.peakPos = ch.currentPos;
            ch.peakTime = now;
        }
        else if (now - ch.peakTime > (PEAK_HOLD_MS / 2) && ch.peakPos > 0)
        {
            ch.peakPos--;
        }
    };

    auto renderRMSChannel = [&](Gfx_Sprite *spr, const ChannelData &ch)
    {
        // Direkter Pufferzugriff
        uint16_t *buffer = (uint16_t *)spr->getBuffer();
        if (!buffer)
            return;

        // Erste Zeile direkt in Puffer schreiben
        for (uint16_t x = 0; x < VU_W; x++)
        {
            uint16_t color;
            const uint16_t bgStripe = (x % 2 == 0) ? TFT_BLACK : 0x2104;

            // Peak immer zeichnen
            if (x >= ch.peakPos && x < ch.peakPos + RMS_PEAK_WIDTH)
            {
                color = RMS_COLOR_PEAK;
            }
            else if (x < ch.currentPos)
            {
                if (x % 2 == 0)
                {
                    if (m_digitalVUStyle == DigitalVUStyle::RAINBOW)
                    {
                        // Regenbogen-Farbverlauf
                        color = rainbowColors(map(x, 0, VU_W, 96, 0));
                    }
                    else
                    {
                        if (x < threshOrange)
                            color = RMS_COLOR_GREEN;
                        else if (x < threshRed)
                            color = RMS_COLOR_ORANGE;
                        else
                            color = RMS_COLOR_RED;
                    }
                }
                else
                {
                    color = TFT_BLACK;
                }
            }
            else
            {
                color = bgStripe;
            }

            buffer[x] = bswap16(color);
        }

        // Restliche Zeilen: die erste Zeile kopieren
        for (uint16_t y = 1; y < RMS_HEIGHT; y++)
        {
            memcpy(&buffer[y * VU_W], &buffer[0], VU_W * sizeof(uint16_t));
        }
    };

    auto renderPPMChannel = [&](Gfx_Sprite *spr, const ChannelData &ch)
    {
        // Direkter Pufferzugriff
        uint16_t *buffer = (uint16_t *)spr->getBuffer();
        if (!buffer)
            return;

        // Direkt in Puffer schreiben (schneller als drawPixel!)
        for (uint16_t x = 0; x < VU_W; x++)
        {
            uint16_t color;

            if (ch.peakPos > 0 && x >= ch.peakPos && x < ch.peakPos + PPM_PEAK_WIDTH)
            {
                color = PPM_COLOR_PEAK;
            }
            else if (x < ch.currentPos)
            {
                if (x < ppmThreshYellow)
                    color = PPM_COLOR_GREEN;
                else if (x < ppmThreshRed)
                    color = PPM_COLOR_YELLOW;
                else
                    color = PPM_COLOR_RED;
            }
            else
            {
                color = PPM_COLOR_BG;
            }

            buffer[x] = bswap16(color);
        }
    };

    updateRMSChannel(rmsLeft, m_rmsChannels[0]);
    updateRMSChannel(rmsRight, m_rmsChannels[1]);
    updatePPMChannel(peakLeft, m_ppmChannels[0]);
    updatePPMChannel(peakRight, m_ppmChannels[1]);

    renderRMSChannel(m_sprVU_Digital_RMS_L, m_rmsChannels[0]);
    renderRMSChannel(m_sprVU_Digital_RMS_R, m_rmsChannels[1]);
    renderPPMChannel(m_sprVU_Digital_PPM_L, m_ppmChannels[0]);
    renderPPMChannel(m_sprVU_Digital_PPM_R, m_ppmChannels[1]);

    // RMS-Balken
    m_sprVU_Digital_RMS_L->pushSprite(VIS_AREA_X, VIS_AREA_Y);
    m_sprVU_Digital_RMS_R->pushSprite(VIS_AREA_X, VIS_AREA_Y + VIS_AREA_H - RMS_HEIGHT);

    // PPM-Balken
    m_sprVU_Digital_PPM_L->pushSprite(VIS_AREA_X, VIS_AREA_Y + 22);
    m_sprVU_Digital_PPM_R->pushSprite(VIS_AREA_X, VIS_AREA_Y + VIS_AREA_H - 23);
}

// ===================================================================
// Spektrum-Anzeige
// ===================================================================

/**
 * @brief Berechnet die gemeinsame FFT und zeichnet das gewählte Spektrum mit der Quellabtastrate
 */
void Visualizer::drawSpectrum(bool init)
{
    if (APP_STATE.getBlockGFX() || !m_spectrum_bands)
        return;

    constexpr int BAND_COUNT = 63; ///< Anzahl der klassischen Frequenzbänder
    constexpr int FFT_BIN_COUNT = FFT_N / 2 + 1; ///< FFT-Werte einschließlich der Nyquist-Frequenz
    constexpr uint32_t CENTERED_FRAME_MS = 17; ///< Mindestabstand der mittigen Spektrumbilder
    constexpr int CENTERED_GRAPH_H = VIS_AREA_H - SPECTRUM_LABEL_H; ///< Gesamthöhe der mittig wachsenden Balken
    constexpr int CENTERED_PEAK_H = 2; ///< Höhe der Spitzenmarkierung
    constexpr int CENTERED_PEAK_GAP = 1; ///< Abstand zwischen Balken und Spitzenmarkierung
    constexpr uint32_t PEAK_HOLD_MS = 800; ///< Haltezeit der Spitzen beider Spektren
    constexpr float CLASSIC_PEAK_FALL_RATE = 60.0f; ///< Rücklauf der klassischen Spitzen in Pixeln pro Sekunde
    constexpr float CENTERED_PEAK_FALL_RATE = 30.0f; ///< Rücklauf der Gesamthöhe; die obere Spitzenmarkierung fällt mit 15 Pixeln pro Sekunde
    constexpr uint16_t BAR_GREEN = 0x6c64; ///< Grüne Balkenfarbe beider Spektren

    const bool centered = m_visMode == VisMode::VIS_SPECTRUM_CENTERED;
    const bool waterfall = m_visMode == VisMode::VIS_WATERFALL;
    const int bandCount = centered ? CENTERED_BAND_COUNT : BAND_COUNT;
    const int graphHeight = centered ? CENTERED_GRAPH_H : VIS_AREA_H;
    const uint32_t now = millis();
    const uint32_t elapsed = now - m_lastSpectrumUpdate;
    const uint32_t frameMs = waterfall ? WATERFALL_FRAME_MS : (centered ? CENTERED_FRAME_MS : 2U);
    if (!init && elapsed < frameMs)
        return;

    // Bandgrenzen in Hz
    static constexpr float BAND_CUTOFF_FREQS[BAND_COUNT + 1] = {
        SPECTRUM_MIN_HZ, 129.00f, 172.00f, 215.00f, 258.00f, 301.00f, 344.00f, 387.00f,
        430.00f, 473.00f, 516.00f, 559.00f, 602.00f, 645.00f, 688.00f, 731.00f,
        774.00f, 817.00f, 860.00f, 946.00f, 1032.00f, 1118.00f, 1204.00f, 1290.00f,
        1376.00f, 1505.00f, 1634.00f, 1763.00f, 1892.00f, 2021.00f, 2193.00f, 2365.00f,
        2537.00f, 2709.00f, 2924.00f, 3139.00f, 3354.00f, 3612.00f, 3870.00f, 4128.00f,
        4429.00f, 4730.00f, 5074.00f, 5418.00f, 5805.00f, 6192.00f, 6622.00f, 7052.00f,
        7525.00f, 7998.00f, 8514.00f, 9030.00f, 9589.00f, 10148.00f, 10750.00f, 11352.00f,
        11568.00f, 11783.00f, 11997.00f, 12210.00f, 12423.00f, 12636.00f, 12850.00f, SPECTRUM_MAX_HZ};

    // Logarithmische Bandgrenzen von 86 Hz bis 12987 Hz
    static constexpr float CENTERED_CUTOFF_FREQS[] = {
        SPECTRUM_MIN_HZ, 98.8613f, 113.6461f, 130.6419f, 150.1795f,
        172.6389f, 198.4572f, 228.1366f, 262.2545f, 301.4748f,
        346.5605f, 398.3889f, 457.9681f, 526.4575f, 605.1895f,
        695.6959f, 799.7376f, 919.3388f, 1056.8264f, 1214.8753f,
        1396.5606f, 1605.4170f, 1845.5079f, 2121.5046f, 2438.7767f,
        2803.4971f, 3222.7616f, 3704.7273f, 4258.7712f, 4895.6727f,
        5627.8232f, 6469.4671f, 7436.9793f, 8549.1835f, 9827.7185f,
        11297.4590f, SPECTRUM_MAX_HZ};
    static_assert(sizeof(CENTERED_CUTOFF_FREQS) / sizeof(CENTERED_CUTOFF_FREQS[0]) == CENTERED_BAND_COUNT + 1,
                  "Die Anzahl der Bandgrenzen muss zur Anzahl der mittigen Balken passen");
    const float *cutoffFreqs = centered ? CENTERED_CUTOFF_FREQS : BAND_CUTOFF_FREQS;

    if (init)
    {
        if (!m_sprVU_Spectrum)
        {
            if (!m_tft->safeCreateSprite(m_sprVU_Spectrum, VIS_AREA_W, VIS_AREA_H))
            {
                LOG_ERROR("Konnte Spektrum-Sprite nicht erstellen!");
                return;
            }
            m_sprVU_Spectrum->setFont(&fonts::DejaVu9);
            m_sprVU_Spectrum->setTextColor(TFT_SILVER);
        }

        clearVisualizationArea();

        if (!m_fft_cfg)
            m_fft_cfg = kiss_fftr_alloc(FFT_N, 0, nullptr, nullptr);

        if (!m_fftWindowInitialized)
        {
            for (int i = 0; i < FFT_N; i++)
                m_fft_window_lut[i] = 0.5f * (1.0f - cosf(6.28318530718f * i / (FFT_N - 1)));

            m_fftWindowInitialized = true;
        }

        if (!m_fft_cfg)
        {
            LOG_ERROR("FFT-Konfiguration konnte nicht erstellt werden!");
            return;
        }

        // Ein Moduswechsel ändert Bandzuordnung und Höhenmaßstab auch bei gleicher Abtastrate.
        m_spectrumSampleRate = 0;
        m_spectrumHadSamples = false;
        m_spectrumTargets.fill(0.0f);
        for (int band = 0; band < bandCount; ++band)
            m_spectrum_bands[band] = {0.0f, 0.0f, 0, 0, 0};
        for (int band = 0; band < CENTERED_BAND_COUNT; ++band)
            m_centeredSpectrumColors[band] = rainbowColors(band * (BAND_COUNT - 1) * 2 / (CENTERED_BAND_COUNT - 1));
        m_centeredSpectrumDirty = true;
        m_lastSpectrumUpdate = now;

        m_sprVU_Spectrum->clear(TFT_BLACK);
        if (waterfall)
        {
            m_waterfallStyle = static_cast<SpectrumStyle>(APP_STATE.getWaterfallStyle());
            drawSpectrumLabels(cutoffFreqs, bandCount);
            drawWaterfall(true, false, false, 0);
            return;
        }
        m_sprVU_Spectrum->pushSprite(VIS_AREA_X, VIS_AREA_Y);
        return;
    }

    if (!m_fft_cfg || !m_sprVU_Spectrum)
        return;

    const float dt = elapsed * 0.001f;
    m_lastSpectrumUpdate = waterfall ? now - elapsed % WATERFALL_FRAME_MS : now;
    uint32_t sampleRate;
    bool hasSamples;
    bool analyze;
    {
        // Der Writer wird nur während der Sample-Übernahme am Puffertausch gehindert.
        ReaderGuard guard(m_readerActive);
        const uint8_t readIdx = m_writeBufferIdx.load(std::memory_order_acquire) ^ 1;
        const int16_t *samplesL = m_samplesL[readIdx].get();
        const int16_t *samplesR = m_samplesR[readIdx].get();
        const uint32_t sampleSeq = m_sampleSeq.load(std::memory_order_acquire);
        sampleRate = m_sampleRate[readIdx];
        const uint16_t sampleCount = m_sampleCount[readIdx];
        hasSamples = samplesL && samplesR && sampleCount > 0 && sampleRate > 0 && APP_STATE.getStationSetDone();
        analyze = (!centered && !waterfall) || sampleSeq != m_spectrumAnalyzedSeq ||
                  sampleRate != m_spectrumSampleRate || hasSamples != m_spectrumHadSamples;

        if (analyze && hasSamples)
        {
            const uint16_t numSamples = (sampleCount > FFT_N) ? FFT_N : sampleCount;
            const uint16_t start = sampleCount - numSamples;
            for (int i = 0; i < numSamples; ++i)
            {
                const int idx = start + i;
                const float mono = 0.5f * (samplesL[idx] + samplesR[idx]);
                m_fft_input[i] = mono * m_fft_window_lut[i];
            }
            if (numSamples < FFT_N)
                memset(m_fft_input.data() + numSamples, 0, (FFT_N - numSamples) * sizeof(kiss_fft_scalar));
        }
        m_spectrumAnalyzedSeq = sampleSeq;
    }
    m_spectrumHadSamples = hasSamples;

    if (sampleRate != m_spectrumSampleRate)
    {
        const float freqToBin = sampleRate ? static_cast<float>(FFT_N) / sampleRate : 0.0f;
        const float nyquist = sampleRate * 0.5f;
        for (int band = 0; !waterfall && band <= bandCount; ++band)
        {
            if (centered)
            {
                // Jede FFT-Stützstelle vertritt eine Binbreite um ihre Mittenfrequenz.
                const float binEdge = fminf(cutoffFreqs[band], nyquist) * freqToBin + 0.5f;
                m_centeredBandEdges[band] = binEdge;
                m_band_bin_indices_lut[band] = static_cast<int>(binEdge);
            }
            else
            {
                m_band_bin_indices_lut[band] = (cutoffFreqs[band] < nyquist)
                                                  ? static_cast<int>(ceilf(cutoffFreqs[band] * freqToBin))
                                                  : FFT_BIN_COUNT;
            }
        }
        for (int band = 0; band < bandCount; ++band)
            m_spectrum_bands[band] = {0.0f, 0.0f, 0, 0, 0};
        m_spectrumSampleRate = sampleRate;
        m_centeredSpectrumDirty = true;
    }

    if (analyze && hasSamples)
        kiss_fftr(m_fft_cfg, m_fft_input.data(), m_fft_output.data());

    if (waterfall)
    {
        drawWaterfall(false, analyze, hasSamples, elapsed);
        return;
    }

    Gfx_Sprite *current_sprite = m_sprVU_Spectrum;
    const float ALPHA_UP = 1.0f - expf(-dt / 0.01f);
    const float ALPHA_DOWN = 1.0f - expf(-dt / 0.25f);
    const float PEAK_FALL_SPEED = (centered ? CENTERED_PEAK_FALL_RATE : CLASSIC_PEAK_FALL_RATE) * dt;

    // Der Bezugspegel verschiebt das dB-Fenster; der Lautstärkeregler verändert die Analyse nicht.
    const float DBHIGH = vuAlignmentDb() + SPECTRUM_TOP_OVER_REFERENCE;
    const float DBLOW = DBHIGH - SPECTRUM_SPAN_DB;
    const float DB_RANGE_INV = 1.0f / (DBHIGH - DBLOW);
    const float OFFSET = 10.0f * log10f(SPECTRUM_SCALE_FACTOR * SPECTRUM_SCALE_FACTOR / FFT_N);

    uint16_t *buffer = static_cast<uint16_t *>(current_sprite->getBuffer());
    if (!buffer)
        return;
    const int sprite_width = current_sprite->width();
    bool changed = m_centeredSpectrumDirty;

    for (int band = 0; band < bandCount; ++band)
    {
        if (analyze)
        {
            float bandPower = 0.0f;
            if (hasSamples)
            {
                const int firstBin = m_band_bin_indices_lut[band];
                const int endBin = m_band_bin_indices_lut[band + 1] + (centered ? 1 : 0);
                for (int bin = firstBin; bin < endBin; ++bin)
                {
                    float weight = 1.0f;
                    if (centered)
                    {
                        // Rand-Bins liefern nur den vom Band überdeckten Anteil ihrer Leistung.
                        if (bin == firstBin)
                            weight -= m_centeredBandEdges[band] - bin;
                        if (bin == endBin - 1)
                            weight -= bin + 1 - m_centeredBandEdges[band + 1];
                    }
                    const float re = m_fft_output[bin].r;
                    const float im = m_fft_output[bin].i;
                    bandPower += (re * re + im * im) * weight;
                }
            }

            float normalized = 0.0f;
            if (bandPower > 1e-10f)
            {
                const float magnitudeDb = 10.0f * log10f(bandPower) + OFFSET;
                normalized = (magnitudeDb - DBLOW) * DB_RANGE_INV;
                if (normalized < 0.0f)
                    normalized = 0.0f;
                else if (normalized > 1.0f)
                    normalized = 1.0f;
            }
            m_spectrumTargets[band] = normalized;
        }

        auto &b = m_spectrum_bands[band];
        const uint16_t previousPos = b.currentPos;
        const uint16_t previousPeak = b.peakPos;
        const float normalized = m_spectrumTargets[band];
        b.smoothedMagnitude += (normalized - b.smoothedMagnitude) * ((normalized > b.smoothedMagnitude) ? ALPHA_UP : ALPHA_DOWN);
        b.currentPos = static_cast<uint16_t>(b.smoothedMagnitude * graphHeight);

        const float cur = b.currentPos;
        if (cur >= b.peakPosFloat)
        {
            b.peakPosFloat = cur;
            b.peakTime = now;
        }
        else if (now - b.peakTime > PEAK_HOLD_MS)
        {
            b.peakPosFloat = fmaxf(cur, b.peakPosFloat - PEAK_FALL_SPEED);
        }
        b.peakPos = static_cast<uint16_t>(fmaxf(1.0f, b.peakPosFloat));
        changed = changed || previousPos != b.currentPos || previousPeak != b.peakPos;

        if (centered)
            continue;

        const int x = band * (BIN_WIDTH + BIN_GAP);
        const int bar_top = VIS_AREA_H - b.currentPos;
        const int peak_y = VIS_AREA_H - b.peakPos;
        uint16_t barColor = BAR_GREEN;
        uint16_t peakColor = TFT_SILVER;
        if (m_spectrumStyle == SpectrumStyle::RAINBOW)
            barColor = peakColor = rainbowColors(band * 2);

        for (int y = 0; y < VIS_AREA_H; ++y)
        {
            uint16_t color;
            if (y == peak_y)
                color = peakColor;
            else if (y < bar_top)
                color = (y % 2 == 0) ? 0x2104 : TFT_BLACK;
            else
                color = (y % 2 == 0) ? barColor : 0x2104;

            uint16_t *line = buffer + y * sprite_width + x;
            for (int i = 0; i < BIN_WIDTH; ++i)
                line[i] = bswap16(color);
        }
    }

    if (centered)
    {
        if (!changed)
            return;

        current_sprite->clear(TFT_BLACK);
        constexpr int CENTER_Y = SPECTRUM_LABEL_H + CENTERED_GRAPH_H / 2; ///< Mittellinie des Balkenfelds
        for (int band = 0; band < bandCount; ++band)
        {
            const auto &b = m_spectrum_bands[band];
            const int x = band * (VIS_AREA_W + CENTERED_BAR_GAP) / bandCount;
            const int nextX = (band + 1) * (VIS_AREA_W + CENTERED_BAR_GAP) / bandCount;
            const int width = nextX - x - CENTERED_BAR_GAP;
            const int halfHeight = b.currentPos > 1 ? b.currentPos / 2 : 1;
            const uint16_t barColor = m_centeredSpectrumStyle == SpectrumStyle::RAINBOW ? m_centeredSpectrumColors[band] : BAR_GREEN;
            const uint16_t peakColor = m_centeredSpectrumStyle == SpectrumStyle::RAINBOW ? barColor : TFT_SILVER;
            current_sprite->fillRect(x, CENTER_Y - halfHeight, width, halfHeight * 2 + 1, barColor);
            const int peakHalfHeight = b.peakPos > 1 ? b.peakPos / 2 : 1;
            int peakY = CENTER_Y - peakHalfHeight - CENTERED_PEAK_H - CENTERED_PEAK_GAP;
            if (peakY < SPECTRUM_LABEL_H)
                peakY = SPECTRUM_LABEL_H;
            current_sprite->fillRect(x, peakY, width, CENTERED_PEAK_H, peakColor);
        }
        m_centeredSpectrumDirty = false;
    }

    drawSpectrumLabels(cutoffFreqs, bandCount);
    current_sprite->pushSprite(centered ? VIS_AREA_X : VIS_AREA_X + 1, VIS_AREA_Y);
}

/**
 * @brief Zeichnet die Frequenzmarken des aktiven Spektrums
 */
void Visualizer::drawSpectrumLabels(const float *cutoffFreqs, int bandCount)
{
    constexpr uint16_t LABEL_FREQUENCIES[] = {125, 500, 1000, 2000, 4000, 8000, 12000}; ///< Frequenzmarken innerhalb der Bandgrenzen in Hz
    const bool centered = m_visMode == VisMode::VIS_SPECTRUM_CENTERED;
    const bool waterfall = m_visMode == VisMode::VIS_WATERFALL;
    Gfx_Sprite *current_sprite = m_sprVU_Spectrum;
    const int labelY = (centered || waterfall) ? 0 : 6;
    for (const uint16_t frequency : LABEL_FREQUENCIES)
    {
        int band = 0;
        while (band < bandCount - 1 && frequency >= cutoffFreqs[band + 1])
            ++band;
        const float fraction = (frequency - cutoffFreqs[band]) / (cutoffFreqs[band + 1] - cutoffFreqs[band]);
        const float bandWidth = centered ? static_cast<float>(VIS_AREA_W + CENTERED_BAR_GAP) / bandCount : BIN_WIDTH + BIN_GAP;
        const int centerX = waterfall
                                ? static_cast<int>(VIS_AREA_W * logf(frequency / cutoffFreqs[0]) / logf(cutoffFreqs[bandCount] / cutoffFreqs[0]))
                                : static_cast<int>((band + fraction) * bandWidth);
        char label[8];
        if (frequency < 1000)
            stringutils::safeFormat(label, "%u", static_cast<unsigned>(frequency));
        else
            stringutils::safeFormat(label, "%uk", static_cast<unsigned>(frequency / 1000));
        const int width = current_sprite->textWidth(label);
        int x = centerX - width / 2;
        if (x < 0)
            x = 0;
        else if (x > VIS_AREA_W - width)
            x = VIS_AREA_W - width;
        current_sprite->drawString(label, x, labelY);
    }
}

/**
 * @brief Schiebt den Spektrumverlauf zeitgesteuert nach unten und ergänzt die aktuelle Farbzeile
 */
void Visualizer::drawWaterfall(bool init, bool analyze, bool hasSamples, uint32_t elapsed)
{
    constexpr int GRAPH_H = VIS_AREA_H - SPECTRUM_LABEL_H; ///< Anzahl der sichtbaren Zeitschritte
    constexpr int SMOOTHING_BITS = 8; ///< Nach acht Halbierungen ist jede Differenz der 8-Bit-Pegelstufen abgebaut
    constexpr float INTERPOLATION_MAX_WIDTH = 0.25f; ///< Interpolation nur bei mehr als vier Pixeln je FFT-Bin
    constexpr float INTERPOLATION_FULL_WIDTH = 0.125f; ///< Vollständige Interpolation ab acht Pixeln je FFT-Bin
    constexpr float SPAN_DB = 48.0f; ///< Dynamikumfang der Wasserfall-Farbskala
    constexpr uint16_t GREEN = 0x6c64; ///< Oberer Pegel der grünen Farbskala
    static constexpr uint16_t COLOR_STOPS[] = {TFT_BLACK, 0x0010, 0x801F, TFT_RED, TFT_YELLOW, TFT_WHITE}; ///< Pegelfarben von Stille bis zum höchsten Farbpegel
    constexpr int COLOR_SEGMENTS = sizeof(COLOR_STOPS) / sizeof(COLOR_STOPS[0]) - 1; ///< Anzahl der Farbübergänge

    if (!m_sprVU_Spectrum)
        return;

    if (init && !m_waterfall)
    {
        void *mem = heap_caps_malloc(sizeof(WaterfallData), MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
        if (!mem)
            mem = malloc(sizeof(WaterfallData));
        if (!mem)
        {
            LOG_ERROR("Konnte Wasserfalldaten nicht allokieren!");
            return;
        }
        memset(mem, 0, sizeof(WaterfallData));
        m_waterfall.reset(static_cast<WaterfallData *>(mem));
    }
    if (!m_waterfall)
        return;

    auto &data = *m_waterfall;
    auto *buffer = static_cast<uint16_t *>(m_sprVU_Spectrum->getBuffer());
    if (!buffer)
        return;
    const int stride = m_sprVU_Spectrum->width();
    uint16_t *graph = buffer + SPECTRUM_LABEL_H * stride;

    if (init)
    {
        data.targetLevels.fill(0);
        data.displayLevels.fill(0);
        data.activeRows = 0;
        data.sampleRate = 0;
        for (size_t i = 0; i < data.colors.size(); ++i)
        {
            const float level = static_cast<float>(i) / (data.colors.size() - 1);
            uint16_t color;
            if (m_waterfallStyle == SpectrumStyle::CLASSIC)
            {
                color = blendColor(GREEN, TFT_BLACK, level);
            }
            else
            {
                const float position = level * COLOR_SEGMENTS;
                const int segment = static_cast<int>(position);
                color = segment < COLOR_SEGMENTS
                            ? blendColor(COLOR_STOPS[segment + 1], COLOR_STOPS[segment], position - segment)
                            : COLOR_STOPS[COLOR_SEGMENTS];
            }
            data.colors[i] = bswap16(color);
        }
        memset(graph, 0, GRAPH_H * stride * sizeof(uint16_t));
        m_sprVU_Spectrum->pushSprite(VIS_AREA_X, VIS_AREA_Y);
        return;
    }

    if (data.sampleRate != m_spectrumSampleRate)
    {
        const float freqToBin = m_spectrumSampleRate ? static_cast<float>(FFT_N) / m_spectrumSampleRate : 0.0f;
        const float nyquist = m_spectrumSampleRate * 0.5f;
        const float logRange = logf(SPECTRUM_MAX_HZ / SPECTRUM_MIN_HZ);
        for (int x = 0; x <= VIS_AREA_W; ++x)
        {
            const float frequency = SPECTRUM_MIN_HZ * expf(logRange * x / VIS_AREA_W);
            data.binEdges[x] = fminf(frequency, nyquist) * freqToBin + 0.5f;
        }
        data.sampleRate = m_spectrumSampleRate;
    }

    if (analyze)
    {
        const float dbHigh = vuAlignmentDb() + SPECTRUM_TOP_OVER_REFERENCE;
        const float dbLow = dbHigh - SPAN_DB;
        const float offset = 10.0f * log10f(SPECTRUM_SCALE_FACTOR * SPECTRUM_SCALE_FACTOR / FFT_N);
        for (int x = 0; x < VIS_AREA_W; ++x)
        {
            float power = 0.0f;
            if (hasSamples)
            {
                const float left = data.binEdges[x];
                const float right = data.binEdges[x + 1];
                const float width = right - left;
                if (width > 0.0f)
                {
                    const int firstBin = static_cast<int>(left);
                    const int lastBin = static_cast<int>(right);
                    for (int bin = firstBin; bin <= lastBin; ++bin)
                    {
                        // Auch ein schmaler Pixel erhält den überdeckten Anteil seiner FFT-Stützstelle.
                        const float weight = fminf(right, bin + 1.0f) - fmaxf(left, static_cast<float>(bin));
                        const float re = m_fft_output[bin].r;
                        const float im = m_fft_output[bin].i;
                        power += (re * re + im * im) * weight;
                    }
                    // Die mittlere Leistung pro Bin hält den Farbpegel unabhängig von der Pixelbandbreite.
                    power /= width;
                    if (width < INTERPOLATION_MAX_WIDTH)
                    {
                        // Stark vergrößerte FFT-Stützstellen erhalten einen Verlauf zwischen ihren Mitten.
                        const float position = (left + right) * 0.5f - 0.5f;
                        const int lowerBin = static_cast<int>(position);
                        const int upperBin = lowerBin < FFT_N / 2 ? lowerBin + 1 : lowerBin;
                        const float fraction = position - lowerBin;
                        const auto &lower = m_fft_output[lowerBin];
                        const auto &upper = m_fft_output[upperBin];
                        const float lowerPower = lower.r * lower.r + lower.i * lower.i;
                        const float upperPower = upper.r * upper.r + upper.i * upper.i;
                        const float interpolatedPower = lowerPower * (1.0f - fraction) + upperPower * fraction;
                        // Zwischen acht und vier Pixeln je FFT-Bin geht die Interpolation stufenlos in die Bandmittelung über.
                        const float interpolation = fminf(1.0f, (INTERPOLATION_MAX_WIDTH - width) / (INTERPOLATION_MAX_WIDTH - INTERPOLATION_FULL_WIDTH));
                        power = power * (1.0f - interpolation) + interpolatedPower * interpolation;
                    }
                }
            }
            float level = 0.0f;
            if (power > 1e-10f)
                level = fminf(1.0f, fmaxf(0.0f, (10.0f * log10f(power) + offset - dbLow) / SPAN_DB));
            data.targetLevels[x] = static_cast<uint8_t>(level * (data.colors.size() - 1));
        }
    }

    if (elapsed < WATERFALL_FRAME_MS)
        return;

    const uint32_t steps = elapsed / WATERFALL_FRAME_MS;
    const int rows = steps < GRAPH_H ? static_cast<int>(steps) : GRAPH_H;
    const int smoothingSteps = rows < SMOOTHING_BITS ? rows : SMOOTHING_BITS;
    bool rowVisible = false;
    for (int x = 0; x < VIS_AREA_W; ++x)
    {
        // Pro 50 ms halbiert sich die Pegeldifferenz; die Rundung zum Ziel beendet auch den Ausklang bei Stille.
        const int target = data.targetLevels[x];
        const int previous = data.displayLevels[x];
        const int distance = previous > target ? previous - target : target - previous;
        const int remainder = distance >> smoothingSteps;
        data.displayLevels[x] = target + (previous > target ? remainder : -remainder);
        rowVisible = rowVisible || data.colors[data.displayLevels[x]] != 0;
    }
    if (!rowVisible && data.activeRows == 0)
        return;

    memmove(graph + rows * stride, graph, (GRAPH_H - rows) * stride * sizeof(uint16_t));
    // Ausgelassene Zeitschritte bleiben leer; die Höhe des Verlaufs bleibt an die reale Zeit gebunden.
    memset(graph, 0, rows * stride * sizeof(uint16_t));
    for (int x = 0; x < VIS_AREA_W; ++x)
        graph[x] = data.colors[data.displayLevels[x]];
    data.activeRows = rowVisible ? GRAPH_H : (data.activeRows > rows ? data.activeRows - rows : 0);
    m_sprVU_Spectrum->pushSprite(VIS_AREA_X, VIS_AREA_Y);
}

// ===================================================================
// Nordlicht
// ===================================================================

/**
 * @brief Zeichnet weiche Nordlicht-Schleier mit langsamer Pegel- und Farbüberblendung
 */
void Visualizer::drawAurora(bool init, float rmsLeft, float rmsRight)
{
    constexpr uint32_t FRAME_MS = 33; ///< Höchstens etwa 30 Bilder pro Sekunde
    constexpr uint32_t SLICE_US = 1500; ///< Rechenbudget je Aufruf, nach jeder Spalte geprüft
    constexpr int COLUMNS_PER_SLICE = 32; ///< Höchstens 32 Spalten je Aufruf
    constexpr uint32_t MAX_FRAME_MS = 100; ///< Begrenzter Bewegungsschritt nach einer Zeichenpause
    constexpr float TAU = 6.28318530718f; ///< Vollkreis für die Wellenphasen
    constexpr float AUDIO_TIME_S = 4.0f; ///< Zeitkonstante der Audioreaktion
    constexpr float COLOR_TIME_S = 1.5f; ///< Zeitkonstante des Farbwechsels
    constexpr float FADE_TIME_S = 0.8f; ///< Zeitkonstante der Einblendung
    constexpr float AUDIO_LOW_DB = -50.0f; ///< Untere Grenze der sanften Musiksteuerung
    constexpr float AUDIO_HIGH_DB = -8.0f; ///< Obere Grenze der sanften Musiksteuerung
    constexpr float BASE_BRIGHTNESS = 0.40f; ///< Grundhelligkeit bei Stille
    constexpr float AUDIO_BRIGHTNESS = 0.25f; ///< Maximaler Helligkeitsbeitrag der Musik
    constexpr float BREATH_DEPTH = 0.15f; ///< Langsame Helligkeitsschwankung jedes Schleiers
    constexpr float AUDIO_WIDTH = 0.18f; ///< Maximale relative Verbreiterung durch den Musikpegel
    constexpr float CURVE_HEIGHT = 0.16f; ///< Höhe der langsamen Hauptwelle relativ zur Zeichenfläche
    constexpr float FOLD_HEIGHT = 0.055f; ///< Höhe der zweiten, flacheren Welle
    constexpr float EDGE_WIDTH = 24.0f; ///< Weiche Ausblendung an den seitlichen Rändern in Pixeln
    constexpr float PROFILE_SIGMAS = 4.0f; ///< Reichweite des vorberechneten Leuchtprofils
    constexpr int POSITION_SCALE = 256; ///< Acht Nachkommabits für die Lage und Breite der Schleier
    constexpr int COLOR_SCALE = 16; ///< Vier Nachkommabits für die RGB-Koeffizienten
    constexpr int GLOW_MAX = 32767; ///< 15 Nachkommabits für die Leuchtstärke
    static constexpr uint8_t DITHER[4][4] = {{0, 8, 2, 10}, {12, 4, 14, 6}, {3, 11, 1, 9}, {15, 7, 13, 5}}; ///< Ortsfestes Raster für Zwischenstufen im RGB565-Format

    /**
     * @brief Form, Bewegung und beide Farbwelten eines Schleiers
     */
    struct Layer
    {
        float center; ///< Mittlere Höhenlage als Anteil der Zeichenfläche
        float width; ///< Breite des Leuchtprofils in Pixeln
        float cycles[2]; ///< Räumliche Wellenanzahl über die gesamte Breite
        float periods[2]; ///< Bewegungsperioden in Sekunden
        float start[2]; ///< Phasen beim Einblenden in Radiant
        uint8_t cool[3]; ///< Kühle RGB-Farbe mit acht Bit pro Kanal
        uint8_t warm[3]; ///< Warme RGB-Farbe mit acht Bit pro Kanal
    };
    static constexpr Layer LAYERS[AURORA_LAYERS] = {
        {0.32f, 9.0f, {0.85f, 1.8f}, {12.0f, 15.0f}, {0.4f, 2.1f}, {22, 170, 136}, {185, 92, 25}},
        {0.50f, 11.0f, {1.1f, 2.2f}, {15.0f, 11.0f}, {2.5f, 4.3f}, {22, 70, 162}, {140, 45, 34}},
        {0.66f, 8.0f, {0.7f, 1.6f}, {9.0f, 14.0f}, {4.7f, 0.8f}, {112, 50, 145}, {148, 76, 69}}}; ///< Drei unabhängig bewegte Farbschleier

    if (APP_STATE.getBlockGFX() || !m_tft)
        return;

    if (init)
    {
        if (!m_sprAurora)
        {
            if (!m_tft->safeCreateSprite(m_sprAurora, VIS_AREA_W, VIS_AREA_H))
            {
                LOG_ERROR("Konnte Nordlicht-Sprite nicht erstellen!");
                return;
            }
        }
        if (!m_aurora)
        {
            void *mem = heap_caps_malloc(sizeof(AuroraData), MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
            if (!mem)
                mem = malloc(sizeof(AuroraData));
            if (!mem)
            {
                LOG_ERROR("Konnte Nordlicht-Daten nicht allokieren!");
                return;
            }
            memset(mem, 0, sizeof(AuroraData));
            m_aurora.reset(static_cast<AuroraData *>(mem));
            for (int i = 0; i <= AURORA_GLOW_STEPS; ++i)
            {
                const float distance = PROFILE_SIGMAS * i / AURORA_GLOW_STEPS;
                m_aurora->glow[i] = static_cast<uint16_t>(GLOW_MAX * expf(-0.5f * distance * distance));
            }
            m_aurora->glow[AURORA_GLOW_STEPS] = 0;
            for (int y = 0; y < VIS_AREA_H; ++y)
            {
                const float envelope = sinf(0.5f * TAU * y / (VIS_AREA_H - 1));
                m_aurora->edgeFade[y] = static_cast<uint16_t>(POSITION_SCALE * envelope * envelope);
            }
        }
        m_auroraStyle = static_cast<AuroraStyle>(APP_STATE.getAuroraStyle());
        for (int layer = 0; layer < AURORA_LAYERS; ++layer)
        {
            for (int wave = 0; wave < 2; ++wave)
                m_aurora->phase[layer][wave] = LAYERS[layer].start[wave];
        }
        m_aurora->audioLevel = 0.0f;
        m_aurora->warmMix = m_auroraStyle == AuroraStyle::WARM ? 1.0f : 0.0f;
        m_aurora->fadeIn = 0.0f;
        m_aurora->lastFrame = millis();
        m_aurora->rendering = false;
        m_aurora->nextColumn = 0;
        clearVisualizationArea();
    }
    if (!m_sprAurora || !m_aurora)
        return;

    auto &data = *m_aurora;
    const uint32_t now = millis();
    if (!init && m_activeModule == AppModule::RETRO_RADIO && !APP_STATE.getStationSetDone())
    {
        // Während der Zeigerfahrt bleibt das Bild stehen; die verstrichene Zeit wird beim Fortsetzen nicht nachgeholt.
        data.lastFrame = now;
        data.rendering = false;
        data.nextColumn = 0;
        return;
    }
    const uint32_t sliceStart = micros();
    if (!data.rendering)
    {
        const uint32_t elapsed = now - data.lastFrame;
        if (!init && elapsed < FRAME_MS)
            return;
        data.lastFrame = now;
        const float dt = (elapsed < MAX_FRAME_MS ? elapsed : MAX_FRAME_MS) * 0.001f;
        float targetLevel = 0.0f;
        if (APP_STATE.getStationSetDone())
        {
            const float db = amplitudeToDb(fmaxf(rmsLeft, rmsRight));
            targetLevel = fminf(1.0f, fmaxf(0.0f, (db - AUDIO_LOW_DB) / (AUDIO_HIGH_DB - AUDIO_LOW_DB)));
        }
        data.audioLevel += (targetLevel - data.audioLevel) * (1.0f - expf(-dt / AUDIO_TIME_S));
        const float targetWarmth = m_auroraStyle == AuroraStyle::WARM ? 1.0f : 0.0f;
        data.warmMix += (targetWarmth - data.warmMix) * (1.0f - expf(-dt / COLOR_TIME_S));
        data.fadeIn += (1.0f - data.fadeIn) * (1.0f - expf(-dt / FADE_TIME_S));

        for (int layer = 0; layer < AURORA_LAYERS; ++layer)
        {
            for (int wave = 0; wave < 2; ++wave)
            {
                data.phase[layer][wave] += TAU * dt / LAYERS[layer].periods[wave];
                if (data.phase[layer][wave] >= TAU)
                    data.phase[layer][wave] -= TAU;
                data.waveSin[layer][wave] = sinf(data.phase[layer][wave]);
                data.waveCos[layer][wave] = cosf(data.phase[layer][wave]);
                const float step = TAU * LAYERS[layer].cycles[wave] / (VIS_AREA_W - 1);
                data.stepSin[layer][wave] = sinf(step);
                data.stepCos[layer][wave] = cosf(step);
            }
            const float brightness = data.fadeIn * (BASE_BRIGHTNESS + AUDIO_BRIGHTNESS * data.audioLevel) * (1.0f - BREATH_DEPTH + BREATH_DEPTH * data.waveSin[layer][1]);
            const float width = LAYERS[layer].width * (1.0f + AUDIO_WIDTH * data.audioLevel);
            data.profileScale[layer] = static_cast<int>(POSITION_SCALE * AURORA_GLOW_STEPS / (PROFILE_SIGMAS * width));
            for (int channel = 0; channel < 3; ++channel)
                data.colors[layer][channel] = COLOR_SCALE * brightness * (LAYERS[layer].cool[channel] + data.warmMix * (LAYERS[layer].warm[channel] - LAYERS[layer].cool[channel]));
        }
        data.nextColumn = 0;
        data.rendering = true;
    }

    if (data.nextColumn == VIS_AREA_W)
    {
        // Die Bildübertragung belegt einen eigenen Aufruf ohne weitere Pixelberechnung.
        m_sprAurora->pushSprite(VIS_AREA_X, VIS_AREA_Y);
        data.rendering = false;
        return;
    }

    auto *buffer = static_cast<uint16_t *>(m_sprAurora->getBuffer());
    if (!buffer)
        return;
    const int stride = m_sprAurora->width();
    int columns = 0;
    while (data.nextColumn < VIS_AREA_W)
    {
        const int x = data.nextColumn;
        const int edgeDistance = x < VIS_AREA_W - 1 - x ? x : VIS_AREA_W - 1 - x;
        const float edge = fminf(1.0f, edgeDistance / EDGE_WIDTH);
        const float edgeFade = edge * edge * (3.0f - 2.0f * edge);
        int center[AURORA_LAYERS];
        uint16_t columnColors[AURORA_LAYERS][3];
        for (int layer = 0; layer < AURORA_LAYERS; ++layer)
        {
            const float height = LAYERS[layer].center + CURVE_HEIGHT * data.waveSin[layer][0] + FOLD_HEIGHT * data.waveSin[layer][1];
            center[layer] = static_cast<int>((VIS_AREA_H - 1) * POSITION_SCALE * height);
            for (int channel = 0; channel < 3; ++channel)
                columnColors[layer][channel] = static_cast<uint16_t>(data.colors[layer][channel] * edgeFade);
            // Die Additionstheoreme verschieben beide Wellen ohne trigonometrische Aufrufe im Pixelpfad.
            for (int wave = 0; wave < 2; ++wave)
            {
                const float sine = data.waveSin[layer][wave];
                const float cosine = data.waveCos[layer][wave];
                data.waveSin[layer][wave] = sine * data.stepCos[layer][wave] + cosine * data.stepSin[layer][wave];
                data.waveCos[layer][wave] = cosine * data.stepCos[layer][wave] - sine * data.stepSin[layer][wave];
            }
        }
        for (int y = 0; y < VIS_AREA_H; ++y)
        {
            uint32_t red = 0, green = 0, blue = 0;
            for (int layer = 0; layer < AURORA_LAYERS; ++layer)
            {
                int distance = y * POSITION_SCALE - center[layer];
                if (distance < 0)
                    distance = -distance;
                const int index = (distance * data.profileScale[layer]) >> 16;
                if (index >= AURORA_GLOW_STEPS)
                    continue;
                const uint32_t glow = (static_cast<uint32_t>(data.glow[index]) * data.edgeFade[y]) >> 8;
                red += glow * columnColors[layer][0];
                green += glow * columnColors[layer][1];
                blue += glow * columnColors[layer][2];
            }
            // RGB-Koeffizienten mit vier und Leuchtwerte mit 15 Nachkommabits ergeben 19 Nachkommabits vor der RGB565-Quantisierung.
            const uint32_t dither = 2 * DITHER[y & 3][x & 3] + 1;
            red = (red + (dither << 17)) >> 22;
            green = (green + (dither << 16)) >> 21;
            blue = (blue + (dither << 17)) >> 22;
            if (red > 31)
                red = 31;
            if (green > 63)
                green = 63;
            if (blue > 31)
                blue = 31;
            buffer[y * stride + x] = bswap16(static_cast<uint16_t>((red << 11) | (green << 5) | blue));
        }
        ++data.nextColumn;
        ++columns;
        if (columns >= COLUMNS_PER_SLICE || static_cast<uint32_t>(micros() - sliceStart) >= SLICE_US)
            return;
    }
}

// ===================================================================
// Wellenform-Anzeige
// ===================================================================

/**
 * @brief Wellenform-Anzeige
 */
void Visualizer::drawWave(bool init)
{
    if (APP_STATE.getBlockGFX())
        return;

    // RAII-Guard: Signalisiert dem Writer, dass gerade gelesen wird
    ReaderGuard guard(m_readerActive);

    // Lese-Puffer ermitteln (Double-Buffer: der NICHT gerade beschrieben wird)
    const uint8_t readIdx = m_writeBufferIdx.load(std::memory_order_acquire) ^ 1;
    const int16_t *samplesL = m_samplesL[readIdx].get();
    const int16_t *samplesR = m_samplesR[readIdx].get();
    uint16_t sampleCount = m_sampleCount[readIdx];

    // Defensive: PSRAM-Allokation könnte fehlgeschlagen sein
    if (!samplesL || !samplesR)
        return;

    if (!APP_STATE.getStationSetDone())
        sampleCount = 0;

    // Initialisierung
    if (init)
    {
    // Ein Puffer genuegt. Ein zweiter lohnt nur, solange eine Uebertragung noch laeuft;
    // beim RGB-Panel liegt der Bildspeicher im PSRAM, und pushSprite ist ein Kopiervorgang.
        if (!m_sprVU_Wave)
        {
            if (!m_tft->safeCreateSprite(m_sprVU_Wave, VIS_AREA_W, VIS_AREA_H))
            {
                LOG_ERROR("Konnte Wave Sprite nicht erstellen!");
                return;
            }
        }

        // Visualisierungshintergrund löschen
        clearVisualizationArea();

        // Erstes Frame mit Mittellinie
        Gfx_Sprite *spr = m_sprVU_Wave;
        spr->clear(Theme::ColorMainBackGround);
        spr->drawFastHLine(0, VIS_AREA_H / 2, VIS_AREA_W, 0x2104);
        spr->pushSprite(VIS_AREA_X, VIS_AREA_Y);

        // Auto-Gain zurücksetzen

        for (uint8_t i = 0; i < WAVE_PEAK_HISTORY_SIZE; i++)
        {
            m_wavePeakHistory[i] = 0.0f;
        }

        m_wavePeakHistoryIdx = 0;
        m_waveSlidingMax = 0.0f;
        m_waveLastPeakUpdate = 0;
        m_waveAmp = 1.0f;
        m_lastWaveUpdateMs = millis();

        return;
    }

    // Fenster: immer nur die letzten 1024 Paare
    constexpr uint16_t WAVE_N = 1024;
    const uint16_t take = (sampleCount >= WAVE_N) ? WAVE_N : sampleCount;
    const uint16_t start = (sampleCount > take) ? (sampleCount - take) : 0;

    Gfx_Sprite *current_sprite = m_sprVU_Wave;
    current_sprite->clear(Theme::ColorMainBackGround);

    if (take < 2)
    {
        current_sprite->pushSprite(VIS_AREA_X, VIS_AREA_Y);
        return;
    }

    // Peak-Tracking der unverstärkten Samples (für Auto-Verstärkung)
    float maxPeak = 0.0f;
    for (uint16_t i = 0; i < take; ++i)
    {
        const uint16_t idx = start + i;
        const float aL = fabsf(samplesL[idx] / 32767.0f);
        const float aR = fabsf(samplesR[idx] / 32767.0f);
        const float m = (aL > aR) ? aL : aR;

        if (m > maxPeak)
            maxPeak = m;
    }

    // Auto-Verstärkung mit gleitendem Fenster
    const uint32_t now = millis();
    m_lastWaveUpdateMs = now;

    // Peak-History alle ~33ms aktualisieren (~30fps)
    if (now - m_waveLastPeakUpdate >= 33)
    {
        m_waveLastPeakUpdate = now;

        m_wavePeakHistory[m_wavePeakHistoryIdx] = maxPeak;
        m_wavePeakHistoryIdx = (m_wavePeakHistoryIdx + 1) % WAVE_PEAK_HISTORY_SIZE;

        // Gleitendes Maximum über die letzten ~3 Sekunden
        m_waveSlidingMax = 0.0f;
        for (uint8_t i = 0; i < WAVE_PEAK_HISTORY_SIZE; i++)
        {
            if (m_wavePeakHistory[i] > m_waveSlidingMax)
                m_waveSlidingMax = m_wavePeakHistory[i];
        }
    }

    // Effektiven Peak verwenden (Sliding Max oder aktueller Peak als Fallback)
    const float effectivePeak = (m_waveSlidingMax > 0.001f) ? m_waveSlidingMax : maxPeak;

    // Auto-Verstärkungs-Regelung (Ziel: 90% Aussteuerung)
    if (effectivePeak > 0.001f)
    {
        const float targetAmp = 0.90f / effectivePeak;

        // Schnell runterregeln bei Übersteuerung
        if (effectivePeak * m_waveAmp > 1.05f)
            m_waveAmp += (targetAmp * 0.95f - m_waveAmp) * 0.3f;
        // Langsam hochregeln bei leisem Signal
        else if (targetAmp > m_waveAmp)
            m_waveAmp += (targetAmp - m_waveAmp) * 0.08f;
        // Mittel runterregeln
        else
            m_waveAmp += (targetAmp - m_waveAmp) * 0.12f;

        // Limits: max. 100× Verstärkung, min. 0.1×
        if (m_waveAmp > 100.0f)
            m_waveAmp = 100.0f;

        if (m_waveAmp < 0.1f)
            m_waveAmp = 0.1f;
    }

    // Zeichnen
    const float xScale = (take > 1) ? (float)(VIS_AREA_W - 1) / (float)(take - 1) : 0.0f;

    if (m_waveformStyle == WaveformStyle::STACKED)
    {
        // Jeder Kanal nutzt eine halbe Höhe
        const float half_h = VIS_AREA_H * 0.5f;
        const float mid_y_L = half_h * 0.5f;    // Mitte oben
        const float mid_y_R = half_h + mid_y_L; // Mitte unten
        const float scale_y = (half_h - 3.0f);  // Nutzt die verfügbare Höhe bis auf einen Rand von 3 px.

        int16_t last_lx = 0, last_ly = (int16_t)mid_y_L;
        int16_t last_rx = 0, last_ry = (int16_t)mid_y_R;

        for (uint16_t i = 0; i < take; ++i)
        {
            const uint16_t idx = start + i;
            const int16_t x = (int16_t)lroundf(i * xScale);

            const float nL = (samplesL[idx] / 32767.0f) * m_waveAmp;
            const float nR = (samplesR[idx] / 32767.0f) * m_waveAmp;

            const int16_t y_l = (int16_t)lroundf(mid_y_L - nL * scale_y);
            const int16_t y_r = (int16_t)lroundf(mid_y_R - nR * scale_y);

            if (x > last_lx)
            {
                current_sprite->drawLine(last_lx, last_ly, x, y_l, TFT_DARKGREEN);
                current_sprite->drawLine(last_rx, last_ry, x, y_r, TFT_DARKGREY);
                last_lx = x;
                last_ly = y_l;
                last_rx = x;
                last_ry = y_r;
            }
            else
            {
                // gleicher Pixel -> nur "Stiftposition" aktualisieren
                last_ly = y_l;
                last_ry = y_r;
            }
        }
    }
    else // OVERLAY
    {
        const float mid_y = VIS_AREA_H * 0.5f;
        const float scale_y = (VIS_AREA_H * 0.5f) - 2.0f;

        int16_t last_x = 0;
        int16_t last_yL = (int16_t)mid_y;
        int16_t last_yR = (int16_t)mid_y;

        for (uint16_t i = 0; i < take; ++i)
        {
            const uint16_t idx = start + i;
            const int16_t x = (int16_t)lroundf(i * xScale);

            const float nL = (samplesL[idx] / 32767.0f) * m_waveAmp;
            const float nR = (samplesR[idx] / 32767.0f) * m_waveAmp;

            const int16_t y_l = (int16_t)lroundf(mid_y - nL * scale_y);
            const int16_t y_r = (int16_t)lroundf(mid_y - nR * scale_y);

            if (x > last_x)
            {
                current_sprite->drawLine(last_x, last_yL, x, y_l, TFT_DARKGREEN);
                current_sprite->drawLine(last_x, last_yR, x, y_r, TFT_DARKGREY);
                last_x = x;
                last_yL = y_l;
                last_yR = y_r;
            }
            else
            {
                last_yL = y_l;
                last_yR = y_r;
            }
        }
    }

    current_sprite->pushSprite(VIS_AREA_X, VIS_AREA_Y);
}

/**
 * @brief Zeichnet die statischen Elemente des Goniometers
 */
void Visualizer::drawGonioStaticElements(Gfx_Sprite *spr, const GonioConfig &cfg)
{
    spr->setFont(&fonts::DejaVu9);

    // Diagonales Kreuz
    const int16_t crossPad = cfg.size * 19 / 126;
    const int16_t crossEnd = cfg.size - crossPad;
    spr->drawLine(crossPad, crossPad, crossEnd, crossEnd, 0x2104);
    spr->drawLine(crossEnd, crossPad, crossPad, crossEnd, 0x2104);

    // Unterbrochene Balken
    const int16_t midStart = cfg.size * 57 / 126;
    const int16_t midEnd = cfg.size * 69 / 126;
    const int16_t barPos = cfg.size * 61 / 126;
    const int16_t barLen = cfg.size * 3 / 126;

    for (int16_t i = 0; i < cfg.size; i += 5)
    {
        if (i < midStart || i > midEnd)
        {
            spr->drawFastHLine(barPos, i, barLen, 0x2104);
            spr->drawFastVLine(i, barPos, barLen, 0x2104);
        }
    }

    // Beschriftungen L / M / R
    spr->setTextColor(Theme::ColorMainText, Theme::ColorMainBackGround);
    const int16_t labelTop = cfg.size * 12 / 126;
    const int16_t labelMidY = cfg.size * 5 / 126;
    spr->drawString("L", cfg.size * 14 / 126, labelTop);
    spr->drawString("M", cfg.center - 5, labelMidY);
    spr->drawString("R", cfg.size * 107 / 126, labelTop);

    // Korrelationswerte -1 / 0 / +1
    spr->setTextColor(Theme::ColorAmber, Theme::ColorMainBackGround);
    const int16_t labelBottom = (cfg.size * 107 / 126) - 2;
    spr->drawString("-1", cfg.size * 5 / 126, labelBottom);
    spr->drawString("0", cfg.corrCenter - 3, labelBottom);
    spr->drawString("+1", (cfg.size * 107 / 126) - 1, labelBottom);

    // Korrelationsbalken (Hintergrund + Mittelmarke)
    const int16_t barX = cfg.size * 5 / 126;
    const int16_t barH = cfg.size * 5 / 126;
    spr->fillRect(barX, cfg.corrBarY, cfg.corrBarW, barH, 0x2104);
    spr->drawFastVLine(cfg.corrCenter, cfg.corrBarY, barH, TFT_GOLD);

    // Rahmen
    spr->drawRect(0, 0, cfg.size, cfg.size, 0x2104);
}

/**
 * @brief Goniometer Anzeige
 */
void Visualizer::drawGonio(bool init, float peakLeft, float peakRight)
{
    if (APP_STATE.getBlockGFX())
        return;

    // RAII-Guard: Signalisiert dem Writer, dass gerade gelesen wird
    ReaderGuard guard(m_readerActive);

    // Lese-Puffer ermitteln
    const uint8_t readIdx = m_writeBufferIdx.load(std::memory_order_acquire) ^ 1;
    const int16_t *samplesL = m_samplesL[readIdx].get();
    const int16_t *samplesR = m_samplesR[readIdx].get();
    uint16_t sampleCount = m_sampleCount[readIdx];

    // Defensive: PSRAM-Allokation könnte fehlgeschlagen sein
    if (!samplesL || !samplesR)
        return;

    if (!APP_STATE.getStationSetDone())
        sampleCount = 0;

    // Ohne neue Abtastwerte entstuende dasselbe Bild noch einmal. Das lohnt nicht: der
    // Zeichenweg kostet gut 7 ms, weil jeder Bildpunkt im PSRAM angefasst werden muss, und
    // dieselbe Schleife bedient auch die Wickeldorne der Kassette. Die eingesparte Zeit kommt
    // also unmittelbar der Laufruhe des uebrigen Bildschirms zugute.
    const uint32_t sampleSeq = m_sampleSeq.load(std::memory_order_acquire);
    if (!init && sampleSeq == m_gonioDrawnSeq)
        return;

    m_gonioDrawnSeq = sampleSeq;

    // Modulabhängige Konfiguration (Position/Größe etc.)
    const GonioConfig &cfg = (m_activeModule == AppModule::RETRO_RADIO)
                                 ? CONFIG_RETRORADIO
                                 : CONFIG_SDPLAYER;

    // Initialisierung
    if (init)
    {
    // Ein Puffer genuegt. Ein zweiter lohnt nur, solange eine Uebertragung noch laeuft;
    // beim RGB-Panel liegt der Bildspeicher im PSRAM, und pushSprite ist ein Kopiervorgang.
        if (!m_sprGonio)
        {
            if (!m_tft->safeCreateSprite(m_sprGonio, cfg.size, cfg.size))
            {
                LOG_ERROR("Konnte m_sprGonio Sprite nicht erstellen!");
                return;
            }
        }

        if (!m_tft->safeCreateSprite(m_sprGonioBg, cfg.size, cfg.size))
        {
            LOG_ERROR("Konnte m_sprGonioBg Sprite nicht erstellen!");
            return;
        }

        // Info-Sprite nur erstellen, wenn nicht vorhanden
        if (!m_sprGonioInfo)
        {
            if (!m_tft->safeCreateSprite(m_sprGonioInfo, 200, 71))
            {
                LOG_ERROR("Konnte m_sprGonioInfo Sprite nicht erstellen!");
                return;
            }
        }

        // Visualisierungshintergrund löschen
        clearVisualizationArea();

        // Info-Sprite zeichnen
        m_sprGonioInfo->clear(0x1082);
        m_sprGonioInfo->loadFont(ComfortaaBold10);

        // PEAK
        m_sprGonioInfo->setTextColor(Theme::ColorRetroRed);
        m_sprGonioInfo->drawString("PEAK", 88, 52);

        // Verstärkungsfaktor: Label statisch zeichnen, Wert-Position dynamisch anhand der Label-Breite ermitteln
        m_sprGonioInfo->setTextColor(TFT_LIGHTGREY);
        m_sprGonioInfo->setTextDatum(textdatum_t::top_left);
        m_sprGonioInfo->drawFastVLine(0, 0, 71, Theme::ColorButtonBorder);

        const __FlashStringHelper *gainLabel = STR(RetroRadioKey::RETRO_VIS_GAIN_LABEL);
        m_sprGonioInfo->drawString(gainLabel, 11, 5);
        const int16_t labelW = m_sprGonioInfo->textWidth(gainLabel);
        m_gainValueX = 11 + labelW + 5;
        m_sprGonioInfo->drawString("x1.0", m_gainValueX, 5);

        // Links
        m_sprGonioInfo->drawString(STR(RetroRadioKey::RETRO_VIS_GONIO_LEFT), 34, 32);
        m_sprGonioInfo->drawFastHLine(11, 46, 2 * SQW + GAP, Theme::ColorMarker);
        drawGonioSquare(baseXL + 0 * (SQW + GAP), baseY, SQW, SQH, GONIO_INFO_COLOR_STANDBY);
        drawGonioSquare(baseXL + 1 * (SQW + GAP), baseY, SQW, SQH, GONIO_INFO_COLOR_STANDBY);

        // Rechts
        m_sprGonioInfo->drawString(STR(RetroRadioKey::RETRO_VIS_GONIO_RIGHT), 138, 32);
        m_sprGonioInfo->drawFastHLine(121, 46, 2 * SQW + GAP, Theme::ColorMarker);
        drawGonioSquare(baseXR + 0 * (SQW + GAP), baseY, SQW, SQH, GONIO_INFO_COLOR_STANDBY);
        drawGonioSquare(baseXR + 1 * (SQW + GAP), baseY, SQW, SQH, GONIO_INFO_COLOR_STANDBY);

        // Anzeigen
        m_sprGonioInfo->pushSprite(513, 354);

        // Bereich nach dem Info-Sprite auffüllen
        m_tft->fillRect(713, 354, 87, 71, 0x1082);

        // eine Linie am rechten Rand zeichnen
        m_tft->drawFastVLine(799, 354, 71, Theme::ColorButtonBorder);

        // Index für Double-Buffering zurücksetzen

        // Auto-Gain und Korrelation bei Mode-Wechsel zurücksetzen
        m_gonioAmp = 1.0f;
        m_gonioCorrPrev = 0.0f;
        m_gonioLastDynamicsUpdate = millis();

        // Peak-History zurücksetzen
        for (uint8_t i = 0; i < GONIO_PEAK_HISTORY_SIZE; i++)
            m_gonioPeakHistory[i] = 0.0f;

        m_gonioPeakHistoryIdx = 0;
        m_gonioSlidingMax = 0.0f;

        // Cache-Variablen für optimiertes Redraw zurücksetzen
        m_lastSquareStateL1 = false;
        m_lastSquareStateL2 = false;
        m_lastSquareStateR1 = false;
        m_lastSquareStateR2 = false;

        // Ungültiger Cache-Wert erzwingt das erste Zeichnen
        m_lastDrawnGain = -999.0f;

        // Den unveränderlichen Untergrund einmal aufbauen. Er wird von hier an in jedes Bild
        // kopiert, statt Kreuz, Balken und Beschriftungen jedes Mal neu zu zeichnen.
        m_sprGonioBg->clear(Theme::ColorMainBackGround);
        drawGonioStaticElements(m_sprGonioBg, cfg);

        Gfx_Sprite *spr = m_sprGonio;
        m_sprGonioBg->pushSprite(spr, 0, 0);

        // Erstes Frame pushen
        spr->pushSprite(cfg.posX, cfg.posY);

        return;
    }

    // Hauptteil

    // Datenpunkte rendern
    // Aktuelles Sprite holen und zum nächsten wechseln
    Gfx_Sprite *current_sprite = m_sprGonio;

    // Untergrund holen; er enthält bereits Kreuz, Teilbalken und Beschriftungen. Roher
    // Kopiervorgang, weil beide Sprites gleich groß und gleich tief sind; pushSprite ginge
    // durch die Farbumsetzung von LovyanGFX und wäre dafür unnötig teuer.
    memcpy(current_sprite->getBuffer(), m_sprGonioBg->getBuffer(),
           static_cast<size_t>(cfg.size) * cfg.size * sizeof(uint16_t));

    // Max. 1024 der jüngsten Stereopaare verarbeiten
    constexpr uint16_t GONIO_N = 1024;
    const uint16_t take = (sampleCount >= GONIO_N) ? GONIO_N : sampleCount;
    const uint16_t start = (sampleCount > take) ? (sampleCount - take) : 0;

    // Keine Samples
    if (take == 0)
    {
        // Nichts zu zeichnen - trotzdem Frame pushen, damit statische Elemente sichtbar bleiben
        current_sprite->pushSprite(cfg.posX, cfg.posY);
        return;
    }

    // Korrelation mit zeitbasierter Ballistik
    float sum_lr = 0.0f, sum_l2 = 0.0f, sum_r2 = 0.0f, corr = 0.0f;

    // Peak-Tracking (unverstärkte Normal-Samples)
    int32_t maxSampleL = 0;
    int32_t maxSampleR = 0;

    // Fester optischer Füllgrad: Figur belegt max. ~90% des verfügbaren Radius
    constexpr float plotFill = 0.90f;
    const float screenScale = fminf((float)cfg.center, (float)cfg.scale) * plotFill;
    constexpr float SAMPLE_TO_NORMALIZED = 1.0f / 32767.0f;
    const float plotSampleScale = m_gonioAmp * SAMPLE_TO_NORMALIZED;

    // Direkter Pufferzugriff
    uint16_t *buffer = (uint16_t *)current_sprite->getBuffer();
    if (!buffer)
        return;

    // Punkte zeichnen & Messwerte akkumulieren
    for (uint16_t i = 0; i < take; i++)
    {
        const uint16_t idx = start + i;

        const int32_t sampleL = samplesL[idx];
        const int32_t sampleR = samplesR[idx];
        const float sampleValueL = static_cast<float>(sampleL);
        const float sampleValueR = static_cast<float>(sampleR);

        // Für die Darstellung normalisieren und verstärken
        const float normL = sampleValueL * plotSampleScale;
        const float normR = sampleValueR * plotSampleScale;

        // Peaks der unverstärkten Samples für nächstes Frame
        const int32_t absSampleL = (sampleL < 0) ? -sampleL : sampleL;
        const int32_t absSampleR = (sampleR < 0) ? -sampleR : sampleR;

        if (absSampleL > maxSampleL)
            maxSampleL = absSampleL;

        if (absSampleR > maxSampleR)
            maxSampleR = absSampleR;

        // Gemeinsame Normalisierung und Darstellungsverstärkung kürzen sich aus der Korrelation heraus
        sum_lr += sampleValueL * sampleValueR;
        sum_l2 += sampleValueL * sampleValueL;
        sum_r2 += sampleValueR * sampleValueR;

        // Mid/Side-Geometrie
        const float xNorm = (normL - normR) * 0.5f;
        const float yNorm = (normL + normR) * 0.5f;

        // Projektion: fester Plot-Scale im Pixelraum
        const int16_t x = cfg.center - (int16_t)(xNorm * screenScale);
        const int16_t y = cfg.center - (int16_t)(yNorm * screenScale);

        // Bounds-Check
        if (x < 0 || x >= cfg.size || y < 0 || y >= cfg.size)
            continue;

        // Farbe bestimmen
        uint16_t color;
        if (m_gonioStyle == GonioStyle::RAINBOW)
        {
            const float distanceSquared = xNorm * xNorm + yNorm * yNorm;
            color = rainbowColors(gonioRainbowColorIndex(distanceSquared));
        }
        else
        {
            color = Theme::ColorGreen;
        }

        buffer[y * cfg.size + x] = bswap16(color);
    }

    // Auto-Verstärkung für den nächsten Frame anpassen
    const int32_t maxSample = (maxSampleL > maxSampleR) ? maxSampleL : maxSampleR;
    const float maxPeak = static_cast<float>(maxSample) * SAMPLE_TO_NORMALIZED;

    const uint32_t now = millis();
    float dynamicsDeltaTime = (now - m_gonioLastDynamicsUpdate) * 0.001f;
    m_gonioLastDynamicsUpdate = now;
    if (dynamicsDeltaTime > 0.1f)
        dynamicsDeltaTime = 0.1f;

    if (now - m_gonioLastPeakUpdate >= 33)
    {
        m_gonioLastPeakUpdate = now;

        m_gonioPeakHistory[m_gonioPeakHistoryIdx] = maxPeak;
        m_gonioPeakHistoryIdx = (m_gonioPeakHistoryIdx + 1) % GONIO_PEAK_HISTORY_SIZE;

        m_gonioSlidingMax = 0.0f;
        for (uint8_t i = 0; i < GONIO_PEAK_HISTORY_SIZE; i++)
            if (m_gonioPeakHistory[i] > m_gonioSlidingMax)
                m_gonioSlidingMax = m_gonioPeakHistory[i];
    }

    const float effectivePeak = (m_gonioSlidingMax > 0.001f) ? m_gonioSlidingMax : maxPeak;

    if (effectivePeak > 0.001f)
    {
        const float target_amp = 1.0f / effectivePeak;
        constexpr float GAIN_OVERLOAD_RATE = 9.0f;
        constexpr float GAIN_RISE_RATE = 2.4f;
        constexpr float GAIN_FALL_RATE = 3.6f;
        float gainTarget;
        float gainRate;

        if (effectivePeak * m_gonioAmp > 1.05f)
        {
            gainTarget = target_amp * 0.95f;
            gainRate = GAIN_OVERLOAD_RATE;
        }
        else if (target_amp > m_gonioAmp)
        {
            gainTarget = target_amp;
            gainRate = GAIN_RISE_RATE;
        }
        else
        {
            gainTarget = target_amp;
            gainRate = GAIN_FALL_RATE;
        }

        const float gainStep = fminf(gainRate * dynamicsDeltaTime, 1.0f);
        m_gonioAmp += (gainTarget - m_gonioAmp) * gainStep;

        if (m_gonioAmp > 500.0f)
            m_gonioAmp = 500.0f;

        if (m_gonioAmp < 0.1f)
            m_gonioAmp = 0.1f;
    }

    // Korrelation und Ballistik
    if (take > 0 && (sum_l2 + sum_r2 > 0.0f))
        corr = sum_lr / (sum_l2 + sum_r2);
    else
        corr = 0.0f;

    float correlationTarget = corr * 2.0f;
    if (correlationTarget > 1.0f)
        correlationTarget = 1.0f;
    else if (correlationTarget < -1.0f)
        correlationTarget = -1.0f;

    constexpr float CORRELATION_RATE = 0.3f;
    const float maxCorrelationStep = CORRELATION_RATE * dynamicsDeltaTime;
    const float correlationDelta = correlationTarget - m_gonioCorrPrev;
    if (correlationDelta > maxCorrelationStep)
        m_gonioCorrPrev += maxCorrelationStep;
    else if (correlationDelta < -maxCorrelationStep)
        m_gonioCorrPrev -= maxCorrelationStep;
    else
        m_gonioCorrPrev = correlationTarget;

    // Beweglicher Marker der Korrelationsanzeige
    const int16_t corrPos = cfg.corrCenter + (int16_t)(cfg.corrRange * m_gonioCorrPrev);
    uint16_t corBox = Theme::ColorAmber;

    if (corrPos > cfg.corrCenter + (cfg.corrRange / 8))
        corBox = Theme::ColorGreen;
    else if (corrPos < cfg.corrCenter - (cfg.corrRange / 8))
        corBox = TFT_RED;

    const int16_t boxW = cfg.size * 4 / 126;
    const int16_t barH = cfg.size * 5 / 126;
    current_sprite->fillRect(corrPos - (boxW / 2), cfg.corrBarY, boxW, barH, corBox);

    // Frame pushen
    current_sprite->pushSprite(cfg.posX, cfg.posY);

    // Info-Anzeige aktualisieren
    drawGonioInfo(m_gonioAmp, peakLeft, peakRight);

}

/**
 * @brief Goniometer VU-Anzeige
 */
void Visualizer::drawGonioVU(bool init, float rmsLeft, float rmsRight)
{
    if (APP_STATE.getBlockGFX())
        return;

    // Bei gestoppter Wiedergabe: VU-Werte zurücksetzen
    if (!APP_STATE.getStationSetDone())
    {
        rmsLeft = rmsRight = 0.0f;
        m_gonioVUValL = m_gonioVUValR = 0.0f;
        m_gonioVUPeakL = m_gonioVUPeakR = 0.0f;
    }

    // Modulabhängige Konfiguration
    const GonioConfig &cfg = (m_activeModule == AppModule::RETRO_RADIO) ? CONFIG_RETRORADIO : CONFIG_SDPLAYER;

    // Initialisierung
    if (init)
    {
        // Sprites nur erstellen, wenn nicht vorhanden
        if (!m_sprGonioVULeft)
        {
            if (!m_tft->safeCreateSprite(m_sprGonioVULeft, 8, cfg.size))
            {
                LOG_ERROR("Konnte Gonio VU-Left Sprite nicht erstellen!");
                return;
            }
        }
        if (!m_sprGonioVUMiddle)
        {
            if (!m_tft->safeCreateSprite(m_sprGonioVUMiddle, 14, cfg.size))
            {
                LOG_ERROR("Konnte Gonio VU-Middle Sprite nicht erstellen!");
                return;
            }
        }
        if (!m_sprGonioVURight)
        {
            if (!m_tft->safeCreateSprite(m_sprGonioVURight, 8, cfg.size))
            {
                LOG_ERROR("Konnte Gonio VU-Right Sprite nicht erstellen!");
                return;
            }
        }

        // Mittelteil: dB-Skala
        m_sprGonioVUMiddle->clear(Theme::ColorMainBackGround);
        m_sprGonioVUMiddle->setFont(&fonts::DejaVu9);
        m_sprGonioVUMiddle->setTextColor(TFT_SILVER, Theme::ColorMainBackGround);

        const float dbRange = VU_DB_HIGH - VU_DB_LOW;

        // Ein Wert, der auf der Skala nicht vorkommt. Stuende hier 0, fiele ausgerechnet die
        // oberste Zeile durch, denn seit die Skala bei Vollaussteuerung endet, ist das die 0 dB.
        int lastTick = static_cast<int>(VU_DB_HIGH) + 1;
        for (uint8_t y = 0; y < cfg.size; ++y)
        {
            int dB = roundf(VU_DB_HIGH - (y * dbRange / cfg.size));
            if (dB != lastTick)
            {
                if (dB == 0)
                {
                    m_sprGonioVUMiddle->drawFastHLine(0, y, 14, TFT_RED);
                    m_sprGonioVUMiddle->setTextColor(TFT_RED, Theme::ColorMainBackGround);
                    m_sprGonioVUMiddle->drawString("0", 4, y + 2);
                    m_sprGonioVUMiddle->setTextColor(TFT_SILVER, Theme::ColorMainBackGround);
                }

                if (dB == -5)
                {
                    m_sprGonioVUMiddle->drawFastHLine(0, y, 14, TFT_DARKGRAY);
                    m_sprGonioVUMiddle->drawString("5", 4, y + 2);
                }

                if (dB == -10)
                {
                    m_sprGonioVUMiddle->drawFastHLine(0, y, 14, TFT_DARKGRAY);
                    m_sprGonioVUMiddle->drawString("10", 0, (VU_DB_LOW == -10) ? y - 10 : y + 2);
                }

                if (dB == -20)
                {
                    m_sprGonioVUMiddle->drawFastHLine(0, y, 14, TFT_DARKGRAY);
                    m_sprGonioVUMiddle->drawString("20", 0, (VU_DB_LOW == -20) ? y - 10 : y + 2);
                }

                if (dB == -30)
                {
                    m_sprGonioVUMiddle->drawFastHLine(0, y, 14, TFT_DARKGRAY);
                    m_sprGonioVUMiddle->drawString("30", 0, (VU_DB_LOW == -30) ? y - 10 : y + 2);
                }

                if (dB == -40)
                {
                    m_sprGonioVUMiddle->drawFastHLine(0, y, 14, TFT_DARKGRAY);
                    m_sprGonioVUMiddle->drawString("40", 0, (VU_DB_LOW == -40) ? y - 10 : y + 2);
                }

                if (dB == -50)
                {
                    m_sprGonioVUMiddle->drawFastHLine(0, y, 14, TFT_DARKGRAY);
                    m_sprGonioVUMiddle->drawString("50", 0, (VU_DB_LOW == -50) ? y - 10 : y + 2);
                }

                lastTick = dB;
            }
        }

        // Mitte anzeigen
        m_sprGonioVUMiddle->pushSprite(cfg.posX + cfg.size + 8 + 8, cfg.posY);

        // Balken vorzeichnen
        for (uint8_t y = 0; y < cfg.size; ++y)
        {
            const uint16_t bgStripe = (y % 2 == 0) ? TFT_BLACK : 0x2104;
            m_sprGonioVULeft->drawFastHLine(0, y, 8, bgStripe);
            m_sprGonioVURight->drawFastHLine(0, y, 8, bgStripe);
        }

        m_sprGonioVULeft->pushSprite(cfg.posX + cfg.size + 8, cfg.posY);
        m_sprGonioVURight->pushSprite(cfg.posX + cfg.size + 8 + 8 + 14, cfg.posY);

        // Zeitstempel/Werte zurücksetzen
        m_lastGonioVUUpdate = millis();
        m_gonioVUValL = m_gonioVUValR = 0.0f;
        m_gonioVUPeakL = m_gonioVUPeakR = 0.0f;
        m_gonioVUPeakTimeL = m_gonioVUPeakTimeR = 0;
        m_lastDrawnValL = m_lastDrawnValR = 0;
        m_lastDrawnPeakL = m_lastDrawnPeakR = 0;

        return;
    }

    // Zeitbasierte Ballistik
    uint32_t now = millis();
    float dt = (now - m_lastGonioVUUpdate) / 1000.0f;
    m_lastGonioVUUpdate = now;

    auto amplitudeToBarHeight = [&](float amplitude) -> float
    {
        float dB = amplitudeToDb(amplitude);
        return (dB - VU_DB_LOW) * cfg.size / (VU_DB_HIGH - VU_DB_LOW);
    };

    float targetValL = amplitudeToBarHeight(rmsLeft);
    float targetValR = amplitudeToBarHeight(rmsRight);

    // Ballistik
    auto stepToward = [&](float current, float target) -> float
    {
        if (target > current)
        {
            current += GONIO_VU_RISE_RATE * dt;
            if (current > target)
                current = target;
        }
        else
        {
            current -= GONIO_VU_FALL_RATE * dt;
            if (current < target)
                current = target;
        }

        if (current < 0.0f)
            current = 0.0f;

        if (current > cfg.size)
            current = static_cast<float>(cfg.size);

        return current;
    };

    m_gonioVUValL = stepToward(m_gonioVUValL, targetValL);
    m_gonioVUValR = stepToward(m_gonioVUValR, targetValR);

    // Peak-Hold
    if (m_gonioVUValL > m_gonioVUPeakL)
    {
        m_gonioVUPeakL = m_gonioVUValL;
        m_gonioVUPeakTimeL = now;
    }
    else if (now - m_gonioVUPeakTimeL > GONIO_PEAK_HOLD_TIME)
    {
        m_gonioVUPeakL -= GONIO_PEAK_FALL_RATE * dt;

        if (m_gonioVUPeakL < m_gonioVUValL)
            m_gonioVUPeakL = m_gonioVUValL;

        if (m_gonioVUPeakL < 0.0f)
            m_gonioVUPeakL = 0.0f;
    }

    if (m_gonioVUValR > m_gonioVUPeakR)
    {
        m_gonioVUPeakR = m_gonioVUValR;
        m_gonioVUPeakTimeR = now;
    }
    else if (now - m_gonioVUPeakTimeR > GONIO_PEAK_HOLD_TIME)
    {
        m_gonioVUPeakR -= GONIO_PEAK_FALL_RATE * dt;

        if (m_gonioVUPeakR < m_gonioVUValR)
            m_gonioVUPeakR = m_gonioVUValR;

        if (m_gonioVUPeakR < 0.0f)
            m_gonioVUPeakR = 0.0f;
    }

    // Aktuelle Werte für Zeichnung
    const uint8_t currentValL = static_cast<uint8_t>(m_gonioVUValL);
    const uint8_t currentValR = static_cast<uint8_t>(m_gonioVUValR);
    const uint8_t currentPeakL = static_cast<uint8_t>(m_gonioVUPeakL);
    const uint8_t currentPeakR = static_cast<uint8_t>(m_gonioVUPeakR);

    // Nur neu zeichnen, wenn sich Pixel-Werte geändert haben
    if (currentValL == m_lastDrawnValL &&
        currentValR == m_lastDrawnValR &&
        currentPeakL == m_lastDrawnPeakL &&
        currentPeakR == m_lastDrawnPeakR)
    {
        return; // Nichts zu tun - Sprites sind noch aktuell
    }

    // Neue Werte als "zuletzt gezeichnet" speichern
    m_lastDrawnValL = currentValL;
    m_lastDrawnValR = currentValR;
    m_lastDrawnPeakL = currentPeakL;
    m_lastDrawnPeakR = currentPeakR;

    // Pixelgenaue Farbschwellen vorberechnen
    auto dBtoY = [&](float dBthr) -> uint8_t
    {
        float y = (dBthr - VU_DB_LOW) * (float)(cfg.size - 1) / (float)(VU_DB_HIGH - VU_DB_LOW);

        if (y < 0.f)
            y = 0.f;

        if (y > (float)(cfg.size - 1))
            y = (float)(cfg.size - 1);

        return static_cast<uint8_t>(lroundf(y));
    };

    // Dieselben Schwellen wie beim breiten Balken der Digitalanzeige, denn beide zeigen jetzt
    // denselben Effektivwert. Frueher lagen sie bei -5 und 0 dB; seit die Skala bei
    // Vollaussteuerung endet, waere der rote Bereich nur noch eine einzige Bildzeile hoch.
    const uint8_t yCutOrange = dBtoY(-12.0f); // ab hier Orange
    const uint8_t yCutRed = dBtoY(-3.0f);     // ab hier Rot

    // Direkter Pufferzugriff
    uint16_t *bufferL = (uint16_t *)m_sprGonioVULeft->getBuffer();
    uint16_t *bufferR = (uint16_t *)m_sprGonioVURight->getBuffer();

    if (!bufferL || !bufferR)
        return;

    // Balken & Peak zeichnen
    for (uint8_t y = 0; y < cfg.size; ++y)
    {
        // Farbwahl je Pixel-Zeile (von unten nach oben gemessen)
        uint16_t segColor;
        if (m_gonioStyle == GonioStyle::RAINBOW)
            // 0..(cfg.size-1) -> Hues 96..0
            segColor = rainbowColors(96 - (y * 96 / (cfg.size - 1)));
        else
            segColor = (y >= yCutRed) ? TFT_RED : (y >= yCutOrange) ? TFT_ORANGE
                                                                    : 0x6c64; // Grün

        const uint16_t bgStripe = (y % 2 == 0) ? TFT_BLACK : 0x2104;
        const bool isPeakL = (y >= currentPeakL && y < currentPeakL + 1);
        const bool isPeakR = (y >= currentPeakR && y < currentPeakR + 1);
        const int drawY = cfg.size - 1 - y; // invertiert zeichnen (oben->unten im Sprite)

        // Farben für diese Zeile bestimmen
        uint16_t colorL, colorR;

        if (y < currentValL)
            colorL = (y % 2 == 0) ? TFT_BLACK : segColor;
        else
            colorL = isPeakL ? TFT_WHITE : bgStripe;

        if (y < currentValR)
            colorR = (y % 2 == 0) ? TFT_BLACK : segColor;
        else
            colorR = isPeakR ? TFT_WHITE : bgStripe;

        colorL = bswap16(colorL);
        colorR = bswap16(colorR);

        // Direkt in Puffer schreiben (8 Pixel breit)
        uint16_t *lineL = &bufferL[drawY * 8];
        uint16_t *lineR = &bufferR[drawY * 8];

        for (uint8_t x = 0; x < 8; x++)
        {
            lineL[x] = colorL;
            lineR[x] = colorR;
        }
    }

    // Anzeigen
    m_sprGonioVULeft->pushSprite(cfg.posX + cfg.size + 8, cfg.posY);
    m_sprGonioVURight->pushSprite(cfg.posX + cfg.size + 8 + 8 + 14, cfg.posY);
}

/**
 * @brief Goniometer Info-Anzeige
 */
void Visualizer::drawGonioInfo(float gainFactor, float peakL, float peakR)
{
    if (APP_STATE.getBlockGFX() || !m_sprGonioInfo)
        return;

    // Signalisiert dem Writer, dass gerade gelesen wird
    ReaderGuard guard(m_readerActive);

    // Lese-Puffer ermitteln
    const uint8_t readIdx = m_writeBufferIdx.load(std::memory_order_acquire) ^ 1;
    const uint16_t sampleCount = m_sampleCount[readIdx];

    // Hilfs-Konstanten
    constexpr float MIN_SIGNAL_THRESH = 1e-4f; // Schwellwert für "kein hörbares Signal"
    const uint32_t now = millis();
    bool needsPush = false; // Flag ob überhaupt gepusht werden muss
    const bool hasSignal = APP_STATE.getStationSetDone() &&
                           sampleCount >= 2 &&
                           (peakL > MIN_SIGNAL_THRESH || peakR > MIN_SIGNAL_THRESH);

    // Gain-Text nur formatieren und zeichnen, wenn sich der sichtbare Wert ändert
    const bool showDecimal = !hasSignal || gainFactor < 10.0f;
    float displayGain = 1.0f;
    if (hasSignal)
        displayGain = showDecimal ? roundf(gainFactor * 10.0f) * 0.1f : roundf(gainFactor);

    // Das Vorzeichen trennt "x10.0" von "x10", ohne den Text vorab zu formatieren
    const float gainCacheKey = showDecimal ? displayGain : -displayGain;
    if (gainCacheKey != m_lastDrawnGain)
    {
        char gainStr[8];
        if (!hasSignal)
            stringutils::safeCopy(gainStr, sizeof(gainStr), "x1.0");
        else if (showDecimal)
            stringutils::safeFormat(gainStr, "x%.1f", displayGain);
        else
            stringutils::safeFormat(gainStr, "x%.0f", displayGain);

        // Gain-Text hat sich geändert - nur diesen Bereich updaten
        const int16_t clipX = m_gainValueX - 5;
        const int16_t clipW = 200 - clipX - 5;
        m_sprGonioInfo->setClipRect(clipX, 5, clipW, 12);      // Nur Gain-Bereich clippen
        m_sprGonioInfo->fillRect(clipX, 5, clipW, 12, 0x1082); // alten Text übermalen
        m_sprGonioInfo->drawString(gainStr, m_gainValueX, 5);
        m_sprGonioInfo->clearClipRect(); // Clip-Bereich zurücksetzen

        m_lastDrawnGain = gainCacheKey;
        needsPush = true;
    }

    // Wenn kein Signal oder gestoppt, dann keine Quadrate aktualisieren
    if (!hasSignal)
    {
        // Alle Quadrate auf aus setzen (falls sie vorher an waren)
        if (m_lastSquareStateL1 || m_lastSquareStateL2 ||
            m_lastSquareStateR1 || m_lastSquareStateR2)
        {
            // Quadrate müssen ausgeschaltet werden
            m_lastSquareStateL1 = false;
            m_lastSquareStateL2 = false;
            m_lastSquareStateR1 = false;
            m_lastSquareStateR2 = false;

            // Alle Quadrate in einem Rutsch neu zeichnen
            m_sprGonioInfo->setClipRect(baseXL, baseY, 162, SQH); // Kompletter Quadrat-Bereich
            drawGonioSquare(baseXL, baseY, SQW, SQH, GONIO_INFO_COLOR_STANDBY);
            drawGonioSquare(baseXL + (SQW + GAP), baseY, SQW, SQH, GONIO_INFO_COLOR_STANDBY);
            drawGonioSquare(baseXR, baseY, SQW, SQH, GONIO_INFO_COLOR_STANDBY);
            drawGonioSquare(baseXR + (SQW + GAP), baseY, SQW, SQH, GONIO_INFO_COLOR_STANDBY);
            m_sprGonioInfo->clearClipRect();
            needsPush = true;
        }

        if (needsPush)
            m_sprGonioInfo->pushSprite(513, 354);

        return;
    }

    // Hold-Timer aktualisieren wenn Schwellen überschritten werden
    if (peakL >= GONIO_INFO_THR1_LINEAR)
        m_infoPeakHoldL1 = now;
    if (peakL >= GONIO_INFO_THR2_LINEAR)
        m_infoPeakHoldL2 = now;
    if (peakR >= GONIO_INFO_THR1_LINEAR)
        m_infoPeakHoldR1 = now;
    if (peakR >= GONIO_INFO_THR2_LINEAR)
        m_infoPeakHoldR2 = now;

    // Lambda für Hold-Check
    auto isLit = [&](uint32_t tLast) -> bool
    {
        return (tLast != 0) && (now - tLast < GONIO_INFO_HOLD_MS);
    };

    // Aktuelle Zustände der Quadrate berechnen
    bool currentStateL1 = isLit(m_infoPeakHoldL1);
    bool currentStateL2 = isLit(m_infoPeakHoldL2);
    bool currentStateR1 = isLit(m_infoPeakHoldR1);
    bool currentStateR2 = isLit(m_infoPeakHoldR2);

    // Quadrate nur updaten wenn sich ihr Zustand geändert hat

    // Links Quadrat 1 (Threshold 1)
    if (currentStateL1 != m_lastSquareStateL1)
    {
        m_sprGonioInfo->setClipRect(baseXL, baseY, SQW, SQH);
        drawGonioSquare(baseXL, baseY, SQW, SQH, currentStateL1 ? GONIO_INFO_COLOR_THR1_ON : GONIO_INFO_COLOR_STANDBY);
        m_sprGonioInfo->clearClipRect();
        m_lastSquareStateL1 = currentStateL1;
        needsPush = true;
    }

    // Links Quadrat 2 (Threshold 2)
    if (currentStateL2 != m_lastSquareStateL2)
    {
        m_sprGonioInfo->setClipRect(baseXL + (SQW + GAP), baseY, SQW, SQH);
        drawGonioSquare(baseXL + (SQW + GAP), baseY, SQW, SQH, currentStateL2 ? GONIO_INFO_COLOR_THR2_ON : GONIO_INFO_COLOR_STANDBY);
        m_sprGonioInfo->clearClipRect();
        m_lastSquareStateL2 = currentStateL2;
        needsPush = true;
    }

    // Rechts Quadrat 1 (Threshold 1)
    if (currentStateR1 != m_lastSquareStateR1)
    {
        m_sprGonioInfo->setClipRect(baseXR, baseY, SQW, SQH);
        drawGonioSquare(baseXR, baseY, SQW, SQH, currentStateR1 ? GONIO_INFO_COLOR_THR1_ON : GONIO_INFO_COLOR_STANDBY);
        m_sprGonioInfo->clearClipRect();
        m_lastSquareStateR1 = currentStateR1;
        needsPush = true;
    }

    // Rechts Quadrat 2 (Threshold 2)
    if (currentStateR2 != m_lastSquareStateR2)
    {
        m_sprGonioInfo->setClipRect(baseXR + (SQW + GAP), baseY, SQW, SQH);
        drawGonioSquare(baseXR + (SQW + GAP), baseY, SQW, SQH, currentStateR2 ? GONIO_INFO_COLOR_THR2_ON : GONIO_INFO_COLOR_STANDBY);
        m_sprGonioInfo->clearClipRect();
        m_lastSquareStateR2 = currentStateR2;
        needsPush = true;
    }

    // Sprite nur pushen wenn sich etwas geändert hat
    if (needsPush)
        m_sprGonioInfo->pushSprite(513, 354);
}

/**
 * @brief Zeichnet ein Quadrat für die Goniometer Info-Anzeige
 */
void Visualizer::drawGonioSquare(int x, int y, int width, int height, uint16_t colorFill)
{
    if (!m_sprGonioInfo)
        return;

    m_sprGonioInfo->fillRect(x, y, width, height, colorFill);
    m_sprGonioInfo->drawRect(x, y, width, height, Theme::ColorButtonBorder);
}

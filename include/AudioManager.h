#ifndef AUDIO_MANAGER_H
#define AUDIO_MANAGER_H

#include <freertos/FreeRTOS.h>  // für portMUX_TYPE
#include <freertos/portmacro.h> // für portMUX_INITIALIZER_UNLOCKED
#include <atomic>               // für std::atomic
#include <stdint.h>             // für uint16_t
#include <vector>               // für std::vector

#include "Audio.h"

// ============================================================================
// Metadaten-Struktur
// ============================================================================

/**
 * @brief Struktur für geparste Metadaten
 *
 * Speichert die extrahierten Metadaten aus ID3-Tags. Verwendet feste Puffer um dynamische Speicherverwaltung zu vermeiden.
 */
struct AudioMetadata
{
    char artist[128]; ///< Interpret/Künstler
    char title[128];  ///< Titel des Songs
    char album[128];  ///< Album-Name
    bool hasArtist;   ///< Flag: Artist wurde gefunden
    bool hasTitle;    ///< Flag: Title wurde gefunden
    bool hasAlbum;    ///< Flag: Album wurde gefunden

    /**
     * @brief Konstruktor - Initialisiert alle Felder
     */
    AudioMetadata() : hasArtist(false), hasTitle(false), hasAlbum(false)
    {
        artist[0] = '\0';
        title[0] = '\0';
        album[0] = '\0';
    }

    /**
     * @brief Setzt alle Metadaten zurück
     */
    void reset()
    {
        artist[0] = '\0';
        title[0] = '\0';
        album[0] = '\0';
        hasArtist = false;
        hasTitle = false;
        hasAlbum = false;
    }
};

// ============================================================================
// Event-Typen
// ============================================================================

/**
 * @brief Enumeration für verschiedene Audio-Event-Typen
 *
 * Diese Enumeration definiert alle möglichen Event-Typen, die vom AudioManager verarbeitet werden können.
 */
enum class AudioManagerEventType
{
    NOTHING,         ///< Kein Event verfügbar
    INFO,            ///< Allgemeine Audio-Informationen
    ID3_ARTIST,      ///< ID3-Tag: Artist/Interpret
    ID3_TITLE,       ///< ID3-Tag: Titel
    ID3_ALBUM,       ///< ID3-Tag: Album
    ID3DATA,         ///< ID3-Tag-Daten (generisch, für andere Tags)
    EOF_AUDIO,       ///< Ende einer Audio-Datei/eines Streams erreicht
    STATION_NAME,    ///< Radio-Stations-Name
    ICY_DESCRIPTION, ///< ICY-Stream-Beschreibung
    STREAM_TITLE,    ///< Stream-Titel-Information
    BITRATE,         ///< Bitrate-Information
    ICY_URL,         ///< ICY-URL-Information
    ICY_LOGO,        ///< ICY-Logo-Information
    LAST_HOST,       ///< Information über den letzten Host
    COVER_IMAGE,     ///< Cover-Bild verfügbar (enthält Vector-Daten)
    LYRICS           ///< Liedtext verfügbar
};

// ============================================================================
// AudioManager-Klasse
// ============================================================================

/**
 * @brief Audio-Manager-Klasse für die Verarbeitung von Audio-Daten und -Events
 *
 * Der AudioManager ist eine zentrale Klasse zur Verwaltung und Analyse von Audio-Streams. Er verarbeitet sowohl die eigentlichen Audio-Samples als auch verschiedene Metadaten und Events, die von der Audio-Library gesendet werden.
 *
 * Hauptfunktionen:
 * - Real-time Audio-Sample-Analyse (RMS, Peak, Mono-Erkennung)
 * - Verlustfreie Event-Queue für alle Events (Text + Cover-Images)
 * - Pufferung von Audio-Samples für weitere Verarbeitung
 * - Thread-sichere Datenübertragung zwischen Audio-Callbacks und Haupt-Thread
 *
 * Die Klasse arbeitet mit einem Producer/Consumer-Pattern, bei dem die Audio-Callbacks als Producer fungieren und der Haupt-Thread als Consumer.
 */
class AudioManager
{
public:
    /**
     * @brief Maximale Länge für Event-Strings in der Queue
     */
    static constexpr size_t MAX_EVENT_STRING_LENGTH = 256;

    // ========================================================================
    // Konstruktor & Initialisierung
    // ========================================================================

    /**
     * @brief Konstruktor - Initialisiert alle Member-Variablen
     *
     * Setzt alle Puffer auf Null und initialisiert die Analyse-Parameter mit Standardwerten.
     */
    AudioManager();

    /**
     * @brief Initialisiert den AudioManager und registriert Callbacks
     * @param audio Referenz auf das Audio-Objekt
     *
     * Diese Funktion muss nach der Erstellung des Audio-Objekts aufgerufen werden, um den zentralen Callback zu registrieren.
     */
    void init(Audio &audio);

    // ========================================================================
    // Event-Queue (verlustfrei, SPSC-Ringpuffer)
    // ========================================================================

    /**
     * @brief Holt das älteste Event aus der Queue
     * @param type (out) Event-Typ des dequeued Events
     * @param out (out) Zielpuffer für Event-Text/Payload (kann nullptr sein)
     * @param len Größe des Ausgabepuffers
     * @param coverData (out, optional) Zeiger auf Vector für Cover-Image-Daten
     * @return true wenn ein Event vorhanden war, false wenn Queue leer
     *
     * Diese Funktion sollte regelmäßig vom Haupt-Thread aufgerufen werden, um alle anstehenden Events zu verarbeiten. Die Queue ist thread-sicher und kann parallel zu den Audio-Callbacks aufgerufen werden. Bei COVER_IMAGE Events wird der Vector in coverData kopiert (falls nicht nullptr). Bei allen anderen Events enthält coverData keine Daten.
     *
     * Verwendungsbeispiel:
     * @code
     * AudioManagerEventType type;
     * char text[AudioManager::MAX_EVENT_STRING_LENGTH];
     * std::vector<uint32_t> coverData;
     *
     * while (g_audioManager.dequeueEvent(type, text, sizeof(text), &coverData)) {
     *     switch (type) {
     *         case AudioManagerEventType::STREAM_TITLE:
     *             Serial.printf("Titel: %s\n", text);
     *             break;
     *         case AudioManagerEventType::COVER_IMAGE:
     *             Serial.printf("Cover: %d Einträge\n", coverData.size() / 2);
     *             break;
     *     }
     * }
     * @endcode
     *
     * @note Die Funktion ist non-blocking und gibt sofort false zurück, wenn keine Events verfügbar sind.
     */
    bool dequeueEvent(AudioManagerEventType &type, char *out, size_t len, std::vector<uint32_t> *coverData = nullptr);

    // ========================================================================
    // Metadaten-Zugriff
    // ========================================================================

    /**
     * @brief Liefert die aktuellen Metadaten
     * @return Konstante Referenz auf die Metadaten-Struktur
     *
     * Die Metadaten-Struktur enthält Artist, Title und Album-Informationen, die aus ID3-Tags extrahiert wurden. Die Struktur wird bei jedem ID3DATA-Event aktualisiert.
     */
    const AudioMetadata &getMetadata() const { return m_metadata; }

    /**
     * @brief Setzt die Metadaten zurück
     *
     * Sollte aufgerufen werden, wenn ein neuer Track startet oder die Metadaten ungültig werden.
     */
    void resetMetadata() { m_metadata.reset(); }

    // ========================================================================
    // Audio-Callback (wird von Audio-Library aufgerufen)
    // ========================================================================

    /**
     * @brief Zentraler Callback für alle Audio-Events
     * @param msg Die Message-Struktur mit Event-Daten
     *
     * Diese Funktion wird von der Audio-Library für alle Events aufgerufen und verteilt sie an die entsprechenden internen Handler.
     *
     * @note Diese Funktion wird aus einem Callback-Kontext aufgerufen.
     */
    void handleAudioCallback(const Audio::msg_t &msg);

    /**
     * @brief Verarbeitet Audio-Samples in Echtzeit
     * @param outBuff Zeiger auf den Audio-Sample-Puffer (Stereo, 32-bit)
     * @param validSamples Anzahl der gültigen Stereo-Sample-Paare
     * @param continueI2S Zeiger auf Flag für I2S-Fortsetzung
     *
     * Diese Funktion analysiert die Audio-Samples und berechnet RMS- und Peak-Werte sowie die Mono-Erkennung. Die Samples werden in internen Puffern zur weiteren Verarbeitung gespeichert.
     *
     * @note Diese Funktion wird aus einem Interrupt-Kontext aufgerufen.
     */
    void task_processAudioSamples32(int32_t *outBuff, int16_t validSamples, bool *continueI2S);

    // ========================================================================
    // Zugriff auf analysierte Audio-Daten
    // ========================================================================

    /**
     * @brief Prüft, ob neue Audio-Daten zur Verarbeitung bereitstehen
     * @return true wenn neue Daten verfügbar sind, false sonst
     *
     * Diese Funktion sollte vor dem Zugriff auf die Audio-Analyse-Daten aufgerufen werden, um sicherzustellen, dass aktuelle Daten vorliegen.
     */
    bool isAudioDataReady() const;

    /**
     * @brief Liefert den RMS-Wert des linken Kanals
     * @return RMS-Wert als skalierter Float-Wert
     *
     * Der RMS (Root Mean Square) Wert gibt die durchschnittliche Signalstärke über einen Zeitraum an.
     */
    float getRMSLeft() const;

    /**
     * @brief Liefert den RMS-Wert des rechten Kanals
     * @return RMS-Wert als skalierter Float-Wert
     */
    float getRMSRight() const;

    /**
     * @brief Liefert den Peak-Wert des linken Kanals
     * @return Peak-Wert als skalierter Float-Wert
     *
     * Der Peak-Wert repräsentiert die maximale Amplitude im analysierten Zeitfenster.
     */
    float getPeakLeft() const;

    /**
     * @brief Liefert den Peak-Wert des rechten Kanals
     * @return Peak-Wert als skalierter Float-Wert
     */
    float getPeakRight() const;

    /**
     * @brief Prüft, ob das aktuelle Signal als Mono erkannt wurde
     * @return true wenn Mono-Signal erkannt, false bei Stereo
     *
     * Die Mono-Erkennung basiert auf einem Toleranz-Algorithmus, der kleine Unterschiede zwischen den Kanälen ignoriert.
     */
    bool isMono() const;

    /**
     * @brief Liefert die Anzahl der gültigen Samples im Puffer
     * @return Anzahl der Samples pro Kanal
     */
    uint16_t getValidSamplesCount() const;

    /**
     * @brief Liefert die Abtastrate des bereitstehenden Sample-Blocks
     * @return Abtastrate in Hz, 0 wenn unbekannt
     * @note Nach isAudioDataReady() und vor consumeAudioData() zusammen mit den Samples lesen.
     */
    uint32_t getSampleRate() const;

    /**
     * @brief Liefert Zeiger auf den Sample-Puffer des linken Kanals
     * @return Konstanter Zeiger auf den internen Sample-Puffer
     *
     * @warning Der Zeiger ist nur gültig, solange isAudioDataReady() true zurückgibt und consumeAudioData() noch nicht aufgerufen wurde.
     */
    const int16_t *getSampleBufferLeft() const;

    /**
     * @brief Liefert Zeiger auf den Sample-Puffer des rechten Kanals
     * @return Konstanter Zeiger auf den internen Sample-Puffer
     *
     * @warning Der Zeiger ist nur gültig, solange isAudioDataReady() true zurückgibt und consumeAudioData() noch nicht aufgerufen wurde.
     */
    const int16_t *getSampleBufferRight() const;

    /**
     * @brief Markiert die aktuellen Audio-Daten als verarbeitet
     *
     * Diese Funktion muss nach der Verarbeitung der Audio-Daten aufgerufen werden, um den AudioManager für neue Daten freizugeben.
     *
     * @note Nach dem Aufruf sind die Sample-Puffer-Zeiger ungültig
     */
    void consumeAudioData();

private:
    // ========================================================================
    // Event-Queue
    // ========================================================================

    /**
     * @brief Struktur für ein Event in der Queue
     *
     * Unterstützt sowohl Text-basierte Events als auch Cover-Image-Daten. Der Vector wird nur bei COVER_IMAGE Events verwendet.
     */
    struct QueuedEvent
    {
        AudioManagerEventType type;         ///< Event-Typ
        char text[MAX_EVENT_STRING_LENGTH]; ///< Event-Text/Payload
        std::vector<uint32_t> coverData;    ///< Cover-Image-Daten (nur bei COVER_IMAGE)
    };

    static constexpr uint8_t EVENT_Q_CAP = 12;            ///< Kapazität der Event-Queue
    std::atomic<uint8_t> m_qHead{0};                      ///< Schreib-Index (Producer)
    std::atomic<uint8_t> m_qTail{0};                      ///< Lese-Index (Consumer)
    QueuedEvent m_evtQ[EVENT_Q_CAP];                      ///< Event-Queue-Puffer
    portMUX_TYPE m_evtMux = portMUX_INITIALIZER_UNLOCKED; ///< Mutex für thread-sichere Queue-Operationen

    /**
     * @brief Legt ein Text-Event in die Queue (thread-sicher)
     * @param type Event-Typ
     * @param text Event-Text (null-terminiert)
     */
    inline void enqueueEvent_(AudioManagerEventType type, const char *text);

    /**
     * @brief Legt ein Cover-Image-Event in die Queue (thread-sicher)
     * @param coverData Vector mit Cover-Image-Positionen und -Längen
     */
    inline void enqueueCoverEvent_(const std::vector<uint32_t> &coverData);

    /**
     * @brief Prüft ob Queue leer ist (nur intern, nicht thread-sicher)
     */
    inline bool qIsEmpty_() const
    {
        return m_qHead.load(std::memory_order_relaxed) == m_qTail.load(std::memory_order_relaxed);
    }

    /**
     * @brief Prüft ob Queue voll ist (nur intern, nicht thread-sicher)
     */
    inline bool qIsFull_() const
    {
        return (m_qHead.load(std::memory_order_relaxed) + 1) % EVENT_Q_CAP == m_qTail.load(std::memory_order_relaxed);
    }

    // ========================================================================
    // Metadaten-Parsing
    // ========================================================================

    AudioMetadata m_metadata; ///< Geparste Metadaten aus ID3-Tags

    /**
     * @brief Parst ID3-Daten und extrahiert Metadaten
     * @param id3String Der rohe ID3-String von der Audio-Library
     * @return true wenn Metadaten gefunden wurden, false sonst
     */
    bool parseId3Metadata(const char *id3String);

    /**
     * @brief Case-insensitive String-Vergleich am Anfang
     * @param str Der zu prüfende String
     * @param prefix Der Präfix nach dem gesucht wird
     * @return true wenn str mit prefix beginnt (case-insensitive)
     */
    bool startsWithIgnoreCase(const char *str, const char *prefix) const;

    /**
     * @brief Findet ein Tag-Muster im String
     * @param str Der zu durchsuchende String
     * @param patterns Array von Suchmustern
     * @param patternCount Anzahl der Suchmuster
     * @return Zeiger auf den Wert nach dem Tag, oder nullptr wenn nicht gefunden
     */
    const char *findTagValue(const char *str, const char **patterns, int patternCount) const;

    /**
     * @brief Prüft ob Metadaten-String gültig ist
     * @param str Der zu prüfende String
     * @return true wenn String gültig erscheint, false wenn verstümmelt
     */
    bool isValidMetadataString(const char *str) const;

    /**
     * @brief Bereinigt ID3-Strings und erkennt UTF-16
     * @param input Eingabe-String (möglicherweise verstümmelt)
     * @param output Ausgabe-Buffer für bereinigten String
     * @param maxLen Maximale Länge des Ausgabe-Buffers
     * @return true wenn String gültig, false wenn leer/ungültig
     */
    bool cleanId3String(const char *input, char *output, size_t maxLen);

    // ========================================================================
    // Event-Handler (werden vom zentralen Callback aufgerufen)
    // ========================================================================

    void handleInfo(const Audio::msg_t &msg);
    void handleId3Data(const Audio::msg_t &msg);
    void handleEof(const Audio::msg_t &msg);
    void handleStationName(const Audio::msg_t &msg);
    void handleIcyDescription(const Audio::msg_t &msg);
    void handleStreamTitle(const Audio::msg_t &msg);
    void handleBitrate(const Audio::msg_t &msg);
    void handleIcyUrl(const Audio::msg_t &msg);
    void handleIcyLogo(const Audio::msg_t &msg);
    void handleLastHost(const Audio::msg_t &msg);
    void handleCoverImage(const Audio::msg_t &msg);
    void handleLyrics(const Audio::msg_t &msg);

    // ========================================================================
    // Audio-Sample-Verarbeitung
    // ========================================================================

    Audio *m_audio = nullptr;  ///< Audio-Quelle für die Abtastrate im Sample-Callback
    uint32_t m_sampleRate = 0; ///< Abtastrate des gepufferten Sample-Blocks in Hz

    int16_t m_sampleBufferLeft[2048];  ///< Puffer für Audio-Samples des linken Kanals
    int16_t m_sampleBufferRight[2048]; ///< Puffer für Audio-Samples des rechten Kanals

    float m_rmsLeft;   ///< RMS-Wert des linken Kanals (skaliert)
    float m_rmsRight;  ///< RMS-Wert des rechten Kanals (skaliert)
    float m_peakLeft;  ///< Peak-Wert des linken Kanals (skaliert)
    float m_peakRight; ///< Peak-Wert des rechten Kanals (skaliert)

    bool m_monoSignal;          ///< Flag für Mono-Signal-Erkennung
    uint16_t m_numValidSamples; ///< Anzahl der gültigen Samples in den Puffern

    /**
     * @brief SPSC-Handshake-Flag für Audio-Daten (Dual-Core-Safe)
     *
     * Einziges Synchronisationssignal zwischen Producer (I2S-Callback, Core 0) und Consumer (Main-Loop, Core 1):
     * - false: Producer darf schreiben, Consumer hat nichts zu lesen
     * - true:  Producer hat geschrieben, Consumer darf lesen
     *
     * acquire/release garantiert, dass alle Daten-Schreiboperationen vor dem store(true) für den Consumer nach dem load() sichtbar sind und umgekehrt.
     */
    std::atomic<bool> m_dataReady{false};

    static constexpr float NORM_FACTOR = 1.0f / 32767.0f;    ///< Normalisierungs-Faktor für 16-bit Samples
    static constexpr float RMS_SCALE_FACTOR = NORM_FACTOR;   ///< 1,0 entspricht Vollausschlag (0 dBFS)
    static constexpr float PEAK_SCALE_FACTOR = NORM_FACTOR;  ///< 1,0 entspricht Vollausschlag (0 dBFS)
    static constexpr float MONO_THRESHOLD_FACTOR = 0.99f;    ///< Schwellenwert für Mono-Erkennung
};

// ============================================================================
// Globale Instanz
// ============================================================================

/**
 * @brief Globale AudioManager-Instanz
 *
 * Kann von überall im Projekt verwendet werden.
 */
extern AudioManager g_audioManager;

// ============================================================================
// Callback-Funktionen für die Audio-Library
// ============================================================================

/**
 * @brief Callback für Audio-Sample-Verarbeitung
 * @param outBuff Zeiger auf den Audio-Sample-Puffer
 * @param validSamples Anzahl der gültigen Samples
 * @param continueI2S Zeiger auf I2S-Fortsetzungs-Flag
 *
 * Wird von der Audio-Library aufgerufen und leitet an den AudioManager weiter.
 */
void audio_visualizer_process_i2s(int32_t *outBuff, int16_t validSamples, bool *continueI2S);

#endif // AUDIO_MANAGER_H

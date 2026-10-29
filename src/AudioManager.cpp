#include "AudioManager.h"

#include "AppState.h"    // für die Stummschaltung
#include "Logger.h"      // für das Loggen
#include "StringUtils.h" // für sichere String-Operationen

// ===================================================================
// Datei-lokale Konstanten
// ===================================================================

namespace
{
    // Audio-Sample-Verarbeitung
    constexpr uint16_t MAX_SAMPLE_BUFFER_SIZE = 2048;
    constexpr int16_t MONO_TOLERANCE_THRESHOLD = 4;
    constexpr int32_t INT16_MIN_AS_INT32 = -32768;
    constexpr int32_t INT16_MAX_AS_INT32 = 32767;

    // String-Validierung
    constexpr size_t MIN_STRING_LENGTH_SUSPICIOUS = 2;
    constexpr unsigned char ASCII_MAX = 0x7F;
    constexpr unsigned char PRINTABLE_MIN = 0x20;
    constexpr char UPPERCASE_TO_LOWERCASE_OFFSET = 32; // 'A' -> 'a'

    // UTF-16 BOM Erkennung
    constexpr unsigned char UTF16_BOM_BYTE1 = 0xFF;
    constexpr unsigned char UTF16_BOM_BYTE2 = 0xFE;

    // UTF-8 kodierte UTF-16 BOM
    constexpr unsigned char UTF8_ENCODED_BOM_BYTE1 = 0xC3;
    constexpr unsigned char UTF8_ENCODED_BOM_BYTE2_1 = 0xBF;
    constexpr unsigned char UTF8_ENCODED_BOM_BYTE2_2 = 0xBE;

    /**
     * @brief Konvertiert ein Q31-Sample mit Sättigung in einen 16-Bit-Wert.
     */
    int16_t q31ToInt16(int32_t sample)
    {
        int32_t value = sample >> 16;
        if (value < INT16_MIN_AS_INT32)
            value = INT16_MIN_AS_INT32;
        if (value > INT16_MAX_AS_INT32)
            value = INT16_MAX_AS_INT32;
        return static_cast<int16_t>(value);
    }
}

// ===================================================================
// Globale Instanz
// ===================================================================

AudioManager g_audioManager;

// ===================================================================
// Konstruktor & Initialisierung
// ===================================================================

/**
 * @brief Konstruktor
 */
AudioManager::AudioManager()
    : m_rmsLeft(0.0f),
      m_rmsRight(0.0f),
      m_peakLeft(0.0f),
      m_peakRight(0.0f),
      m_monoSignal(false),
      m_numValidSamples(0)
{
    // Audio-Sample-Puffer mit Nullen initialisieren
    memset(m_sampleBufferLeft, 0, sizeof(m_sampleBufferLeft));
    memset(m_sampleBufferRight, 0, sizeof(m_sampleBufferRight));
}

/**
 * @brief Initialisiert den AudioManager und registriert Callbacks
 */
void AudioManager::init(Audio &audio)
{
    m_audio = &audio;

    // Lambda-Funktion als Callback registrieren
    Audio::audio_info_callback = [this](Audio::msg_t msg)
    {
        this->handleAudioCallback(msg);
    };
}

// ===================================================================
// Event-Queue: Enqueue/Dequeue
// ===================================================================

/**
 * @brief Legt ein Text-Event in die Queue (thread-sicher)
 */
inline void AudioManager::enqueueEvent_(AudioManagerEventType type, const char *text)
{
    if (!text)
        return;

    portENTER_CRITICAL(&m_evtMux);

    const uint8_t head = m_qHead.load(std::memory_order_relaxed);

    if (!qIsFull_())
    {
        // Normales Einfügen
        m_evtQ[head].type = type;
        stringutils::safeCopy(m_evtQ[head].text, text);
        m_evtQ[head].coverData.clear(); // Cover-Daten leer lassen
        m_qHead.store((head + 1) % EVENT_Q_CAP, std::memory_order_relaxed);
    }
    else
    {
        // Queue voll - ältestes Event überschreiben
        m_evtQ[head].type = type;
        stringutils::safeCopy(m_evtQ[head].text, text);
        m_evtQ[head].coverData.clear(); // Cover-Daten leer lassen
        m_qHead.store((head + 1) % EVENT_Q_CAP, std::memory_order_relaxed);
        m_qTail.store((m_qTail.load(std::memory_order_relaxed) + 1) % EVENT_Q_CAP, std::memory_order_relaxed);
    }

    portEXIT_CRITICAL(&m_evtMux);
}

/**
 * @brief Legt ein Cover-Image-Event in die Queue (thread-sicher)
 */
inline void AudioManager::enqueueCoverEvent_(const std::vector<uint32_t> &coverData)
{
    portENTER_CRITICAL(&m_evtMux);

    const uint8_t head = m_qHead.load(std::memory_order_relaxed);

    if (!qIsFull_())
    {
        // Normales Einfügen
        m_evtQ[head].type = AudioManagerEventType::COVER_IMAGE;
        m_evtQ[head].text[0] = '\0'; // Text leer lassen
        m_evtQ[head].coverData = coverData;
        m_qHead.store((head + 1) % EVENT_Q_CAP, std::memory_order_relaxed);
    }
    else
    {
        // Queue voll - ältestes Event überschreiben
        m_evtQ[head].type = AudioManagerEventType::COVER_IMAGE;
        m_evtQ[head].text[0] = '\0'; // Text leer lassen
        m_evtQ[head].coverData = coverData;
        m_qHead.store((head + 1) % EVENT_Q_CAP, std::memory_order_relaxed);
        m_qTail.store((m_qTail.load(std::memory_order_relaxed) + 1) % EVENT_Q_CAP, std::memory_order_relaxed);
    }

    portEXIT_CRITICAL(&m_evtMux);
}

/**
 * @brief Holt das älteste Event aus der Queue
 */
bool AudioManager::dequeueEvent(AudioManagerEventType &type, char *out, size_t len, std::vector<uint32_t> *coverData)
{
    portENTER_CRITICAL(&m_evtMux);

    const bool empty = qIsEmpty_();
    if (!empty)
    {
        const uint8_t tail = m_qTail.load(std::memory_order_relaxed);
        const auto &ev = m_evtQ[tail];
        type = ev.type;

        // Text kopieren (falls vorhanden und Ausgabepuffer gegeben)
        if (out && len)
            stringutils::safeCopy(out, len, ev.text);

        // Cover-Daten kopieren (falls vorhanden und Ausgabepuffer gegeben)
        if (coverData && !ev.coverData.empty())
            *coverData = ev.coverData;

        m_qTail.store((tail + 1) % EVENT_Q_CAP, std::memory_order_relaxed);
    }

    portEXIT_CRITICAL(&m_evtMux);
    return !empty;
}

// ===================================================================
// Metadaten-Parsing
// ===================================================================

/**
 * @brief Vergleicht den Anfang zweier Zeichenfolgen ohne Beachtung der Groß- und Kleinschreibung.
 */
bool AudioManager::startsWithIgnoreCase(const char *str, const char *prefix) const
{
    if (!str || !prefix)
        return false;

    while (*prefix)
    {
        char c1 = *str;
        char c2 = *prefix;

        // Zu Kleinbuchstaben konvertieren für Vergleich
        if (c1 >= 'A' && c1 <= 'Z')
            c1 += UPPERCASE_TO_LOWERCASE_OFFSET;

        if (c2 >= 'A' && c2 <= 'Z')
            c2 += UPPERCASE_TO_LOWERCASE_OFFSET;

        if (c1 != c2)
            return false;

        str++;
        prefix++;
    }

    return true;
}

/**
 * @brief Findet Tag-Wert basierend auf verschiedenen Mustern
 */
const char *AudioManager::findTagValue(const char *str, const char **patterns, int patternCount) const
{
    if (!str)
        return nullptr;

    for (int i = 0; i < patternCount; i++)
    {
        // Zeiger auf den Wert nach dem Tag zurückgeben
        if (startsWithIgnoreCase(str, patterns[i]))
            return str + strlen(patterns[i]);
    }

    return nullptr;
}

/**
 * @brief Prüft ob String sinnvoll ist (nicht nur verstümmelte Zeichen)
 */
bool AudioManager::isValidMetadataString(const char *str) const
{
    if (!str || str[0] == '\0')
        return false;

    size_t len = strlen(str);

    // Zu kurz -> ist verdächtig
    if (len <= MIN_STRING_LENGTH_SUSPICIOUS)
    {
        // Prüfen ob alle Zeichen > 0x7F (High-Bytes = wahrscheinlich verstümmelt)
        bool allHighBytes = true;
        for (size_t i = 0; i < len; i++)
        {
            if ((unsigned char)str[i] <= ASCII_MAX)
            {
                allHighBytes = false;
                break;
            }
        }

        if (allHighBytes)
            return false; // Verstümmelt
    }

    // Prüfen auf bekannte UTF-16-BOM-Zeichen (als UTF-8 kodiert oder roh)
    if (len >= 2)
    {
        unsigned char c0 = (unsigned char)str[0];
        unsigned char c1 = (unsigned char)str[1];

        // Rohe UTF-16 LE BOM (0xFF 0xFE)
        if (c0 == UTF16_BOM_BYTE1 && c1 == UTF16_BOM_BYTE2)
            return false;

        // UTF-8-kodierte BOM (0xC3 0xBF oder 0xC3 0xBE)
        if (c0 == UTF8_ENCODED_BOM_BYTE1 && (c1 == UTF8_ENCODED_BOM_BYTE2_1 || c1 == UTF8_ENCODED_BOM_BYTE2_2))
        {
            // Wenn direkt danach noch 0xC3 0xBx kommt, ist es UTF-16-Müll
            if (len >= 4)
            {
                unsigned char c2 = (unsigned char)str[2];
                unsigned char c3 = (unsigned char)str[3];
                if (c2 == UTF8_ENCODED_BOM_BYTE1 && (c3 == UTF8_ENCODED_BOM_BYTE2_1 || c3 == UTF8_ENCODED_BOM_BYTE2_2))
                    return false;
            }
        }
    }

    return true;
}

/**
 * @brief Parst ID3-Metadaten aus verschiedenen Formaten
 */
bool AudioManager::parseId3Metadata(const char *id3String)
{
    if (!id3String || id3String[0] == '\0')
        return false;

    // Artist-Muster (von spezifisch zu allgemein)
    const char *artistPatterns[] = {
        "Lead artist(s)/Lead performer(s)/Soloist(s)/Performing group: ",
        "ARTIST=",
        "Artist=",
        "Artist: ",
        "Artist:",
        "BAND=",
        "Band=",
        "Band: ",
        "Band:"};

    // Title-Muster
    const char *titlePatterns[] = {
        "Title/Songname/Content description: ",
        "TITLE=",
        "Title=",
        "Title: ",
        "Title:"};

    // Album-Muster
    const char *albumPatterns[] = {
        "Album/Movie/Show title: ",
        "ALBUM=",
        "Album=",
        "Album: ",
        "Album:"};

    bool foundAny = false;
    char cleanedValue[MAX_EVENT_STRING_LENGTH];

    // Artist prüfen
    const char *value = findTagValue(id3String, artistPatterns, sizeof(artistPatterns) / sizeof(artistPatterns[0]));
    if (value)
    {
        if (cleanId3String(value, cleanedValue, sizeof(cleanedValue)) && isValidMetadataString(cleanedValue))
        {
            stringutils::safeCopy(m_metadata.artist, cleanedValue);
            m_metadata.hasArtist = true;
            foundAny = true;
            LOG_AUDIO_F("Metadaten Artist: %s", m_metadata.artist);

            enqueueEvent_(AudioManagerEventType::ID3_ARTIST, m_metadata.artist);
        }
    }

    // Title prüfen
    value = findTagValue(id3String, titlePatterns, sizeof(titlePatterns) / sizeof(titlePatterns[0]));
    if (value)
    {
        if (cleanId3String(value, cleanedValue, sizeof(cleanedValue)) && isValidMetadataString(cleanedValue))
        {
            stringutils::safeCopy(m_metadata.title, cleanedValue);
            m_metadata.hasTitle = true;
            foundAny = true;
            LOG_AUDIO_F("Metadaten Title: %s", m_metadata.title);

            enqueueEvent_(AudioManagerEventType::ID3_TITLE, m_metadata.title);
        }
    }

    // Album prüfen
    value = findTagValue(id3String, albumPatterns, sizeof(albumPatterns) / sizeof(albumPatterns[0]));
    if (value)
    {
        if (cleanId3String(value, cleanedValue, sizeof(cleanedValue)) && isValidMetadataString(cleanedValue))
        {
            stringutils::safeCopy(m_metadata.album, cleanedValue);
            m_metadata.hasAlbum = true;
            foundAny = true;
            LOG_AUDIO_F("Metadaten Album: %s", m_metadata.album);

            enqueueEvent_(AudioManagerEventType::ID3_ALBUM, m_metadata.album);
        }
    }

    return foundAny;
}

/**
 * @brief Bereinigt ID3-Strings und erkennt UTF-16 (ignoriert verstümmelte)
 */
bool AudioManager::cleanId3String(const char *input, char *output, size_t maxLen)
{
    if (!input || !output || maxLen == 0)
        return false;

    size_t len = strlen(input);

    // UTF-16 BOM erkennen und ignorieren (Library kann es nicht dekodieren)
    if (len >= 2 && ((unsigned char)input[0] == UTF16_BOM_BYTE1 && (unsigned char)input[1] == UTF16_BOM_BYTE2))
    {
        // UTF-16 erkannt - Library hat es verstümmelt, ignorieren
        output[0] = '\0';
        return false;
    }

    // Normale Prüfung auf kaputte Zeichen
    bool hasGarbage = false;

    for (size_t i = 0; i < len; i++)
    {
        unsigned char c = input[i];
        // Nicht-druckbare Zeichen (aber NICHT UTF-8 Fortsetzungs-Bytes)
        if ((c < PRINTABLE_MIN && c != 0x00 && c != '\n' && c != '\r') || (c == 0xFF || c == 0xFE))
        {
            hasGarbage = true;
            break;
        }
    }

    if (hasGarbage)
    {
        output[0] = '\0';
        return false;
    }

    // String ist OK (UTF-8 oder ASCII), kopieren
    stringutils::safeCopy(output, maxLen, input);

    // Prüfen ob nicht leer
    size_t i = 0;
    while (output[i] != '\0')
    {
        if (output[i] != ' ' && output[i] != '\t' && output[i] != '\n' && output[i] != '\r')
            return true;

        i++;
    }

    return false;
}

// ===================================================================
// Zentraler Callback-Handler
// ===================================================================

/**
 * @brief Zentraler Callback für alle Audio-Events
 */
void AudioManager::handleAudioCallback(const Audio::msg_t &msg)
{
    // Event-Typ auswerten und an entsprechenden Handler weiterleiten
    switch (msg.e)
    {
    case Audio::evt_info:
        handleInfo(msg);
        break;

    case Audio::evt_id3data:
        handleId3Data(msg);
        break;

    case Audio::evt_eof:
        handleEof(msg);
        break;

    case Audio::evt_name:
        handleStationName(msg);
        break;

    case Audio::evt_icydescription:
        handleIcyDescription(msg);
        break;

    case Audio::evt_streamtitle:
        handleStreamTitle(msg);
        break;

    case Audio::evt_bitrate:
        handleBitrate(msg);
        break;

    case Audio::evt_icyurl:
        handleIcyUrl(msg);
        break;

    case Audio::evt_icylogo:
        handleIcyLogo(msg);
        break;

    case Audio::evt_lasthost:
        handleLastHost(msg);
        break;

    case Audio::evt_image:
        handleCoverImage(msg);
        break;

    case Audio::evt_lyrics:
        handleLyrics(msg);
        break;

    case Audio::evt_log:
        // Log-Events ignorieren
        break;

    default:
        // Unbekanntes Event ignorieren
        break;
    }
}

// ===================================================================
// Event-Handler-Implementierungen
// ===================================================================

/**
 * @brief Verarbeitet allgemeine Audio-Informationen
 */
void AudioManager::handleInfo(const Audio::msg_t &msg)
{
    if (!msg.msg)
        return;

    enqueueEvent_(AudioManagerEventType::INFO, msg.msg);
}

/**
 * @brief Verarbeitet ID3-Tag-Daten
 */
void AudioManager::handleId3Data(const Audio::msg_t &msg)
{
    if (!msg.msg)
        return;

    LOG_AUDIO_F("ID3 Data: %s", msg.msg);

    // Metadaten parsen (fügt selbst Events in Queue ein)
    parseId3Metadata(msg.msg);
}

/**
 * @brief Behandelt das Ende einer Audio-Datei/eines Streams
 */
void AudioManager::handleEof(const Audio::msg_t &msg)
{
    const char *eofText = msg.msg ? msg.msg : "EOF";

    LOG_AUDIO_F("Audio EOF: %s", eofText);

    enqueueEvent_(AudioManagerEventType::EOF_AUDIO, eofText);
}

/**
 * @brief Verarbeitet Stations-Namen
 */
void AudioManager::handleStationName(const Audio::msg_t &msg)
{
    if (!msg.msg)
        return;

    LOG_AUDIO_F("Station Name: %s", msg.msg);

    enqueueEvent_(AudioManagerEventType::STATION_NAME, msg.msg);
}

/**
 * @brief Verarbeitet ICY-Stream-Beschreibung
 */
void AudioManager::handleIcyDescription(const Audio::msg_t &msg)
{
    if (!msg.msg)
        return;

    LOG_AUDIO_F("ICY Description: %s", msg.msg);

    enqueueEvent_(AudioManagerEventType::ICY_DESCRIPTION, msg.msg);
}

/**
 * @brief Verarbeitet Stream-Titel
 */
void AudioManager::handleStreamTitle(const Audio::msg_t &msg)
{
    if (!msg.msg)
        return;

    LOG_AUDIO_F("Stream Title: %s", msg.msg);

    enqueueEvent_(AudioManagerEventType::STREAM_TITLE, msg.msg);
}

/**
 * @brief Verarbeitet Bitrate-Informationen
 */
void AudioManager::handleBitrate(const Audio::msg_t &msg)
{
    if (!msg.msg)
        return;

    LOG_AUDIO_F("Bitrate: %s", msg.msg);

    enqueueEvent_(AudioManagerEventType::BITRATE, msg.msg);
}

/**
 * @brief Verarbeitet ICY-URL
 */
void AudioManager::handleIcyUrl(const Audio::msg_t &msg)
{
    if (!msg.msg)
        return;

    LOG_AUDIO_F("ICY URL: %s", msg.msg);

    enqueueEvent_(AudioManagerEventType::ICY_URL, msg.msg);
}

/**
 * @brief Verarbeitet ICY-Logo-Informationen
 */
void AudioManager::handleIcyLogo(const Audio::msg_t &msg)
{
    if (!msg.msg)
        return;

    LOG_AUDIO_F("ICY Logo: %s", msg.msg);

    enqueueEvent_(AudioManagerEventType::ICY_LOGO, msg.msg);
}

/**
 * @brief Verarbeitet Last-Host-Informationen
 */
void AudioManager::handleLastHost(const Audio::msg_t &msg)
{
    if (!msg.msg)
        return;

    LOG_AUDIO_F("Last Host: %s", msg.msg);

    enqueueEvent_(AudioManagerEventType::LAST_HOST, msg.msg);
}

/**
 * @brief Verarbeitet Cover-Image-Daten
 */
void AudioManager::handleCoverImage(const Audio::msg_t &msg)
{
    LOG_AUDIO_F("Cover Image: %u entries", (unsigned)msg.vec.size() / 2);

    enqueueCoverEvent_(msg.vec);
}

/**
 * @brief Verarbeitet Liedtext-Daten
 */
void AudioManager::handleLyrics(const Audio::msg_t &msg)
{
    if (!msg.msg)
        return;

    LOG_AUDIO_F("Lyrics: %s", msg.msg);

    enqueueEvent_(AudioManagerEventType::LYRICS, msg.msg);
}

// ===================================================================
// Audio-Sample-Verarbeitung
// ===================================================================

/**
 * @brief Audio-Sample-Verarbeitung für 32-Bit-Samples
 */
void AudioManager::task_processAudioSamples32(int32_t *outBuff, int16_t validSamples, bool *continueI2S)
{
    { extern volatile uint32_t g_audioCbCount; g_audioCbCount = g_audioCbCount + 1; } // Messzähler Datennachschub aus der Audio-Library
    if (continueI2S)
        *continueI2S = true;

    if (!outBuff || validSamples <= 0)
        return;

    if (m_dataReady.load(std::memory_order_acquire))
        return;

    const uint16_t samplesToProcess = (validSamples > static_cast<int16_t>(MAX_SAMPLE_BUFFER_SIZE))
                                          ? static_cast<uint16_t>(MAX_SAMPLE_BUFFER_SIZE)
                                          : static_cast<uint16_t>(validSamples);

    int32_t currentMaxL = 0;
    int32_t currentMaxR = 0;
    int64_t currentSumSquareL = 0;
    int64_t currentSumSquareR = 0;
    uint32_t monoSampleCount = 0;

    const int32_t *__restrict__ src = outBuff;
    int16_t *__restrict__ dstL = m_sampleBufferLeft;
    int16_t *__restrict__ dstR = m_sampleBufferRight;

    for (uint16_t i = 0; i < samplesToProcess; ++i)
    {
        const int32_t sampleL = q31ToInt16(src[0]);
        const int32_t sampleR = q31ToInt16(src[1]);
        src += 2;

        dstL[i] = static_cast<int16_t>(sampleL);
        dstR[i] = static_cast<int16_t>(sampleR);

        currentSumSquareL += sampleL * sampleL;
        currentSumSquareR += sampleR * sampleR;

        const int32_t absL = (sampleL < 0) ? -sampleL : sampleL;
        const int32_t absR = (sampleR < 0) ? -sampleR : sampleR;
        if (absL > currentMaxL)
            currentMaxL = absL;
        if (absR > currentMaxR)
            currentMaxR = absR;

        const int32_t diff = sampleL - sampleR;
        const int32_t absDiff = (diff < 0) ? -diff : diff;
        if (absDiff < MONO_TOLERANCE_THRESHOLD)
            ++monoSampleCount;
    }

    const float invSamples = 1.0f / static_cast<float>(samplesToProcess);

    // Gemessen wird vor dem Lautstärkeregler, die Werte sind also Programmpegel. Der Ausschlag
    // hängt damit am Sender und nicht daran, wie laut gerade gehört wird.
    m_rmsLeft = sqrtf(static_cast<float>(currentSumSquareL) * invSamples) * RMS_SCALE_FACTOR;
    m_rmsRight = sqrtf(static_cast<float>(currentSumSquareR) * invSamples) * RMS_SCALE_FACTOR;

    m_peakLeft = static_cast<float>(currentMaxL) * PEAK_SCALE_FACTOR;
    m_peakRight = static_cast<float>(currentMaxR) * PEAK_SCALE_FACTOR;

    // Stumm heißt: alle Anzeigen auf null. Die Stummschaltung senkt die Lautstärke, und die
    // liegt hinter der Messstelle; ohne diese Abfrage liefen Zeiger und Balken einfach weiter.
    if (APP_STATE.getIsMuted())
    {
        memset(dstL, 0, static_cast<size_t>(samplesToProcess) * sizeof(int16_t));
        memset(dstR, 0, static_cast<size_t>(samplesToProcess) * sizeof(int16_t));
        m_rmsLeft = m_rmsRight = 0.0f;
        m_peakLeft = m_peakRight = 0.0f;
    }

    const float monoRatio = static_cast<float>(monoSampleCount) * invSamples;
    m_monoSignal = (monoRatio > MONO_THRESHOLD_FACTOR);

    m_numValidSamples = samplesToProcess;
    m_sampleRate = m_audio ? m_audio->getSampleRate() : 0;
    m_dataReady.store(true, std::memory_order_release);
}

// ===================================================================
// Getter-Funktionen für Audio-Daten
// ===================================================================

/**
 * @brief Prüft, ob neue Audio-Daten verfügbar sind
 */
bool AudioManager::isAudioDataReady() const
{
    return m_dataReady.load(std::memory_order_acquire);
}

/**
 * @brief Liefert RMS-Wert links
 */
float AudioManager::getRMSLeft() const
{
    return m_rmsLeft;
}

/**
 * @brief Liefert RMS-Wert rechts
 */
float AudioManager::getRMSRight() const
{
    return m_rmsRight;
}

/**
 * @brief Liefert Peak-Wert links
 */
float AudioManager::getPeakLeft() const
{
    return m_peakLeft;
}

/**
 * @brief Liefert Peak-Wert rechts
 */
float AudioManager::getPeakRight() const
{
    return m_peakRight;
}

/**
 * @brief Prüft ob Mono-Signal
 */
bool AudioManager::isMono() const
{
    return m_monoSignal;
}

/**
 * @brief Liefert Anzahl der gültigen Samples
 */
uint16_t AudioManager::getValidSamplesCount() const
{
    return m_numValidSamples;
}

/**
 * @brief Liefert die Abtastrate des gepufferten Sample-Blocks
 */
uint32_t AudioManager::getSampleRate() const
{
    return m_sampleRate;
}

/**
 * @brief Liefert Sample-Buffer links
 */
const int16_t *AudioManager::getSampleBufferLeft() const
{
    return m_sampleBufferLeft;
}

/**
 * @brief Liefert Sample-Buffer rechts
 */
const int16_t *AudioManager::getSampleBufferRight() const
{
    return m_sampleBufferRight;
}

/**
 * @brief Markiert Audio-Daten als verarbeitet
 */
void AudioManager::consumeAudioData()
{
    m_dataReady.store(false, std::memory_order_release);
}

// ===================================================================
// Callback-Funktion für Audio-Library
// ===================================================================

/**
 * @brief Callback für Audio-Sample-Verarbeitung
 */
void audio_visualizer_process_i2s(int32_t *outBuff, int16_t validSamples, bool *continueI2S)
{
    g_audioManager.task_processAudioSamples32(outBuff, validSamples, continueI2S);
}

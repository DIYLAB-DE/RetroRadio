#include "HttpService.h"

#include <WiFiClient.h>
#include <esp_heap_caps.h>
#include "miniz.h"

#include "Logger.h"

// ===================================================================
// Kennung gegenüber fremden Diensten
// ===================================================================

// Die Firmware-Kennung kommt über das Build-Flag FW_VERSION.
#ifndef FW_VERSION
#define FW_VERSION "1.0"
#endif

namespace
{
    constexpr const char *HTTP_USER_AGENT = "RetroRadio/" FW_VERSION " (+https://github.com/DIYLAB-DE)"; ///< Kennung, die Nominatim für seine Nutzungsregeln verlangt
}

// ===================================================================
// INTERNE HELFER
// ===================================================================

namespace
{
    /**
     * @brief Gibt einen FreeRTOS-Mutex beim Verlassen des Scopes frei.
     */
    class MutexGuard
    {
    public:
        /**
         * @brief Übernimmt den angegebenen Mutex für die Lebensdauer des Guards.
         */
        explicit MutexGuard(SemaphoreHandle_t mutex)
            : m_mutex(mutex)
        {
            m_locked = m_mutex && xSemaphoreTake(m_mutex, portMAX_DELAY) == pdTRUE;
        }

        /**
         * @brief Gibt den gehaltenen Mutex frei.
         */
        ~MutexGuard()
        {
            if (m_locked)
                xSemaphoreGive(m_mutex);
        }

        /**
         * @brief Prüft, ob der Mutex erfolgreich übernommen wurde.
         */
        bool locked() const { return m_locked; }

    private:
        SemaphoreHandle_t m_mutex = nullptr; ///< Gesperrter Mutex
        bool m_locked = false;               ///< true wenn der Mutex gehalten wird
    };

    /**
     * @brief Nur-Schreib-Stream, der in einen festen Puffer schreibt
     *
     * Dient als Senke für HTTPClient::writeToStream(): der de-chunkte Response-Body wird blockweise direkt in den übergebenen PSRAM-Puffer geschrieben, ohne Zwischen-String. Bei vollem Puffer werden weitere Bytes verworfen und truncated() meldet true. Optional wird gedrosselt gelesen (Blöcke mit Pausen), und ein gesetztes Abbruch-Flag lässt write() nichts mehr annehmen, womit writeToStream sofort zurückkehrt.
     */
    class BufferStream : public Stream
    {
    public:
        /**
         * @brief Initialisiert den Stream mit einem festen Zielpuffer.
         */
        BufferStream(char *buffer, size_t capacity, bool throttled = false, const std::atomic<bool> *abortFlag = nullptr)
            : m_buffer(buffer), m_capacity(capacity), m_throttled(throttled), m_abortFlag(abortFlag) {}

        /**
         * @brief Schreibt ein einzelnes Byte in den Zielpuffer.
         */
        size_t write(uint8_t b) override
        {
            if (aborted())
                return 0;

            if (m_length + 1 >= m_capacity)
            {
                m_truncated = true;
                return 0;
            }
            m_buffer[m_length++] = static_cast<char>(b);
            throttlePause(1);
            return 1;
        }

        /**
         * @brief Schreibt einen Datenblock in den Zielpuffer.
         */
        size_t write(const uint8_t *data, size_t size) override
        {
            if (aborted())
                return 0;

            if (m_length >= m_capacity - 1)
            {
                m_truncated = true;
                return 0;
            }
            const size_t space = (m_capacity - 1) - m_length;
            const size_t n = (size < space) ? size : space;
            memcpy(m_buffer + m_length, data, n);
            m_length += n;
            if (n < size)
                m_truncated = true;
            throttlePause(n);
            return n;
        }

        /**
         * @brief Meldet, dass der schreibgeschützte Stream keine lesbaren Bytes bereitstellt.
         */
        int available() override { return 0; }

        /**
         * @brief Weist Lesezugriffe auf den schreibgeschützten Stream zurück.
         */
        int read() override { return -1; }

        /**
         * @brief Weist Vorschauzugriffe auf den schreibgeschützten Stream zurück.
         */
        int peek() override { return -1; }

        /**
         * @brief Liefert die Anzahl der geschriebenen Bytes.
         */
        size_t length() const { return m_length; }    ///< Anzahl geschriebener Bytes

        /**
         * @brief Prüft, ob Daten wegen eines vollen Puffers verworfen wurden.
         */
        bool truncated() const { return m_truncated; } ///< true wenn der Puffer überlief

    private:
        /**
         * @brief Meldet, ob der Abbruch angefordert wurde (write nimmt dann nichts mehr an, writeToStream bricht ab).
         */
        bool aborted() const { return m_abortFlag && m_abortFlag->load(); }

        /**
         * @brief Legt bei gedrosseltem Empfang nach jedem vollen Block eine kurze Pause ein.
         */
        void throttlePause(size_t written)
        {
            if (!m_throttled)
                return;

            m_sincePause += written;
            if (m_sincePause >= HttpServiceCfg::THROTTLE_CHUNK_BYTES)
            {
                m_sincePause = 0;
                vTaskDelay(pdMS_TO_TICKS(HttpServiceCfg::THROTTLE_PAUSE_MS));
            }
        }

        char *m_buffer;           ///< Ziel-Puffer
        size_t m_capacity;        ///< Kapazität des Puffers in Bytes
        size_t m_length = 0;      ///< Bisher geschriebene Bytes
        bool m_truncated = false; ///< Puffer-Überlauf aufgetreten
        bool m_throttled = false; ///< true = Pausen zwischen den Blöcken einlegen
        size_t m_sincePause = 0;  ///< Geschriebene Bytes seit der letzten Pause
        const std::atomic<bool> *m_abortFlag = nullptr; ///< Optionales Abbruch-Flag (z.B. Modul-Shutdown)
    };
}

// ===================================================================
// SINGLETON
// ===================================================================

/**
 * @brief Singleton-Instanz
 */
HttpService& HttpService::instance()
{
    static HttpService instance;
    return instance;
}

/**
 * @brief Destruktor - gibt PSRAM frei
 */
HttpService::~HttpService()
{
    end();
}

// ===================================================================
// INITIALISIERUNG
// ===================================================================

/**
 * @brief Initialisiert den Service und allokiert PSRAM-Buffer
 */
bool HttpService::begin()
{
    if (m_initialized)
    {
        LOG_WARNING("HttpService bereits initialisiert");
        return true;
    }

    // Prüfe PSRAM-Verfügbarkeit
    if (!psramFound())
    {
        LOG_ERROR("HttpService: Kein PSRAM verfügbar!");
        return false;
    }

    const size_t freePsram  = ESP.getFreePsram();
    const size_t bufferSize = HttpServiceCfg::RESPONSE_BUFFER_SIZE;
    const size_t headroom   = HttpServiceCfg::JSON_PSRAM_HEADROOM;

    if (freePsram < bufferSize + headroom)
    {
        LOG_ERROR_F("HttpService: Nicht genug PSRAM! Benoetigt: %u KB + %u KB Reserve, Frei: %u KB",
                    bufferSize / 1024, headroom / 1024, freePsram / 1024);
        return false;
    }

    // Response-Buffer im PSRAM allokieren
    m_responseBuffer = (char*)ps_malloc(bufferSize);
    if (!m_responseBuffer)
    {
        LOG_ERROR("HttpService: PSRAM-Allokation für Response-Buffer fehlgeschlagen");
        return false;
    }

    // Buffer mit Nullbyte initialisieren
    m_responseBuffer[0] = '\0';

    m_initialized = true;
    m_successCount = 0;
    m_errorCount = 0;

    const size_t freePsramAfter = ESP.getFreePsram();
    LOG_SUCCESS_F("HttpService initialisiert (Buffer: %u KB in PSRAM, Frei: %.2f MB)",
                  HttpServiceCfg::RESPONSE_BUFFER_SIZE / 1024,
                  freePsramAfter / (1024.0f * 1024.0f));

    return true;
}

/**
 * @brief Gibt alle Ressourcen frei
 */
void HttpService::end()
{
    if (m_responseBuffer)
    {
        free(m_responseBuffer);
        m_responseBuffer = nullptr;
    }

    if (m_compressedBuffer)
    {
        free(m_compressedBuffer);
        m_compressedBuffer = nullptr;
    }

    if (m_inflater)
    {
        free(m_inflater);
        m_inflater = nullptr;
    }

    if (m_mutex)
    {
        vSemaphoreDelete(m_mutex);
        m_mutex = nullptr;
    }

    m_initialized = false;
    LOG_INFO("HttpService beendet");
}

// ===================================================================
// HTTP METHODEN
// ===================================================================

/**
 * @brief Führt eine HTTP-GET-Anfrage aus.
 */
HttpResponse HttpService::get(const char* url, std::atomic<bool>* abortFlag)
{
    return executeRequest(url, "GET", nullptr, nullptr, abortFlag);
}

/**
 * @brief Führt eine HTTP-GET-Anfrage mit gedrosseltem Empfang aus.
 */
HttpResponse HttpService::getThrottled(const char* url, std::atomic<bool>* abortFlag)
{
    return executeRequest(url, "GET", nullptr, nullptr, abortFlag, nullptr, 0, nullptr, 0, true);
}

/**
 * @brief Führt eine HTTP-GET-Anfrage mit gzip-Kompression und gedrosseltem Empfang aus.
 */
HttpResponse HttpService::getCompressed(const char* url, std::atomic<bool>* abortFlag)
{
    return executeRequest(url, "GET", nullptr, nullptr, abortFlag, nullptr, 0, nullptr, 0, true, true);
}

/**
 * @brief Führt eine HTTP-GET-Anfrage mit Zusatzheadern aus.
 */
HttpResponse HttpService::getWithHeaders(const char* url, const HttpHeader *headers,
                                         size_t headerCount,
                                         std::atomic<bool>* abortFlag)
{
    return executeRequest(url, "GET", nullptr, nullptr, abortFlag, headers, headerCount);
}

/**
 * @brief Führt eine HTTP-GET-Anfrage mit Zusatzheadern und eigener Antwortkopie aus.
 */
HttpResponse HttpService::getWithHeadersCopy(const char* url, const HttpHeader *headers,
                                             size_t headerCount,
                                             char *responseBuffer,
                                             size_t responseBufferSize,
                                             std::atomic<bool>* abortFlag)
{
    return executeRequest(url, "GET", nullptr, nullptr, abortFlag, headers, headerCount,
                          responseBuffer, responseBufferSize);
}

/**
 * @brief HTTP POST Request
 */
HttpResponse HttpService::post(const char* url, const char* payload,
                               const char* contentType, std::atomic<bool>* abortFlag)
{
    return executeRequest(url, "POST", payload, contentType, abortFlag);
}

/**
 * @brief HTTP POST Request mit eigener Response-Kopie.
 */
HttpResponse HttpService::postCopy(const char* url, const char* payload,
                                   const char* contentType,
                                   char *responseBuffer,
                                   size_t responseBufferSize,
                                   std::atomic<bool>* abortFlag)
{
    return executeRequest(url, "POST", payload, contentType, abortFlag, nullptr, 0,
                          responseBuffer, responseBufferSize);
}

/**
 * @brief HTTP POST Request mit Zusatzheadern
 */
HttpResponse HttpService::postWithHeaders(const char* url, const char* payload,
                                          const char* contentType,
                                          const HttpHeader *headers,
                                          size_t headerCount,
                                          std::atomic<bool>* abortFlag)
{
    return executeRequest(url, "POST", payload, contentType, abortFlag, headers, headerCount);
}

/**
 * @brief Zentrale Request-Implementierung
 */
HttpResponse HttpService::executeRequest(const char* url, const char* method,
                                         const char* payload, const char* contentType,
                                         std::atomic<bool>* abortFlag,
                                         const HttpHeader *headers,
                                         size_t headerCount,
                                         char *responseCopyBuffer,
                                         size_t responseCopyBufferSize,
                                         bool throttled,
                                         bool compressed)
{
    HttpResponse response;
    response.httpCode = -1;
    response.data = nullptr;
    response.length = 0;
    response.requestTimeMs = 0;

    if (!ensureMutex())
    {
        response.httpCode = static_cast<int>(HttpError::BeginFailed);
        return response;
    }

    MutexGuard guard(m_mutex);
    if (!guard.locked())
    {
        response.httpCode = static_cast<int>(HttpError::BeginFailed);
        return response;
    }

    // Validierung
    if (!m_initialized)
    {
        LOG_ERROR("HttpService: Nicht initialisiert!");
        response.httpCode = static_cast<int>(HttpError::WiFiDisconnected);
        return response;
    }

    if (!url || url[0] == '\0')
    {
        LOG_ERROR("HttpService: URL ist leer");
        response.httpCode = static_cast<int>(HttpError::EmptyUrl);
        return response;
    }

    // Buffer leeren
    m_responseBuffer[0] = '\0';

    HTTPClient http;
    http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
    http.setRedirectLimit(HttpServiceCfg::HTTP_REDIRECT_LIMIT);
    http.setTimeout(m_timeoutMs);
    // HTTP/1.0 erzwingt Content-Length oder Connection-Close statt Chunked-Transfer. Komprimierte Antworten brauchen HTTP/1.1, weil der HTTPClient den Accept-Encoding-Header nur dann sendet; Chunked-Transfer decodiert er in writeToStream selbst.
    http.useHTTP10(!compressed);
    if (compressed)
        http.setAcceptEncoding("gzip");

    int httpCode = -1;
    const unsigned long totalStartTime = millis();

    // POST-Requests werden nie wiederholt - Seiteneffekte könnten doppelt ausgelöst werden
    const bool isPost      = (strcmp(method, "POST") == 0);
    const int  maxAttempts = isPost ? 1 : m_maxAttempts;

    // Retry-Loop (bei POST genau ein Versuch)
    for (int retry = 0; retry < maxAttempts; ++retry)
    {
        // Abbruch prüfen
        if (abortFlag && abortFlag->load())
        {
            LOG_INFO("HttpService: Request abgebrochen (Shutdown)");
            response.httpCode = static_cast<int>(HttpError::Aborted);
            m_errorCount++;
            return response;
        }

        // Bei Retry kurz warten
        if (retry > 0)
        {
            LOG_INFO_F("HttpService: Retry %d/%d nach Fehler (Code: %d)",
                       retry, maxAttempts, httpCode);
            // Retry-Delay in 50-ms-Scheiben, damit abortFlag zeitnah wirkt
            constexpr int RETRY_SLICE_MS = 50;
            for (int waited = 0; waited < HttpServiceCfg::HTTP_RETRY_DELAY_MS; waited += RETRY_SLICE_MS)
            {
                if (abortFlag && abortFlag->load())
                    break;
                vTaskDelay(pdMS_TO_TICKS(RETRY_SLICE_MS));
            }
        }

        // Verbindung aufbauen
        if (!http.begin(url))
        {
            LOG_ERROR_F("HttpService: http.begin() fehlgeschlagen für: %s", url);
            httpCode = static_cast<int>(HttpError::BeginFailed);
            continue;
        }

        // Standard-Header setzen. Die Kennung nennt Anwendung, Fassung und Fundstelle, weil Nominatim das für seine Nutzungsregeln verlangt und andere Dienste eine erkennbare Kennung erwarten.
        http.addHeader("User-Agent", HTTP_USER_AGENT);
        http.addHeader("Accept", "application/json");

        if (compressed)
        {
            const char* collectKeys[] = {"Content-Encoding"};
            http.collectHeaders(collectKeys, 1);
        }
        
        if (contentType && payload)
        {
            http.addHeader("Content-Type", contentType);
        }

        for (size_t i = 0; headers && i < headerCount; ++i)
        {
            if (headers[i].name && headers[i].name[0] && headers[i].value)
                http.addHeader(headers[i].name, headers[i].value);
        }

        // Request ausführen
        const unsigned long requestStart = millis();

        if (isPost && payload)
        {
            httpCode = http.POST((uint8_t*)payload, strlen(payload));
        }
        else
        {
            httpCode = http.GET();
        }

        response.requestTimeMs = millis() - requestStart;

        LOG_INFO_F("HttpService: Response nach %lu ms, Code: %d", 
                   response.requestTimeMs, httpCode);

        // Bei Erfolg oder Client-Fehler (4xx) nicht wiederholen
        if (httpCode >= 0 && httpCode < 500)
            break;

        http.end();
    }

    // Abbruch nochmal prüfen
    if (abortFlag && abortFlag->load())
    {
        http.end();
        response.httpCode = static_cast<int>(HttpError::Aborted);
        m_errorCount++;
        return response;
    }

    response.httpCode = httpCode;

    // Bei Erfolg (alle 2xx): Response lesen wenn Body vorhanden
    if (httpCode >= 200 && httpCode < 300)
    {
        if (httpCode == HTTP_CODE_NO_CONTENT)
        {
            // 204: gültiger Erfolg, kein Body erwartet
            m_successCount++;
            LOG_INFO("HttpService: 204 No Content");
        }
        else
        {
            size_t bytesRead     = 0;
            size_t expectedBytes = 0;
            bool   truncated     = false;
            int    readError     = static_cast<int>(HttpError::ReadFailed);
            bool   readOk        = false;

            // Komprimierte Antwort: Body zunächst in den Kompressionspuffer lesen, dann in den Antwortpuffer entpacken
            const bool gzipBody = compressed && http.header("Content-Encoding").indexOf("gzip") >= 0;

            if (gzipBody)
            {
                if (ensureCompressedBuffers() &&
                    readResponseToBuffer(http, m_compressedBuffer, HttpServiceCfg::COMPRESSED_BUFFER_SIZE, bytesRead, expectedBytes, truncated, abortFlag, throttled) &&
                    !truncated)
                {
                    readOk = inflateGzip(m_compressedBuffer, bytesRead, bytesRead);
                    expectedBytes = bytesRead;
                    if (!readOk)
                        readError = static_cast<int>(HttpError::DecompressFailed);
                }
            }
            else
            {
                readOk = readResponseToBuffer(http, m_responseBuffer, HttpServiceCfg::RESPONSE_BUFFER_SIZE, bytesRead, expectedBytes, truncated, abortFlag, throttled);
            }

            if (readOk)
            {
                response.data           = m_responseBuffer;
                response.length         = bytesRead;
                response.expectedLength = expectedBytes;
                response.truncated      = truncated;
                m_successCount++;

                if (truncated)
                {
                    LOG_WARNING_F("HttpService: %u Bytes empfangen (ABGESCHNITTEN von %u)",
                                  bytesRead, expectedBytes);
                }
                else
                {
                    LOG_INFO_F("HttpService: %u Bytes empfangen", bytesRead);
                }
            }
            else
            {
                LOG_ERROR("HttpService: Konnte Response nicht lesen");
                response.httpCode = readError;
                m_errorCount++;
            }
        }
    }
    else
    {
        m_errorCount++;

        if (httpCode > 0)
        {
            LOG_WARNING_F("HttpService: HTTP-Fehler %d", httpCode);
        }
        else
        {
            LOG_ERROR_F("HttpService: Verbindungsfehler %d", httpCode);
        }
    }

    http.end();
    response.requestTimeMs = millis() - totalStartTime;

    if (responseCopyBuffer && responseCopyBufferSize > 0 && response.data && response.length > 0)
    {
        size_t copyLen = response.length;
        if (copyLen >= responseCopyBufferSize)
        {
            copyLen = responseCopyBufferSize - 1;
            response.truncated = true;
        }

        memcpy(responseCopyBuffer, response.data, copyLen);
        responseCopyBuffer[copyLen] = '\0';
        response.data = responseCopyBuffer;
        response.length = copyLen;
    }

    return response;
}

/**
 * @brief Erstellt den Mutex für den gemeinsamen Response-Buffer bei Bedarf.
 */
bool HttpService::ensureMutex()
{
    if (m_mutex)
        return true;

    m_mutex = xSemaphoreCreateMutex();
    if (!m_mutex)
        LOG_ERROR("HttpService: Mutex konnte nicht erstellt werden");

    return m_mutex != nullptr;
}

/**
 * @brief Liest Response-Daten in PSRAM-Buffer
 *
 * Schreibt den Body bei unbekannter Länge (HTTP/1.0 Connection-Close) per writeToStream direkt in den Puffer, liest bei bekannter Content-Length direkt vom Stream.
 */
bool HttpService::readResponseToBuffer(HTTPClient& http, char* buffer, size_t capacity, size_t& outLength,
                                       size_t& outExpected, bool& outTruncated,
                                       const std::atomic<bool>* abortFlag, bool throttled)
{
    outLength    = 0;
    outExpected  = 0;
    outTruncated = false;

    const int contentLength = http.getSize();
    LOG_INFO_F("HttpService: Content-Length: %d", contentLength);

    if (contentLength <= 0)
    {
        LOG_INFO("HttpService: Unbekannte Länge - schreibe Body direkt in den Puffer");

        BufferStream sink(buffer, capacity, throttled, abortFlag);
        const int written = http.writeToStream(&sink);

        buffer[sink.length()] = '\0';
        outLength    = sink.length();
        outExpected  = sink.length();
        outTruncated = sink.truncated();

        if (outLength == 0)
        {
            LOG_ERROR_F("HttpService: Leere Response (writeToStream: %d)", written);
            return false;
        }

        if (outTruncated)
        {
            LOG_WARNING_F("HttpService: Response ABGESCHNITTEN bei %u Bytes (Puffer voll)", outLength);
        }
        else
        {
            LOG_INFO_F("HttpService: %u Bytes via writeToStream gelesen", outLength);
        }
        return true;
    }

    // Bekannte Content-Length: direkt vom Stream lesen
    LOG_INFO("HttpService: Bekannte Länge - lese direkt vom Stream");

    WiFiClient* stream = http.getStreamPtr();
    if (!stream)
    {
        LOG_ERROR("HttpService: Kein Stream verfügbar");
        return false;
    }

    const size_t maxRead = min((size_t)contentLength, capacity - 1);
    outExpected = (size_t)contentLength;
    size_t totalRead = 0;
    unsigned long lastDataTime = millis();
    const unsigned long DATA_TIMEOUT_MS = HttpServiceCfg::HTTP_STREAM_TIMEOUT_MS;

    while (totalRead < maxRead)
    {
        if (abortFlag && abortFlag->load())
        {
            LOG_INFO_F("HttpService: Stream-Lesen abgebrochen nach %u Bytes", totalRead);
            break;
        }

        if (millis() - lastDataTime > DATA_TIMEOUT_MS)
        {
            LOG_WARNING_F("HttpService: Daten-Timeout nach %u Bytes", totalRead);
            break;
        }

        const int available = stream->available();

        if (available > 0)
        {
            size_t toRead = min((size_t)available, maxRead - totalRead);
            if (throttled)
                toRead = min(toRead, HttpServiceCfg::THROTTLE_CHUNK_BYTES);

            const int bytesRead = stream->readBytes(buffer + totalRead, toRead);

            if (bytesRead > 0)
            {
                totalRead    += bytesRead;
                lastDataTime  = millis();

                // Pause je Block: bremst über das TCP-Fenster auch den Sender
                if (throttled && totalRead < maxRead)
                    vTaskDelay(pdMS_TO_TICKS(HttpServiceCfg::THROTTLE_PAUSE_MS));
            }
        }
        else
        {
            if (totalRead >= maxRead)
                break;

            if (!stream->connected())
            {
                LOG_INFO_F("HttpService: Verbindung geschlossen nach %u Bytes", totalRead);
                break;
            }

            vTaskDelay(pdMS_TO_TICKS(10));
        }
    }

    buffer[totalRead] = '\0';
    outLength = totalRead;

    if (totalRead >= maxRead && (size_t)contentLength > capacity - 1)
    {
        outTruncated = true;
        LOG_WARNING_F("HttpService: Response ABGESCHNITTEN - %u von %u Bytes gelesen",
                      totalRead, (size_t)contentLength);
    }

    return totalRead > 0;
}

/**
 * @brief Legt Kompressionspuffer und Entpackerzustand bei Bedarf einmalig im PSRAM an
 */
bool HttpService::ensureCompressedBuffers()
{
    if (!m_compressedBuffer)
    {
        m_compressedBuffer = static_cast<char*>(heap_caps_malloc(HttpServiceCfg::COMPRESSED_BUFFER_SIZE, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT));
        if (!m_compressedBuffer)
            m_compressedBuffer = static_cast<char*>(malloc(HttpServiceCfg::COMPRESSED_BUFFER_SIZE));
    }

    if (!m_inflater)
    {
        m_inflater = static_cast<tinfl_decompressor*>(heap_caps_malloc(sizeof(tinfl_decompressor), MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT));
        if (!m_inflater)
            m_inflater = static_cast<tinfl_decompressor*>(malloc(sizeof(tinfl_decompressor)));
    }

    if (!m_compressedBuffer || !m_inflater)
        LOG_ERROR("HttpService: Allokation für gzip-Puffer fehlgeschlagen");

    return m_compressedBuffer && m_inflater;
}

/**
 * @brief Entpackt einen gzip-Body in den Antwortpuffer
 *
 * Überspringt den gzip-Kopf samt optionaler Felder, entpackt den Deflate-Strom mit tinfl aus dem ROM in den Antwortpuffer und vergleicht die Originalgröße aus dem gzip-Ende. Der Antwortpuffer wird nullterminiert.
 */
bool HttpService::inflateGzip(const char* input, size_t inputLength, size_t& outLength)
{
    constexpr size_t GZIP_HEADER_SIZE = 10;
    constexpr size_t GZIP_TRAILER_SIZE = 8;
    constexpr size_t GZIP_SIZE_FIELD = 4;
    constexpr size_t GZIP_EXTRA_LEN_FIELD = 2;
    constexpr uint8_t GZIP_ID1 = 0x1F;
    constexpr uint8_t GZIP_ID2 = 0x8B;
    constexpr uint8_t GZIP_METHOD_DEFLATE = 8;
    constexpr uint8_t GZIP_FLAG_HCRC = 0x02;
    constexpr uint8_t GZIP_FLAG_EXTRA = 0x04;
    constexpr uint8_t GZIP_FLAG_NAME = 0x08;
    constexpr uint8_t GZIP_FLAG_COMMENT = 0x10;

    outLength = 0;
    const uint8_t* in = reinterpret_cast<const uint8_t*>(input);

    if (inputLength < GZIP_HEADER_SIZE + GZIP_TRAILER_SIZE || in[0] != GZIP_ID1 || in[1] != GZIP_ID2 || in[2] != GZIP_METHOD_DEFLATE)
    {
        LOG_ERROR("HttpService: Kein gültiger gzip-Kopf");
        return false;
    }

    const uint8_t flags = in[3];
    size_t pos = GZIP_HEADER_SIZE;

    if (flags & GZIP_FLAG_EXTRA)
    {
        if (pos + GZIP_EXTRA_LEN_FIELD > inputLength)
            return false;
        pos += GZIP_EXTRA_LEN_FIELD + (static_cast<size_t>(in[pos]) | (static_cast<size_t>(in[pos + 1]) << 8));
    }

    if (flags & GZIP_FLAG_NAME)
    {
        while (pos < inputLength && in[pos] != 0)
            ++pos;
        ++pos;
    }

    if (flags & GZIP_FLAG_COMMENT)
    {
        while (pos < inputLength && in[pos] != 0)
            ++pos;
        ++pos;
    }

    if (flags & GZIP_FLAG_HCRC)
        pos += 2;

    if (pos + GZIP_TRAILER_SIZE > inputLength)
    {
        LOG_ERROR("HttpService: gzip-Kopf länger als die Antwort");
        return false;
    }

    size_t inSize = inputLength - GZIP_TRAILER_SIZE - pos;
    size_t outSize = HttpServiceCfg::RESPONSE_BUFFER_SIZE - 1;
    mz_uint8* out = reinterpret_cast<mz_uint8*>(m_responseBuffer);

    tinfl_init(m_inflater);
    const tinfl_status status = tinfl_decompress(m_inflater, in + pos, &inSize, out, out, &outSize, TINFL_FLAG_USING_NON_WRAPPING_OUTPUT_BUF);

    if (status != TINFL_STATUS_DONE)
    {
        LOG_ERROR_F("HttpService: gzip-Entpacken fehlgeschlagen (Status %d, %u Bytes entpackt)", static_cast<int>(status), outSize);
        return false;
    }

    m_responseBuffer[outSize] = '\0';
    outLength = outSize;

    // Die letzten vier Bytes des gzip-Endes tragen die Originalgröße modulo 2^32
    const uint8_t* sizeField = in + inputLength - GZIP_SIZE_FIELD;
    const uint32_t originalSize = static_cast<uint32_t>(sizeField[0]) | (static_cast<uint32_t>(sizeField[1]) << 8) | (static_cast<uint32_t>(sizeField[2]) << 16) | (static_cast<uint32_t>(sizeField[3]) << 24);
    if (originalSize != outSize)
        LOG_WARNING_F("HttpService: gzip-Größe weicht ab (%lu erwartet, %u entpackt)", static_cast<unsigned long>(originalSize), outSize);

    LOG_INFO_F("HttpService: gzip entpackt, %u -> %u Bytes", inputLength, outSize);
    return true;
}

#ifndef HTTP_SERVICE_H
#define HTTP_SERVICE_H

#include <Arduino.h>
#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <WiFi.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

#include <atomic>
#include <functional>

// Entpackerzustand von miniz (tinfl) aus dem ROM; vollständig nur in HttpService.cpp eingebunden
struct tinfl_decompressor_tag;

// ============================================================================
// KONFIGURATION
// ============================================================================

namespace HttpServiceCfg
{
    constexpr size_t RESPONSE_BUFFER_SIZE = 1024 * 1024; ///< 1 MB Response-Buffer in PSRAM (Tagesschau-News ~580 KB, großzügiger Puffer)
    constexpr size_t JSON_PSRAM_HEADROOM = 48 * 1024;  ///< Mindest-Reserve in PSRAM für ArduinoJson-Allokationen zur Laufzeit
    constexpr int HTTP_TIMEOUT_MS         = 15000; ///< Verbindungs- und Antwort-Timeout für HTTPClient in Millisekunden
    constexpr int HTTP_STREAM_TIMEOUT_MS  = 10000; ///< Max. Wartezeit auf neue Daten beim direkten Stream-Lesen (unabhängig vom HTTPClient-Timeout)
    constexpr int HTTP_MAX_ATTEMPTS = 3;               ///< Gesamtanzahl der Versuche pro Request (1 = kein Retry)
    constexpr int HTTP_RETRY_DELAY_MS = 1000;           ///< Wartezeit zwischen Retries in ms
    constexpr int HTTP_REDIRECT_LIMIT = 5;              ///< Maximale Anzahl Redirects
    constexpr size_t THROTTLE_CHUNK_BYTES = 4096;      ///< Gedrosselter Empfang: Blockgröße zwischen zwei Pausen
    constexpr int THROTTLE_PAUSE_MS = 10;              ///< Gedrosselter Empfang: Pause nach jedem Block (deckelt den Durchsatz auf ca. 400 KB/s)
    constexpr size_t COMPRESSED_BUFFER_SIZE = 128 * 1024; ///< Puffer für gzip-komprimierte Antworten (getCompressed), einmalig im PSRAM angelegt
}

// ============================================================================
// PSRAM ALLOCATOR FÜR ARDUINOJSON 7.x
// ============================================================================

/**
 * @brief Custom Allocator für ArduinoJson 7.x, der PSRAM verwendet
 * 
 * Erbt von ArduinoJson::Allocator und implementiert die erforderlichen virtuellen Methoden. Sorgt dafür, dass alle JSON-Operationen im PSRAM stattfinden und nicht den knappen Heap fragmentieren.
 * 
 * Verwendung:
 * @code
 * JsonDocument doc(HTTP_SERVICE.getPsramAllocator());
 * @endcode
 */
struct PsramAllocator : ArduinoJson::Allocator
{
    /**
     * @brief Allokiert Speicher im PSRAM
     * @param size Anzahl der benötigten Bytes
     * @return Zeiger auf allokierten Speicher oder nullptr bei Fehler
     */
    void* allocate(size_t size) override
    {
        return ps_malloc(size);
    }

    /**
     * @brief Gibt PSRAM-Speicher frei
     * @param ptr Zeiger auf freizugebenden Speicher
     */
    void deallocate(void* ptr) override
    {
        if (ptr)
            free(ptr);
    }

    /**
     * @brief Reallociert PSRAM-Speicher
     * @param ptr Ursprünglicher Zeiger
     * @param new_size Neue Größe in Bytes
     * @return Zeiger auf reallozierten Speicher oder nullptr bei Fehler
     */
    void* reallocate(void* ptr, size_t new_size) override
    {
        return ps_realloc(ptr, new_size);
    }
};

// ============================================================================
// HTTP RESPONSE STRUKTUR
// ============================================================================

/**
 * @brief Ergebnis einer HTTP-Anfrage
 * 
 * Enthält den HTTP-Status-Code und einen Zeiger auf die Response-Daten im PSRAM-Buffer. Der Buffer ist nur bis zum nächsten Request gültig!
 */
/**
 * @brief Fehlercodes für interne HttpService-Fehler (negativ, kein HTTP-Statuscode)
 */
enum class HttpError : int
{
    None           =    0,  ///< Kein Fehler
    WiFiDisconnected = -100, ///< WLAN nicht verbunden
    EmptyUrl       = -101, ///< URL ist leer oder null
    BeginFailed    = -102, ///< http.begin() fehlgeschlagen
    ReadFailed     = -103, ///< Response konnte nicht gelesen werden
    DecompressFailed = -104, ///< gzip-Antwort konnte nicht entpackt werden
    Aborted        = -200, ///< Request durch abortFlag abgebrochen
};

struct HttpResponse
{
    int httpCode = -1;               ///< HTTP Status-Code (200, 404, etc.) oder negativer Fehlercode
    const char* data = nullptr;      ///< Zeiger auf Response-Daten (null-terminiert)
    size_t length = 0;               ///< Tatsächlich gelesene Bytes
    size_t expectedLength = 0;       ///< Erwartete Länge laut Content-Length (0 = unbekannt/chunked)
    bool truncated = false;          ///< true wenn Antwort den Buffer überschritten hat
    unsigned long requestTimeMs = 0; ///< Dauer der Anfrage in Millisekunden

    /**
     * @brief Prüft ob die Anfrage erfolgreich war
     * @return true bei HTTP 2xx Status
     */
    bool isSuccess() const { return httpCode >= 200 && httpCode < 300; }

    /**
     * @brief Prüft ob Daten vorhanden sind
     * @return true wenn data != nullptr und length > 0
     */
    bool hasData() const { return data != nullptr && length > 0; }
};

/**
 * @brief Einfacher HTTP-Header für optionale Request-Zusatzheader.
 */
struct HttpHeader
{
    const char *name = nullptr;  ///< Header-Name
    const char *value = nullptr; ///< Header-Wert
};

// ============================================================================
// HTTP SERVICE KLASSE
// ============================================================================

/**
 * @brief Zentraler HTTP-Service mit PSRAM-Buffern
 * 
 * Singleton-Klasse, die alle HTTP-Anfragen des Systems verwaltet. Verwendet persistente Buffer im PSRAM, um Heap-Fragmentierung zu vermeiden.
 * 
 * Features:
 * - Persistenter Response-Buffer im PSRAM (kein getString()!)
 * - PSRAM-Allocator für ArduinoJson
 * - Automatische Retries bei Fehlern
 * - Redirect-Unterstützung (wichtig für Google Apps Script)
 * - Abbruch-Unterstützung für sauberes Shutdown
 * 
 * Verwendung:
 * @code
 * // Initialisierung (einmal in setup())
 * if (!HTTP_SERVICE.begin()) {
 *     LOG_ERROR("HttpService init fehlgeschlagen!");
 * }
 * 
 * // HTTP GET Request
 * HttpResponse resp = HTTP_SERVICE.get("https://api.example.com/data");
 * if (resp.isSuccess()) {
 *     // resp.data enthält die Antwort (im PSRAM)
 *     JsonDocument doc(HTTP_SERVICE.getPsramAllocator());
 *     deserializeJson(doc, resp.data);
 * }
 * @endcode
 * 
 * @note Der Response-Buffer wird bei jedem neuen Request überschrieben!
 *       Daten müssen vor dem nächsten Request verarbeitet werden.
 */
class HttpService
{
public:
    /**
     * @brief Zugriff auf die Singleton-Instanz
     * @return Referenz auf die einzige HttpService-Instanz
     */
    static HttpService& instance();

    // Singleton: Kopieren und Zuweisen verbieten
    HttpService(const HttpService&) = delete;
    HttpService& operator=(const HttpService&) = delete;

    /**
     * @brief Initialisiert den HttpService
     * 
     * Allokiert die PSRAM-Buffer. Muss einmal beim Programmstart aufgerufen werden, bevor HTTP-Anfragen gestellt werden.
     * 
     * @return true bei Erfolg, false wenn PSRAM-Allokation fehlschlägt
     */
    bool begin();

    /**
     * @brief Gibt alle Ressourcen frei
     * 
     * Gibt die PSRAM-Buffer frei. Sollte normalerweise nicht aufgerufen werden, da der Service persistent ist.
     */
    void end();

    /**
     * @brief Prüft ob der Service initialisiert ist
     * @return true wenn begin() erfolgreich war
     */
    bool isInitialized() const { return m_initialized; }

    // ========================================================================
    // HTTP METHODEN
    // ========================================================================

    /**
     * @brief Führt einen HTTP GET Request aus
     * 
     * Die Response wird in den internen PSRAM-Buffer geschrieben. Bei Fehlern wird automatisch ein Retry durchgeführt.
     * 
     * @param url Vollständige URL inkl. Schema (https://...)
     * @param abortFlag Optional: Atomic-Flag zum Abbrechen des Requests
     * @return HttpResponse mit Status und Daten
     * 
     * @note Der zurückgegebene data-Zeiger ist nur bis zum nächsten
     *       Request gültig! Daten sofort verarbeiten.
     */
    HttpResponse get(const char* url, std::atomic<bool>* abortFlag = nullptr);

    /**
     * @brief Führt einen HTTP GET Request mit gedrosseltem Empfang aus
     *
     * Wie get(), liest den Response-Body aber in Blöcken von THROTTLE_CHUNK_BYTES mit THROTTLE_PAUSE_MS Pause dazwischen. Über das TCP-Fenster bremst das auch den Sender, dadurch bleibt neben Funkempfang und Kopierarbeit genug PSRAM-Bandbreite für das DMA-Auslesen des RGB-Framebuffers. Für Hintergrund-Abrufe gedacht, deren Dauer unkritisch ist; große Antworten brauchen entsprechend länger.
     *
     * @param url Vollständige URL inkl. Schema (https://...)
     * @param abortFlag Optional: Atomic-Flag zum Abbrechen des Requests
     * @return HttpResponse mit Status und Daten
     */
    HttpResponse getThrottled(const char* url, std::atomic<bool>* abortFlag = nullptr);

    /**
     * @brief Führt einen HTTP GET Request mit gzip-Kompression und gedrosseltem Empfang aus
     *
     * Wie getThrottled(), fordert die Antwort aber mit Accept-Encoding gzip an. Liefert der Server gzip, wird der Body in einen eigenen Puffer von COMPRESSED_BUFFER_SIZE gelesen und mit dem tinfl-Entpacker aus dem ROM in den Antwortpuffer entpackt; liefert er unkomprimiert, verhält sich der Abruf wie getThrottled(). Für große JSON-Antworten gedacht, deren Funkempfang das Display stört: die übertragene Datenmenge sinkt typischerweise auf ein Fünftel bis ein Achtel. Für diesen Abruf wird HTTP/1.1 verwendet, weil der HTTPClient den Accept-Encoding-Header nur dann sendet.
     *
     * @param url Vollständige URL inkl. Schema (https://...)
     * @param abortFlag Optional: Atomic-Flag zum Abbrechen des Requests
     * @return HttpResponse mit Status und entpackten Daten; DecompressFailed, wenn die gzip-Antwort nicht entpackt werden konnte
     */
    HttpResponse getCompressed(const char* url, std::atomic<bool>* abortFlag = nullptr);

    /**
     * @brief Führt einen HTTP GET Request mit zusätzlichen Headern aus.
     * @param url Vollständige URL inkl. Schema
     * @param headers Header-Array, darf nullptr sein
     * @param headerCount Anzahl Header im Array
     * @param abortFlag Optional: Atomic-Flag zum Abbrechen des Requests
     * @return HttpResponse mit Status und Daten
     */
    HttpResponse getWithHeaders(const char* url, const HttpHeader *headers,
                                size_t headerCount,
                                std::atomic<bool>* abortFlag = nullptr);

    /**
     * @brief Führt einen HTTP GET Request aus und kopiert die Antwort in einen eigenen Puffer.
     * @param url Vollständige URL inkl. Schema
     * @param headers Header-Array, darf nullptr sein
     * @param headerCount Anzahl Header im Array
     * @param responseBuffer Zielpuffer für die Antwort
     * @param responseBufferSize Größe des Zielpuffers
     * @param abortFlag Optional: Atomic-Flag zum Abbrechen des Requests
     * @return HttpResponse mit Status und Daten im Zielpuffer
     */
    HttpResponse getWithHeadersCopy(const char* url, const HttpHeader *headers,
                                    size_t headerCount,
                                    char *responseBuffer,
                                    size_t responseBufferSize,
                                    std::atomic<bool>* abortFlag = nullptr);

    /**
     * @brief Führt einen HTTP POST Request aus
     * 
     * @param url Vollständige URL
     * @param payload POST-Body (JSON, Form-Data, etc.)
     * @param contentType Content-Type Header (default: application/json)
     * @param abortFlag Optional: Atomic-Flag zum Abbrechen
     * @return HttpResponse mit Status und Daten
     */
    HttpResponse post(const char* url, const char* payload, 
                      const char* contentType = "application/json",
                      std::atomic<bool>* abortFlag = nullptr);

    /**
     * @brief Führt einen HTTP POST Request aus und kopiert die Antwort in einen eigenen Puffer.
     * @param url Vollständige URL
     * @param payload POST-Body
     * @param contentType Content-Type Header
     * @param responseBuffer Zielpuffer für die Antwort
     * @param responseBufferSize Größe des Zielpuffers
     * @param abortFlag Optional: Atomic-Flag zum Abbrechen
     * @return HttpResponse mit Status und Daten im Zielpuffer
     */
    HttpResponse postCopy(const char* url, const char* payload,
                          const char* contentType,
                          char *responseBuffer,
                          size_t responseBufferSize,
                          std::atomic<bool>* abortFlag = nullptr);

    /**
     * @brief Führt einen HTTP POST Request mit zusätzlichen Headern aus.
     * @param url Vollständige URL
     * @param payload POST-Body
     * @param contentType Content-Type Header
     * @param headers Header-Array, darf nullptr sein
     * @param headerCount Anzahl Header im Array
     * @param abortFlag Optional: Atomic-Flag zum Abbrechen
     * @return HttpResponse mit Status und Daten
     */
    HttpResponse postWithHeaders(const char* url, const char* payload,
                                 const char* contentType,
                                 const HttpHeader *headers, size_t headerCount,
                                 std::atomic<bool>* abortFlag = nullptr);

    // ========================================================================
    // PSRAM ALLOCATOR
    // ========================================================================

    /**
     * @brief Gibt den PSRAM-Allocator für ArduinoJson zurück
     * 
     * Verwendung:
     * @code
     * JsonDocument doc(HTTP_SERVICE.getPsramAllocator());
     * deserializeJson(doc, responseData);
     * @endcode
     * 
     * @return Zeiger auf den PsramAllocator (bleibt gültig für Lebenszeit des Services)
     */
    PsramAllocator* getPsramAllocator() { return &m_psramAllocator; }

    // ========================================================================
    // KONFIGURATION
    // ========================================================================

    /**
     * @brief Setzt das HTTP-Timeout
     * @param timeoutMs Timeout in Millisekunden
     */
    void setTimeout(int timeoutMs) { m_timeoutMs = timeoutMs; }

    /**
     * @brief Setzt die maximale Anzahl Retries
     * @param retries Anzahl der Wiederholungsversuche
     */
    void setMaxAttempts(int attempts) { m_maxAttempts = attempts; } ///< Gesamtanzahl Versuche setzen (mind. 1)

    // ========================================================================
    // STATISTIKEN
    // ========================================================================

    /**
     * @brief Gibt die Größe des Response-Buffers zurück
     * @return Buffergröße in Bytes
     */
    size_t getBufferSize() const { return HttpServiceCfg::RESPONSE_BUFFER_SIZE; }

    /**
     * @brief Gibt den freien PSRAM zurück
     * @return Freier PSRAM in Bytes
     */
    size_t getFreePsram() const { return ESP.getFreePsram(); }

    /**
     * @brief Gibt Anzahl erfolgreicher Requests zurück
     * @return Request-Zähler
     */
    uint32_t getSuccessCount() const { return m_successCount; }

    /**
     * @brief Gibt Anzahl fehlgeschlagener Requests zurück
     * @return Fehler-Zähler
     */
    uint32_t getErrorCount() const { return m_errorCount; }

private:
    /**
     * @brief Privater Konstruktor (Singleton)
     */
    HttpService() = default;

    /**
     * @brief Destruktor
     */
    ~HttpService();

    /**
     * @brief Interne Request-Implementierung
     * @param url Ziel-URL
     * @param method HTTP-Methode ("GET" oder "POST")
     * @param payload POST-Payload (nullptr für GET)
     * @param contentType Content-Type für POST
     * @param abortFlag Abbruch-Flag
     * @param throttled true = Body gedrosselt lesen (Blöcke mit Pausen)
     * @param compressed true = Antwort mit Accept-Encoding gzip anfordern und bei Bedarf entpacken
     * @return HttpResponse
     */
    HttpResponse executeRequest(const char* url, const char* method,
                                const char* payload, const char* contentType,
                                std::atomic<bool>* abortFlag,
                                const HttpHeader *headers = nullptr,
                                size_t headerCount = 0,
                                char *responseCopyBuffer = nullptr,
                                size_t responseCopyBufferSize = 0,
                                bool throttled = false,
                                bool compressed = false);

    /**
     * @brief Erstellt den Mutex für den gemeinsamen Response-Buffer bei Bedarf.
     */
    bool ensureMutex();

    /**
     * @brief Liest Response-Daten in PSRAM-Buffer
     * @param http HTTPClient-Instanz
     * @param buffer Zielpuffer für den Body
     * @param capacity Größe des Zielpuffers in Bytes
     * @param outLength Ausgabe: Gelesene Bytes
     * @param throttled true = in Blöcken mit Pausen lesen
     * @return true bei Erfolg
     */
    bool readResponseToBuffer(HTTPClient& http, char* buffer, size_t capacity, size_t& outLength, size_t& outExpected, bool& outTruncated,
                               const std::atomic<bool>* abortFlag = nullptr, bool throttled = false);

    /**
     * @brief Legt Kompressionspuffer und Entpackerzustand bei Bedarf einmalig im PSRAM an
     * @return true wenn beide vorhanden sind
     */
    bool ensureCompressedBuffers();

    /**
     * @brief Entpackt einen gzip-Body in den Antwortpuffer
     * @param input Komprimierte Daten inkl. gzip-Kopf und -Ende
     * @param inputLength Länge der komprimierten Daten
     * @param outLength Ausgabe: entpackte Bytes im Antwortpuffer
     * @return true bei Erfolg
     */
    bool inflateGzip(const char* input, size_t inputLength, size_t& outLength);

    // ========================================================================
    // MEMBER-VARIABLEN
    // ========================================================================

    bool m_initialized = false;         ///< Service initialisiert?
    SemaphoreHandle_t m_mutex = nullptr; ///< Schutz für gemeinsamen Response-Buffer
    char* m_responseBuffer = nullptr;   ///< Response-Buffer im PSRAM
    char* m_compressedBuffer = nullptr; ///< Puffer für gzip-komprimierte Antworten im PSRAM, einmalig bei Bedarf angelegt
    tinfl_decompressor_tag* m_inflater = nullptr; ///< tinfl-Entpackerzustand im PSRAM, einmalig bei Bedarf angelegt
    PsramAllocator m_psramAllocator;    ///< PSRAM-Allocator für JSON

    // Konfiguration
    int m_timeoutMs = HttpServiceCfg::HTTP_TIMEOUT_MS;
    int m_maxAttempts = HttpServiceCfg::HTTP_MAX_ATTEMPTS;

    // Statistiken
    uint32_t m_successCount = 0;        ///< Erfolgreiche Requests
    uint32_t m_errorCount = 0;          ///< Fehlgeschlagene Requests
};

/**
 * @brief Makro für kürzeren Zugriff auf den HttpService
 * 
 * Verwendung:
 * @code
 * HTTP_SERVICE.get("https://...");
 * @endcode
 */
#define HTTP_SERVICE HttpService::instance()

#endif // HTTP_SERVICE_H

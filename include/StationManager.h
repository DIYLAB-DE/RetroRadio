#ifndef STATIONMANAGER_H
#define STATIONMANAGER_H

#include <cstddef>
#include <cstdint>
#include <mutex>

// ===================================================================
// Größenkonstanten für Station-Strings
// ===================================================================

static constexpr size_t STATION_SHORTNAME_SIZE = 24;   ///< z.B. "Bayern 3", "SWR Aktuell"
static constexpr size_t STATION_NAME_SIZE      = 64;   ///< Vollständiger Sendername
static constexpr size_t STATION_URL_SIZE       = 256;  ///< Streaming-URLs

/**
 * @brief Struktur zur Repräsentation einer Radiostation (Laufzeit, im RAM)
 *
 * Diese Struktur verwendet feste Char-Arrays statt Heap-Allokation. Vorteile:
 * - Keine Heap-Fragmentierung
 * - Deterministischer Speicherverbrauch (~350 Bytes pro Station)
 * - Trivial kopierbar (POD-ähnlich)
 * - Kein manuelles Speichermanagement nötig
 */
struct Station {
    int16_t x;                              ///< X-Position der Station in der Skala
    int16_t y;                              ///< Y-Position der Station in der Skala
    int16_t lastPos;                        ///< Letzte Zeigerposition auf der Skala in Pixeln
    char shortName[STATION_SHORTNAME_SIZE]; ///< Kurzer Name der Station
    char name[STATION_NAME_SIZE];           ///< Vollständiger Name der Station
    char url[STATION_URL_SIZE];             ///< Stream-URL der Station
    bool preset;                            ///< Flag ob Station als Preset gespeichert ist

    /**
     * @brief Konstruktor - initialisiert alle Felder mit Nullen/Leerstrings
     */
    Station()
        : x(0), y(0), lastPos(0)
        , shortName{'\0'}, name{'\0'}, url{'\0'}
        , preset(false)
    {}
};

static constexpr int MAX_STATIONS = 70;  ///< Maximale Anzahl von Stationen

// ===================================================================
// Flash-Partition: Binärformat-Strukturen
// ===================================================================

/// Magic-Number zur Identifikation gültiger Stationsdaten: "STNS"
static constexpr uint32_t STATIONS_MAGIC = 0x534E5453;

/// Aktuelle Format-Version (bei Strukturänderungen erhöhen)
static constexpr uint16_t STATIONS_FORMAT_VERSION = 1;

/**
 * @brief Header am Anfang der Stations-Partition
 *
 * Wird bei jedem Lesen geprüft. Bei ungültigem Magic oder falscher Version werden die PROGMEM-Defaults geladen.
 */
struct StationsHeader {
    uint32_t magic;          ///< Muss STATIONS_MAGIC sein
    uint16_t version;        ///< Format-Version
    uint16_t count;          ///< Anzahl gespeicherter Stationen
    uint32_t crc32;          ///< CRC32 über alle StationRecord-Daten
    uint32_t reserved;       ///< Reserviert für spätere Erweiterungen
};

/**
 * @brief Kompakter Stationsdatensatz für die Flash-Partition
 *
 * Enthält nur die persistenten Felder ohne Laufzeitdaten (x, y, lastPos). sizeof(StationRecord) = 348 Bytes -> 70 Stationen = ~24 KB
 */
struct StationRecord {
    char shortName[STATION_SHORTNAME_SIZE]; ///< Kurzname
    char name[STATION_NAME_SIZE];           ///< Vollständiger Name
    char url[STATION_URL_SIZE];             ///< Stream-URL
    uint8_t preset;                         ///< Favorit-Flag (0/1)
    uint8_t padding[3];                     ///< Alignment auf 4 Bytes
};

// ===================================================================
// StationManager
// ===================================================================

/**
 * @brief Manager-Klasse für Radiostationen
 *
 * Verwaltet ein Array von Station-Objekten und bietet Funktionen zum Laden, Speichern und Manipulieren der Stationsliste.
 *
 * Speicherung: Dedizierte Flash-Partition ("stations") statt LittleFS. Fallback: PROGMEM-Defaults aus StationsDefault.h bei leerer/korrupter Partition.
 *
 * Speicherverbrauch RAM: ~70 × 350 Bytes ≈ 24 KB (statisch, kein Heap)
 */
class StationManager {
public:
    /**
     * @brief Konstruktor - initialisiert den StationManager
     */
    StationManager();

    /**
     * @brief Destruktor
     */
    ~StationManager() = default;

    /**
     * @brief Lädt Stationen aus der Flash-Partition
     *
     * Ablauf:
     * 1. Partition "stations" suchen
     * 2. Header lesen und validieren (Magic, Version, CRC32)
     * 3. Bei gültigem Header: StationRecords laden
     * 4. Bei ungültigem Header: PROGMEM-Defaults laden
     *
     * @return true wenn erfolgreich geladen (Partition oder Defaults)
     */
    bool loadStations();

    /**
     * @brief Speichert Stationen in die Flash-Partition
     *
     * Schreibt Header + StationRecords binär in die "stations"-Partition. Die Partition wird vor dem Schreiben gelöscht (Erase).
     *
     * @return true wenn erfolgreich gespeichert
     */
    bool saveStations();

    /**
     * @brief Lädt die PROGMEM-Defaults in das RAM-Array
     *
     * Wird automatisch aufgerufen wenn die Partition leer oder korrupt ist. Kann auch manuell für einen Factory-Reset der Senderliste genutzt werden.
     *
     * @note Ruft NICHT automatisch saveStations() auf!
     */
    void loadDefaults();

    /**
     * @brief Setzt die Senderliste auf Werkseinstellungen zurück
     *
     * Lädt PROGMEM-Defaults und speichert sie sofort in die Partition.
     *
     * @return true wenn erfolgreich zurückgesetzt und gespeichert
     */
    bool factoryReset();

    /**
     * @brief Gibt die Anzahl der geladenen Stationen zurück
     * @return Anzahl der Stationen
     */
    int getStationCount() const;

    /**
     * @brief Gibt einen Pointer auf das Stations-Array zurück
     * @return Pointer auf das erste Element des Stations-Arrays
     * @warning Pointer nicht über längere Zeit halten! Bei Multithreading kann sich das Array während der Nutzung ändern.
     */
    Station* getStations();

    /**
     * @brief Gibt einen const Pointer auf das Stations-Array zurück
     * @return Const Pointer auf das erste Element des Stations-Arrays
     * @warning Pointer nicht über längere Zeit halten! Bei Multithreading kann sich das Array während der Nutzung ändern.
     */
    const Station* getStations() const;

    /**
     * @brief Gibt die URL einer Station anhand ihrer ID zurück
     * @param id Index der gewünschten Station
     * @return Pointer auf die URL-String oder nullptr bei ungültiger ID
     */
    const char* getStationUrl(int id) const;

    /**
     * @brief Gibt eine einzelne Station zurück (thread-safe Kopie)
     * @param index Index der gewünschten Station
     * @param outStation Ausgabe-Referenz für die Station
     * @return true wenn Station existiert, false bei ungültigem Index
     * @note Erstellt eine Kopie - sicher für Multithreading
     */
    bool getStation(int index, Station& outStation) const;

    /**
     * @brief Führt eine Funktion mit Lock auf dem Stations-Array aus
     * @tparam Func Callback-Typ: void(const Station*, int count)
     * @param callback Funktion die mit dem Array aufgerufen wird
     * @note Das Array ist während des Callbacks vollständig geschützt. Ideal für Webinterface, Serialisierung, UI-Listen etc.
     *
     * Beispiel:
     * @code
     * manager.withStations([](const Station* stations, int count) {
     *     for (int i = 0; i < count; i++) {
     *         Serial.println(stations[i].name);
     *     }
     * });
     * @endcode
     */
    template<typename Func>
    void withStations(Func&& callback) const {
        std::lock_guard<std::mutex> lock(m_mutex);
        callback(m_stations, m_stationCount);
    }

    /**
     * @brief Führt eine Funktion mit Lock auf dem Stations-Array aus (nicht-const)
     * @tparam Func Callback-Typ: void(Station*, int count)
     * @param callback Funktion die mit dem Array aufgerufen wird
     * @note Erlaubt Modifikationen am Array unter Lock-Schutz
     */
    template<typename Func>
    void withStationsMutable(Func&& callback) {
        std::lock_guard<std::mutex> lock(m_mutex);
        callback(m_stations, m_stationCount);
    }

    /**
     * @brief Löscht alle Stationen (setzt Count auf 0)
     */
    void clearStations();

    /**
     * @brief Fügt eine neue Station hinzu
     * @param station Referenz auf die hinzuzufügende Station
     * @return true wenn erfolgreich hinzugefügt, false wenn Array voll
     */
    bool addStation(const Station& station);

private:
    Station m_stations[MAX_STATIONS];  ///< Array der Stationen (~24 KB)
    int m_stationCount;                ///< Aktuelle Anzahl der Stationen
    mutable std::mutex m_mutex;        ///< Schützt Zugriff bei Multithreading

    /**
     * @brief Berechnet CRC32 über die StationRecords im RAM
     * @param records Pointer auf StationRecord-Array
     * @param count Anzahl der Records
     * @return CRC32-Prüfsumme
     */
    static uint32_t calculateCRC32(const StationRecord* records, int count);
};

#endif  // STATIONMANAGER_H

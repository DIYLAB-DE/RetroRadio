#ifndef DS1307_RTC_H
#define DS1307_RTC_H

#include <stdint.h>
#include <Wire.h>
#include <sys/time.h>  // Für timeval, settimeofday
#include <time.h>      // Für time_t, tm struct, mktime, localtime_r

// DS1307 I2C Adresse
#define DS1307_ADDRESS 0x68

// Register-Adressen
#define DS1307_REG_SECONDS 0x00
#define DS1307_REG_MINUTES 0x01
#define DS1307_REG_HOURS 0x02
#define DS1307_REG_DAYOFWEEK 0x03
#define DS1307_REG_DATE 0x04
#define DS1307_REG_MONTH 0x05
#define DS1307_REG_YEAR 0x06
#define DS1307_REG_CONTROL 0x07  // Nicht aktiv genutzt in dieser Lib, aber definiert
#define DS1307_REG_NVRAM_START 0x08
#define DS1307_REG_NVRAM_END 0x3F  // Inklusiv Ende des NVRAM

// NVRAM Größe (56 Bytes: 0x08 bis 0x3F)
#define DS1307_NVRAM_SIZE (DS1307_REG_NVRAM_END - DS1307_REG_NVRAM_START + 1)

// Makros für NVRAM-Bereichsprüfung
#define NVRAM_ADDR_VALID(addr) ((addr) < DS1307_NVRAM_SIZE)
#define NVRAM_RANGE_VALID(addr, len) (((addr) + (len)) <= DS1307_NVRAM_SIZE)

/**
 * @brief Struktur zur Speicherung von Datum und Uhrzeit
 *
 * Diese Struktur enthält alle relevanten Zeitinformationen im dezimalen Format. Der Wochentag folgt der ISO-8601 Konvention (1=Montag bis 7=Sonntag).
 */
struct DateTime {
    uint16_t year = 0;      ///< Jahr (z.B. 2024)
    uint8_t month = 0;      ///< Monat (1-12)
    uint8_t day = 0;        ///< Tag (1-31)
    uint8_t hour = 0;       ///< Stunde (0-23)
    uint8_t minute = 0;     ///< Minute (0-59)
    uint8_t second = 0;     ///< Sekunde (0-59)
    uint8_t dayOfWeek = 0;  ///< Wochentag (1=Montag, 2=Dienstag, ..., 7=Sonntag)
};

/**
 * @brief DS1307 Real-Time Clock Bibliothek
 *
 * Diese Klasse stellt eine vollständige Schnittstelle zum DS1307 RTC-Chip bereit. Sie unterstützt Datum/Zeit-Operationen, NVRAM-Zugriff und Systemzeitsynchronisation.
 *
 * @note Die Bibliothek verwendet keine String-Klasse für maximale Kompatibilität.
 * @note Ist die RTC-Hardware nicht erreichbar, werden alle I2C-Zugriffe stillschweigend zu No-Ops (Reads liefern 0/false, Writes geben false zurück). isPresent() signalisiert den Zustand nach aussen.
 */
class DS1307_RTC {
   private:
    TwoWire* _wire;
    bool m_present = false;  ///< true, wenn die RTC beim letzten begin() über I2C erreicht wurde

    // BCD Konvertierung
    uint8_t bcdToDec(uint8_t bcd);
    uint8_t decToBcd(uint8_t dec);

    // I2C Hilfsfunktionen
    bool writeRegister(uint8_t reg, uint8_t data);
    uint8_t readRegister(uint8_t reg);
    bool writeMultiple(uint8_t reg, uint8_t* data, uint8_t length);
    bool readMultiple(uint8_t reg, uint8_t* data, uint8_t length);

   public:
    /**
     * @brief Konstruktor für DS1307_RTC
     *
     * Erstellt eine neue Instanz der DS1307 RTC-Klasse
     *
     * @param wire Zeiger auf das zu verwendende TwoWire-Interface (standardmäßig &Wire)
     */
    DS1307_RTC(TwoWire* wire = &Wire);

    /**
     * @brief Initialisiert die RTC-Kommunikation
     *
     * Prüft die I2C-Verbindung zum DS1307 und startet die Uhr falls sie angehalten ist. Der interne Präsenz-Flag wird entsprechend gesetzt. Mehrfachaufruf ist unschädlich, die Funktion ist idempotent.
     *
     * @return true bei erfolgreicher Initialisierung, false bei Fehler (RTC nicht erreichbar)
     */
    bool begin();

    /**
     * @brief Gibt zurück, ob die RTC beim letzten begin() erreichbar war
     *
     * Solange false zurückgegeben wird, sind alle Lese-/Schreibzugriffe No-Ops.
     *
     * @return true wenn die RTC vorhanden ist, sonst false
     */
    bool isPresent() const { return m_present; }

    /**
     * @brief Prüft ob die RTC-Uhr läuft
     *
     * Überprüft das Clock Halt (CH) Bit im Sekundenregister. Ohne erreichbare RTC wird false zurückgegeben.
     *
     * @return true wenn die Uhr läuft, false wenn sie angehalten oder die RTC nicht erreichbar ist
     */
    bool isRunning();

    /**
     * @brief Setzt Datum und Uhrzeit der RTC
     *
     * Schreibt die kompletten Datum/Zeit-Informationen in die RTC-Register. Alle Werte werden auf Gültigkeit überprüft.
     *
     * @param dt DateTime-Struktur mit den zu setzenden Werten
     * @return true bei Erfolg, false bei ungültigen Werten, I2C-Fehler oder fehlender RTC
     */
    bool setDateTime(const DateTime& dt);

    /**
     * @brief Liest das aktuelle Datum und die Uhrzeit von der RTC
     *
     * Liest alle Zeitregister der RTC und konvertiert sie vom BCD-Format. Ist die RTC nicht erreichbar oder die Übertragung schlägt fehl, wird dt auf definierte Null-Werte zurückgesetzt und false zurückgegeben. Aufrufer dürfen dt im Fehlerfall nicht verwenden.
     *
     * @param dt Referenz auf DateTime-Struktur zur Speicherung der gelesenen Werte
     * @return true bei Erfolg, false bei I2C-Fehler oder fehlender RTC
     */
    bool getDateTime(DateTime& dt);

    /**
     * @brief Setzt nur die Uhrzeit (behält Datum bei)
     *
     * Convenience-Funktion zum Setzen nur der Zeitkomponenten. Das aktuelle Datum bleibt unverändert.
     *
     * @param hour Stunde (0-23)
     * @param minute Minute (0-59)
     * @param second Sekunde (0-59)
     * @return true bei Erfolg, false bei Fehler
     */
    bool setTime(uint8_t hour, uint8_t minute, uint8_t second);

    /**
     * @brief Setzt nur das Datum (behält Uhrzeit bei)
     *
     * Convenience-Funktion zum Setzen nur der Datumskomponenten. Die aktuelle Uhrzeit bleibt unverändert.
     *
     * @param year Jahr (2000-2099)
     * @param month Monat (1-12)
     * @param day Tag (1-31)
     * @return true bei Erfolg, false bei Fehler
     */
    bool setDate(uint16_t year, uint8_t month, uint8_t day);

    /**
     * @brief Startet den RTC-Oszillator
     *
     * Löscht das Clock Halt (CH) Bit im Sekundenregister
     *
     * @return true bei Erfolg, false bei I2C-Fehler
     */
    bool startClock();

    /**
     * @brief Stoppt den RTC-Oszillator
     *
     * Setzt das Clock Halt (CH) Bit im Sekundenregister
     *
     * @return true bei Erfolg, false bei I2C-Fehler
     */
    bool stopClock();

    /**
     * @brief Schreibt ein Byte in das NVRAM
     *
     * Das DS1307 verfügt über 56 Bytes NVRAM (Adressen 0-55)
     *
     * @param address NVRAM-Adresse (0-55)
     * @param data Zu schreibendes Byte
     * @return true bei Erfolg, false bei ungültiger Adresse oder I2C-Fehler
     */
    bool writeNVRAM(uint8_t address, uint8_t data);

    /**
     * @brief Liest ein Byte aus dem NVRAM
     *
     * @param address NVRAM-Adresse (0-55)
     * @return Gelesenes Byte oder 0 bei ungültiger Adresse
     */
    uint8_t readNVRAM(uint8_t address);

    /**
     * @brief Schreibt einen Datenblock in das NVRAM
     *
     * Ermöglicht das effiziente Schreiben mehrerer aufeinanderfolgender Bytes
     *
     * @param address Start-NVRAM-Adresse
     * @param data Zeiger auf die zu schreibenden Daten
     * @param length Anzahl der zu schreibenden Bytes
     * @return true bei Erfolg, false bei Bereichsüberschreitung oder I2C-Fehler
     */
    bool writeNVRAMBlock(uint8_t address, const uint8_t* data, uint8_t length);

    /**
     * @brief Liest einen Datenblock aus dem NVRAM
     *
     * @param address Start-NVRAM-Adresse
     * @param data Zeiger auf den Puffer für die gelesenen Daten
     * @param length Anzahl der zu lesenden Bytes
     * @return true bei Erfolg, false bei Bereichsüberschreitung oder I2C-Fehler
     */
    bool readNVRAMBlock(uint8_t address, uint8_t* data, uint8_t length);

    /**
     * @brief Löscht das komplette NVRAM
     *
     * Setzt alle 56 NVRAM-Bytes auf 0
     *
     * @return true bei Erfolg, false bei I2C-Fehler
     */
    bool clearNVRAM();

    /**
     * @brief Schreibt einen booleschen Wert in das NVRAM
     *
     * @param address NVRAM-Adresse
     * @param value Boolescher Wert (true/false)
     * @return true bei Erfolg, false bei Fehler
     */
    bool writeBool(uint8_t address, bool value);

    /**
     * @brief Liest einen booleschen Wert aus dem NVRAM
     *
     * @param address NVRAM-Adresse
     * @return true wenn Wert != 0, false sonst
     */
    bool readBool(uint8_t address);

    /**
     * @brief Schreibt einen vorzeichenbehafteten 8-Bit-Wert in das NVRAM
     *
     * @param address NVRAM-Adresse
     * @param value 8-Bit signed Integer (-128 bis +127)
     * @return true bei Erfolg, false bei Fehler
     */
    bool writeInt8(uint8_t address, int8_t value);

    /**
     * @brief Liest einen vorzeichenbehafteten 8-Bit-Wert aus dem NVRAM
     *
     * @param address NVRAM-Adresse
     * @return 8-Bit signed Integer oder 0 bei ungültiger Adresse
     */
    int8_t readInt8(uint8_t address);

    /**
     * @brief Schreibt einen vorzeichenlosen 8-Bit-Wert in das NVRAM
     *
     * @param address NVRAM-Adresse
     * @param value 8-Bit unsigned Integer (0 bis 255)
     * @return true bei Erfolg, false bei Fehler
     */
    bool writeUint8(uint8_t address, uint8_t value);

    /**
     * @brief Liest einen vorzeichenlosen 8-Bit-Wert aus dem NVRAM
     *
     * @param address NVRAM-Adresse
     * @return 8-Bit unsigned Integer
     */
    uint8_t readUint8(uint8_t address);

    /**
     * @brief Schreibt einen 16-Bit-Wert in das NVRAM (Little Endian)
     *
     * Belegt 2 aufeinanderfolgende NVRAM-Bytes
     *
     * @param address Start-NVRAM-Adresse
     * @param value 16-Bit unsigned Integer (0 bis 65535)
     * @return true bei Erfolg, false bei Bereichsüberschreitung oder Fehler
     */
    bool writeUint16(uint8_t address, uint16_t value);

    /**
     * @brief Liest einen 16-Bit-Wert aus dem NVRAM (Little Endian)
     *
     * @param address Start-NVRAM-Adresse
     * @return 16-Bit unsigned Integer oder 0 bei Fehler
     */
    uint16_t readUint16(uint8_t address);

    /**
     * @brief Schreibt einen 32-Bit-Wert in das NVRAM (Little Endian)
     *
     * Belegt 4 aufeinanderfolgende NVRAM-Bytes
     *
     * @param address Start-NVRAM-Adresse
     * @param value 32-Bit unsigned Integer
     * @return true bei Erfolg, false bei Bereichsüberschreitung oder Fehler
     */
    bool writeUint32(uint8_t address, uint32_t value);

    /**
     * @brief Liest einen 32-Bit-Wert aus dem NVRAM (Little Endian)
     *
     * @param address Start-NVRAM-Adresse
     * @return 32-Bit unsigned Integer oder 0 bei Fehler
     */
    uint32_t readUint32(uint8_t address);

    /**
     * @brief Konvertiert die RTC-Zeit zu Unix-Timestamp (Epoch)
     *
     * @return Unix-Timestamp (Sekunden seit 1.1.1970 UTC) oder 0 bei Fehler
     */
    uint32_t getEpoch();

    /**
     * @brief Setzt die RTC-Zeit aus Unix-Timestamp
     *
     * @param epoch Unix-Timestamp (Sekunden seit 1.1.1970 UTC)
     * @return true bei Erfolg, false bei Fehler
     */
    bool setEpoch(uint32_t epoch);

    /**
     * @brief Synchronisiert RTC mit der aktuellen Systemzeit
     *
     * Übernimmt die aktuelle Systemzeit des ESP32 in die RTC. Nützlich nach WiFi-Zeitsynchronisation.
     *
     * @return true bei Erfolg, false bei Fehler
     */
    bool syncFromSystem();

    /**
     * @brief Setzt die Systemzeit auf die RTC-Zeit
     *
     * Übernimmt die RTC-Zeit als neue Systemzeit des ESP32
     *
     * @return true bei Erfolg, false bei Fehler
     */
    bool setAsSystemTime();

    /**
     * @brief Statische Hilfsfunktion zum Abrufen der lokalen Systemzeit
     *
     * @return tm-Struktur mit der aktuellen lokalen Systemzeit
     */
    static struct tm getCurrentLocalTime();

    /**
     * @brief Prüft, ob die aktuelle Systemzeit plausibel ist
     *
     * Plausibel heisst: Jahr >= 2024. Ohne NTP-Sync und ohne RTC-Hydration beginnt die ESP32-Systemzeit bei 1970. Diese Funktion erlaubt Anzeigen zu entscheiden, ob sie echtes Datum oder einen Platzhalter darstellen.
     *
     * @return true wenn die Systemzeit in einem plausiblen Bereich liegt
     */
    static bool isSystemTimeValid();

    /**
     * @brief Formatiert Datum und Zeit mit Standard strftime Format
     *
     * Verwendet die Standard C strftime Funktion zur Formatierung. Unterstützt alle Standard-Formatspezifikatoren.
     *
     * @param buffer Zielpuffer für den formatierten String
     * @param bufferSize Größe des Zielpuffers
     * @param formatStr Format-String im strftime-Format
     * @return Anzahl der geschriebenen Zeichen oder 0 bei Fehler
     *
     * @example
     * char buffer[32];
     * rtc.getFormattedDateTime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S");
     * // Ergebnis: "2024-01-15 14:30:25"
     */
    size_t getFormattedDateTime(char* buffer, size_t bufferSize, const char* formatStr);

    /**
     * @brief Gibt Datum und Zeit in vordefinierten lokalisierten Formaten aus
     *
     * Bietet sechs vordefinierte Formate. Wochentag- und Monatsnamen sowie die Trennzeichen werden anhand der aktuellen UI-Sprache aufgelöst.
     *
     * @param buffer Zielpuffer für den formatierten String
     * @param bufferSize Größe des Zielpuffers (empfohlen: mindestens 64 Zeichen)
     * @param format Format-Auswahl:
     *               0 = Vollständig: "Montag, 15. Januar 2024 14:30:25" / "Monday, 15 January 2024 14:30:25"
     *               1 = Kurz: "Mo, 15.01.2024 14:30" / "Mon, 15/01/2024 14:30"
     *               2 = Nur Datum: "15. Januar 2024" / "15 January 2024"
     *               3 = Nur Zeit: "14:30:25" (sprachneutral)
     *               4 = ISO-Format: "2024-01-15 14:30:25" (sprachneutral)
     *               5 = Standard: "15.01.2024 14:30:25" / "15/01/2024 14:30:25" (default)
     * @return Anzahl der geschriebenen Zeichen oder 0 bei Fehler
     *
     * Bei nicht erreichbarer RTC wird ein lokalisierter Fehlertext geschrieben und dessen Länge zurückgegeben.
     */
    size_t getLocalizedDateTimeString(char* buffer, size_t bufferSize, uint8_t format = 5);
};

#endif  // DS1307_RTC_H

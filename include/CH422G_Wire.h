#ifndef CH422G_WIRE_H
#define CH422G_WIRE_H

#include <stdint.h>
#include <Wire.h>

/**
 * @file CH422G_Wire.h
 * @brief Treiber für den CH422G I/O-Expander
 * 
 * Der CH422G ist ein I/O-Expander mit folgenden Eigenschaften:
 * - 8 bidirektionale I/O-Pins (IO0-IO7): können Input oder Output sein
 * - 4 Open-Collector/Push-Pull Outputs (OC0-OC3): nur Output
 * - Alle IO-Pins teilen sich den gleichen Modus (alle Input ODER alle Output)
 * 
 * WICHTIG: Der CH422G verwendet verschiedene I2C-Adressen für jedes Register! Dies ist ungewöhnlich und unterscheidet sich von normalen I2C-Geräten.
 * 
 * Pin-Mapping:
 *   Pin 0-7:  IO0-IO7 (bidirektional, über REG_WR_IO / REG_RD_IO)
 *   Pin 8-11: OC0-OC3 (nur Output, über REG_WR_OC)
 * 
 * @note Diese Klasse ist kompatibel mit der Arduino Wire-Bibliothek und
 *       funktioniert mit dem neuen ESP32 Arduino Core 3.x (driver_ng).
 */

class CH422G_Wire {
public:
    // =========================================================================
    // Öffentliche Konstanten
    // =========================================================================

    // Maximale Anzahl an IO-Pins (0-7)
    static constexpr uint8_t IO_PIN_COUNT = 8;
    
    // Maximale Anzahl an OC-Pins (8-11, intern 0-3)
    static constexpr uint8_t OC_PIN_COUNT = 4;
    
    // Gesamte Pin-Anzahl (12 Pins: IO0-7 + OC0-3)
    static constexpr uint8_t TOTAL_PIN_COUNT = IO_PIN_COUNT + OC_PIN_COUNT;

    // =========================================================================
    // Konstruktor & Initialisierung
    // =========================================================================

    /**
     * @brief Konstruktor
     * @param wire Referenz auf TwoWire-Objekt (z.B. Wire oder Wire1)
     */
    explicit CH422G_Wire(TwoWire& wire = Wire);

    /**
     * @brief Initialisiert den IO-Expander und setzt Standardwerte
     * @return true bei Erfolg, false bei I2C-Kommunikationsfehler
     * 
     * Setzt folgende Standardkonfiguration:
     * - Alle IO-Pins (0-7) auf Output-Modus
     * - Push-Pull-Modus für OC-Pins
     * - Alle Outputs auf HIGH
     * 
     * @note Wire.begin() muss vorher aufgerufen werden!
     */
    bool begin();

    // =========================================================================
    // Pin-Modus Konfiguration
    // =========================================================================

    /**
     * @brief Setzt alle IO-Pins (0-7) auf Input-Modus
     * @return true bei Erfolg
     * 
     * @warning Alle IO-Pins werden gleichzeitig umgeschaltet!
     *          Der CH422G erlaubt keine individuelle Konfiguration.
     */
    bool setAllIOInput();

    /**
     * @brief Setzt alle IO-Pins (0-7) auf Output-Modus
     * @return true bei Erfolg
     */
    bool setAllIOOutput();

    /**
     * @brief Setzt die OC-Pins (8-11) auf Open-Drain-Modus
     * @return true bei Erfolg
     * 
     * Im Open-Drain-Modus kann ein Pin nur nach GND ziehen. Für HIGH wird ein externer Pull-Up-Widerstand benötigt.
     */
    bool setOCOpenDrain();

    /**
     * @brief Setzt die OC-Pins (8-11) auf Push-Pull-Modus
     * @return true bei Erfolg
     * 
     * Im Push-Pull-Modus kann ein Pin aktiv HIGH oder LOW treiben.
     */
    bool setOCPushPull();

    // =========================================================================
    // Ausgang Schreiben (Output)
    // =========================================================================

    /**
     * @brief Setzt einen einzelnen Pin auf HIGH oder LOW
     * @param pin Pin-Nummer (0-11)
     *            - Pin 0-7:  IO-Pins (müssen im Output-Modus sein)
     *            - Pin 8-11: OC-Pins (immer Output)
     * @param level HIGH oder LOW
     * @return true bei Erfolg, false bei ungültigem Pin oder I2C-Fehler
     */
    bool digitalWrite(uint8_t pin, uint8_t level);

    /**
     * @brief Schreibt ein Byte auf alle IO-Pins (0-7)
     * @param value Bitmuster (Bit 0 = IO0, Bit 7 = IO7)
     * @return true bei Erfolg
     */
    bool writeIO(uint8_t value);

    /**
     * @brief Schreibt ein Nibble auf alle OC-Pins (8-11)
     * @param value Bitmuster (Bit 0 = OC0/Pin8, Bit 3 = OC3/Pin11)
     * @return true bei Erfolg
     * 
     * @note Nur die unteren 4 Bits werden verwendet!
     */
    bool writeOC(uint8_t value);

    /**
     * @brief Schreibt alle 12 Pins auf einmal
     * @param value 16-Bit-Wert (Bits 0-7 = IO, Bits 8-11 = OC)
     * @return true bei Erfolg
     */
    bool writeAll(uint16_t value);

    // =========================================================================
    // Eingang Lesen (Input)
    // =========================================================================

    /**
     * @brief Liest den Zustand eines einzelnen IO-Pins (0-7)
     * @param pin Pin-Nummer (0-7)
     * @return HIGH, LOW oder -1 bei Fehler
     * 
     * @note OC-Pins (8-11) können nicht gelesen werden!
     */
    int digitalRead(uint8_t pin);

    /**
     * @brief Liest alle IO-Pins (0-7) als Byte
     * @param value Referenz für den gelesenen Wert
     * @return true bei Erfolg
     */
    bool readIO(uint8_t& value);

    // =========================================================================
    // Status & Debugging
    // =========================================================================

    /**
     * @brief Gibt den aktuell gecachten IO-Wert zurück (ohne Hardware-Zugriff)
     * @return Zuletzt geschriebener IO-Wert
     */
    uint8_t getCachedIO() const { return m_wrIO; }

    /**
     * @brief Gibt den aktuell gecachten OC-Wert zurück (ohne Hardware-Zugriff)
     * @return Zuletzt geschriebener OC-Wert
     */
    uint8_t getCachedOC() const { return m_wrOC; }

    /**
     * @brief Prüft, ob die IO-Pins im Output-Modus sind
     * @return true wenn Output-Modus aktiv
     */
    bool isIOOutput() const { return (m_wrSet & BIT_IO_OE) != 0; }

    /**
     * @brief Prüft, ob die OC-Pins im Open-Drain-Modus sind
     * @return true wenn Open-Drain aktiv
     */
    bool isOCOpenDrain() const { return (m_wrSet & BIT_OD_EN) != 0; }

private:
    // =========================================================================
    // I2C Register-Adressen (7-Bit)
    // =========================================================================
    // Der CH422G verwendet verschiedene I2C-Adressen für jedes Register! Die 8-Bit-Adressen aus dem Datenblatt werden durch 2 geteilt.

    // System-Konfigurationsregister (Output Enable, Open-Drain Enable)
    static constexpr uint8_t REG_WR_SET = 0x48 >> 1;  ///< 0x24

    // OC-Output-Register (Open-Collector Pins OC0-OC3)
    static constexpr uint8_t REG_WR_OC = 0x46 >> 1;   ///< 0x23

    // IO-Output-Register (Bidirektionale Pins IO0-IO7)
    static constexpr uint8_t REG_WR_IO = 0x70 >> 1;   ///< 0x38

    // IO-Input-Register (Lesen der IO-Pin-Zustände)
    static constexpr uint8_t REG_RD_IO = 0x4D >> 1;   ///< 0x26

    // =========================================================================
    // Bit-Definitionen für WR_SET Register
    // =========================================================================

    /// Bit 0: IO Output Enable (1 = Output, 0 = Input)
    static constexpr uint8_t BIT_IO_OE = (1 << 0);

    /// Bit 2: Open-Drain Enable für OC-Pins (1 = Open-Drain, 0 = Push-Pull)
    static constexpr uint8_t BIT_OD_EN = (1 << 2);

    // =========================================================================
    // Standardwerte nach Reset
    // =========================================================================

    // Default: IO_OE=1 (Output-Modus), OD_EN=0 (Push-Pull)
    static constexpr uint8_t DEFAULT_WR_SET = 0x01;

    // Default: Alle OC-Pins HIGH
    static constexpr uint8_t DEFAULT_WR_OC = 0x0F;

    // Default: Alle IO-Pins HIGH
    static constexpr uint8_t DEFAULT_WR_IO = 0xFF;

    // =========================================================================
    // Private Member
    // =========================================================================

    TwoWire& m_wire;     ///< Referenz auf Wire-Objekt
    uint8_t m_wrSet;     ///< Gecachter Wert des WR_SET-Registers
    uint8_t m_wrOC;      ///< Gecachter Wert des WR_OC-Registers
    uint8_t m_wrIO;      ///< Gecachter Wert des WR_IO-Registers

    // =========================================================================
    // Private Methoden
    // =========================================================================

    /**
     * @brief Schreibt ein Byte in ein Register
     * @param reg Register-Adresse (wird als I2C-Adresse verwendet!)
     * @param value Zu schreibender Wert
     * @return true bei Erfolg (ACK empfangen)
     */
    bool writeRegister(uint8_t reg, uint8_t value);

    /**
     * @brief Liest ein Byte aus einem Register
     * @param reg Register-Adresse (wird als I2C-Adresse verwendet!)
     * @param value Referenz für gelesenen Wert
     * @return true bei Erfolg
     */
    bool readRegister(uint8_t reg, uint8_t& value);
};

#endif // CH422G_WIRE_H

#include "CH422G_Wire.h"

// ===================================================================
// Konstruktor
// ===================================================================

/**
 * @brief Konstruktor mit Wire-Referenz
 */
CH422G_Wire::CH422G_Wire(TwoWire &wire)
    : m_wire(wire), m_wrSet(DEFAULT_WR_SET), m_wrOC(DEFAULT_WR_OC), m_wrIO(DEFAULT_WR_IO)
{
}

// ===================================================================
// Initialisierung
// ===================================================================

/**
 * @brief Initialisiert den IO-Expander mit Standardkonfiguration
 */
bool CH422G_Wire::begin()
{
    // Alle Register auf Standardwerte setzen
    bool ok = true;

    ok &= writeRegister(REG_WR_SET, m_wrSet);
    ok &= writeRegister(REG_WR_OC, m_wrOC);
    ok &= writeRegister(REG_WR_IO, m_wrIO);

    return ok;
}

// ===================================================================
// Pin-Modus Konfiguration
// ===================================================================

/**
 * @brief Alle IO-Pins auf Input setzen
 */
bool CH422G_Wire::setAllIOInput()
{
    m_wrSet &= ~BIT_IO_OE;
    bool ok = writeRegister(REG_WR_SET, m_wrSet);

    // Kurze Verzögerung nach Umschalten auf Input (lt. Datenblatt empfohlen)
    if (ok)
        delay(2);

    return ok;
}

/**
 * @brief Alle IO-Pins auf Output setzen
 */
bool CH422G_Wire::setAllIOOutput()
{
    m_wrSet |= BIT_IO_OE;
    return writeRegister(REG_WR_SET, m_wrSet);
}

/**
 * @brief OC-Pins auf Open-Drain setzen
 */
bool CH422G_Wire::setOCOpenDrain()
{
    m_wrSet |= BIT_OD_EN;
    return writeRegister(REG_WR_SET, m_wrSet);
}

/**
 * @brief OC-Pins auf Push-Pull setzen
 */
bool CH422G_Wire::setOCPushPull()
{
    m_wrSet &= ~BIT_OD_EN;
    return writeRegister(REG_WR_SET, m_wrSet);
}

// ===================================================================
// Digitales Schreiben
// ===================================================================

/**
 * @brief Einzelnen Pin schreiben (IO oder OC)
 */
bool CH422G_Wire::digitalWrite(uint8_t pin, uint8_t level)
{
    // Bereichsprüfung
    if (pin >= TOTAL_PIN_COUNT)
        return false;

    // IO-Pins (0-7)
    if (pin < IO_PIN_COUNT)
    {
        if (level)
            m_wrIO |= (1 << pin);
        else
            m_wrIO &= ~(1 << pin);

        return writeRegister(REG_WR_IO, m_wrIO);
    }

    // OC-Pins (8-11 -> intern 0-3)
    uint8_t ocPin = pin - IO_PIN_COUNT;
    if (level)
        m_wrOC |= (1 << ocPin);
    else
        m_wrOC &= ~(1 << ocPin);

    return writeRegister(REG_WR_OC, m_wrOC);
}

/**
 * @brief Alle IO-Pins schreiben
 */
bool CH422G_Wire::writeIO(uint8_t value)
{
    m_wrIO = value;
    return writeRegister(REG_WR_IO, m_wrIO);
}

/**
 * @brief Alle OC-Pins schreiben
 */
bool CH422G_Wire::writeOC(uint8_t value)
{
    m_wrOC = value & 0x0F; // Nur untere 4 Bits verwenden
    return writeRegister(REG_WR_OC, m_wrOC);
}

/**
 * @brief Alle Pins auf einmal schreiben
 */
bool CH422G_Wire::writeAll(uint16_t value)
{
    uint8_t io = value & 0xFF;
    uint8_t oc = (value >> 8) & 0x0F;

    bool ok = true;

    // Nur schreiben, wenn sich der Wert geändert hat (Optimierung)
    if (io != m_wrIO)
    {
        m_wrIO = io;
        ok &= writeRegister(REG_WR_IO, m_wrIO);
    }

    if (oc != m_wrOC)
    {
        m_wrOC = oc;
        ok &= writeRegister(REG_WR_OC, m_wrOC);
    }

    return ok;
}

// ===================================================================
// Digitales Lesen
// ===================================================================

/**
 * @brief Einzelnen IO-Pin lesen
 */
int CH422G_Wire::digitalRead(uint8_t pin)
{
    // Nur IO-Pins können gelesen werden
    if (pin >= IO_PIN_COUNT)
        return -1;

    uint8_t value = 0;
    if (!readRegister(REG_RD_IO, value))
        return -1;

    return (value & (1 << pin)) ? HIGH : LOW;
}

/**
 * @brief Alle IO-Pins lesen
 */
bool CH422G_Wire::readIO(uint8_t &value)
{
    return readRegister(REG_RD_IO, value);
}

// ===================================================================
// Private I2C-Methoden
// ===================================================================

/**
 * @brief Register schreiben (Register-Adresse = I2C-Adresse!)
 */
bool CH422G_Wire::writeRegister(uint8_t reg, uint8_t value)
{
    // WICHTIG: Der CH422G verwendet die Register-Adresse als I2C-Adresse! Das ist ungewöhnlich, aber so funktioniert der Chip.
    m_wire.beginTransmission(reg);
    m_wire.write(value);
    return (m_wire.endTransmission() == 0);
}

/**
 * @brief Register lesen (Register-Adresse = I2C-Adresse!)
 */
bool CH422G_Wire::readRegister(uint8_t reg, uint8_t &value)
{
    // WICHTIG: Der CH422G verwendet die Register-Adresse als I2C-Adresse!
    m_wire.requestFrom(static_cast<int>(reg), 1);

    if (m_wire.available())
    {
        value = m_wire.read();
        return true;
    }

    return false;
}

#include "DS1307_RTC.h"
#include "StringUtils.h"
#include "I18n.h"

// ===================================================================
// KONSTANTEN
// ===================================================================

namespace
{
    // Bit-Masken für RTC-Register
    constexpr uint8_t CH_BIT_MASK = 0x80;          // Clock Halt Bit (Sekunden-Register)
    constexpr uint8_t CH_BIT_CLEAR_MASK = 0x7F;    // Maske zum Löschen des CH-Bits
    constexpr uint8_t HOUR_24H_FORMAT_MASK = 0x3F; // 24-Stunden-Format Maske
    constexpr uint8_t DAY_OF_WEEK_MASK = 0x07;     // Wochentag-Maske (3 Bits)
    constexpr uint8_t BCD_HIGH_NIBBLE_MASK = 0xF0; // Oberes Nibble für BCD
    constexpr uint8_t BCD_LOW_NIBBLE_MASK = 0x0F;  // Unteres Nibble für BCD
    constexpr uint8_t BYTE_MASK = 0xFF;            // Vollständiges Byte

    // Jahresoffsets
    constexpr uint16_t TM_YEAR_BASE = 1900;     // tm_year Basis (Jahre seit 1900)
    constexpr uint16_t DS1307_YEAR_BASE = 2000; // DS1307 Jahr Basis
    constexpr uint16_t DS1307_YEAR_MIN = 2000;  // Minimales unterstütztes Jahr
    constexpr uint16_t DS1307_YEAR_MAX = 2099;  // Maximales unterstütztes Jahr

    // Schwelle für isSystemTimeValid(): darunter gilt die Systemzeit als unkonfiguriert
    constexpr uint16_t MIN_VALID_SYSTEM_YEAR = 2024;

    // Datums- und Zeitgrenzen
    constexpr uint8_t MONTH_MIN = 1;  // Januar
    constexpr uint8_t MONTH_MAX = 12; // Dezember
    constexpr uint8_t DAY_MIN = 1;
    constexpr uint8_t DAY_MAX = 31;
    constexpr uint8_t HOUR_MAX = 23;
    constexpr uint8_t MINUTE_MAX = 59;
    constexpr uint8_t SECOND_MAX = 59;
    constexpr uint8_t DAY_OF_WEEK_MIN = 1; // Montag (ISO 8601)
    constexpr uint8_t DAY_OF_WEEK_MAX = 7; // Sonntag (ISO 8601)

    // Wochentag-Konvertierung
    constexpr uint8_t SUNDAY_ISO_8601 = 7; // Sonntag in ISO 8601 (Montag=1)
    constexpr uint8_t SUNDAY_TM_WDAY = 0;  // Sonntag in tm_wday (Sonntag=0)

    // Bit-Shifts
    constexpr uint8_t BCD_HIGH_NIBBLE_SHIFT = 4; // Shift für oberes BCD-Nibble
    constexpr uint8_t BITS_PER_BYTE = 8;         // Bits pro Byte

    // Array-Größen
    constexpr uint8_t DATETIME_REGISTER_COUNT = 7; // Anzahl RTC-Register für Datum/Zeit
    constexpr uint8_t UINT16_BYTE_COUNT = 2;       // Bytes für uint16_t
    constexpr uint8_t UINT32_BYTE_COUNT = 4;       // Bytes für uint32_t

    // BCD-Konvertierung
    constexpr uint8_t BCD_DECIMAL_BASE = 10; // Dezimalbasis für BCD

    // Monat-Array-Index
    constexpr uint8_t MONTH_ARRAY_OFFSET = 1; // Monat 1-12 -> Array Index 0-11

    // Format-IDs für getLocalizedDateTimeString()
    constexpr uint8_t FORMAT_FULL = 0;    // "Montag, 15. Januar 2024 14:30:25" / "Monday, 15 January 2024 14:30:25"
    constexpr uint8_t FORMAT_SHORT = 1;   // "Mo, 15.01.2024 14:30" / "Mon, 15/01/2024 14:30"
    constexpr uint8_t FORMAT_DATE = 2;    // "15. Januar 2024" / "15 January 2024"
    constexpr uint8_t FORMAT_TIME = 3;    // "14:30:25"
    constexpr uint8_t FORMAT_ISO = 4;     // "2024-01-15 14:30:25"
    constexpr uint8_t FORMAT_DEFAULT = 5; // "15.01.2024 14:30:25" / "15/01/2024 14:30:25"
}

// ===================================================================
// KONSTRUKTOR UND INITIALISIERUNG
// ===================================================================

/**
 * @brief Konstruktor für DS1307_RTC
 */
DS1307_RTC::DS1307_RTC(TwoWire *wire) : _wire(wire) {}

/**
 * @brief Initialisiert die RTC-Kommunikation
 */
bool DS1307_RTC::begin()
{
    // Präsenz-Flag vor dem I2C-Ping zurücksetzen, damit die folgenden readRegister/writeRegister-Aufrufe freigegeben werden, sobald der Ping erfolgreich war.
    m_present = false;

    _wire->beginTransmission(DS1307_ADDRESS);
    if (_wire->endTransmission() != 0)
        return false;

    m_present = true;

    // Prüfe CH-Bit und starte Uhr falls nötig
    uint8_t seconds = readRegister(DS1307_REG_SECONDS);
    if (seconds & CH_BIT_MASK)
    {
        if (!startClock())
        {
            m_present = false;
            return false;
        }
    }

    return true;
}

// ===================================================================
// CLOCK CONTROL
// ===================================================================

/**
 * @brief Prüft ob die RTC-Uhr läuft
 */
bool DS1307_RTC::isRunning()
{
    if (!m_present)
        return false;

    uint8_t seconds = readRegister(DS1307_REG_SECONDS);

    return !(seconds & CH_BIT_MASK);
}

/**
 * @brief Startet den RTC-Oszillator
 */
bool DS1307_RTC::startClock()
{
    uint8_t seconds = readRegister(DS1307_REG_SECONDS);
    seconds &= CH_BIT_CLEAR_MASK; // CH-Bit löschen

    return writeRegister(DS1307_REG_SECONDS, seconds);
}

/**
 * @brief Stoppt den RTC-Oszillator
 */
bool DS1307_RTC::stopClock()
{
    uint8_t seconds = readRegister(DS1307_REG_SECONDS);
    seconds |= CH_BIT_MASK; // CH-Bit setzen

    return writeRegister(DS1307_REG_SECONDS, seconds);
}

// ===================================================================
// DATUM/ZEIT LESEN UND SETZEN
// ===================================================================

/**
 * @brief Setzt Datum und Uhrzeit der RTC
 */
bool DS1307_RTC::setDateTime(const DateTime &dt)
{
    // Wertebereich prüfen
    if (dt.month < MONTH_MIN || dt.month > MONTH_MAX ||
        dt.day < DAY_MIN || dt.day > DAY_MAX ||
        dt.hour > HOUR_MAX || dt.minute > MINUTE_MAX || dt.second > SECOND_MAX ||
        dt.dayOfWeek < DAY_OF_WEEK_MIN || dt.dayOfWeek > DAY_OF_WEEK_MAX ||
        dt.year < DS1307_YEAR_MIN || dt.year > DS1307_YEAR_MAX)
    {
        return false;
    }

    uint8_t data[DATETIME_REGISTER_COUNT];
    uint8_t current_seconds_ch_bit = readRegister(DS1307_REG_SECONDS) & CH_BIT_MASK;

    // BCD-Konvertierung mit CH-Bit Erhaltung
    data[0] = decToBcd(dt.second) | current_seconds_ch_bit;
    data[1] = decToBcd(dt.minute);
    data[2] = decToBcd(dt.hour) & HOUR_24H_FORMAT_MASK; // 24h-Format
    data[3] = decToBcd(dt.dayOfWeek);
    data[4] = decToBcd(dt.day);
    data[5] = decToBcd(dt.month);
    data[6] = decToBcd(dt.year - DS1307_YEAR_BASE);

    return writeMultiple(DS1307_REG_SECONDS, data, DATETIME_REGISTER_COUNT);
}

/**
 * @brief Liest das aktuelle Datum und die Uhrzeit von der RTC
 */
bool DS1307_RTC::getDateTime(DateTime &dt)
{
    // dt grundsätzlich auf definierte Null-Werte setzen, damit Aufrufer, die den Rückgabewert ignorieren, keinen Stack-Müll lesen.
    dt = DateTime{};

    uint8_t data[DATETIME_REGISTER_COUNT];

    if (!readMultiple(DS1307_REG_SECONDS, data, DATETIME_REGISTER_COUNT))
        return false;

    // BCD zu Dezimal mit Maskierung
    dt.second = bcdToDec(data[0] & CH_BIT_CLEAR_MASK);
    dt.minute = bcdToDec(data[1]);
    dt.hour = bcdToDec(data[2] & HOUR_24H_FORMAT_MASK);
    dt.dayOfWeek = bcdToDec(data[3] & DAY_OF_WEEK_MASK);
    dt.day = bcdToDec(data[4]);
    dt.month = bcdToDec(data[5]);
    dt.year = bcdToDec(data[6]) + DS1307_YEAR_BASE;

    return true;
}

/**
 * @brief Setzt nur die Uhrzeit (behält Datum bei)
 */
bool DS1307_RTC::setTime(uint8_t hour, uint8_t minute, uint8_t second)
{
    DateTime dt;
    if (!getDateTime(dt))
        return false;

    dt.hour = hour;
    dt.minute = minute;
    dt.second = second;

    return setDateTime(dt);
}

/**
 * @brief Setzt nur das Datum (behält Uhrzeit bei)
 */
bool DS1307_RTC::setDate(uint16_t year, uint8_t month, uint8_t day)
{
    DateTime dt;
    if (!getDateTime(dt))
        return false;

    dt.year = year;
    dt.month = month;
    dt.day = day;

    return setDateTime(dt);
}

// ===================================================================
// SYSTEM-ZEIT SYNCHRONISATION
// ===================================================================

/**
 * @brief Synchronisiert RTC mit der aktuellen Systemzeit
 */
bool DS1307_RTC::syncFromSystem()
{
    time_t now_epoch;
    time(&now_epoch);

    struct tm timeinfo;
    localtime_r(&now_epoch, &timeinfo);

    DateTime dt;
    dt.year = timeinfo.tm_year + TM_YEAR_BASE;
    dt.month = timeinfo.tm_mon + MONTH_ARRAY_OFFSET;
    dt.day = timeinfo.tm_mday;
    dt.hour = timeinfo.tm_hour;
    dt.minute = timeinfo.tm_min;
    dt.second = timeinfo.tm_sec;
    dt.dayOfWeek = (timeinfo.tm_wday == SUNDAY_TM_WDAY) ? SUNDAY_ISO_8601 : timeinfo.tm_wday; // So=0 -> 7

    return setDateTime(dt);
}

/**
 * @brief Setzt die Systemzeit auf die RTC-Zeit
 */
bool DS1307_RTC::setAsSystemTime()
{
    DateTime dt;
    if (!getDateTime(dt))
        return false;

    struct tm tm_to_set;
    tm_to_set.tm_year = dt.year - TM_YEAR_BASE;
    tm_to_set.tm_mon = dt.month - MONTH_ARRAY_OFFSET;
    tm_to_set.tm_mday = dt.day;
    tm_to_set.tm_hour = dt.hour;
    tm_to_set.tm_min = dt.minute;
    tm_to_set.tm_sec = dt.second;
    tm_to_set.tm_wday = (dt.dayOfWeek == SUNDAY_ISO_8601) ? SUNDAY_TM_WDAY : dt.dayOfWeek; // So=7 -> 0
    tm_to_set.tm_isdst = -1;

    time_t t = mktime(&tm_to_set);
    if (t == -1)
        return false;

    struct timeval tv = {t, 0};

    return (settimeofday(&tv, nullptr) == 0);
}

// ===================================================================
// NVRAM ZUGRIFF
// ===================================================================

/**
 * @brief Schreibt ein Byte in das NVRAM
 */
bool DS1307_RTC::writeNVRAM(uint8_t address, uint8_t data)
{
    if (!NVRAM_ADDR_VALID(address))
        return false;

    return writeRegister(DS1307_REG_NVRAM_START + address, data);
}

/**
 * @brief Liest ein Byte aus dem NVRAM
 */
uint8_t DS1307_RTC::readNVRAM(uint8_t address)
{
    if (!NVRAM_ADDR_VALID(address))
        return 0;

    return readRegister(DS1307_REG_NVRAM_START + address);
}

/**
 * @brief Schreibt einen Datenblock in das NVRAM
 */
bool DS1307_RTC::writeNVRAMBlock(uint8_t address, const uint8_t *data, uint8_t length)
{
    if (!NVRAM_RANGE_VALID(address, length) || length == 0)
        return false;

    return writeMultiple(DS1307_REG_NVRAM_START + address, const_cast<uint8_t *>(data), length);
}

/**
 * @brief Liest einen Datenblock aus dem NVRAM
 */
bool DS1307_RTC::readNVRAMBlock(uint8_t address, uint8_t *data, uint8_t length)
{
    if (!NVRAM_RANGE_VALID(address, length) || length == 0)
        return false;

    return readMultiple(DS1307_REG_NVRAM_START + address, data, length);
}

/**
 * @brief Löscht das komplette NVRAM
 */
bool DS1307_RTC::clearNVRAM()
{
    uint8_t zeros[DS1307_NVRAM_SIZE];
    memset(zeros, 0, DS1307_NVRAM_SIZE);

    return writeNVRAMBlock(0, zeros, DS1307_NVRAM_SIZE);
}

// ===================================================================
// NVRAM DATENTYP-WRAPPER
// ===================================================================

/**
 * @brief Schreibt einen booleschen Wert in das NVRAM
 */
bool DS1307_RTC::writeBool(uint8_t address, bool value)
{
    return writeNVRAM(address, value ? 1 : 0);
}

/**
 * @brief Liest einen booleschen Wert aus dem NVRAM
 */
bool DS1307_RTC::readBool(uint8_t address)
{
    if (!NVRAM_ADDR_VALID(address))
        return false;
    return readNVRAM(address) != 0;
}

/**
 * @brief Schreibt einen vorzeichenbehafteten 8-Bit-Wert in das NVRAM
 */
bool DS1307_RTC::writeInt8(uint8_t address, int8_t value)
{
    return writeNVRAM(address, static_cast<uint8_t>(value));
}

/**
 * @brief Liest einen vorzeichenbehafteten 8-Bit-Wert aus dem NVRAM
 */
int8_t DS1307_RTC::readInt8(uint8_t address)
{
    if (!NVRAM_ADDR_VALID(address))
        return 0;

    return static_cast<int8_t>(readNVRAM(address));
}

/**
 * @brief Schreibt einen vorzeichenlosen 8-Bit-Wert in das NVRAM
 */
bool DS1307_RTC::writeUint8(uint8_t address, uint8_t value)
{
    return writeNVRAM(address, value);
}

/**
 * @brief Liest einen vorzeichenlosen 8-Bit-Wert aus dem NVRAM
 */
uint8_t DS1307_RTC::readUint8(uint8_t address)
{
    return readNVRAM(address);
}

/**
 * @brief Schreibt einen 16-Bit-Wert in das NVRAM (Little Endian)
 */
bool DS1307_RTC::writeUint16(uint8_t address, uint16_t value)
{
    if (!NVRAM_RANGE_VALID(address, sizeof(uint16_t)))
        return false;

    uint8_t data[UINT16_BYTE_COUNT];
    data[0] = value & BYTE_MASK;                    // LSB
    data[1] = (value >> BITS_PER_BYTE) & BYTE_MASK; // MSB

    return writeNVRAMBlock(address, data, UINT16_BYTE_COUNT);
}

/**
 * @brief Liest einen 16-Bit-Wert aus dem NVRAM (Little Endian)
 */
uint16_t DS1307_RTC::readUint16(uint8_t address)
{
    if (!NVRAM_RANGE_VALID(address, sizeof(uint16_t)))
        return 0;

    uint8_t data[UINT16_BYTE_COUNT];
    if (!readNVRAMBlock(address, data, UINT16_BYTE_COUNT))
        return 0;

    return (static_cast<uint16_t>(data[1]) << BITS_PER_BYTE) | data[0];
}

/**
 * @brief Schreibt einen 32-Bit-Wert in das NVRAM (Little Endian)
 */
bool DS1307_RTC::writeUint32(uint8_t address, uint32_t value)
{
    if (!NVRAM_RANGE_VALID(address, sizeof(uint32_t)))
        return false;

    uint8_t data[UINT32_BYTE_COUNT];
    for (int i = 0; i < UINT32_BYTE_COUNT; ++i)
    {
        data[i] = (value >> (BITS_PER_BYTE * i)) & BYTE_MASK;
    }

    return writeNVRAMBlock(address, data, UINT32_BYTE_COUNT);
}

/**
 * @brief Liest einen 32-Bit-Wert aus dem NVRAM (Little Endian)
 */
uint32_t DS1307_RTC::readUint32(uint8_t address)
{
    if (!NVRAM_RANGE_VALID(address, sizeof(uint32_t)))
        return 0;

    uint8_t data[UINT32_BYTE_COUNT];
    if (!readNVRAMBlock(address, data, UINT32_BYTE_COUNT))
        return 0;

    uint32_t value = 0;
    for (int i = 0; i < UINT32_BYTE_COUNT; ++i)
    {
        value |= (static_cast<uint32_t>(data[i])) << (BITS_PER_BYTE * i);
    }

    return value;
}

// ===================================================================
// EPOCH (UNIX-TIMESTAMP) KONVERTIERUNG
// ===================================================================

/**
 * @brief Konvertiert die RTC-Zeit zu Unix-Timestamp (Epoch)
 */
uint32_t DS1307_RTC::getEpoch()
{
    DateTime dt;
    if (!getDateTime(dt))
        return 0;

    struct tm timeinfo;
    timeinfo.tm_year = dt.year - TM_YEAR_BASE;
    timeinfo.tm_mon = dt.month - MONTH_ARRAY_OFFSET;
    timeinfo.tm_mday = dt.day;
    timeinfo.tm_hour = dt.hour;
    timeinfo.tm_min = dt.minute;
    timeinfo.tm_sec = dt.second;
    timeinfo.tm_isdst = -1;

    time_t epoch = mktime(&timeinfo);

    return (epoch == -1) ? 0 : static_cast<uint32_t>(epoch);
}

/**
 * @brief Setzt die RTC-Zeit aus Unix-Timestamp
 */
bool DS1307_RTC::setEpoch(uint32_t epoch)
{
    time_t epoch_time_t = epoch;
    struct tm timeinfo;

    if (localtime_r(&epoch_time_t, &timeinfo) == nullptr)
        return false;

    DateTime dt;
    dt.year = timeinfo.tm_year + TM_YEAR_BASE;
    dt.month = timeinfo.tm_mon + MONTH_ARRAY_OFFSET;
    dt.day = timeinfo.tm_mday;
    dt.hour = timeinfo.tm_hour;
    dt.minute = timeinfo.tm_min;
    dt.second = timeinfo.tm_sec;
    dt.dayOfWeek = (timeinfo.tm_wday == SUNDAY_TM_WDAY) ? SUNDAY_ISO_8601 : timeinfo.tm_wday;

    return setDateTime(dt);
}

// ===================================================================
// ZEIT-FORMATIERUNG
// ===================================================================

/**
 * @brief Statische Hilfsfunktion zum Abrufen der lokalen Systemzeit
 */
struct tm DS1307_RTC::getCurrentLocalTime()
{
    time_t now = time(nullptr);
    struct tm timeinfo;
    localtime_r(&now, &timeinfo);

    return timeinfo;
}

/**
 * @brief Prüft, ob die aktuelle Systemzeit plausibel ist
 */
bool DS1307_RTC::isSystemTimeValid()
{
    time_t now = time(nullptr);
    struct tm timeinfo;
    localtime_r(&now, &timeinfo);

    return (timeinfo.tm_year + TM_YEAR_BASE) >= MIN_VALID_SYSTEM_YEAR;
}

// Standard strftime Formatierung
/**
 * @brief Formatiert Datum und Zeit mit Standard strftime Format
 */
size_t DS1307_RTC::getFormattedDateTime(char *buffer, size_t bufferSize, const char *formatStr)
{
    if (!buffer || bufferSize == 0 || !formatStr)
        return 0;

    DateTime rtc_dt;
    if (!getDateTime(rtc_dt))
    {
        return I18n::format(buffer, bufferSize, CommonKey::COMMON_ERR_RTC);
    }

    struct tm timeinfo{};
    timeinfo.tm_year = rtc_dt.year - TM_YEAR_BASE;
    timeinfo.tm_mon = rtc_dt.month - MONTH_ARRAY_OFFSET;
    timeinfo.tm_mday = rtc_dt.day;
    timeinfo.tm_hour = rtc_dt.hour;
    timeinfo.tm_min = rtc_dt.minute;
    timeinfo.tm_sec = rtc_dt.second;
    timeinfo.tm_wday = (rtc_dt.dayOfWeek == SUNDAY_ISO_8601) ? SUNDAY_TM_WDAY : rtc_dt.dayOfWeek;
    timeinfo.tm_isdst = -1;

    return strftime(buffer, bufferSize, formatStr, &timeinfo);
}

// Lokalisierte vordefinierte Formate
/**
 * @brief Gibt Datum und Zeit in vordefinierten lokalisierten Formaten aus
 */
size_t DS1307_RTC::getLocalizedDateTimeString(char *buffer, size_t bufferSize, uint8_t format)
{
    if (!buffer || bufferSize == 0)
        return 0;

    DateTime dt;
    if (!getDateTime(dt))
    {
        return I18n::format(buffer, bufferSize, CommonKey::COMMON_ERR_RTC);
    }

    // Wochentag-Index anpassen: DS1307 nutzt ISO 8601 (1=Mo, 7=So), unsere Common-Keys folgen tm_wday-Konvention (0=So, 6=Sa).
    const int wdayIdx = (dt.dayOfWeek == SUNDAY_ISO_8601) ? SUNDAY_TM_WDAY : dt.dayOfWeek;

    // Schlüssel für Wochentag und Monat in der aktuellen Sprache auflösen. Welche Form (kurz/lang) gilt, hängt vom gewählten Format ab.
    const bool wantLongNames = (format == FORMAT_FULL || format == FORMAT_DATE);
    const auto wdKey = static_cast<CommonKey>(
        static_cast<size_t>(wantLongNames ? CommonKey::COMMON_WEEKDAY_LONG_SUN
                                          : CommonKey::COMMON_WEEKDAY_SUN) +
        wdayIdx);
    const auto monthIdx = dt.month - MONTH_ARRAY_OFFSET;
    const auto monKey = static_cast<CommonKey>(
        static_cast<size_t>(wantLongNames ? CommonKey::COMMON_MONTH_LONG_JAN
                                          : CommonKey::COMMON_MONTH_JAN) +
        monthIdx);

    // Format-Key wählen
    CommonKey fmtKey;
    switch (format)
    {
    case FORMAT_FULL:
        fmtKey = CommonKey::COMMON_DT_FORMAT_FULL;
        break;
    case FORMAT_SHORT:
        fmtKey = CommonKey::COMMON_DT_FORMAT_SHORT;
        break;
    case FORMAT_DATE:
        fmtKey = CommonKey::COMMON_DT_FORMAT_DATE;
        break;
    case FORMAT_TIME:
        fmtKey = CommonKey::COMMON_DT_FORMAT_TIME;
        break;
    case FORMAT_ISO:
        fmtKey = CommonKey::COMMON_DT_FORMAT_ISO;
        break;
    default:
        fmtKey = CommonKey::COMMON_DT_FORMAT_DEFAULT;
        break;
    }

    // Wochentag-/Monatsnamen in lokale Puffer kopieren, damit sie als C-Strings an die {0:%s}/{1:%s}-Platzhalter übergeben werden können.
    char wdBuf[16] = {0};
    char monBuf[16] = {0};
    strncpy_P(wdBuf, reinterpret_cast<PGM_P>(I18n::str(wdKey)), sizeof(wdBuf) - 1);
    strncpy_P(monBuf, reinterpret_cast<PGM_P>(I18n::str(monKey)), sizeof(monBuf) - 1);

    return I18n::format(buffer, bufferSize, fmtKey,
                        wdBuf, monBuf,
                        static_cast<int>(dt.day),
                        static_cast<int>(dt.month),
                        static_cast<int>(dt.year),
                        static_cast<int>(dt.hour),
                        static_cast<int>(dt.minute),
                        static_cast<int>(dt.second));
}

// ===================================================================
// PRIVATE HILFSFUNKTIONEN - BCD KONVERTIERUNG
// ===================================================================

/**
 * @brief Konvertiert einen BCD-Wert in eine Dezimalzahl.
 */
uint8_t DS1307_RTC::bcdToDec(uint8_t bcd)
{
    return ((bcd & BCD_HIGH_NIBBLE_MASK) >> BCD_HIGH_NIBBLE_SHIFT) * BCD_DECIMAL_BASE + (bcd & BCD_LOW_NIBBLE_MASK);
}

/**
 * @brief Konvertiert eine Dezimalzahl in einen BCD-Wert.
 */
uint8_t DS1307_RTC::decToBcd(uint8_t dec)
{
    return ((dec / BCD_DECIMAL_BASE) << BCD_HIGH_NIBBLE_SHIFT) | (dec % BCD_DECIMAL_BASE);
}

// ===================================================================
// PRIVATE HILFSFUNKTIONEN - I2C KOMMUNIKATION
// ===================================================================

/**
 * @brief Schreibt ein Byte in das angegebene RTC-Register.
 */
bool DS1307_RTC::writeRegister(uint8_t reg, uint8_t data)
{
    if (!m_present)
        return false;

    _wire->beginTransmission(DS1307_ADDRESS);
    _wire->write(reg);
    _wire->write(data);

    return (_wire->endTransmission() == 0);
}

/**
 * @brief Liest ein Byte aus dem angegebenen RTC-Register.
 */
uint8_t DS1307_RTC::readRegister(uint8_t reg)
{
    if (!m_present)
        return 0;

    _wire->beginTransmission(DS1307_ADDRESS);
    _wire->write(reg);

    if (_wire->endTransmission(false) != 0)
        return 0;

    if (_wire->requestFrom(static_cast<uint8_t>(DS1307_ADDRESS), static_cast<uint8_t>(1)) != 1)
        return 0;

    return _wire->available() ? _wire->read() : 0;
}

/**
 * @brief Schreibt mehrere Bytes ab dem angegebenen RTC-Register.
 */
bool DS1307_RTC::writeMultiple(uint8_t reg, uint8_t *data, uint8_t length)
{
    if (!m_present)
        return false;

    _wire->beginTransmission(DS1307_ADDRESS);
    _wire->write(reg);

    for (uint8_t i = 0; i < length; i++)
    {
        _wire->write(data[i]);
    }

    return (_wire->endTransmission() == 0);
}

/**
 * @brief Liest mehrere Bytes ab dem angegebenen RTC-Register.
 */
bool DS1307_RTC::readMultiple(uint8_t reg, uint8_t *data, uint8_t length)
{
    if (!m_present)
        return false;

    _wire->beginTransmission(DS1307_ADDRESS);
    _wire->write(reg);

    if (_wire->endTransmission(false) != 0)
        return false;

    if (_wire->requestFrom(static_cast<uint8_t>(DS1307_ADDRESS), length) != length)
        return false;

    for (uint8_t i = 0; i < length; i++)
    {
        if (_wire->available())
            data[i] = _wire->read();
        else
            return false;
    }

    return true;
}

#include "Workshop.h"

#include <cmath>
#include <cstring>

#include "I18n.h"

#include "WorkshopTool_AsciiTable.h"     ///< Implementierung der ASCII-Referenztabelle
#include "WorkshopTool_BitEditor.h"      ///< Implementierung des Bit-Editors
#include "WorkshopTool_BmiCalc.h"        ///< Implementierung des BMI-Rechners
#include "WorkshopTool_Calculator.h"     ///< Implementierung des Grundrechenarten-Rechners
#include "WorkshopTool_CircleOfFifths.h" ///< Implementierung des Quintenzirkels
#include "WorkshopTool_ColorConverter.h" ///< Implementierung des Farb-Konverters
#include "WorkshopTool_Decibel.h"        ///< Implementierung des Dezibel-Rechners
#include "WorkshopTool_Dice.h"           ///< Implementierung des Würfel-Werkzeugs
#include "WorkshopTool_FlightRadar.h"    ///< Implementierung des Flugradars
#include "WorkshopTool_Game2048.h"       ///< Implementierung des 2048-Schiebepuzzles
#include "WorkshopTool_InchMm.h"         ///< Implementierung des Zoll/mm-Umrechners
#include "WorkshopTool_LedCalculator.h"  ///< Implementierung des LED-Vorwiderstand-Rechners
#include "WorkshopTool_Lm317.h"          ///< Implementierung des LM317-Spannungsregler-Rechners
#include "WorkshopTool_LogicGate.h"      ///< Implementierung des Logikgatter-Simulators
#include "WorkshopTool_Lotto.h"          ///< Implementierung des normalen Lotto-Spiels
#include "WorkshopTool_LottoSystem.h"    ///< Implementierung des Lotto-Vollsystemspiels
#include "WorkshopTool_Metronome.h"      ///< Implementierung des Metronoms
#include "WorkshopTool_Mill.h"           ///< Implementierung des Mühle-Brettspiels
#include "WorkshopTool_LightsOut.h"      ///< Implementierung des Lights-Out-Knobelspiels
#include "WorkshopTool_Ne555.h"          ///< Implementierung des NE555-Timer-Rechners
#include "WorkshopTool_OhmCalculator.h"  ///< Implementierung des Ohm-Rechners
#include "WorkshopTool_OpAmp.h"          ///< Implementierung des Op-Amp-Rechners
#include "WorkshopTool_PeriodicTable.h"  ///< Implementierung des Periodensystems
#include "WorkshopTool_Pressure.h"       ///< Implementierung des Druck-Umrechners
#include "WorkshopTool_RcFilter.h"       ///< Implementierung des RC/RL-Filter-Rechners
#include "WorkshopTool_LcFilter.h"       ///< Implementierung des LC-Filter-Rechners (2. Ordnung)
#include "WorkshopTool_UkwResonator.h"   ///< Implementierung des UKW-Schwingkreis-Rechners (80...150 MHz)
#include "WorkshopTool_Resistor.h"       ///< Implementierung des Widerstandsfarbcode-Rechners
#include "WorkshopTool_SmdResistor.h"    ///< Implementierung des SMD-Widerstandscode-Decoders
#include "WorkshopTool_ScrewRef.h"       ///< Implementierung der Schrauben-Referenz
#include "WorkshopTool_SiPrefixes.h"     ///< Implementierung der SI-Präfix-Referenztabelle
#include "WorkshopTool_ShutTheBox.h"     ///< Implementierung des Shut-the-Box-Würfelspiels
#include "WorkshopTool_StitchGauge.h"    ///< Implementierung des Maschenprobe-Rechners
#include "WorkshopTool_StitchDistributor.h" ///< Implementierung der Maschenverteilung
#include "WorkshopTool_SleeveTaper.h"    ///< Implementierung der Ärmelschräge
#include "WorkshopTool_Temp.h"           ///< Implementierung des Temperatur-Umrechners
#include "WorkshopTool_VoltageDivider.h" ///< Implementierung des Spannungsteiler-Rechners
#include "WorkshopTool_WorldClock.h"     ///< Implementierung der Weltzeit-Uhr

// Fonts
#include "fonts/ComfortaaBold14.h"
#include "fonts/ComfortaaBold16.h"
#include "fonts/ComfortaaBold18.h"
#include "fonts/ComfortaaBold30.h"
#include "IconShapes.h"

// QR-Code-Renderer mit PSRAM-Allocator
#define QR_CODE_RENDERER_MALLOC(sz) heap_caps_malloc((sz), MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT)
#define QR_CODE_RENDERER_FREE(ptr) free((ptr))
#include "QRCodeRenderer.h"

// ===================================================================
// Anonymer Namespace
// ===================================================================

namespace
{
    // Farben des gezeichneten QR-Symbols, an der früheren Bitmap abgelesen
    constexpr uint16_t ICON_QR_COLOR = 0xA556;      ///< Strichfarbe des QR-Symbols
    constexpr uint16_t ICON_QR_BACKGROUND = 0x2104; ///< Freie Fläche des QR-Symbols

    constexpr uint16_t BTN_BORDER = 0x39E8;
    constexpr uint16_t BTN_FILL = 0x0000;
    constexpr uint16_t QR_MODAL_BORDER = Theme::ColorDialogBorder;
    constexpr uint16_t QR_MODAL_BG = Theme::ColorMainBackGround;
    constexpr uint16_t QR_MODAL_TEXT = 0xBDF7;
    constexpr uint16_t QR_MODAL_TEXT_DIM = 0x7BEF;
    constexpr uint16_t QR_MODAL_HINT_MARK = TFT_RED;
    constexpr int QR_MODAL_HINT_MARK_GAP = 3;
    constexpr int QR_MODAL_HINT_MARK_Y_OFFSET = -2;
    constexpr int QR_MODAL_W = 360;
    constexpr int QR_MODAL_H = 370;
    constexpr int QR_MODAL_X = (WorkshopCfg::SCREEN_W - QR_MODAL_W) / 2;
    constexpr int QR_MODAL_Y = (WorkshopCfg::SCREEN_H - QR_MODAL_H) / 2;
    constexpr int QR_MODAL_PAD = 18;
    constexpr int QR_MODAL_SEPARATOR_Y = QR_MODAL_Y + 50;
    constexpr int QR_MODAL_SEPARATOR_H = 2;
    constexpr int QR_MODAL_QR_GAP = 10;
    constexpr int QR_MODAL_QR_TOP_ADJUST = 4;
    constexpr int QR_MODAL_QR_SIZE = 226;
    constexpr int QR_MODAL_QR_X = QR_MODAL_X + (QR_MODAL_W - QR_MODAL_QR_SIZE) / 2;
    constexpr int QR_MODAL_QR_Y = QR_MODAL_SEPARATOR_Y + QR_MODAL_SEPARATOR_H + QR_MODAL_QR_GAP + QR_MODAL_QR_TOP_ADJUST;
    constexpr int QR_MODAL_HINT_Y = QR_MODAL_Y + 302;
    constexpr int QR_MODAL_RADIUS = 4;
    constexpr size_t QR_MODAL_TEXT_BUF_SIZE = 192;

    /**
     * @brief Prüft, ob ein PROGMEM-Zeiger auf eine leere Zeichenfolge zeigt.
     *
     * Direktes `pgm[0] != '\0'` wäre unsicher; ESP32-PROGMEM benötigt `pgm_read_byte`. Helfer für die Prüfung auf einen leeren Untertitel vor der Umstellung auf lokalisierte Stack-Zeichenfolgen über `subtitle[0] != '\0'` lief.
     */
    inline bool pgmIsEmpty(const __FlashStringHelper *fs)
    {
        if (!fs)
            return true;
        return pgm_read_byte(reinterpret_cast<PGM_P>(fs)) == 0;
    }

    /**
     * @brief Begrenzt einen Ganzzahlwert auf den angegebenen Bereich.
     */
    inline int clampi(int v, int lo, int hi)
    {
        if (v < lo)
            return lo;

        if (v > hi)
            return hi;

        return v;
    }
} // namespace

// ===================================================================
// STATISCHE DATENBASIS
// ===================================================================

const WorkshopCategoryDef Workshop::s_categories[] = {
    {WorkshopCategory::MISC,        WorkshopKey::WORKSHOP_CAT_MISC_TITLE,        WorkshopKey::WORKSHOP_CAT_MISC_SUMMARY},
    {WorkshopCategory::GAMES,       WorkshopKey::WORKSHOP_CAT_GAMES_TITLE,       WorkshopKey::WORKSHOP_CAT_GAMES_SUMMARY},
    {WorkshopCategory::ELECTRONICS, WorkshopKey::WORKSHOP_CAT_ELECTRONICS_TITLE, WorkshopKey::WORKSHOP_CAT_ELECTRONICS_SUMMARY},
    {WorkshopCategory::UNITS,       WorkshopKey::WORKSHOP_CAT_UNITS_TITLE,       WorkshopKey::WORKSHOP_CAT_UNITS_SUMMARY},
    {WorkshopCategory::DIGITAL,     WorkshopKey::WORKSHOP_CAT_DIGITAL_TITLE,     WorkshopKey::WORKSHOP_CAT_DIGITAL_SUMMARY},
    {WorkshopCategory::KNITTING,    WorkshopKey::WORKSHOP_CAT_KNITTING_TITLE,    WorkshopKey::WORKSHOP_CAT_KNITTING_SUMMARY},
    {WorkshopCategory::MUSIC,       WorkshopKey::WORKSHOP_CAT_MUSIC_TITLE,       WorkshopKey::WORKSHOP_CAT_MUSIC_SUMMARY},
};

const WorkshopPageDef Workshop::s_pages[] = {
    {WorkshopPageId::NONE, WorkshopCategory::MISC, WorkshopPageType::CALCULATOR, WorkshopKey::WORKSHOP_EMPTY, WorkshopKey::WORKSHOP_EMPTY},

    // Werkzeuge
    {WorkshopPageId::CALCULATOR,      WorkshopCategory::MISC, WorkshopPageType::CALCULATOR, WorkshopKey::WORKSHOP_TOOL_CALCULATOR_TITLE,      WorkshopKey::WORKSHOP_TOOL_CALCULATOR_SUBTITLE},
    {WorkshopPageId::WORLD_CLOCK,     WorkshopCategory::MISC, WorkshopPageType::REFERENCE,  WorkshopKey::WORKSHOP_TOOL_WORLD_CLOCK_TITLE,     WorkshopKey::WORKSHOP_TOOL_WORLD_CLOCK_SUBTITLE},
    {WorkshopPageId::FLIGHT_RADAR,    WorkshopCategory::MISC, WorkshopPageType::REFERENCE,  WorkshopKey::WORKSHOP_FLIGHT_RADAR_TITLE,         WorkshopKey::WORKSHOP_FLIGHT_RADAR_SUBTITLE},
    {WorkshopPageId::COLOR_CONVERTER, WorkshopCategory::MISC, WorkshopPageType::CALCULATOR, WorkshopKey::WORKSHOP_TOOL_COLOR_CONVERTER_TITLE, WorkshopKey::WORKSHOP_TOOL_COLOR_CONVERTER_SUBTITLE},
    {WorkshopPageId::PERIODIC_TABLE,  WorkshopCategory::MISC, WorkshopPageType::REFERENCE,  WorkshopKey::WORKSHOP_TOOL_PERIODIC_TABLE_TITLE,  WorkshopKey::WORKSHOP_TOOL_PERIODIC_TABLE_SUBTITLE},
    {WorkshopPageId::SCREW_REFERENCE, WorkshopCategory::MISC, WorkshopPageType::REFERENCE,  WorkshopKey::WORKSHOP_TOOL_SCREW_REFERENCE_TITLE, WorkshopKey::WORKSHOP_TOOL_SCREW_REFERENCE_SUBTITLE},
    {WorkshopPageId::BMI_CALCULATOR,  WorkshopCategory::MISC, WorkshopPageType::CALCULATOR, WorkshopKey::WORKSHOP_TOOL_BMI_CALCULATOR_TITLE,  WorkshopKey::WORKSHOP_TOOL_BMI_CALCULATOR_SUBTITLE},
    {WorkshopPageId::GAME_2048,       WorkshopCategory::GAMES, WorkshopPageType::REFERENCE, WorkshopKey::WORKSHOP_TOOL_GAME_2048_TITLE,       WorkshopKey::WORKSHOP_TOOL_GAME_2048_SUBTITLE},
    {WorkshopPageId::MILL_GAME,       WorkshopCategory::GAMES, WorkshopPageType::REFERENCE, WorkshopKey::WORKSHOP_TOOL_MILL_GAME_TITLE,       WorkshopKey::WORKSHOP_TOOL_MILL_GAME_SUBTITLE},
    {WorkshopPageId::LIGHTS_OUT,      WorkshopCategory::GAMES, WorkshopPageType::REFERENCE, WorkshopKey::WORKSHOP_TOOL_LIGHTS_OUT_TITLE,      WorkshopKey::WORKSHOP_TOOL_LIGHTS_OUT_SUBTITLE},
    {WorkshopPageId::DICE,            WorkshopCategory::GAMES, WorkshopPageType::REFERENCE, WorkshopKey::WORKSHOP_TOOL_DICE_TITLE,            WorkshopKey::WORKSHOP_TOOL_DICE_SUBTITLE},
    {WorkshopPageId::SHUT_THE_BOX,    WorkshopCategory::GAMES, WorkshopPageType::REFERENCE, WorkshopKey::WORKSHOP_SHUT_THE_BOX_TITLE,         WorkshopKey::WORKSHOP_SHUT_THE_BOX_SUBTITLE},
    {WorkshopPageId::LOTTO_NORMAL,    WorkshopCategory::GAMES, WorkshopPageType::REFERENCE, WorkshopKey::WORKSHOP_LOTTO_NORMAL_PAGE_TITLE,    WorkshopKey::WORKSHOP_LOTTO_NORMAL_SUBTITLE},
    {WorkshopPageId::LOTTO_SYSTEM,    WorkshopCategory::GAMES, WorkshopPageType::REFERENCE, WorkshopKey::WORKSHOP_LOTTO_PAGE_TITLE,           WorkshopKey::WORKSHOP_LOTTO_SUBTITLE},

    // Elektronik
    {WorkshopPageId::RESISTOR_COLOR_CODE,     WorkshopCategory::ELECTRONICS, WorkshopPageType::REFERENCE,  WorkshopKey::WORKSHOP_TOOL_RESISTOR_TITLE,        WorkshopKey::WORKSHOP_TOOL_RESISTOR_SUBTITLE},
    {WorkshopPageId::SMD_RESISTOR_CODE,       WorkshopCategory::ELECTRONICS, WorkshopPageType::REFERENCE,  WorkshopKey::WORKSHOP_TOOL_SMD_RESISTOR_TITLE,    WorkshopKey::WORKSHOP_TOOL_SMD_RESISTOR_SUBTITLE},
    {WorkshopPageId::OHM_CALCULATOR,          WorkshopCategory::ELECTRONICS, WorkshopPageType::CALCULATOR, WorkshopKey::WORKSHOP_TOOL_OHM_CALCULATOR_TITLE,  WorkshopKey::WORKSHOP_TOOL_OHM_CALCULATOR_SUBTITLE},
    {WorkshopPageId::VOLTAGE_DIVIDER,         WorkshopCategory::ELECTRONICS, WorkshopPageType::CALCULATOR, WorkshopKey::WORKSHOP_TOOL_VOLTAGE_DIVIDER_TITLE, WorkshopKey::WORKSHOP_TOOL_VOLTAGE_DIVIDER_SUBTITLE},
    {WorkshopPageId::LED_RESISTOR_CALCULATOR, WorkshopCategory::ELECTRONICS, WorkshopPageType::CALCULATOR, WorkshopKey::WORKSHOP_TOOL_LED_CALCULATOR_TITLE,  WorkshopKey::WORKSHOP_TOOL_LED_CALCULATOR_SUBTITLE},
    {WorkshopPageId::RC_RL_FILTER_CALCULATOR, WorkshopCategory::ELECTRONICS, WorkshopPageType::CALCULATOR, WorkshopKey::WORKSHOP_TOOL_RC_FILTER_TITLE,       WorkshopKey::WORKSHOP_TOOL_RC_FILTER_SUBTITLE},
    {WorkshopPageId::LC_FILTER_CALCULATOR,    WorkshopCategory::ELECTRONICS, WorkshopPageType::CALCULATOR, WorkshopKey::WORKSHOP_TOOL_LC_FILTER_TITLE,       WorkshopKey::WORKSHOP_TOOL_LC_FILTER_SUBTITLE},
    {WorkshopPageId::UKW_RESONATOR,           WorkshopCategory::ELECTRONICS, WorkshopPageType::CALCULATOR, WorkshopKey::WORKSHOP_TOOL_UKW_RESONATOR_TITLE,   WorkshopKey::WORKSHOP_TOOL_UKW_RESONATOR_SUBTITLE},
    {WorkshopPageId::LM317_CALCULATOR,        WorkshopCategory::ELECTRONICS, WorkshopPageType::CALCULATOR, WorkshopKey::WORKSHOP_TOOL_LM317_TITLE,           WorkshopKey::WORKSHOP_TOOL_LM317_SUBTITLE},
    {WorkshopPageId::DECIBEL_CALCULATOR,      WorkshopCategory::ELECTRONICS, WorkshopPageType::CALCULATOR, WorkshopKey::WORKSHOP_TOOL_DECIBEL_TITLE,         WorkshopKey::WORKSHOP_TOOL_DECIBEL_SUBTITLE},
    {WorkshopPageId::NE555_CALCULATOR,        WorkshopCategory::ELECTRONICS, WorkshopPageType::CALCULATOR, WorkshopKey::WORKSHOP_TOOL_NE555_TITLE,           WorkshopKey::WORKSHOP_TOOL_NE555_SUBTITLE},
    {WorkshopPageId::OPAMP_CALCULATOR,        WorkshopCategory::ELECTRONICS, WorkshopPageType::CALCULATOR, WorkshopKey::WORKSHOP_TOOL_OPAMP_TITLE,           WorkshopKey::WORKSHOP_TOOL_OPAMP_SUBTITLE},

    // Einheiten
    {WorkshopPageId::INCH_MM_CALCULATOR,  WorkshopCategory::UNITS, WorkshopPageType::CALCULATOR, WorkshopKey::WORKSHOP_TOOL_INCH_MM_TITLE,    WorkshopKey::WORKSHOP_TOOL_INCH_MM_SUBTITLE},
    {WorkshopPageId::TEMP_CALCULATOR,     WorkshopCategory::UNITS, WorkshopPageType::CALCULATOR, WorkshopKey::WORKSHOP_TOOL_TEMP_TITLE,       WorkshopKey::WORKSHOP_TOOL_TEMP_SUBTITLE},
    {WorkshopPageId::PRESSURE_CALCULATOR, WorkshopCategory::UNITS, WorkshopPageType::CALCULATOR, WorkshopKey::WORKSHOP_TOOL_PRESSURE_TITLE,   WorkshopKey::WORKSHOP_TOOL_PRESSURE_SUBTITLE},
    {WorkshopPageId::SI_PREFIXES,         WorkshopCategory::UNITS, WorkshopPageType::REFERENCE,  WorkshopKey::WORKSHOP_TOOL_SI_PREFIXES_TITLE, WorkshopKey::WORKSHOP_TOOL_SI_PREFIXES_SUBTITLE},

    // Digital
    {WorkshopPageId::ASCII_TABLE,    WorkshopCategory::DIGITAL, WorkshopPageType::REFERENCE,  WorkshopKey::WORKSHOP_TOOL_ASCII_TABLE_TITLE, WorkshopKey::WORKSHOP_TOOL_ASCII_TABLE_SUBTITLE},
    {WorkshopPageId::BIT_EDITOR,     WorkshopCategory::DIGITAL, WorkshopPageType::CALCULATOR, WorkshopKey::WORKSHOP_TOOL_BIT_EDITOR_TITLE,  WorkshopKey::WORKSHOP_TOOL_BIT_EDITOR_SUBTITLE},
    {WorkshopPageId::LOGIC_GATE_SIM, WorkshopCategory::DIGITAL, WorkshopPageType::REFERENCE,  WorkshopKey::WORKSHOP_TOOL_LOGIC_GATE_TITLE,  WorkshopKey::WORKSHOP_TOOL_LOGIC_GATE_SUBTITLE},

    // Stricken
    {WorkshopPageId::STITCH_GAUGE,       WorkshopCategory::KNITTING, WorkshopPageType::CALCULATOR, WorkshopKey::WORKSHOP_TOOL_STITCH_GAUGE_TITLE,       WorkshopKey::WORKSHOP_TOOL_STITCH_GAUGE_SUBTITLE},
    {WorkshopPageId::STITCH_DISTRIBUTOR, WorkshopCategory::KNITTING, WorkshopPageType::CALCULATOR, WorkshopKey::WORKSHOP_TOOL_STITCH_DISTRIBUTOR_TITLE, WorkshopKey::WORKSHOP_TOOL_STITCH_DISTRIBUTOR_SUBTITLE},
    {WorkshopPageId::SLEEVE_TAPER,       WorkshopCategory::KNITTING, WorkshopPageType::CALCULATOR, WorkshopKey::WORKSHOP_TOOL_SLEEVE_TAPER_TITLE,       WorkshopKey::WORKSHOP_TOOL_SLEEVE_TAPER_SUBTITLE},

    // Musik
    {WorkshopPageId::CIRCLE_OF_FIFTHS, WorkshopCategory::MUSIC, WorkshopPageType::REFERENCE,  WorkshopKey::WORKSHOP_TOOL_CIRCLE_OF_FIFTHS_TITLE, WorkshopKey::WORKSHOP_TOOL_CIRCLE_OF_FIFTHS_SUBTITLE},
    {WorkshopPageId::METRONOME,        WorkshopCategory::MUSIC, WorkshopPageType::CALCULATOR, WorkshopKey::WORKSHOP_TOOL_METRONOME_TITLE,        WorkshopKey::WORKSHOP_TOOL_METRONOME_SUBTITLE},
};

const WorkshopListEntry Workshop::s_flatList[] = {
    // Werkzeuge
    {WorkshopEntryType::SECTION_HEADER, WorkshopCategory::MISC, WorkshopPageId::NONE,            WorkshopKey::WORKSHOP_CAT_MISC_TITLE},
    {WorkshopEntryType::TOOL_ITEM,      WorkshopCategory::MISC, WorkshopPageId::CALCULATOR,      WorkshopKey::WORKSHOP_TOOL_CALCULATOR_TITLE},
    {WorkshopEntryType::TOOL_ITEM,      WorkshopCategory::MISC, WorkshopPageId::WORLD_CLOCK,     WorkshopKey::WORKSHOP_TOOL_WORLD_CLOCK_TITLE},
    {WorkshopEntryType::TOOL_ITEM,      WorkshopCategory::MISC, WorkshopPageId::FLIGHT_RADAR,    WorkshopKey::WORKSHOP_FLIGHT_RADAR_TITLE},
    {WorkshopEntryType::TOOL_ITEM,      WorkshopCategory::MISC, WorkshopPageId::COLOR_CONVERTER, WorkshopKey::WORKSHOP_TOOL_COLOR_CONVERTER_TITLE},
    {WorkshopEntryType::TOOL_ITEM,      WorkshopCategory::MISC, WorkshopPageId::PERIODIC_TABLE,  WorkshopKey::WORKSHOP_TOOL_PERIODIC_TABLE_TITLE},
    {WorkshopEntryType::TOOL_ITEM,      WorkshopCategory::MISC, WorkshopPageId::SCREW_REFERENCE, WorkshopKey::WORKSHOP_TOOL_SCREW_REFERENCE_LABEL},
    {WorkshopEntryType::TOOL_ITEM,      WorkshopCategory::MISC, WorkshopPageId::BMI_CALCULATOR,  WorkshopKey::WORKSHOP_TOOL_BMI_CALCULATOR_TITLE},

    // Spiele
    {WorkshopEntryType::SECTION_HEADER, WorkshopCategory::GAMES, WorkshopPageId::NONE,       WorkshopKey::WORKSHOP_CAT_GAMES_TITLE},
    {WorkshopEntryType::TOOL_ITEM,      WorkshopCategory::GAMES, WorkshopPageId::GAME_2048,  WorkshopKey::WORKSHOP_TOOL_GAME_2048_TITLE},
    {WorkshopEntryType::TOOL_ITEM,      WorkshopCategory::GAMES, WorkshopPageId::MILL_GAME,  WorkshopKey::WORKSHOP_TOOL_MILL_GAME_TITLE},
    {WorkshopEntryType::TOOL_ITEM,      WorkshopCategory::GAMES, WorkshopPageId::LIGHTS_OUT, WorkshopKey::WORKSHOP_TOOL_LIGHTS_OUT_TITLE},
    {WorkshopEntryType::TOOL_ITEM,      WorkshopCategory::GAMES, WorkshopPageId::DICE,       WorkshopKey::WORKSHOP_TOOL_DICE_TITLE},
    {WorkshopEntryType::TOOL_ITEM,      WorkshopCategory::GAMES, WorkshopPageId::SHUT_THE_BOX, WorkshopKey::WORKSHOP_SHUT_THE_BOX_TITLE},
    {WorkshopEntryType::TOOL_ITEM,      WorkshopCategory::GAMES, WorkshopPageId::LOTTO_NORMAL, WorkshopKey::WORKSHOP_LOTTO_NORMAL_TITLE},
    {WorkshopEntryType::TOOL_ITEM,      WorkshopCategory::GAMES, WorkshopPageId::LOTTO_SYSTEM, WorkshopKey::WORKSHOP_LOTTO_TITLE},

    // Elektronik
    {WorkshopEntryType::SECTION_HEADER, WorkshopCategory::ELECTRONICS, WorkshopPageId::NONE,                    WorkshopKey::WORKSHOP_CAT_ELECTRONICS_TITLE},
    {WorkshopEntryType::TOOL_ITEM,      WorkshopCategory::ELECTRONICS, WorkshopPageId::RESISTOR_COLOR_CODE,     WorkshopKey::WORKSHOP_TOOL_RESISTOR_TITLE},
    {WorkshopEntryType::TOOL_ITEM,      WorkshopCategory::ELECTRONICS, WorkshopPageId::SMD_RESISTOR_CODE,       WorkshopKey::WORKSHOP_TOOL_SMD_RESISTOR_TITLE},
    {WorkshopEntryType::TOOL_ITEM,      WorkshopCategory::ELECTRONICS, WorkshopPageId::OHM_CALCULATOR,          WorkshopKey::WORKSHOP_TOOL_OHM_CALCULATOR_TITLE},
    {WorkshopEntryType::TOOL_ITEM,      WorkshopCategory::ELECTRONICS, WorkshopPageId::VOLTAGE_DIVIDER,         WorkshopKey::WORKSHOP_TOOL_VOLTAGE_DIVIDER_TITLE},
    {WorkshopEntryType::TOOL_ITEM,      WorkshopCategory::ELECTRONICS, WorkshopPageId::LED_RESISTOR_CALCULATOR, WorkshopKey::WORKSHOP_TOOL_LED_CALCULATOR_TITLE},
    {WorkshopEntryType::TOOL_ITEM,      WorkshopCategory::ELECTRONICS, WorkshopPageId::RC_RL_FILTER_CALCULATOR, WorkshopKey::WORKSHOP_TOOL_RC_FILTER_TITLE},
    {WorkshopEntryType::TOOL_ITEM,      WorkshopCategory::ELECTRONICS, WorkshopPageId::LC_FILTER_CALCULATOR,    WorkshopKey::WORKSHOP_TOOL_LC_FILTER_TITLE},
    {WorkshopEntryType::TOOL_ITEM,      WorkshopCategory::ELECTRONICS, WorkshopPageId::UKW_RESONATOR,           WorkshopKey::WORKSHOP_TOOL_UKW_RESONATOR_TITLE},
    {WorkshopEntryType::TOOL_ITEM,      WorkshopCategory::ELECTRONICS, WorkshopPageId::LM317_CALCULATOR,        WorkshopKey::WORKSHOP_TOOL_LM317_TITLE},
    {WorkshopEntryType::TOOL_ITEM,      WorkshopCategory::ELECTRONICS, WorkshopPageId::DECIBEL_CALCULATOR,      WorkshopKey::WORKSHOP_TOOL_DECIBEL_TITLE},
    {WorkshopEntryType::TOOL_ITEM,      WorkshopCategory::ELECTRONICS, WorkshopPageId::NE555_CALCULATOR,        WorkshopKey::WORKSHOP_TOOL_NE555_TITLE},
    {WorkshopEntryType::TOOL_ITEM,      WorkshopCategory::ELECTRONICS, WorkshopPageId::OPAMP_CALCULATOR,        WorkshopKey::WORKSHOP_TOOL_OPAMP_TITLE},
    // Einheiten
    {WorkshopEntryType::SECTION_HEADER, WorkshopCategory::UNITS, WorkshopPageId::NONE,                WorkshopKey::WORKSHOP_CAT_UNITS_TITLE},
    {WorkshopEntryType::TOOL_ITEM,      WorkshopCategory::UNITS, WorkshopPageId::INCH_MM_CALCULATOR,  WorkshopKey::WORKSHOP_TOOL_INCH_MM_TITLE},
    {WorkshopEntryType::TOOL_ITEM,      WorkshopCategory::UNITS, WorkshopPageId::TEMP_CALCULATOR,     WorkshopKey::WORKSHOP_TOOL_TEMP_TITLE},
    {WorkshopEntryType::TOOL_ITEM,      WorkshopCategory::UNITS, WorkshopPageId::PRESSURE_CALCULATOR, WorkshopKey::WORKSHOP_TOOL_PRESSURE_TITLE},
    {WorkshopEntryType::TOOL_ITEM,      WorkshopCategory::UNITS, WorkshopPageId::SI_PREFIXES,         WorkshopKey::WORKSHOP_TOOL_SI_PREFIXES_TITLE},
    // Digital
    {WorkshopEntryType::SECTION_HEADER, WorkshopCategory::DIGITAL, WorkshopPageId::NONE,           WorkshopKey::WORKSHOP_CAT_DIGITAL_TITLE},
    {WorkshopEntryType::TOOL_ITEM,      WorkshopCategory::DIGITAL, WorkshopPageId::ASCII_TABLE,    WorkshopKey::WORKSHOP_TOOL_ASCII_TABLE_TITLE},
    {WorkshopEntryType::TOOL_ITEM,      WorkshopCategory::DIGITAL, WorkshopPageId::BIT_EDITOR,     WorkshopKey::WORKSHOP_TOOL_BIT_EDITOR_TITLE},
    {WorkshopEntryType::TOOL_ITEM,      WorkshopCategory::DIGITAL, WorkshopPageId::LOGIC_GATE_SIM, WorkshopKey::WORKSHOP_TOOL_LOGIC_GATE_TITLE},
    // Stricken
    {WorkshopEntryType::SECTION_HEADER, WorkshopCategory::KNITTING, WorkshopPageId::NONE,               WorkshopKey::WORKSHOP_CAT_KNITTING_TITLE},
    {WorkshopEntryType::TOOL_ITEM,      WorkshopCategory::KNITTING, WorkshopPageId::STITCH_GAUGE,       WorkshopKey::WORKSHOP_TOOL_STITCH_GAUGE_TITLE},
    {WorkshopEntryType::TOOL_ITEM,      WorkshopCategory::KNITTING, WorkshopPageId::STITCH_DISTRIBUTOR, WorkshopKey::WORKSHOP_TOOL_STITCH_DISTRIBUTOR_TITLE},
    {WorkshopEntryType::TOOL_ITEM,      WorkshopCategory::KNITTING, WorkshopPageId::SLEEVE_TAPER,       WorkshopKey::WORKSHOP_TOOL_SLEEVE_TAPER_TITLE},
    // Musik
    {WorkshopEntryType::SECTION_HEADER, WorkshopCategory::MUSIC, WorkshopPageId::NONE,             WorkshopKey::WORKSHOP_CAT_MUSIC_TITLE},
    {WorkshopEntryType::TOOL_ITEM,      WorkshopCategory::MUSIC, WorkshopPageId::CIRCLE_OF_FIFTHS, WorkshopKey::WORKSHOP_TOOL_CIRCLE_OF_FIFTHS_TITLE},
    {WorkshopEntryType::TOOL_ITEM,      WorkshopCategory::MUSIC, WorkshopPageId::METRONOME,        WorkshopKey::WORKSHOP_TOOL_METRONOME_TITLE},
};

const WorkshopWebLinkDef Workshop::s_webLinks[] = {
    {WorkshopPageId::CALCULATOR, ""},
    {WorkshopPageId::WORLD_CLOCK, ""},
    {WorkshopPageId::FLIGHT_RADAR, "https://opensky-network.org/"},
    {WorkshopPageId::COLOR_CONVERTER, ""},
    {WorkshopPageId::PERIODIC_TABLE, ""},
    {WorkshopPageId::SCREW_REFERENCE, ""},
    {WorkshopPageId::BMI_CALCULATOR, ""},
    {WorkshopPageId::GAME_2048, ""},
    {WorkshopPageId::MILL_GAME, ""},
    {WorkshopPageId::LIGHTS_OUT, ""},
    {WorkshopPageId::DICE, ""},
    {WorkshopPageId::SHUT_THE_BOX, ""},
    {WorkshopPageId::LOTTO_NORMAL, ""},
    {WorkshopPageId::LOTTO_SYSTEM, ""},
    {WorkshopPageId::RESISTOR_COLOR_CODE, "https://www.digikey.de/de/resources/online-conversion-calculators"},
    {WorkshopPageId::SMD_RESISTOR_CODE, "https://www.digikey.de/de/resources/online-conversion-calculators"},
    {WorkshopPageId::OHM_CALCULATOR, "https://www.digikey.de/de/resources/online-conversion-calculators"},
    {WorkshopPageId::VOLTAGE_DIVIDER, "https://www.digikey.de/de/resources/online-conversion-calculators"},
    {WorkshopPageId::LED_RESISTOR_CALCULATOR, "https://www.digikey.de/de/resources/online-conversion-calculators"},
    {WorkshopPageId::RC_RL_FILTER_CALCULATOR, "https://www.digikey.de/de/resources/online-conversion-calculators"},
    {WorkshopPageId::LC_FILTER_CALCULATOR, "https://www.digikey.de/de/resources/online-conversion-calculators"},
    {WorkshopPageId::UKW_RESONATOR, "https://www.digikey.de/de/resources/online-conversion-calculators"},
    {WorkshopPageId::LM317_CALCULATOR, "https://www.digikey.de/de/resources/online-conversion-calculators"},
    {WorkshopPageId::DECIBEL_CALCULATOR, "https://www.digikey.de/de/resources/online-conversion-calculators"},
    {WorkshopPageId::NE555_CALCULATOR, "https://www.digikey.de/de/resources/online-conversion-calculators"},
    {WorkshopPageId::OPAMP_CALCULATOR, "https://www.digikey.de/de/resources/online-conversion-calculators"},
    {WorkshopPageId::INCH_MM_CALCULATOR, ""},
    {WorkshopPageId::TEMP_CALCULATOR, ""},
    {WorkshopPageId::PRESSURE_CALCULATOR, ""},
    {WorkshopPageId::SI_PREFIXES, ""},
    {WorkshopPageId::ASCII_TABLE, ""},
    {WorkshopPageId::BIT_EDITOR, ""},
    {WorkshopPageId::LOGIC_GATE_SIM, ""},
    {WorkshopPageId::STITCH_GAUGE, ""},
    {WorkshopPageId::STITCH_DISTRIBUTOR, ""},
    {WorkshopPageId::SLEEVE_TAPER, ""},
    {WorkshopPageId::CIRCLE_OF_FIFTHS, ""},
    {WorkshopPageId::METRONOME, ""},
};

const int Workshop::FLAT_LIST_COUNT = static_cast<int>(sizeof(Workshop::s_flatList) / sizeof(Workshop::s_flatList[0]));
const int Workshop::WEB_LINK_COUNT = static_cast<int>(sizeof(Workshop::s_webLinks) / sizeof(Workshop::s_webLinks[0]));

// ===================================================================
// KONSTRUKTOR / DESTRUKTOR
// ===================================================================

/**
 * @brief Konstruktor
 */
Workshop::Workshop()
{
}

/**
 * @brief Destruktor
 */
Workshop::~Workshop()
{
    // Aktives Werkzeug geordnet beenden
    if (m_activeTool)
    {
        m_activeTool->onExit();
        delete m_activeTool;
        m_activeTool = nullptr;
    }

    if (m_qrModalBackup)
    {
        free(m_qrModalBackup);
        m_qrModalBackup = nullptr;
        m_qrModalBackupBytes = 0;
    }

    deleteSprites();
}

// ===================================================================
// INITIALISIERUNG
// ===================================================================

/**
 * @brief Initialisiert das Workshop-Modul.
 */
bool Workshop::init(LGFX &tft, const WorkshopCallbacks &callbacks)
{
    m_tft = &tft;
    m_callbacks = callbacks;

    if (!createSprites())
    {
        LOG_ERROR("Workshop: Sprite-Erzeugung fehlgeschlagen");
        return false;
    }

    initButtons();

    // Startzustand: erster TOOL_ITEM = Rechner (Index 1 in s_flatList)
    m_selectedFlatIdx = 1;
    m_scroll.reset();
    m_tap.reset();

    m_tft->clear(WorkshopCfg::C_BG);
    m_layoutDrawn = false;
    invalidateAll();

    m_touchWasActive = false;
    m_touchStartedInMenu = false;
    m_toolTouchActive = false;
    m_footerToolTouchActive = false;
    m_headerQrTouchActive = false;
    m_headerQrPressed = false;
    m_qrModalVisible = false;
    m_qrModalTouchActive = false;
    m_qrModalTouchOutside = false;
    m_lastDrawnSecond = -1;
    m_lastScrollOffset = -1;
    m_lastSubtitleText[0] = '\0';

    // Initiales Werkzeug für den Starteintrag erstellen
    m_activeTool = createTool(s_flatList[m_selectedFlatIdx].pageId);
    if (m_activeTool)
    {
        m_activeTool->setOwner(this);
        m_activeTool->onEnter(m_tft);
    }

    m_initialized = true;
    LOG_SUCCESS("Workshop-Modul erfolgreich initialisiert");
    return true;
}

/**
 * @brief Erstellt alle PSRAM-Sprites.
 */
bool Workshop::createSprites()
{
    if (!m_tft)
        return false;

    if (!m_tft->safeCreateSprite(m_menuSprite, WorkshopCfg::MENU_W, WorkshopCfg::MENU_SPRITE_H))
    {
        deleteSprites();
        return false;
    }

    if (!m_tft->safeCreateSprite(m_scrollbarSprite, WorkshopCfg::SCROLLBAR_W, WorkshopCfg::SCREEN_H))
    {
        deleteSprites();
        return false;
    }

    if (!m_tft->safeCreateSprite(m_workSprite, WorkshopCfg::WORK_W, WorkshopCfg::WORK_H))
    {
        deleteSprites();
        return false;
    }

    if (!m_tft->safeCreateSprite(m_clockSprite, WorkshopCfg::CLOCK_W, WorkshopCfg::CLOCK_H))
    {
        deleteSprites();
        return false;
    }

    m_menuSprite->clear(WorkshopCfg::C_BG);
    m_scrollbarSprite->clear(WorkshopCfg::C_SCROLLBAR_TRACK);
    m_workSprite->clear(WorkshopCfg::C_BG);
    m_clockSprite->clear(WorkshopCfg::C_BG);
    return true;
}

/**
 * @brief Gibt alle Sprites frei und setzt Zeiger auf nullptr.
 */
void Workshop::deleteSprites()
{
    if (!m_tft)
        return;

    m_tft->safeDeleteSprite(m_menuSprite);
    m_tft->safeDeleteSprite(m_scrollbarSprite);
    m_tft->safeDeleteSprite(m_workSprite);
    m_tft->safeDeleteSprite(m_clockSprite);
}

/**
 * @brief Initialisiert Footer-Buttons.
 */
void Workshop::initButtons()
{
    m_exitButton.initButton(
        m_tft,                      // Display-Object
        WorkshopCfg::EXIT_BTN_CX,   // X-Mitte des Buttons
        WorkshopCfg::EXIT_BTN_CY,   // Y-Mitte des Buttons
        WorkshopCfg::EXIT_BTN_SIZE, // Breite des Buttons
        WorkshopCfg::EXIT_BTN_SIZE, // Höhe des Buttons
        4,                          // Eckradius
        BTN_BORDER,                 // Rahmenfarbe
        BTN_FILL,                   // Füllfarbe
        uint16_t(TFT_DARKGREY),     // Textfarbe
        "^POWER:32:D206^",          // Beschriftung (Power-Symbol, Größe 32)
        nullptr);                   // Kein Icon

    m_exitButton.setFont(ComfortaaBold30);
}

// ===================================================================
// MODULTYP / DIRTY-FLAGS
// ===================================================================

/**
 * @brief Gibt den Modultyp zurück.
 */
AppModule Workshop::type() const
{
    return AppModule::WORKSHOP;
}

/**
 * @brief Menüliste zum Neuzeichnen markieren.
 */
void Workshop::invalidateMenu()
{
    m_menuDirty = true;
}

/**
 * @brief Arbeitsbereich zum Neuzeichnen markieren.
 */
void Workshop::invalidateWork()
{
    m_workFrameDirty = true;
    m_workContentDirty = true;
}

/**
 * @brief Nur den Inhaltsbereich des Arbeitsbereichs zum Neuzeichnen markieren.
 */
void Workshop::invalidateWorkContent()
{
    m_workContentDirty = true;
}

/**
 * @brief Pausiert oder reaktiviert das automatische Neuzeichnen des Werkzeug-Inhalts.
 */
void Workshop::setWorkContentPaused(bool paused)
{
    if (m_workContentPaused == paused)
        return;

    m_workContentPaused = paused;
    if (!paused)
        m_workContentDirty = true; // Beim Aufheben der Pause vollen Redraw nachholen
}

/**
 * @brief Fordert einen vollständigen Redraw des Arbeitsbereichs an.
 */
void Workshop::requestWorkRedraw()
{
    invalidateWork();
}

/**
 * @brief Fordert einen Redraw der Fußzeile an.
 */
void Workshop::requestFooterRedraw()
{
    m_footerDirty = true;
}

/**
 * @brief Alle Bereiche zum Neuzeichnen markieren.
 */
void Workshop::invalidateAll()
{
    m_menuDirty = true;
    invalidateWork();
    m_footerDirty = true;
}

// ===================================================================
// HAUPTLOOP
// ===================================================================

/**
 * @brief Hauptzeichenfunktion, wird zyklisch aufgerufen.
 */
void Workshop::draw()
{
    if (!m_initialized || !m_tft)
        return;

    // GFX-Sperre: kein Touch-Polling, kein Zeichnen. Laufender Drag wird sauber beendet und die Touch-Tracking-Flags werden auf "kein Touch aktiv" gesetzt, damit nach dem Unblock kein hängender Release oder Geister-Drag bleibt. Dirty-Flags und Last-Werte bleiben unangetastet, damit die Change-Detection den Nachholbedarf im ersten unblockierten Frame korrekt erkennt.
    if (APP_STATE.getBlockGFX())
    {
        if (m_scroll.isDragging)
            handleScrollTouch(0, 0, false);

        m_touchWasActive = false;
        m_touchStartedInMenu = false;
        m_toolTouchActive = false;
        m_footerToolTouchActive = false;
        return;
    }

    int32_t tx = 0;
    int32_t ty = 0;
    const bool touched = m_tft->getTouch(&tx, &ty);

    if (m_qrModalVisible)
    {
        handleQrModalTouch(tx, ty, touched);
        m_touchWasActive = touched;
        drawDigitalClock(WorkshopCfg::CLOCK_PUSH_X, WorkshopCfg::CLOCK_PUSH_Y);
        return;
    }

    // ===================================================================
    // Touch-Handling
    // ===================================================================

    // Modaler Sperrmodus: Wenn ein aktives Werkzeug einen Dialog geöffnet hat (m_workContentPaused == true), werden Menü, Exit-Button, Scrolling, Scrollbar und der Werkzeug-Footer komplett stillgelegt. Touch-Events gehen ausschließlich an das Werkzeug weiter, damit dieses seine MessageBox pflegen kann. Die Box fängt ihre eigenen Buttons direkt vom Display ab und signalisiert das Ende des modalen Zustands über ihren Callback.
    if (m_workContentPaused)
    {
        if (m_activeTool)
        {
            if (touched)
            {
                m_toolTouchActive = true;
                m_activeTool->handleTouch(tx, ty, true);
            }
            else
            {
                // Immer an das Werkzeug durchreichen - die MessageBox braucht den regelmäßigen Tick auch ohne aktiven Touch, um Timeouts und Button-Release zu erkennen.
                if (m_toolTouchActive)
                    m_toolTouchActive = false;

                m_activeTool->handleTouch(0, 0, false);
            }
        }

        // Laufendes Menü-Dragging sauber beenden, falls beim Öffnen der Box ein Drag aktiv war.
        if (!touched && m_scroll.isDragging)
            handleScrollTouch(0, 0, false);

        m_touchStartedInMenu = false;
        m_touchWasActive = touched;
    }
    else
    {
        handleButtonTouch(tx, ty, touched);
        const bool headerQrConsumed = handleHeaderQrButtonTouch(tx, ty, touched);

        // Touch-Start-Bereich einmalig beim ersten Frame eines neuen Touches merken. Beim Loslassen (touched=false) liefert getTouch() oft (0,0) -> der Treffertest würde scheitern. Deshalb wird das Loslassen immer an den Bereich weitergeleitet, in dem der Touch begonnen hat.
        if (touched && !m_touchWasActive)
            m_touchStartedInMenu = (tx >= WorkshopCfg::MENU_X && tx < WorkshopCfg::MENU_X + WorkshopCfg::MENU_W && ty >= 0 && ty < WorkshopCfg::MENU_SPRITE_H);

        if (!headerQrConsumed && (m_touchStartedInMenu || (touched && tx >= WorkshopCfg::MENU_X && tx < WorkshopCfg::MENU_X + WorkshopCfg::MENU_W && ty >= 0 && ty < WorkshopCfg::MENU_SPRITE_H)))
            handleMenuTouch(tx, ty, touched);
        else if (!touched && m_scroll.isDragging)
            // Loslassen außerhalb des Menüs beendet noch laufenden Drag
            handleScrollTouch(tx, ty, false);

        if (!touched)
            m_touchStartedInMenu = false;

        // Touch-Weiterleitung an aktives Werkzeug
        if (m_activeTool && !headerQrConsumed)
        {
            // Strikt begrenzter Inhaltsbereich: rechts/unten durch WORK_W/WORK_H begrenzt und der Footer-Bereich explizit ausgenommen, damit ein Touch nicht im selben Frame an handleTouch() UND handleFooterTouch() zugestellt wird (sonst Doppelauslösungen bei content-aktiven Tools).
            const bool inContentArea = (tx >= WorkshopCfg::WORK_X && tx < WorkshopCfg::WORK_X + WorkshopCfg::WORK_W && ty >= WorkshopCfg::WORK_Y + WorkshopCfg::WORK_CONTENT_Y && ty < WorkshopCfg::WORK_Y + WorkshopCfg::WORK_H);
            const bool inFooterArea = (tx >= WorkshopCfg::TOOL_FOOTER_X && tx < WorkshopCfg::TOOL_FOOTER_X + WorkshopCfg::TOOL_FOOTER_W && ty >= WorkshopCfg::TOOL_FOOTER_Y && ty < WorkshopCfg::TOOL_FOOTER_Y + WorkshopCfg::TOOL_FOOTER_H);

            if (touched && inContentArea && !inFooterArea)
            {
                // Touch-Start oder laufender Touch im Werkzeugbereich
                m_toolTouchActive = true;

                if (m_activeTool->handleTouch(tx, ty, true))
                    invalidateWorkContent();
            }
            else if (!touched && m_toolTouchActive)
            {
                // Touch-Ende weiterleiten, damit das Werkzeug die Taste auslöst
                m_toolTouchActive = false;

                if (m_activeTool->handleTouch(0, 0, false))
                    invalidateWorkContent();
            }
        }

        // Footer-Tool Touch-Weiterleitung
        if (m_activeTool && !headerQrConsumed)
        {
            const bool inFooterToolArea = (tx >= WorkshopCfg::TOOL_FOOTER_X && tx < WorkshopCfg::TOOL_FOOTER_X + WorkshopCfg::TOOL_FOOTER_W && ty >= WorkshopCfg::TOOL_FOOTER_Y && ty < WorkshopCfg::TOOL_FOOTER_Y + WorkshopCfg::TOOL_FOOTER_H);

            if (touched && inFooterToolArea)
            {
                m_footerToolTouchActive = true;

                if (m_activeTool->handleFooterTouch(tx, ty, true))
                {
                    invalidateWork();
                    m_footerDirty = true;
                }
            }
            else if (!touched && m_footerToolTouchActive)
            {
                m_footerToolTouchActive = false;

                if (m_activeTool->handleFooterTouch(0, 0, false))
                {
                    invalidateWork();
                    m_footerDirty = true;
                }
            }
        }
    } // end else (!m_workContentPaused)

    m_touchWasActive = touched;

    if (m_qrModalVisible)
    {
        drawDigitalClock(WorkshopCfg::CLOCK_PUSH_X, WorkshopCfg::CLOCK_PUSH_Y);
        return;
    }

    // Scroll-Physik
    updateScrollPhysics();

    // Während aktiv gescrollt wird (Drag oder Coast oberhalb der Stop-Schwelle) wird der rechte Bereich temporär eingefroren.
    const bool freezeRightSideDuringMenuScroll =
        !m_workContentPaused &&
        (m_scroll.isDragging ||
         std::fabs(m_scroll.velocity) > WorkshopCfg::SCROLL_STOP_VELOCITY);

    // Einmaliges Grundlayout
    if (!m_layoutDrawn)
    {
        drawStaticLayout();
        m_layoutDrawn = true;
        invalidateAll();
    }

    // Selektives Neuzeichnen - die folgenden Invalidate-Quellen bleiben während des Scroll-Freezes stumm.
    if (!freezeRightSideDuringMenuScroll)
    {
        // Periodischer Redraw nur wenn sich der Cursor-Zustand tatsächlich ändert. So werden genau zwei Redraws pro Blink-Zyklus ausgelöst (alle 400 ms).
        {
            static bool s_lastCursorOn = false;
            const WorkshopPageId activeId = s_flatList[m_selectedFlatIdx].pageId;
            if (m_activeTool && !m_workContentPaused && activeId != WorkshopPageId::COLOR_CONVERTER && activeId != WorkshopPageId::PERIODIC_TABLE && activeId != WorkshopPageId::ASCII_TABLE && activeId != WorkshopPageId::SI_PREFIXES && activeId != WorkshopPageId::WORLD_CLOCK && activeId != WorkshopPageId::FLIGHT_RADAR && activeId != WorkshopPageId::GAME_2048 && activeId != WorkshopPageId::SHUT_THE_BOX && activeId != WorkshopPageId::LOTTO_SYSTEM && activeId != WorkshopPageId::METRONOME)
            {
                const bool cursorNow = (millis() % 800UL) < 400UL;
                if (cursorNow != s_lastCursorOn)
                {
                    s_lastCursorOn = cursorNow;
                    invalidateWorkContent();
                }
            }
        }

        // Werkzeuge mit eigener Animation (z.B. Metronom-Pendel) fordern hier pro Frame einen Redraw an.
        if (m_activeTool && !m_workContentPaused && m_activeTool->needsAnimationFrame())
            invalidateWorkContent();

        // Minutengenauer Redraw für die Weltzeit-Uhr. Das Werkzeug steht in der Cursor-Blink-Ausnahmeliste oben und erhält hier ein gezieltes invalidateWorkContent() ausschließlich beim Wechsel der aktuellen Lokalminute. Stundenwechsel sind darin enthalten.
        {
            static int s_lastMinute = -1;
            const WorkshopPageId activeIdMin = s_flatList[m_selectedFlatIdx].pageId;
            if (m_activeTool && activeIdMin == WorkshopPageId::WORLD_CLOCK)
            {
                const struct tm nowLocal = APP_STATE.getRTC().getCurrentLocalTime();
                if (nowLocal.tm_min != s_lastMinute)
                {
                    s_lastMinute = nowLocal.tm_min;
                    invalidateWorkContent();
                }
            }
            else
            {
                s_lastMinute = -1; // Reset bei Werkzeugwechsel
            }
        }

        // Subtitle-Wechsel detektieren -> kompletten Workspace neu zeichnen
        if (m_activeTool)
        {
            const char *currentSub = m_activeTool->getSubtitle();
            if (!currentSub)
                currentSub = "";

            if (strcmp(currentSub, m_lastSubtitleText) != 0)
            {
                stringutils::safeCopy(m_lastSubtitleText, currentSub);
                m_workFrameDirty = true;
            }
        }
    }

    // Menü-Redraw mit Throttle.
    static uint32_t s_lastMenuPushMs = 0;
    constexpr uint32_t MENU_REDRAW_MIN_INTERVAL_MS = 16;
    const uint32_t nowMs = millis();
    const bool offsetChanged = (m_scroll.scrollOffset != m_lastScrollOffset);
    const bool throttleElapsed = (nowMs - s_lastMenuPushMs) >= MENU_REDRAW_MIN_INTERVAL_MS;
    const bool menuNeedsRedraw = m_menuDirty || (offsetChanged && throttleElapsed);

    if (menuNeedsRedraw)
    {
        drawMenuList();
        s_lastMenuPushMs = nowMs;
    }

    // Rechte Seite nur zeichnen, wenn sie nicht temporär eingefroren ist.
    if (!freezeRightSideDuringMenuScroll)
    {
        if (m_workFrameDirty)
            drawWorkspace();
        else if (m_workContentDirty && !m_workContentPaused)
            drawWorkspaceContentOnly();

        // Footer wird nur bei Dirty-Flag neu gezeichnet.
        if (m_footerDirty)
            drawFooter();
    }

    // Scrollbar zusammen mit dem Menü neu zeichnen
    if (menuNeedsRedraw)
        drawScrollbar();

    // Uhr: interner Redraw nur bei Sekundenänderung
    drawDigitalClock(WorkshopCfg::CLOCK_PUSH_X, WorkshopCfg::CLOCK_PUSH_Y);
}

/**
 * @brief Zeichnet das einmalige statische Grundlayout.
 */
void Workshop::drawStaticLayout()
{
    m_tft->clear(WorkshopCfg::C_BG);
}

// ===================================================================
// MENÜLISTE
// ===================================================================

/**
 * @brief Zeichnet die Menüliste in den Menu-Sprite.
 */
void Workshop::drawMenuList()
{
    if (!m_menuSprite)
        return;

    const int totalH = getTotalListHeight();
    m_scroll.maxScroll = (totalH > WorkshopCfg::SCREEN_H)
                             ? (totalH - WorkshopCfg::SCREEN_H)
                             : 0;

    m_menuSprite->clear(WorkshopCfg::C_BG);
    m_menuSprite->loadFont(ComfortaaBold16);

    for (int i = 0; i < FLAT_LIST_COUNT; ++i)
    {
        const WorkshopListEntry &entry = s_flatList[i];
        const int rowH = (entry.type == WorkshopEntryType::SECTION_HEADER)
                             ? WorkshopCfg::SECTION_ROW_H
                             : WorkshopCfg::ITEM_ROW_H;

        const int y = getEntryY(i) - m_scroll.scrollOffset;

        // Culling
        if (y + rowH <= 0 || y >= WorkshopCfg::MENU_SPRITE_H)
            continue;

        if (entry.type == WorkshopEntryType::SECTION_HEADER)
        {
            // Sektionskopf: dunkle Einfärbung der Kategorie-Farbe als Hintergrund, voller Akzent-Balken links, heller Text
            const uint16_t catColor = getSectionColor(entry.category);
            const uint16_t tintColor = scaleColor(catColor, 0.45f); // ~45 % Helligkeit

            m_menuSprite->fillRect(0, y, WorkshopCfg::MENU_W, rowH, tintColor);

            // Linker Akzent-Balken in voller Kategorie-Farbe
            m_menuSprite->fillRect(0, y, WorkshopCfg::ITEM_ACCENT_W, rowH, catColor);

            m_menuSprite->setTextDatum(ML_DATUM);
            m_menuSprite->setTextColor(WorkshopCfg::C_SEC_HDR_TXT, tintColor);
            m_menuSprite->drawString(I18n::str(entry.labelKey), WorkshopCfg::ITEM_TEXT_X, y + rowH / 2);
        }
        else
        {
            // Werkzeug-Eintrag
            const bool selected = (i == m_selectedFlatIdx);
            const uint16_t catColor = getSectionColor(entry.category);

            // Hintergrund: ausgewählt = dunkle Einfärbung der Kategoriefarbe (~25 %), sonst transparent
            const uint16_t selBg = scaleColor(catColor, 0.25f);
            if (selected)
                m_menuSprite->fillRect(0, y, WorkshopCfg::MENU_W, rowH, selBg);

            // Linker Akzent-Balken: ausgewählt = volle Farbe, sonst ~50 %
            const uint16_t accentCol = selected ? catColor : scaleColor(catColor, 0.45f);
            m_menuSprite->fillRect(0, y, WorkshopCfg::ITEM_ACCENT_W, rowH, accentCol);

            m_menuSprite->setTextDatum(ML_DATUM);
            m_menuSprite->setTextColor(
                selected ? WorkshopCfg::C_TEXT_BRIGHT : WorkshopCfg::C_TEXT,
                selected ? selBg : WorkshopCfg::C_BG);

            m_menuSprite->drawString(I18n::str(entry.labelKey), WorkshopCfg::ITEM_TEXT_X, y + rowH / 2);

            // Trennlinie zwischen den Menüeinträgen
            m_menuSprite->drawFastHLine(WorkshopCfg::ITEM_ACCENT_W, y + rowH - 1, WorkshopCfg::MENU_W - WorkshopCfg::ITEM_ACCENT_W, WorkshopCfg::C_SEP_LINE);
        }
    }

    m_menuSprite->unloadFont();
    m_menuSprite->pushSprite(WorkshopCfg::MENU_X, 0);

    m_menuDirty = false;
    m_lastScrollOffset = m_scroll.scrollOffset;
}

// ===================================================================
// SCROLLBAR
// ===================================================================

/**
 * @brief Zeichnet die Scrollbar direkt auf das TFT.
 */
void Workshop::drawScrollbar()
{
    if (!m_scrollbarSprite)
        return;

    // Track immer mit Track-Farbe füllen
    m_scrollbarSprite->clear(WorkshopCfg::C_SCROLLBAR_TRACK);

    const int totalH = getTotalListHeight();
    if (totalH <= WorkshopCfg::SCREEN_H || m_scroll.maxScroll <= 0)
    {
        m_scrollbarSprite->pushSprite(WorkshopCfg::SCROLLBAR_X, 0);
        return;
    }

    // Thumb-Höhe: Anteil sichtbarer Bereich (volle Menühöhe = SCREEN_H) an Gesamtinhalt
    const float viewRatio = static_cast<float>(WorkshopCfg::SCREEN_H) / static_cast<float>(totalH);
    int thumbH = std::max(20, static_cast<int>(WorkshopCfg::SCREEN_H * viewRatio));

    // Thumb-Position: geclampter Offset -> keine Artefakte beim Gummiband-Overscroll
    const int clampedOffset = clampi(m_scroll.scrollOffset, 0, m_scroll.maxScroll);
    const float scrollRatio = static_cast<float>(clampedOffset) / static_cast<float>(m_scroll.maxScroll);
    const int thumbY = static_cast<int>(scrollRatio * (WorkshopCfg::SCREEN_H - thumbH));

    // Beim Drag aktive Farbe, sonst normale Farbe (wie Contacts)
    const uint16_t thumbColor = m_scroll.isDragging
                                    ? WorkshopCfg::C_SCROLLBAR_DRAG
                                    : WorkshopCfg::C_SCROLLBAR_NORMAL;

    m_scrollbarSprite->fillRoundRect(0, thumbY, WorkshopCfg::SCROLLBAR_W, thumbH, 2, thumbColor);
    m_scrollbarSprite->pushSprite(WorkshopCfg::SCROLLBAR_X, 0);
}

// ===================================================================
// ARBEITSBEREICH
// ===================================================================

/**
 * @brief Zeichnet den Arbeitsbereich.
 */
void Workshop::drawWorkspace()
{
    if (!m_workSprite)
        return;

    const WorkshopListEntry &entry = s_flatList[m_selectedFlatIdx];
    const WorkshopPageDef *page = getPageDef(entry.pageId);

    // Hintergrund: schwarzer Rand, dann dunkelblaugrauer RoundRect
    m_workSprite->clear(WorkshopCfg::C_BG);
    m_workSprite->fillSmoothRoundRect(0, 0, m_workSprite->width(), m_workSprite->height(), WorkshopCfg::WORK_CORNER_R, WorkshopCfg::C_WORK_BG);

    // Titelbox Titelbox - rechts um 10px breiter beim Periodensystem (kein Scrollbar-Abstand nötig)
    const int titleBoxW = (entry.pageId == WorkshopPageId::PERIODIC_TABLE)
                              ? m_workSprite->width() - WorkshopCfg::TITLE_BOX_X * 2 + 10
                              : m_workSprite->width() - WorkshopCfg::TITLE_BOX_X * 2;

    m_workSprite->fillSmoothRoundRect(WorkshopCfg::TITLE_BOX_X, WorkshopCfg::TITLE_BOX_Y, titleBoxW, WorkshopCfg::TITLE_BOX_H, WorkshopCfg::WORK_CORNER_R, WorkshopCfg::C_WORK_TITLE_BG);

    // Haupttitel
    const WorkshopKey titleKey = ((entry.pageId == WorkshopPageId::LOTTO_NORMAL || entry.pageId == WorkshopPageId::LOTTO_SYSTEM) && page)
                                     ? page->titleKey
                                     : entry.labelKey;

    m_workSprite->loadFont(ComfortaaBold30);
    m_workSprite->setTextDatum(TL_DATUM);
    m_workSprite->setTextColor(Theme::ColorGreen, WorkshopCfg::C_WORK_TITLE_BG);
    m_workSprite->drawString(I18n::str(titleKey), WorkshopCfg::TITLE_TEXT_X, WorkshopCfg::TITLE_TEXT_Y);
    m_workSprite->unloadFont();

    // Untertitel - dynamisch wenn das Werkzeug einen liefert, sonst aus PageDef
    const char *dynamicSub = m_activeTool ? m_activeTool->getSubtitle() : nullptr;
    const __FlashStringHelper *staticSub = (page ? I18n::str(page->subtitleKey) : nullptr);

    const bool hasDynamic = (dynamicSub && dynamicSub[0] != '\0');
    const bool hasStatic = !pgmIsEmpty(staticSub);

    if (hasDynamic || hasStatic)
    {
        m_workSprite->loadFont(ComfortaaBold16);
        m_workSprite->setTextDatum(TL_DATUM);
        m_workSprite->setTextColor(WorkshopCfg::C_WORK_SUB, WorkshopCfg::C_WORK_TITLE_BG);
        if (hasDynamic)
            m_workSprite->drawString(dynamicSub, WorkshopCfg::TITLE_TEXT_X, WorkshopCfg::SUB_TEXT_Y);
        else
            m_workSprite->drawString(staticSub, WorkshopCfg::TITLE_TEXT_X, WorkshopCfg::SUB_TEXT_Y);
        m_workSprite->unloadFont();
    }

    drawHeaderQrButton(titleBoxW);

    // Seiteninhalt
    drawWorkspaceContent(entry.pageId);
    m_workSprite->pushSprite(WorkshopCfg::WORK_X, WorkshopCfg::WORK_Y);
    m_workFrameDirty = false;
    m_workContentDirty = false;
}

/**
 * @brief Zeichnet den optionalen Website-Button in der Titelbox.
 */
void Workshop::drawHeaderQrButton(int titleBoxW)
{
    if (!m_workSprite || !hasActivePageWebUrl())
        return;

    const int x = WorkshopCfg::TITLE_BOX_X + titleBoxW - WorkshopCfg::HEADER_QR_BTN_PAD_R - WorkshopCfg::HEADER_QR_BTN_SIZE;
    drawFakeQrButtonIcon(x, WorkshopCfg::HEADER_QR_BTN_Y, m_headerQrPressed);
}

/**
 * @brief Zeichnet das nicht lesbare QR-Symbol im Website-Button.
 */
void Workshop::drawFakeQrButtonIcon(int x, int y, bool pressed)
{
    if (!m_workSprite)
        return;

    (void)pressed;
    m_workSprite->pushImage(x, y, WorkshopCfg::HEADER_QR_BTN_SIZE, WorkshopCfg::HEADER_QR_BTN_SIZE,
                            IconShapes::cached(IconId::QrCode, ICON_QR_COLOR, ICON_QR_BACKGROUND));
}

/**
 * @brief Zeichnet nur den Inhaltsbereich im vorhandenen Work-Sprite neu.
 */
void Workshop::drawWorkspaceContentOnly()
{
    if (!m_workSprite)
        return;

    const WorkshopListEntry &entry = s_flatList[m_selectedFlatIdx];

    constexpr int TOOL_TOP_OFFSET = 7;
    constexpr int ox = WorkshopCfg::TITLE_MARGIN;
    constexpr int oy = WorkshopCfg::WORK_CONTENT_Y - TOOL_TOP_OFFSET;
    constexpr int w = WorkshopCfg::WORK_W - WorkshopCfg::TITLE_MARGIN * 2;
    constexpr int h = WorkshopCfg::WORK_H - WorkshopCfg::WORK_CONTENT_Y - WorkshopCfg::TITLE_MARGIN + TOOL_TOP_OFFSET;

    if (m_activeTool && m_activeTool->needsAnimationFrame())
    {
        int clipX = 0;
        int clipY = 0;
        int clipW = 0;
        int clipH = 0;
        if (m_activeTool->getPushBoundingBox(clipX, clipY, clipW, clipH))
        {
            m_activeTool->drawAnimationFrame(m_workSprite, ox, oy, w, h);

            if (m_activeTool->pushAnimationFrame(m_tft, m_workSprite, WorkshopCfg::WORK_X, WorkshopCfg::WORK_Y))
            {
                m_workContentDirty = false;
                return;
            }

            if (m_tft && clipW > 0 && clipH > 0)
            {
                m_tft->setClipRect(WorkshopCfg::WORK_X + clipX,
                                   WorkshopCfg::WORK_Y + clipY,
                                   clipW,
                                   clipH);
                m_workSprite->pushSprite(WorkshopCfg::WORK_X, WorkshopCfg::WORK_Y);
                m_tft->clearClipRect();
            }
            else
            {
                m_workSprite->pushSprite(WorkshopCfg::WORK_X, WorkshopCfg::WORK_Y);
            }

            m_workContentDirty = false;
            return;
        }
    }

    // Farb-Konverter, Periodensystem und ASCII-Tabelle verwalten den Sprite-Inhalt selbst.
    if (entry.pageId != WorkshopPageId::COLOR_CONVERTER && entry.pageId != WorkshopPageId::PERIODIC_TABLE && entry.pageId != WorkshopPageId::ASCII_TABLE)
    {
        // Inhaltsbereich vollständig auf Arbeitsbereich-Hintergrund zurücksetzen ohne Rahmen und Titelbox neu zu zeichnen.
        m_workSprite->fillRect(ox, oy, w, h, WorkshopCfg::C_WORK_BG);
    }

    drawWorkspaceContent(entry.pageId);
    m_workSprite->pushSprite(WorkshopCfg::WORK_X, WorkshopCfg::WORK_Y);
    m_workContentDirty = false;
}

/**
 * @brief Dispatcher für den Seiteninhalt des Arbeitsbereichs.
 */
void Workshop::drawWorkspaceContent(WorkshopPageId pageId)
{
    // Aktives Werkzeug übernimmt die Darstellung
    if (m_activeTool)
    {
        constexpr int TOOL_TOP_OFFSET = 7;
        constexpr int ox = WorkshopCfg::TITLE_MARGIN;
        constexpr int oy = WorkshopCfg::WORK_CONTENT_Y - TOOL_TOP_OFFSET;
        constexpr int w = WorkshopCfg::WORK_W - WorkshopCfg::TITLE_MARGIN * 2;
        constexpr int h = WorkshopCfg::WORK_H - WorkshopCfg::WORK_CONTENT_Y - WorkshopCfg::TITLE_MARGIN + TOOL_TOP_OFFSET;
        m_activeTool->draw(m_workSprite, ox, oy, w, h);
        return;
    }

    // Fallback wenn createTool() fehlgeschlagen ist (z.B. OOM)
    const WorkshopPageDef *page = getPageDef(pageId);
    if (!page || pageId == WorkshopPageId::NONE)
        return;

    constexpr int TEXT_X = 18;
    constexpr int TEXT_Y = WorkshopCfg::WORK_CONTENT_Y + 20;

    m_workSprite->loadFont(ComfortaaBold16);
    m_workSprite->setTextDatum(ML_DATUM);
    m_workSprite->setTextColor(WorkshopCfg::C_ACCENT, WorkshopCfg::C_WORK_BG);
    m_workSprite->drawString(I18n::str(WorkshopKey::WORKSHOP_ERR_HEADER), TEXT_X, TEXT_Y);

    m_workSprite->setTextColor(WorkshopCfg::C_TEXT_DIM, WorkshopCfg::C_WORK_BG);
    m_workSprite->drawString(I18n::str(WorkshopKey::WORKSHOP_ERR_TOOL_LOAD_FAILED), TEXT_X, TEXT_Y + 32);
    m_workSprite->unloadFont();
}

// ===================================================================
// FUSSZEILE
// ===================================================================

/**
 * @brief Zeichnet die Fußzeile und alle Footer-Buttons.
 */
void Workshop::drawFooter()
{
    if (!m_tft)
        return;

    // Footer-Bereich löschen
    m_tft->fillRect(WorkshopCfg::DIVIDER_X + 1, WorkshopCfg::FOOTER_Y, WorkshopCfg::CLOCK_PUSH_X - WorkshopCfg::DIVIDER_X - 1, WorkshopCfg::FOOTER_H, WorkshopCfg::C_BG);
    m_exitButton.drawButton();

    // Werkzeugspezifische Steuerelemente im Footer-Bereich zeichnen
    if (m_activeTool)
        m_activeTool->drawFooter(m_tft, WorkshopCfg::TOOL_FOOTER_X, WorkshopCfg::TOOL_FOOTER_Y, WorkshopCfg::TOOL_FOOTER_W, WorkshopCfg::TOOL_FOOTER_H);

    m_footerDirty = false;
}

// ===================================================================
// DIGITALUHR
// ===================================================================

/**
 * @brief Zeichnet die Digitaluhr in den Clock-Sprite und pusht ihn.
 */
void Workshop::drawDigitalClock(int32_t x, int32_t y)
{
    if (!m_clockSprite)
        return;

    DS1307_RTC *rtc = &APP_STATE.getRTC();
    Clocks::drawDigitalClockPlain(m_clockSprite, rtc, m_lastDrawnSecond, x, y, false, true);
}

// ===================================================================
// TOUCH-HANDLING
// ===================================================================

/**
 * @brief Verarbeitet Touch in der Menüliste.
 */
void Workshop::handleMenuTouch(int32_t tx, int32_t ty, bool touched)
{
    handleScrollTouch(tx, ty, touched);

    // Tap-Auswertung beim Loslassen
    if (!touched && m_touchWasActive)
    {
        if (m_tap.isTap)
        {
            // Tap-Y mit Scroll-Offset in absolute Listenposition umrechnen
            const int absY = m_tap.startY + m_scroll.scrollOffset;
            const int idx = getEntryAtY(absY);

            if (idx >= 0 && idx < FLAT_LIST_COUNT && s_flatList[idx].type == WorkshopEntryType::TOOL_ITEM)
                selectItem(idx);
        }

        // Tap-State nach jedem Loslassen zurücksetzen
        m_tap.reset();
    }
}

/**
 * @brief Aktualisiert den Scroll-Zustand während eines Touch-Drags.
 */
void Workshop::handleScrollTouch(int32_t tx, int32_t ty, bool touched)
{
    // tx wird für Tap-Tracking benutzt (Start-X und Bewegungs-Schwellwert), ty ist die primäre Achse für Drag/Scroll.

    if (touched)
    {
        if (!m_scroll.isDragging)
        {
            // Drag-Start
            m_scroll.isDragging = true;
            m_scroll.dragStartY = ty;
            m_scroll.dragStartOffset = m_scroll.scrollOffset;
            m_scroll.velocity = 0.0f;
            m_lastTouchY = ty;
            m_lastTouchTime = millis();
            m_tap.startX = tx;
            m_tap.startY = ty;
            m_tap.isTap = true;
        }
        else
        {
            // Drag läuft: Offset aktualisieren (Verstärkung 1.5x).
            const int delta = ty - m_scroll.dragStartY;
            m_scroll.setOffset(m_scroll.dragStartOffset - static_cast<int>(delta * 1.5f));

            // Velocity für Momentum berechnen
            const unsigned long now = millis();
            const int timeDelta = static_cast<int>(now - m_lastTouchTime);
            if (timeDelta > 0 && timeDelta < 100)
            {
                const float rawV = static_cast<float>(m_lastTouchY - ty) * 1000.0f / static_cast<float>(timeDelta);
                // Glättung: 70 % neuer Wert + 30 % alter Wert, dann 1.3x
                m_scroll.velocity = (rawV * 0.7f + m_scroll.velocity * 0.3f) * 1.3f;
            }

            // Tap-Erkennung: überschrittener Threshold -> kein Tap
            if (m_tap.exceedsThreshold(tx, ty))
                m_tap.isTap = false;

            m_lastTouchY = ty;
            m_lastTouchTime = now;

            // Gummiband-Dämpfung außerhalb der Grenzen (nur visueller Widerstand)
            if (m_scroll.scrollOffset < 0)
            {
                m_scroll.setOffset(static_cast<int>(m_scroll.scrollOffset * 0.35f));
            }
            else if (m_scroll.scrollOffset > m_scroll.maxScroll)
            {
                const int excess = m_scroll.scrollOffset - m_scroll.maxScroll;
                m_scroll.setOffset(m_scroll.maxScroll + static_cast<int>(excess * 0.35f));
            }
        }
    }
    else if (m_scroll.isDragging)
    {
        // Drag beendet
        m_scroll.isDragging = false;

        // Velocity auf sinnvollen Bereich begrenzen
        if (m_scroll.velocity > 2500.0f)
            m_scroll.velocity = 2500.0f;

        if (m_scroll.velocity < -2500.0f)
            m_scroll.velocity = -2500.0f;
    }
}

/**
 * @brief Verarbeitet Touch-Events der Footer-Buttons.
 */
void Workshop::handleButtonTouch(int32_t tx, int32_t ty, bool touched)
{
    const bool onExit = touched && m_exitButton.contains(tx, ty);
    const bool wasPressed = m_exitButton.isPressed();

    m_exitButton.press(onExit);

    // Nur bei State-Wechsel neu zeichnen
    if (onExit != wasPressed)
        m_exitButton.drawButton(onExit);

    if (m_exitButton.justReleased() && m_callbacks.onExitRequested)
        m_callbacks.onExitRequested();
}

/**
 * @brief Verarbeitet Touch-Events des Website-Buttons im Header.
 */
bool Workshop::handleHeaderQrButtonTouch(int32_t tx, int32_t ty, bool touched)
{
    if (!hasActivePageWebUrl())
    {
        if (m_headerQrPressed)
        {
            m_headerQrPressed = false;
            m_workFrameDirty = true;
        }
        m_headerQrTouchActive = false;
        return false;
    }

    const int bx = getHeaderQrButtonX();
    const int by = WorkshopCfg::WORK_Y + WorkshopCfg::HEADER_QR_BTN_Y;
    const bool onButton = touched &&
                          tx >= bx &&
                          tx < bx + WorkshopCfg::HEADER_QR_BTN_SIZE &&
                          ty >= by &&
                          ty < by + WorkshopCfg::HEADER_QR_BTN_SIZE;

    if (touched && onButton)
    {
        m_headerQrTouchActive = true;
        if (!m_headerQrPressed)
        {
            m_headerQrPressed = true;
            m_workFrameDirty = true;
        }
        return true;
    }

    if (!touched && m_headerQrTouchActive)
    {
        const bool shouldOpen = m_headerQrPressed;
        m_headerQrTouchActive = false;
        if (m_headerQrPressed)
        {
            m_headerQrPressed = false;
            m_workFrameDirty = true;
        }
        if (shouldOpen)
            showQrModal();
        return true;
    }

    if (touched && m_headerQrTouchActive)
    {
        if (m_headerQrPressed != onButton)
        {
            m_headerQrPressed = onButton;
            m_workFrameDirty = true;
        }
        return true;
    }

    return false;
}

/**
 * @brief Aktualisiert das QR-Modal und schließt es bei Tap außerhalb.
 */
void Workshop::handleQrModalTouch(int32_t tx, int32_t ty, bool touched)
{
    const bool inside = tx >= QR_MODAL_X &&
                        tx < QR_MODAL_X + QR_MODAL_W &&
                        ty >= QR_MODAL_Y &&
                        ty < QR_MODAL_Y + QR_MODAL_H;

    if (touched && !m_qrModalTouchActive)
    {
        m_qrModalTouchActive = true;
        m_qrModalTouchOutside = !inside;
        return;
    }

    if (!touched && m_qrModalTouchActive)
    {
        const bool closeByOutsideTap = m_qrModalTouchOutside;
        m_qrModalTouchActive = false;
        m_qrModalTouchOutside = false;

        if (closeByOutsideTap)
            closeQrModal();
    }
}

// ===================================================================
// SCROLL-PHYSIK
// ===================================================================

/**
 * @brief Führt einen Schritt der Momentum-Scroll-Physik aus.
 *
 * Zeitbasiert (dt aus millis()): die Coast-Bewegung und die Gummiband- Rückkehr werden über die tatsächlich vergangene Zeit integriert, so dass das Scrollverhalten unabhängig von der draw()-Frequenz ist. Der Offset wird über den Float-Akkumulator m_scroll.scrollOffsetF geführt, damit Sub-Pixel-Anteile nicht durch Integer-Truncation verloren gehen.
 */
void Workshop::updateScrollPhysics()
{
    if (m_scroll.isDragging)
    {
        // Während des Ziehens steuert der Touch-Handler den Versatz direkt. Der Zeitstempel wird zurückgesetzt, damit der erste Auslaufschritt nach dem Loslassen mit einem sauberen, kleinen dt startet.
        m_scroll.lastTickMs = millis();
        return;
    }

    // dt seit letztem Update bestimmen, gegen Sprünge nach Pausen begrenzen
    const uint32_t now = millis();
    if (m_scroll.lastTickMs == 0)
        m_scroll.lastTickMs = now;

    float dt = (now - m_scroll.lastTickMs) / 1000.0f;
    m_scroll.lastTickMs = now;

    if (dt <= 0.0f)
        return;

    if (dt > WorkshopCfg::SCROLL_DT_MAX)
        dt = WorkshopCfg::SCROLL_DT_MAX;

    // Gummiband: oberhalb der oberen Grenze
    if (m_scroll.scrollOffset < 0)
    {
        const float remain = powf(WorkshopCfg::SCROLL_BOUNCE_REMAIN_PER_SEC, dt);
        const float newF = m_scroll.scrollOffsetF * remain;
        if (newF > -0.5f)
        {
            m_scroll.setOffset(0);
            m_scroll.velocity = 0.0f;
        }
        else
        {
            m_scroll.scrollOffsetF = newF;
            m_scroll.scrollOffset = static_cast<int32_t>(newF);
        }
        return;
    }

    // Gummiband: unterhalb der unteren Grenze
    if (m_scroll.scrollOffset > m_scroll.maxScroll)
    {
        const float remain = powf(WorkshopCfg::SCROLL_BOUNCE_REMAIN_PER_SEC, dt);
        const float excessF = m_scroll.scrollOffsetF - static_cast<float>(m_scroll.maxScroll);
        const float newExcess = excessF * remain;
        if (newExcess < 0.5f)
        {
            m_scroll.setOffset(m_scroll.maxScroll);
            m_scroll.velocity = 0.0f;
        }
        else
        {
            m_scroll.scrollOffsetF = static_cast<float>(m_scroll.maxScroll) + newExcess;
            m_scroll.scrollOffset = static_cast<int32_t>(m_scroll.scrollOffsetF);
        }
        return;
    }

    // Momentum-Coasting: Bewegung und Reibung zeitbasiert integrieren
    if (m_scroll.velocity > WorkshopCfg::SCROLL_STOP_VELOCITY || m_scroll.velocity < -WorkshopCfg::SCROLL_STOP_VELOCITY)
    {
        m_scroll.addF(m_scroll.velocity * dt);
        m_scroll.velocity *= powf(WorkshopCfg::SCROLL_FRICTION_PER_SEC, dt);

        if (m_scroll.velocity < WorkshopCfg::SCROLL_STOP_VELOCITY && m_scroll.velocity > -WorkshopCfg::SCROLL_STOP_VELOCITY)
            m_scroll.velocity = 0.0f;

        constrainScroll();
    }
}

/**
 * @brief Begrenzt scrollOffset auf [0, maxScroll].
 */
void Workshop::constrainScroll()
{
    if (m_scroll.isDragging)
        return;

    if (m_scroll.scrollOffset < 0)
    {
        m_scroll.setOffset(0);
        m_scroll.velocity = 0.0f;
    }
    else if (m_scroll.scrollOffset > m_scroll.maxScroll)
    {
        m_scroll.setOffset(m_scroll.maxScroll);
        m_scroll.velocity = 0.0f;
    }
}

// ===================================================================
// NAVIGATION
// ===================================================================

/**
 * @brief Wählt einen TOOL_ITEM-Eintrag aus und wechselt das aktive Werkzeug.
 */
void Workshop::selectItem(int flatIdx)
{
    if (flatIdx < 0 || flatIdx >= FLAT_LIST_COUNT)
        return;

    if (s_flatList[flatIdx].type != WorkshopEntryType::TOOL_ITEM)
        return;

    // Re-Klick auf bereits aktives Werkzeug ignorieren - sonst würde der bestehende Tool-Zustand verworfen und das Tool unnötig neu aufgebaut.
    if (flatIdx == m_selectedFlatIdx && m_activeTool)
        return;

    m_selectedFlatIdx = flatIdx;

    // Altes Werkzeug geordnet beenden und freigeben
    if (m_activeTool)
    {
        m_activeTool->onExit();
        delete m_activeTool;
        m_activeTool = nullptr;
    }

    // Subtitle-Snapshot verwerfen, damit der erste draw() den neuen Subtitle übernimmt
    m_lastSubtitleText[0] = '\0';

    // Touch-Tracking des Werkzeugbereichs zurücksetzen
    m_toolTouchActive = false;
    m_footerToolTouchActive = false;
    m_headerQrTouchActive = false;
    m_headerQrPressed = false;

    // Neues Werkzeug instantiieren und aktivieren
    m_activeTool = createTool(s_flatList[flatIdx].pageId);
    if (m_activeTool)
    {
        m_activeTool->setOwner(this);
        m_activeTool->onEnter(m_tft);
    }

    invalidateMenu();
    invalidateWork();
    m_footerDirty = true;
    ensureSelectedVisible();
}

/**
 * @brief Stellt sicher, dass der gewählte Eintrag vollständig sichtbar ist.
 */
void Workshop::ensureSelectedVisible()
{
    if (m_selectedFlatIdx < 0 || m_selectedFlatIdx >= FLAT_LIST_COUNT)
        return;

    const int entryY = getEntryY(m_selectedFlatIdx);
    const int entryH = WorkshopCfg::ITEM_ROW_H;

    // Oben abgeschnitten: nach oben scrollen
    if (entryY < m_scroll.scrollOffset)
        m_scroll.setOffset(entryY);

    // Unten abgeschnitten: nach unten scrollen
    const int bottom = entryY + entryH;
    if (bottom > m_scroll.scrollOffset + WorkshopCfg::SCREEN_H)
        m_scroll.setOffset(bottom - WorkshopCfg::SCREEN_H);

    constrainScroll();
    m_menuDirty = true;
}

// ===================================================================
// WERKZEUG-VERWALTUNG
// ===================================================================

/**
 * @brief Instantiiert das zu einer Seiten-ID passende Werkzeug.
 */
WorkshopTool *Workshop::createTool(WorkshopPageId id)
{
    switch (id)
    {
    case WorkshopPageId::CALCULATOR:
        return new WorkshopTool_Calculator();

    case WorkshopPageId::WORLD_CLOCK:
        return new WorkshopTool_WorldClock();

    case WorkshopPageId::FLIGHT_RADAR:
        return new WorkshopTool_FlightRadar();

    case WorkshopPageId::RESISTOR_COLOR_CODE:
        return new WorkshopTool_Resistor();

    case WorkshopPageId::SMD_RESISTOR_CODE:
        return new WorkshopTool_SmdResistor();

    case WorkshopPageId::OHM_CALCULATOR:
        return new WorkshopTool_OhmCalculator();

    case WorkshopPageId::VOLTAGE_DIVIDER:
        return new WorkshopTool_VoltageDivider();

    case WorkshopPageId::LED_RESISTOR_CALCULATOR:
        return new WorkshopTool_LedCalculator();

    case WorkshopPageId::RC_RL_FILTER_CALCULATOR:
        return new WorkshopTool_RcFilter();

    case WorkshopPageId::LC_FILTER_CALCULATOR:
        return new WorkshopTool_LcFilter();

    case WorkshopPageId::UKW_RESONATOR:
        return new WorkshopTool_UkwResonator();

    case WorkshopPageId::LM317_CALCULATOR:
        return new WorkshopTool_Lm317();

    case WorkshopPageId::DECIBEL_CALCULATOR:
        return new WorkshopTool_Decibel();

    case WorkshopPageId::NE555_CALCULATOR:
        return new WorkshopTool_Ne555();

    case WorkshopPageId::OPAMP_CALCULATOR:
        return new WorkshopTool_OpAmp();

    case WorkshopPageId::COLOR_CONVERTER:
        return new WorkshopTool_ColorConverter();

    case WorkshopPageId::INCH_MM_CALCULATOR:
        return new WorkshopTool_InchMm();

    case WorkshopPageId::TEMP_CALCULATOR:
        return new WorkshopTool_Temp();

    case WorkshopPageId::PRESSURE_CALCULATOR:
        return new WorkshopTool_Pressure();

    case WorkshopPageId::PERIODIC_TABLE:
        return new WorkshopTool_PeriodicTable();

    case WorkshopPageId::ASCII_TABLE:
        return new WorkshopTool_AsciiTable();

    case WorkshopPageId::BIT_EDITOR:
        return new WorkshopTool_BitEditor();

    case WorkshopPageId::LOGIC_GATE_SIM:
        return new WorkshopTool_LogicGate();

    case WorkshopPageId::SI_PREFIXES:
        return new WorkshopTool_SiPrefixes();

    case WorkshopPageId::SCREW_REFERENCE:
        return new WorkshopTool_ScrewRef();

    case WorkshopPageId::BMI_CALCULATOR:
        return new WorkshopTool_BmiCalc();

    case WorkshopPageId::GAME_2048:
        return new WorkshopTool_Game2048();

    case WorkshopPageId::MILL_GAME:
        return new WorkshopTool_Mill();

    case WorkshopPageId::LIGHTS_OUT:
        return new WorkshopTool_LightsOut();

    case WorkshopPageId::DICE:
        return new WorkshopTool_Dice();

    case WorkshopPageId::SHUT_THE_BOX:
        return new WorkshopTool_ShutTheBox();

    case WorkshopPageId::LOTTO_NORMAL:
        return new WorkshopTool_Lotto();

    case WorkshopPageId::LOTTO_SYSTEM:
        return new WorkshopTool_LottoSystem();

    case WorkshopPageId::STITCH_GAUGE:
        return new WorkshopTool_StitchGauge();

    case WorkshopPageId::STITCH_DISTRIBUTOR:
        return new WorkshopTool_StitchDistributor();

    case WorkshopPageId::SLEEVE_TAPER:
        return new WorkshopTool_SleeveTaper();

    case WorkshopPageId::CIRCLE_OF_FIFTHS:
        return new WorkshopTool_CircleOfFifths();

    case WorkshopPageId::METRONOME:
        return new WorkshopTool_Metronome();

    default:
        return nullptr;
    }
}

// ===================================================================
// HILFSFUNKTIONEN
// ===================================================================

/**
 * @brief Berechnet die Y-Position eines Listeneintrags.
 */
int Workshop::getEntryY(int flatIdx) const
{
    int y = 0;
    for (int i = 0; i < flatIdx && i < FLAT_LIST_COUNT; ++i)
    {
        y += (s_flatList[i].type == WorkshopEntryType::SECTION_HEADER)
                 ? WorkshopCfg::SECTION_ROW_H
                 : WorkshopCfg::ITEM_ROW_H;
    }
    return y;
}

/**
 * @brief Ermittelt den Listenindex an einer absoluten Y-Position.
 */
int Workshop::getEntryAtY(int absY) const
{
    int y = 0;
    for (int i = 0; i < FLAT_LIST_COUNT; ++i)
    {
        const int rowH = (s_flatList[i].type == WorkshopEntryType::SECTION_HEADER)
                             ? WorkshopCfg::SECTION_ROW_H
                             : WorkshopCfg::ITEM_ROW_H;

        if (absY >= y && absY < y + rowH)
            return i;

        y += rowH;
    }
    return -1;
}

/**
 * @brief Berechnet die Gesamthöhe des Listeninhalts.
 */
int Workshop::getTotalListHeight() const
{
    int h = 0;
    for (int i = 0; i < FLAT_LIST_COUNT; ++i)
    {
        h += (s_flatList[i].type == WorkshopEntryType::SECTION_HEADER)
                 ? WorkshopCfg::SECTION_ROW_H
                 : WorkshopCfg::ITEM_ROW_H;
    }
    return h;
}

/**
 * @brief Gibt die Streifenfarbe einer Kategorie zurück.
 */
uint16_t Workshop::getSectionColor(WorkshopCategory cat) const
{
    switch (cat)
    {
    case WorkshopCategory::MISC:
        return WorkshopCfg::C_SEC_MISC;
    case WorkshopCategory::GAMES:
        return WorkshopCfg::C_SEC_GAMES;
    case WorkshopCategory::ELECTRONICS:
        return WorkshopCfg::C_SEC_ELECTRONICS;
    case WorkshopCategory::UNITS:
        return WorkshopCfg::C_SEC_UNITS;
    case WorkshopCategory::DIGITAL:
        return WorkshopCfg::C_SEC_DIGITAL;
    case WorkshopCategory::KNITTING:
        return WorkshopCfg::C_SEC_KNITTING;
    case WorkshopCategory::MUSIC:
        return WorkshopCfg::C_SEC_MUSIC;
    default:
        return WorkshopCfg::C_SEC_INACTIVE;
    }
}

/**
 * @brief Sucht die PageDef-Struktur zu einer Seiten-ID.
 */
const WorkshopPageDef *Workshop::getPageDef(WorkshopPageId id) const
{
    const int count = static_cast<int>(sizeof(s_pages) / sizeof(s_pages[0]));
    for (int i = 0; i < count; ++i)
    {
        if (s_pages[i].id == id)
            return &s_pages[i];
    }
    return nullptr;
}

/**
 * @brief Sucht die Website-URL zu einer Seiten-ID.
 */
const char *Workshop::getPageWebUrl(WorkshopPageId id) const
{
    for (int i = 0; i < WEB_LINK_COUNT; ++i)
    {
        if (s_webLinks[i].id == id)
            return s_webLinks[i].url ? s_webLinks[i].url : "";
    }
    return "";
}

/**
 * @brief Gibt die Website-URL der aktuell aktiven Seite zurück.
 */
const char *Workshop::getActivePageWebUrl() const
{
    if (m_selectedFlatIdx < 0 || m_selectedFlatIdx >= FLAT_LIST_COUNT)
        return "";

    return getPageWebUrl(s_flatList[m_selectedFlatIdx].pageId);
}

/**
 * @brief Prüft ob die aktive Seite einen Website-Link besitzt.
 */
bool Workshop::hasActivePageWebUrl() const
{
    const char *url = getActivePageWebUrl();
    return url && url[0] != '\0';
}

/**
 * @brief Berechnet die absolute X-Position des Website-Buttons.
 */
int Workshop::getHeaderQrButtonX() const
{
    const WorkshopListEntry &entry = s_flatList[m_selectedFlatIdx];
    const int titleBoxW = (entry.pageId == WorkshopPageId::PERIODIC_TABLE)
                              ? WorkshopCfg::WORK_W - WorkshopCfg::TITLE_BOX_X * 2 + 10
                              : WorkshopCfg::WORK_W - WorkshopCfg::TITLE_BOX_X * 2;
    const int spriteX = WorkshopCfg::TITLE_BOX_X + titleBoxW - WorkshopCfg::HEADER_QR_BTN_PAD_R - WorkshopCfg::HEADER_QR_BTN_SIZE;
    return WorkshopCfg::WORK_X + spriteX;
}

/**
 * @brief Zeigt das QR-Modal für die aktive Werkzeugseite an.
 */
void Workshop::showQrModal()
{
    if (m_qrModalVisible || !hasActivePageWebUrl() || !m_tft)
        return;

    m_qrModalTouchActive = false;
    m_qrModalTouchOutside = false;
    backupQrModal();
    m_qrModalVisible = true;
    drawQrModal();
}

/**
 * @brief Schließt das QR-Modal und stellt den Hintergrund wieder her.
 */
void Workshop::closeQrModal()
{
    if (!m_qrModalVisible || !m_tft)
        return;

    m_tft->startWrite();
    if (m_qrModalBackupValid && m_qrModalBackup)
    {
        m_tft->pushImage(QR_MODAL_X, QR_MODAL_Y, QR_MODAL_W, QR_MODAL_H, m_qrModalBackup);
    }
    else
    {
        m_tft->fillRect(QR_MODAL_X, QR_MODAL_Y, QR_MODAL_W, QR_MODAL_H, WorkshopCfg::C_BG);
        m_layoutDrawn = false;
        invalidateAll();
    }
    m_tft->endWrite();

    m_qrModalVisible = false;
    m_qrModalTouchActive = false;
    m_qrModalTouchOutside = false;
    m_qrModalBackupValid = false;
    m_headerQrPressed = false;
    invalidateWork();
}

/**
 * @brief Sichert den Hintergrundbereich des QR-Modals.
 */
void Workshop::backupQrModal()
{
    const size_t neededBytes = static_cast<size_t>(QR_MODAL_W) * static_cast<size_t>(QR_MODAL_H) * sizeof(lgfx::rgb565_t);
    if (m_qrModalBackupBytes != neededBytes)
    {
        if (m_qrModalBackup)
        {
            free(m_qrModalBackup);
            m_qrModalBackup = nullptr;
            m_qrModalBackupBytes = 0;
        }

        m_qrModalBackup = static_cast<lgfx::rgb565_t *>(heap_caps_malloc(neededBytes, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT));
        if (!m_qrModalBackup)
            m_qrModalBackup = static_cast<lgfx::rgb565_t *>(malloc(neededBytes));

        if (m_qrModalBackup)
            m_qrModalBackupBytes = neededBytes;
    }

    if (!m_qrModalBackup)
    {
        m_qrModalBackupValid = false;
        LOG_ERROR("Workshop: Kein Speicher für QR-Modal-Backup");
        return;
    }

    m_tft->startWrite();
    m_tft->readRect(QR_MODAL_X, QR_MODAL_Y, QR_MODAL_W, QR_MODAL_H, m_qrModalBackup);
    m_tft->endWrite();
    m_qrModalBackupValid = true;
}

/**
 * @brief Zeichnet das QR-Modal mit echtem QR-Code und Hinweistext.
 */
void Workshop::drawQrModal()
{
    if (!m_tft)
        return;

    const char *url = getActivePageWebUrl();
    if (!url || url[0] == '\0')
        return;

    m_tft->startWrite();
    m_tft->fillSmoothRoundRect(QR_MODAL_X, QR_MODAL_Y, QR_MODAL_W, QR_MODAL_H, QR_MODAL_RADIUS, QR_MODAL_BORDER);
    m_tft->fillSmoothRoundRect(QR_MODAL_X + 1, QR_MODAL_Y + 1, QR_MODAL_W - 2, QR_MODAL_H - 2, QR_MODAL_RADIUS, QR_MODAL_BG);

    m_tft->loadFont(ComfortaaBold18);
    m_tft->setTextDatum(TL_DATUM);
    m_tft->setTextColor(Theme::ColorTitle, QR_MODAL_BG);
    m_tft->drawString(STR(WorkshopKey::WORKSHOP_QR_MODAL_TITLE), QR_MODAL_X + QR_MODAL_PAD, QR_MODAL_Y + 18);
    m_tft->unloadFont();

    m_tft->fillRect(QR_MODAL_X + QR_MODAL_PAD, QR_MODAL_SEPARATOR_Y, QR_MODAL_W - 2 * QR_MODAL_PAD, QR_MODAL_SEPARATOR_H, Theme::ColorDialogSeparator);
    m_tft->fillRect(QR_MODAL_QR_X - 4, QR_MODAL_QR_Y - 4, QR_MODAL_QR_SIZE + 8, QR_MODAL_QR_SIZE + 8, TFT_WHITE);
    m_tft->endWrite();

    QRCodeRenderer::Options opt;
    opt.ecc = QRCodeRenderer::Ecc::Medium;
    opt.borderModules = 4;
    opt.fgColor = TFT_BLACK;
    opt.bgColor = TFT_WHITE;
    opt.drawBackground = true;

    const bool ok = QRCodeRenderer::drawCentered(*m_tft, QR_MODAL_QR_X, QR_MODAL_QR_Y, QR_MODAL_QR_SIZE, QR_MODAL_QR_SIZE, url, opt);

    m_tft->startWrite();
    m_tft->setTextDatum(TL_DATUM);
    if (ok)
    {
        const int hintX = QR_MODAL_X + QR_MODAL_PAD;

        m_tft->setFont(&fonts::Font4);
        m_tft->setTextColor(QR_MODAL_HINT_MARK, QR_MODAL_BG);
        m_tft->drawString("*", hintX, QR_MODAL_HINT_Y + QR_MODAL_HINT_MARK_Y_OFFSET);
        const int hintMarkW = m_tft->textWidth("*") + QR_MODAL_HINT_MARK_GAP;
        m_tft->setFont(nullptr);

        char hintText[QR_MODAL_TEXT_BUF_SIZE];
        STRF(hintText, WorkshopKey::WORKSHOP_QR_MODAL_HINT);

        m_tft->loadFont(ComfortaaBold14);
        m_tft->setTextColor(QR_MODAL_TEXT, QR_MODAL_BG);
        drawQrModalText(hintX + hintMarkW, QR_MODAL_HINT_Y, QR_MODAL_W - 2 * QR_MODAL_PAD - hintMarkW, hintText);
        m_tft->unloadFont();
    }
    else
    {
        char errorText[QR_MODAL_TEXT_BUF_SIZE];
        STRF(errorText, WorkshopKey::WORKSHOP_QR_MODAL_ERROR);

        m_tft->loadFont(ComfortaaBold14);
        m_tft->setTextColor(QR_MODAL_TEXT_DIM, QR_MODAL_BG);
        drawQrModalText(QR_MODAL_X + QR_MODAL_PAD, QR_MODAL_HINT_Y, QR_MODAL_W - 2 * QR_MODAL_PAD, errorText);
        m_tft->unloadFont();
    }
    m_tft->endWrite();
}

/**
 * @brief Zeichnet mehrzeiligen Text im QR-Modal.
 */
void Workshop::drawQrModalText(int x, int y, int maxWidth, const char *text)
{
    if (!m_tft || !text || text[0] == '\0')
        return;

    char line[180] = {};
    char word[48] = {};
    int yOffset = 0;
    const int lineHeight = m_tft->fontHeight() + 3;
    const char *pos = text;

    while (*pos)
    {
        int wordLen = 0;
        while (*pos && *pos != ' ' && wordLen < static_cast<int>(sizeof(word)) - 1)
            word[wordLen++] = *pos++;

        word[wordLen] = '\0';
        while (*pos == ' ')
            ++pos;

        if (word[0] == '\0')
            continue;

        char trial[180] = {};
        stringutils::safeCopy(trial, sizeof(trial), line);
        if (trial[0] != '\0')
            stringutils::safeAppend(trial, sizeof(trial), " ");
        stringutils::safeAppend(trial, sizeof(trial), word);

        if (m_tft->textWidth(trial) <= maxWidth)
        {
            stringutils::safeCopy(line, sizeof(line), trial);
        }
        else
        {
            if (line[0] != '\0')
            {
                m_tft->drawString(line, x, y + yOffset);
                yOffset += lineHeight;
            }
            stringutils::safeCopy(line, sizeof(line), word);
        }
    }

    if (line[0] != '\0')
        m_tft->drawString(line, x, y + yOffset);
}

// ===================================================================
// SOUND-AUSGABE
// ===================================================================

/**
 * @brief Reicht den Sound-Pfad an den hinterlegten Callback durch.
 */
void Workshop::playSound(const char *path)
{
    if (!path)
        return;

    if (m_callbacks.onPlaySound)
        m_callbacks.onPlaySound(path);
}

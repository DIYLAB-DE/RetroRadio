#include "Strings_workshop.h"
#include "I18n.h"

#include <pgmspace.h>

// ===================================================================
// PROGMEM-Literale (Deutsch)
// ===================================================================

namespace
{
    // Sonder-Key: leerer Text
    const char DE_EMPTY[] PROGMEM = "";

    // Kategorie-Titel
    const char DE_CAT_MISC_TITLE[]        PROGMEM = "Werkzeuge";
    const char DE_CAT_GAMES_TITLE[]       PROGMEM = "Spiele";
    const char DE_CAT_ELECTRONICS_TITLE[] PROGMEM = "Elektronik";
    const char DE_CAT_UNITS_TITLE[]       PROGMEM = "Einheiten";
    const char DE_CAT_DIGITAL_TITLE[]     PROGMEM = "Digital";
    const char DE_CAT_KNITTING_TITLE[]    PROGMEM = "Stricken";
    const char DE_CAT_MUSIC_TITLE[]       PROGMEM = "Musik";

    // Kategorie-Kurzbeschreibungen
    const char DE_CAT_MISC_SUMMARY[]        PROGMEM = "Referenzen und Sonstiges";
    const char DE_CAT_GAMES_SUMMARY[]       PROGMEM = "Knobel- und Brettspiele";
    const char DE_CAT_ELECTRONICS_SUMMARY[] PROGMEM = "Rechner und Referenzen";
    const char DE_CAT_UNITS_SUMMARY[]       PROGMEM = "Länge, Temp., Druck, SI";
    const char DE_CAT_DIGITAL_SUMMARY[]     PROGMEM = "Bit-Editor, ASCII, Binärwerkzeuge";
    const char DE_CAT_KNITTING_SUMMARY[]    PROGMEM = "Maschen, Garn, Strickfestigkeit";
    const char DE_CAT_MUSIC_SUMMARY[]       PROGMEM = "Tempo, Takt, Stimmung";

    // Werkzeug-Titel
    const char DE_TOOL_CALCULATOR_TITLE[]         PROGMEM = "Rechner";
    const char DE_TOOL_WORLD_CLOCK_TITLE[]        PROGMEM = "Weltzeit-Uhr";
    const char DE_TOOL_COLOR_CONVERTER_TITLE[]    PROGMEM = "Farb-Konverter";
    const char DE_TOOL_PERIODIC_TABLE_TITLE[]     PROGMEM = "Periodensystem";
    const char DE_TOOL_SCREW_REFERENCE_TITLE[]    PROGMEM = "Schrauben";
    const char DE_TOOL_BMI_CALCULATOR_TITLE[]     PROGMEM = "BMI-Rechner";
    const char DE_TOOL_GAME_2048_TITLE[]          PROGMEM = "2048";
    const char DE_TOOL_MILL_GAME_TITLE[]          PROGMEM = "Mühle";
    const char DE_TOOL_LIGHTS_OUT_TITLE[]         PROGMEM = "Lights Out";
    const char DE_TOOL_DICE_TITLE[]               PROGMEM = "Würfel";
    const char DE_TOOL_RESISTOR_TITLE[]           PROGMEM = "Widerstandsfarbcode";
    const char DE_TOOL_OHM_CALCULATOR_TITLE[]     PROGMEM = "Ohmsches Gesetz";
    const char DE_TOOL_VOLTAGE_DIVIDER_TITLE[]    PROGMEM = "Spannungsteiler";
    const char DE_TOOL_LED_CALCULATOR_TITLE[]     PROGMEM = "LED-Vorwiderstand";
    const char DE_TOOL_RC_FILTER_TITLE[]          PROGMEM = "RC/RL-Filter";
    const char DE_TOOL_LC_FILTER_TITLE[]          PROGMEM = "LC-Filter";
    const char DE_TOOL_UKW_RESONATOR_TITLE[]      PROGMEM = "UKW-Schwingkreis";
    const char DE_TOOL_LM317_TITLE[]              PROGMEM = "LM317-Regler";
    const char DE_TOOL_DECIBEL_TITLE[]            PROGMEM = "Dezibel";
    const char DE_TOOL_NE555_TITLE[]              PROGMEM = "NE555-Timer";
    const char DE_TOOL_OPAMP_TITLE[]              PROGMEM = "Op-Amp";
    const char DE_TOOL_INCH_MM_TITLE[]            PROGMEM = "Zoll <-> mm";
    const char DE_TOOL_TEMP_TITLE[]               PROGMEM = "Temperatur";
    const char DE_TOOL_PRESSURE_TITLE[]           PROGMEM = "Druck";
    const char DE_TOOL_SI_PREFIXES_TITLE[]        PROGMEM = "SI-Präfixe";
    const char DE_TOOL_ASCII_TABLE_TITLE[]        PROGMEM = "ASCII-Tabelle";
    const char DE_TOOL_BIT_EDITOR_TITLE[]         PROGMEM = "Bit-Editor";
    const char DE_TOOL_LOGIC_GATE_TITLE[]         PROGMEM = "Logikgatter";
    const char DE_TOOL_STITCH_GAUGE_TITLE[]       PROGMEM = "Maschenprobe";
    const char DE_TOOL_STITCH_DISTRIBUTOR_TITLE[] PROGMEM = "Maschenverteilung";
    const char DE_TOOL_SLEEVE_TAPER_TITLE[]       PROGMEM = "Ärmelschräge";
    const char DE_TOOL_CIRCLE_OF_FIFTHS_TITLE[]   PROGMEM = "Quintenzirkel";
    const char DE_TOOL_METRONOME_TITLE[]          PROGMEM = "Metronom";

    // Werkzeug-Untertitel
    const char DE_TOOL_CALCULATOR_SUBTITLE[]         PROGMEM = "Dez / Hex / Bin (16 Bit)";
    const char DE_TOOL_WORLD_CLOCK_SUBTITLE[]        PROGMEM = "Acht Metropolen, Standardzeit";
    const char DE_TOOL_COLOR_CONVERTER_SUBTITLE[]    PROGMEM = "RGB / HEX / RGB565";
    const char DE_TOOL_PERIODIC_TABLE_SUBTITLE[]     PROGMEM = "Periodensystem der Elemente";
    const char DE_TOOL_SCREW_REFERENCE_SUBTITLE[]    PROGMEM = "Metrisch (DIN 13)";
    const char DE_TOOL_BMI_CALCULATOR_SUBTITLE[]     PROGMEM = "Body Mass Index";
    const char DE_TOOL_GAME_2048_SUBTITLE[]          PROGMEM = "Schiebepuzzle für die Pause";
    const char DE_TOOL_MILL_GAME_SUBTITLE[]          PROGMEM = "Brettspiel für die Pause";
    const char DE_TOOL_LIGHTS_OUT_SUBTITLE[]         PROGMEM = "Knobelspiel für die Pause";
    const char DE_TOOL_DICE_SUBTITLE[]               PROGMEM = "Zufallszahlen aus Hardware-RNG";
    const char DE_TOOL_RESISTOR_SUBTITLE[]           PROGMEM = "4- und 5-Band";
    const char DE_TOOL_OHM_CALCULATOR_SUBTITLE[]     PROGMEM = "U / I / R / P";
    const char DE_TOOL_VOLTAGE_DIVIDER_SUBTITLE[]    PROGMEM = "R1 / R2 / Uout";
    const char DE_TOOL_LED_CALCULATOR_SUBTITLE[]     PROGMEM = "Widerstand, Normwert, Verlustleistung";
    const char DE_TOOL_RC_FILTER_SUBTITLE[]          PROGMEM = "Tiefpass / Hochpass, 1. Ordnung";
    const char DE_TOOL_LC_FILTER_SUBTITLE[]          PROGMEM = "Tiefpass / Hochpass, 2. Ordnung";
    const char DE_TOOL_UKW_RESONATOR_SUBTITLE[]      PROGMEM = "Luftspulen-Resonanzkreis 80...150 MHz";
    const char DE_TOOL_LM317_SUBTITLE[]              PROGMEM = "Vref / R1 / R2 / Vout";
    const char DE_TOOL_DECIBEL_SUBTITLE[]            PROGMEM = "Spannung / Leistung / dB";
    const char DE_TOOL_NE555_SUBTITLE[]              PROGMEM = "Astabile Kippstufe";
    const char DE_TOOL_OPAMP_SUBTITLE[]              PROGMEM = "Invertierender Verstärker";
    const char DE_TOOL_INCH_MM_SUBTITLE[]            PROGMEM = "Bidirektionale Umrechnung";
    const char DE_TOOL_TEMP_SUBTITLE[]               PROGMEM = "C <-> F <-> K";
    const char DE_TOOL_PRESSURE_SUBTITLE[]           PROGMEM = "bar / psi / atm / Pa";
    const char DE_TOOL_SI_PREFIXES_SUBTITLE[]        PROGMEM = "Piko bis Tera - Symbol, Potenz, Beispiel";
    const char DE_TOOL_ASCII_TABLE_SUBTITLE[]        PROGMEM = "Zeichen 0-127 mit DEC / HEX / OCT / BIN";
    const char DE_TOOL_BIT_EDITOR_SUBTITLE[]         PROGMEM = "8-Bit Binär-Editor";
    const char DE_TOOL_LOGIC_GATE_SUBTITLE[]         PROGMEM = "AND / OR / XOR / NAND / NOR / NOT";
    const char DE_TOOL_STITCH_GAUGE_SUBTITLE[]       PROGMEM = "Strickprobe und Zielmaße umrechnen";
    const char DE_TOOL_STITCH_DISTRIBUTOR_SUBTITLE[] PROGMEM = "Zu- und Abnahmen über eine Runde";
    const char DE_TOOL_SLEEVE_TAPER_SUBTITLE[]       PROGMEM = "Zunahmen und Abnahmen über mehrere Reihen";
    const char DE_TOOL_CIRCLE_OF_FIFTHS_SUBTITLE[]   PROGMEM = "Tonarten, Vorzeichen, Stufenharmonik";
    const char DE_TOOL_METRONOME_SUBTITLE[]          PROGMEM = "Tempo und Taktart, optionaler Klick";

    // Abweichende Menübeschriftung für SCREW_REFERENCE
    const char DE_TOOL_SCREW_REFERENCE_LABEL[] PROGMEM = "Schrauben-Referenz";

    // Fehlermeldungen
    const char DE_ERR_HEADER[]           PROGMEM = "[ Fehler ]";
    const char DE_ERR_TOOL_LOAD_FAILED[] PROGMEM = "Werkzeug konnte nicht geladen werden.";

    // Temp-Tool
    const char DE_TEMP_LABEL_CELSIUS[]    PROGMEM = "Celsius";
    const char DE_TEMP_LABEL_FAHRENHEIT[] PROGMEM = "Fahrenh.";
    const char DE_TEMP_LABEL_KELVIN[]     PROGMEM = "Kelvin";

    const char DE_TEMP_UNIT_CELSIUS[]    PROGMEM = "°C";
    const char DE_TEMP_UNIT_FAHRENHEIT[] PROGMEM = "°F";
    const char DE_TEMP_UNIT_KELVIN[]     PROGMEM = "K";

    const char DE_TEMP_REF_M40[] PROGMEM = "-40  Extremkälte";
    const char DE_TEMP_REF_0[]   PROGMEM = "0  Gefrierpunkt";
    const char DE_TEMP_REF_20[]  PROGMEM = "20  Raumtemp.";
    const char DE_TEMP_REF_37[]  PROGMEM = "37  Körpertemp.";
    const char DE_TEMP_REF_100[] PROGMEM = "100  Siedepunkt";
    const char DE_TEMP_REF_150[] PROGMEM = "150";
    const char DE_TEMP_REF_200[] PROGMEM = "200";

    const char DE_TEMP_FORMULA_C2F[] PROGMEM = "°F = °C x 9/5 + 32";
    const char DE_TEMP_FORMULA_F2C[] PROGMEM = "°C = (°F - 32) x 5/9";
    const char DE_TEMP_FORMULA_K2C[] PROGMEM = "°C = K - 273,15";

    // InchMm-Tool
    const char DE_INCHMM_LABEL_INCH[] PROGMEM = "Zoll";
    const char DE_INCHMM_LABEL_MM[]   PROGMEM = "mm";

    const char DE_INCHMM_UNIT_INCH[] PROGMEM = "in";
    const char DE_INCHMM_UNIT_MM[]   PROGMEM = "mm";

    const char DE_INCHMM_FRAC_1_16[]  PROGMEM = "1/16\"";
    const char DE_INCHMM_FRAC_1_8[]   PROGMEM = "1/8\"";
    const char DE_INCHMM_FRAC_3_16[]  PROGMEM = "3/16\"";
    const char DE_INCHMM_FRAC_1_4[]   PROGMEM = "1/4\"";
    const char DE_INCHMM_FRAC_5_16[]  PROGMEM = "5/16\"";
    const char DE_INCHMM_FRAC_3_8[]   PROGMEM = "3/8\"";
    const char DE_INCHMM_FRAC_7_16[]  PROGMEM = "7/16\"";
    const char DE_INCHMM_FRAC_1_2[]   PROGMEM = "1/2\"";
    const char DE_INCHMM_FRAC_9_16[]  PROGMEM = "9/16\"";
    const char DE_INCHMM_FRAC_5_8[]   PROGMEM = "5/8\"";
    const char DE_INCHMM_FRAC_11_16[] PROGMEM = "11/16\"";
    const char DE_INCHMM_FRAC_3_4[]   PROGMEM = "3/4\"";
    const char DE_INCHMM_FRAC_13_16[] PROGMEM = "13/16\"";
    const char DE_INCHMM_FRAC_7_8[]   PROGMEM = "7/8\"";
    const char DE_INCHMM_FRAC_15_16[] PROGMEM = "15/16\"";
    const char DE_INCHMM_FRAC_1[]     PROGMEM = "1\"";

    const char DE_INCHMM_INFO_FACTOR[] PROGMEM = "1 Zoll = 25,4 mm";

    // Pressure-Tool
    const char DE_PRESSURE_UNIT_BAR[] PROGMEM = "bar";
    const char DE_PRESSURE_UNIT_PSI[] PROGMEM = "psi";
    const char DE_PRESSURE_UNIT_ATM[] PROGMEM = "atm";
    const char DE_PRESSURE_UNIT_PA[]  PROGMEM = "Pa";

    const char DE_PRESSURE_REF_HDR_APPLICATION[] PROGMEM = "Anwendung";

    const char DE_PRESSURE_REF_BLOOD[]      PROGMEM = "Blutdruck (systol.)";
    const char DE_PRESSURE_REF_ATMOSPHERE[] PROGMEM = "Atmosphäre (1 atm)";
    const char DE_PRESSURE_REF_TYRE_CAR[]   PROGMEM = "Reifendruck PKW";
    const char DE_PRESSURE_REF_TYRE_TRUCK[] PROGMEM = "Reifendruck LKW";
    const char DE_PRESSURE_REF_PNEUMATIC[]  PROGMEM = "Pneumatik Werkstatt";
    const char DE_PRESSURE_REF_WATER_PIPE[] PROGMEM = "Wasserleitung";
    const char DE_PRESSURE_REF_DIVE_TANK[]  PROGMEM = "Tauchflasche";
    const char DE_PRESSURE_REF_HYDRAULIC[]  PROGMEM = "Hydraulikpresse";

    const char DE_PRESSURE_INFO_BAR[] PROGMEM = "1 bar = 100.000 Pa";
    const char DE_PRESSURE_INFO_PSI[] PROGMEM = "1 psi = 6.894,757 Pa";
    const char DE_PRESSURE_INFO_ATM[] PROGMEM = "1 atm = 101.325 Pa";
    const char DE_PRESSURE_INFO_PA[]  PROGMEM = "1 Pa = 0,00001 bar";

    // WorldClock-Tool
    const char DE_WORLDCLOCK_CITY_LOS_ANGELES[] PROGMEM = "Los Angeles";
    const char DE_WORLDCLOCK_CITY_NEW_YORK[]    PROGMEM = "New York";
    const char DE_WORLDCLOCK_CITY_LONDON[]      PROGMEM = "London";
    const char DE_WORLDCLOCK_CITY_BERLIN[]      PROGMEM = "Berlin";
    const char DE_WORLDCLOCK_CITY_MOSCOW[]      PROGMEM = "Moskau";
    const char DE_WORLDCLOCK_CITY_DUBAI[]       PROGMEM = "Dubai";
    const char DE_WORLDCLOCK_CITY_TOKYO[]       PROGMEM = "Tokio";
    const char DE_WORLDCLOCK_CITY_SYDNEY[]      PROGMEM = "Sydney";

    // StitchDistributor-Tool
    const char DE_STITCH_DISTRIBUTOR_LABEL_FROM[]    PROGMEM = "Von";
    const char DE_STITCH_DISTRIBUTOR_LABEL_TO[]      PROGMEM = "Auf";
    const char DE_STITCH_DISTRIBUTOR_UNIT_STITCHES[] PROGMEM = "M";

    const char DE_STITCH_DISTRIBUTOR_TITLE_DISTRIBUTION[] PROGMEM = "Verteilung";
    const char DE_STITCH_DISTRIBUTOR_TITLE_NO_CHANGE[]    PROGMEM = "Keine Änderung";
    const char DE_STITCH_DISTRIBUTOR_TITLE_INCREASE[]     PROGMEM = "Zunahme";
    const char DE_STITCH_DISTRIBUTOR_TITLE_DECREASE[]     PROGMEM = "Abnahme";

    const char DE_STITCH_DISTRIBUTOR_ACTION_INC_WITH_PLAIN[] PROGMEM = "dann 1 Masche zunehmen";
    const char DE_STITCH_DISTRIBUTOR_ACTION_DEC_WITH_PLAIN[] PROGMEM = "dann 2 Maschen zusammen";
    const char DE_STITCH_DISTRIBUTOR_ACTION_INC_NO_PLAIN[]   PROGMEM = "1 Masche zunehmen";
    const char DE_STITCH_DISTRIBUTOR_ACTION_DEC_NO_PLAIN[]   PROGMEM = "2 Maschen zusammen";

    const char DE_STITCH_DISTRIBUTOR_HINT_EMPTY_1[]      PROGMEM = "Bitte Maschenzahlen";
    const char DE_STITCH_DISTRIBUTOR_HINT_EMPTY_2[]      PROGMEM = "eingeben.";
    const char DE_STITCH_DISTRIBUTOR_HINT_NO_CHANGE_1[]  PROGMEM = "Maschenzahl bleibt gleich.";
    const char DE_STITCH_DISTRIBUTOR_HINT_NO_CHANGE_2[]  PROGMEM = "Runde gerade stricken.";
    const char DE_STITCH_DISTRIBUTOR_HINT_INFEASIBLE_1[] PROGMEM = "Differenz zu groß";
    const char DE_STITCH_DISTRIBUTOR_HINT_INFEASIBLE_2[] PROGMEM = "für eine einzelne Runde.";

    const char DE_STITCH_DISTRIBUTOR_FMT_SEG_WITH_PLAIN[] PROGMEM = "{0:%d} mal Strick {1:%d},";
    const char DE_STITCH_DISTRIBUTOR_FMT_SEG_NO_PLAIN[]   PROGMEM = "{0:%d} mal:";

    // SleeveTaper-Tool
    const char DE_SLEEVE_TAPER_LABEL_FROM[]    PROGMEM = "Von";
    const char DE_SLEEVE_TAPER_LABEL_TO[]      PROGMEM = "Auf";
    const char DE_SLEEVE_TAPER_LABEL_OVER[]    PROGMEM = "Über";
    const char DE_SLEEVE_TAPER_UNIT_STITCHES[] PROGMEM = "M";
    const char DE_SLEEVE_TAPER_UNIT_ROWS[]     PROGMEM = "R";

    const char DE_SLEEVE_TAPER_TITLE_SLEEVE[]   PROGMEM = "Ärmelschräge";
    const char DE_SLEEVE_TAPER_TITLE_NO_TAPER[] PROGMEM = "Keine Schräge";
    const char DE_SLEEVE_TAPER_TITLE_INCREASE[] PROGMEM = "Zunahme";
    const char DE_SLEEVE_TAPER_TITLE_DECREASE[] PROGMEM = "Abnahme";

    const char DE_SLEEVE_TAPER_ACTION_INC_BOTH[] PROGMEM = "beidseitig 1 M zunehmen";
    const char DE_SLEEVE_TAPER_ACTION_DEC_BOTH[] PROGMEM = "beidseitig 1 M abnehmen";

    const char DE_SLEEVE_TAPER_HINT_EMPTY_1[]      PROGMEM = "Bitte alle Werte";
    const char DE_SLEEVE_TAPER_HINT_EMPTY_2[]      PROGMEM = "eingeben.";
    const char DE_SLEEVE_TAPER_HINT_NO_CHANGE_1[]  PROGMEM = "Maschenzahl bleibt gleich.";
    const char DE_SLEEVE_TAPER_HINT_NO_CHANGE_2[]  PROGMEM = "Keine Schräge nötig.";
    const char DE_SLEEVE_TAPER_HINT_ODD_1[]        PROGMEM = "Differenz ist ungerade.";
    const char DE_SLEEVE_TAPER_HINT_ODD_2[]        PROGMEM = "Bitte Anfang oder Ende";
    const char DE_SLEEVE_TAPER_HINT_ODD_3[]        PROGMEM = "um 1 anpassen.";
    const char DE_SLEEVE_TAPER_HINT_TOO_MANY_INC[] PROGMEM = "Zu viele Zunahmen";
    const char DE_SLEEVE_TAPER_HINT_TOO_MANY_DEC[] PROGMEM = "Zu viele Abnahmen";
    const char DE_SLEEVE_TAPER_HINT_TOO_MANY_2[]   PROGMEM = "für diese Reihen-Anzahl.";

    const char DE_SLEEVE_TAPER_FMT_ROWS[]     PROGMEM = "{0:%d} mal alle {1:%d}. Reihe";
    const char DE_SLEEVE_TAPER_FMT_ROWS_AND[] PROGMEM = "{0:%d} mal alle {1:%d}. Reihe und";

    // StitchGauge-Tool
    const char DE_STITCH_GAUGE_LABEL_STITCHES_PER_10CM[] PROGMEM = "M / 10 cm";
    const char DE_STITCH_GAUGE_LABEL_WIDTH[]             PROGMEM = "Breite";
    const char DE_STITCH_GAUGE_LABEL_STITCHES[]          PROGMEM = "Maschen";
    const char DE_STITCH_GAUGE_LABEL_ROWS_PER_10CM[]     PROGMEM = "R / 10 cm";
    const char DE_STITCH_GAUGE_LABEL_HEIGHT[]            PROGMEM = "Höhe";
    const char DE_STITCH_GAUGE_LABEL_ROWS[]              PROGMEM = "Reihen";

    const char DE_STITCH_GAUGE_UNIT_STITCHES[] PROGMEM = "M";
    const char DE_STITCH_GAUGE_UNIT_CM[]       PROGMEM = "cm";
    const char DE_STITCH_GAUGE_UNIT_ROWS[]     PROGMEM = "R";

    const char DE_STITCH_GAUGE_VIS_CM[]              PROGMEM = "{0:%.4g} cm";
    const char DE_STITCH_GAUGE_VIS_STITCHES[]        PROGMEM = "{0:%.4g} M";
    const char DE_STITCH_GAUGE_VIS_ROWS[]            PROGMEM = "{0:%.4g} R";
    const char DE_STITCH_GAUGE_VIS_STITCHES_PER_10[] PROGMEM = "{0:%.4g} M / 10 cm";
    const char DE_STITCH_GAUGE_VIS_ROWS_PER_10[]     PROGMEM = "{0:%.4g} R / 10 cm";

    // BmiCalc-Tool
    const char DE_BMI_ZONE_SEVERE_UNDER[] PROGMEM = "Starkes Untergewicht";
    const char DE_BMI_ZONE_UNDER[]        PROGMEM = "Untergewicht";
    const char DE_BMI_ZONE_NORMAL[]       PROGMEM = "Normalgewicht";
    const char DE_BMI_ZONE_OVER[]         PROGMEM = "Übergewicht";
    const char DE_BMI_ZONE_OBESE_1[]      PROGMEM = "Adipositas Grad I";
    const char DE_BMI_ZONE_OBESE_2[]      PROGMEM = "Adipositas Grad II";
    const char DE_BMI_ZONE_OBESE_3[]      PROGMEM = "Adipositas Grad III";

    const char DE_BMI_LABEL_HEIGHT[] PROGMEM = "Größe";
    const char DE_BMI_LABEL_WEIGHT[] PROGMEM = "Gewicht";

    const char DE_BMI_UNIT_CM[] PROGMEM = "cm";
    const char DE_BMI_UNIT_KG[] PROGMEM = "kg";

    const char DE_BMI_NOTE_ADULTS[] PROGMEM = "Gilt für Erwachsene von 19 bis 65.";

    const char DE_BMI_FMT_RESULT[] PROGMEM = "BMI = {0}";

    // OhmCalculator-Tool
    const char DE_OHM_NAME_VOLTAGE[]    PROGMEM = "(Spannung)";
    const char DE_OHM_NAME_CURRENT[]    PROGMEM = "(Strom)";
    const char DE_OHM_NAME_RESISTANCE[] PROGMEM = "(Widerstand)";
    const char DE_OHM_NAME_POWER[]      PROGMEM = "(Leistung)";

    // Decibel-Tool
    const char DE_DECIBEL_SECTION_VOLTAGE[] PROGMEM = "Spannung";
    const char DE_DECIBEL_SECTION_POWER[]   PROGMEM = "Leistung";

    const char DE_DECIBEL_NAME_INPUT[]  PROGMEM = "(Eingang)";
    const char DE_DECIBEL_NAME_OUTPUT[] PROGMEM = "(Ausgang)";
    const char DE_DECIBEL_NAME_LEVEL[]  PROGMEM = "(Pegel)";

    // LogicGate-Tool
    const char DE_LOGIC_GATE_DESC_AND[]  PROGMEM = "AND - Konjunktion";
    const char DE_LOGIC_GATE_DESC_OR[]   PROGMEM = "OR - Disjunktion";
    const char DE_LOGIC_GATE_DESC_XOR[]  PROGMEM = "XOR - Exklusiv-Oder";
    const char DE_LOGIC_GATE_DESC_NAND[] PROGMEM = "NAND - Negierte Konjunktion";
    const char DE_LOGIC_GATE_DESC_NOR[]  PROGMEM = "NOR - Negierte Disjunktion";
    const char DE_LOGIC_GATE_DESC_NOT[]  PROGMEM = "NOT - Inverter";

    // RcFilter-Tool
    const char DE_RC_FILTER_BTN_LOW_PASS[]  PROGMEM = "tiefpass";
    const char DE_RC_FILTER_BTN_HIGH_PASS[] PROGMEM = "hochpass";

    const char DE_RC_FILTER_SUB_LP_RC[] PROGMEM = "Tiefpass RC, 1. Ordnung";
    const char DE_RC_FILTER_SUB_LP_RL[] PROGMEM = "Tiefpass RL, 1. Ordnung";
    const char DE_RC_FILTER_SUB_HP_RC[] PROGMEM = "Hochpass RC, 1. Ordnung";
    const char DE_RC_FILTER_SUB_HP_RL[] PROGMEM = "Hochpass RL, 1. Ordnung";

    const char DE_RC_FILTER_SLOPE_LP[] PROGMEM = "-20 dB/Dek";
    const char DE_RC_FILTER_SLOPE_HP[] PROGMEM = "+20 dB/Dek";

    // LcFilter-Tool
    const char DE_LC_FILTER_BTN_LOW_PASS[]  PROGMEM = "tiefpass";
    const char DE_LC_FILTER_BTN_HIGH_PASS[] PROGMEM = "hochpass";

    const char DE_LC_FILTER_SUB_LP[] PROGMEM = "Tiefpass LC, 2. Ordnung";
    const char DE_LC_FILTER_SUB_HP[] PROGMEM = "Hochpass LC, 2. Ordnung";

    const char DE_LC_FILTER_SLOPE_LP[] PROGMEM = "-40 dB/Dek";
    const char DE_LC_FILTER_SLOPE_HP[] PROGMEM = "+40 dB/Dek";

    // UkwResonator-Tool
    const char DE_UKW_RESONATOR_SUB_D05[] PROGMEM = "Luftspule, d = 0,5 mm Cu";
    const char DE_UKW_RESONATOR_SUB_D08[] PROGMEM = "Luftspule, d = 0,8 mm Cu";
    const char DE_UKW_RESONATOR_SUB_D10[] PROGMEM = "Luftspule, d = 1,0 mm Cu";
    const char DE_UKW_RESONATOR_SUB_D15[] PROGMEM = "Luftspule, d = 1,5 mm Cu";

    const char DE_UKW_RESONATOR_FMT_TURNS[]         PROGMEM = "N = {0} Wdg.";
    const char DE_UKW_RESONATOR_FMT_Q_APPROX[]      PROGMEM = "Q ca. {0:%d}";
    const char DE_UKW_RESONATOR_WARN_OVERLAP[]      PROGMEM = "Wdg. überlappen!";
    const char DE_UKW_RESONATOR_WARN_OUT_OF_RANGE[] PROGMEM = "außerhalb!";

    // Ne555-Tool
    const char DE_NE555_BTN_ASTABLE[]    PROGMEM = "astabil";
    const char DE_NE555_BTN_MONOSTABLE[] PROGMEM = "monostabil";

    const char DE_NE555_SUB_ASTABLE[]    PROGMEM = "Astabile Kippstufe";
    const char DE_NE555_SUB_MONOSTABLE[] PROGMEM = "Monostabile Kippstufe";

    // OpAmp-Tool
    const char DE_OPAMP_BTN_INVERTING[]     PROGMEM = "invertierend";
    const char DE_OPAMP_BTN_NON_INVERTING[] PROGMEM = "nicht invertierend";

    const char DE_OPAMP_SUB_INVERTING[]     PROGMEM = "Invertierender Verstärker";
    const char DE_OPAMP_SUB_NON_INVERTING[] PROGMEM = "Nicht invertierender Verstärker";

    // SiPrefixes-Tool
    const char DE_SI_PREFIXES_HDR_PREFIX[]  PROGMEM = "Präfix";
    const char DE_SI_PREFIXES_HDR_SYMBOL[]  PROGMEM = "Symbol";
    const char DE_SI_PREFIXES_HDR_POWER[]   PROGMEM = "Potenz";
    const char DE_SI_PREFIXES_HDR_EXAMPLE[] PROGMEM = "Beispiel";

    const char DE_SI_PREFIXES_HINT_TAP[] PROGMEM = "Präfix antippen für Umrechnungshinweis";

    const char DE_SI_PREFIXES_NAME_TERA[]  PROGMEM = "Tera";
    const char DE_SI_PREFIXES_NAME_GIGA[]  PROGMEM = "Giga";
    const char DE_SI_PREFIXES_NAME_MEGA[]  PROGMEM = "Mega";
    const char DE_SI_PREFIXES_NAME_KILO[]  PROGMEM = "Kilo";
    const char DE_SI_PREFIXES_NAME_BASE[]  PROGMEM = "Basis";
    const char DE_SI_PREFIXES_NAME_MILLI[] PROGMEM = "Milli";
    const char DE_SI_PREFIXES_NAME_MICRO[] PROGMEM = "Mikro";
    const char DE_SI_PREFIXES_NAME_NANO[]  PROGMEM = "Nano";
    const char DE_SI_PREFIXES_NAME_PICO[]  PROGMEM = "Piko";

    // ScrewRef-Tool
    const char DE_SCREW_REF_BTN_COARSE[] PROGMEM = "metrisch";
    const char DE_SCREW_REF_BTN_FINE[]   PROGMEM = "fein";

    const char DE_SCREW_REF_SUB_COARSE[] PROGMEM = "Metrisch (DIN 13)";
    const char DE_SCREW_REF_SUB_FINE[]   PROGMEM = "Feingewinde (DIN 13)";

    const char DE_SCREW_REF_FMT_TITLE_COARSE[] PROGMEM = "{0} x {1:%.4g}";
    const char DE_SCREW_REF_FMT_TITLE_FINE[]   PROGMEM = "{0} x {1:%.4g} (fein)";

    const char DE_SCREW_REF_NO_FINE_THREAD[] PROGMEM = "kein Feingewinde";

    const char DE_SCREW_REF_LABEL_PITCH[]       PROGMEM = "Steigung";
    const char DE_SCREW_REF_LABEL_TAPPING[]     PROGMEM = "Kernloch-Bohrung";
    const char DE_SCREW_REF_LABEL_CLEARANCE[]   PROGMEM = "Durchgangsbohrung";
    const char DE_SCREW_REF_LABEL_AF_SOCKET[]   PROGMEM = "SW Inbus (DIN 912)";
    const char DE_SCREW_REF_LABEL_AF_HEX[]      PROGMEM = "SW Sechskant (DIN 934)";
    const char DE_SCREW_REF_LABEL_HEAD_HEIGHT[] PROGMEM = "Kopfhöhe (DIN 912)";
    const char DE_SCREW_REF_LABEL_HEAD_DIA[]    PROGMEM = "Kopf-Ø (DIN 912)";
    const char DE_SCREW_REF_LABEL_TORQUE[]      PROGMEM = "Drehmoment";

    const char DE_SCREW_REF_FMT_DIM_SW[]    PROGMEM = "SW {0:%.4g}";
    const char DE_SCREW_REF_FMT_DIM_DRILL[] PROGMEM = "Bohrung {0:%.4g}";

    // ColorConverter-Tool
    const char DE_COLOR_CONVERTER_RGB_HINT[] PROGMEM = "Display-RGB nach der RGB565-Quantisierung";

    // AsciiTable-Tool
    const char DE_ASCII_TABLE_SUB_EXTENDED[] PROGMEM = "Zeichen 128-255 mit DEC / HEX / OCT / BIN";
    const char DE_ASCII_TABLE_HINT_TAP[]     PROGMEM = "Tippe auf ein Zeichen";
    const char DE_ASCII_TABLE_NOT_DEFINED[]  PROGMEM = "nicht definiert";

    // BitEditor-Tool
    const char DE_BIT_EDITOR_SUB_8[]  PROGMEM = "8-Bit Binär-Editor";
    const char DE_BIT_EDITOR_SUB_16[] PROGMEM = "16-Bit Binär-Editor";
    const char DE_BIT_EDITOR_SUB_32[] PROGMEM = "32-Bit Binär-Editor";

    // Calculator-Tool
    const char DE_CALCULATOR_ERROR[] PROGMEM = "Fehler";

    // Gemeinsame Dialogschaltflächen
    const char DE_DIALOG_CLOSE[]  PROGMEM = "schließen";
    const char DE_DIALOG_OK[]     PROGMEM = "ok";
    const char DE_DIALOG_CANCEL[] PROGMEM = "abbruch";

    // Metronom-Tool
    const char DE_METRONOME_HELP_TITLE[] PROGMEM = "METRONOM - BEDIENUNG";
    const char DE_METRONOME_HELP_TEXT[]  PROGMEM =
        "ANZEIGE: Die Punkte zeigen die Schläge im Takt. Der aktuelle Schlag leuchtet hell, der erste Schlag ist farblich hervorgehoben. In der Mitte stehen das Tempo in Schlägen pro Minute und die italienische Tempo-Bezeichnung.\n\n"
        "TEMPO: Mit - und + das Tempo ändern. Mit 'tap' mehrfach im gewünschten Tempo tippen, das Metronom berechnet den Wert automatisch.\n\n"
        "TAKT: Die Taktart wechselt zwischen 2/4, 3/4, 4/4 und 6/8.\n\n"
        "START: Mit 'start' das Metronom starten, mit 'stop' anhalten. Tippen auf die Anzeige startet und stoppt ebenfalls.";
    const char DE_METRONOME_HINT_TAP_TO_START[] PROGMEM = "Tippen zum Starten";

    // Quintenzirkel-Tool
    const char DE_CIRCLE_OF_FIFTHS_FUNC_TONIC[]       PROGMEM = "Tonika";
    const char DE_CIRCLE_OF_FIFTHS_FUNC_SUBDOMINANT[] PROGMEM = "Subdominante";
    const char DE_CIRCLE_OF_FIFTHS_FUNC_DOMINANT[]    PROGMEM = "Dominante";
    const char DE_CIRCLE_OF_FIFTHS_FUNC_PARALLEL[]    PROGMEM = "T-Parallele";

    const char DE_CIRCLE_OF_FIFTHS_SUFFIX_MAJOR[]   PROGMEM = "Dur";
    const char DE_CIRCLE_OF_FIFTHS_HEADER_SCALE[]   PROGMEM = "Tonleiter";
    const char DE_CIRCLE_OF_FIFTHS_HEADER_DEGREES[] PROGMEM = "Stufen";

    const char DE_CIRCLE_OF_FIFTHS_FMT_HEADER_KEY[] PROGMEM = "{0}-Dur";

    const char DE_CIRCLE_OF_FIFTHS_NO_ACCIDENTALS[] PROGMEM = "ohne Vorzeichen";
    const char DE_CIRCLE_OF_FIFTHS_UNIT_SHARP_SG[]  PROGMEM = "Kreuz";
    const char DE_CIRCLE_OF_FIFTHS_UNIT_SHARP_PL[]  PROGMEM = "Kreuze";
    const char DE_CIRCLE_OF_FIFTHS_UNIT_FLAT_SG[]   PROGMEM = "b";
    const char DE_CIRCLE_OF_FIFTHS_UNIT_FLAT_PL[]   PROGMEM = "Bs";

    const char DE_CIRCLE_OF_FIFTHS_FMT_SUB_NONE[]      PROGMEM = "{0}-Dur (keine Vorzeichen)";
    const char DE_CIRCLE_OF_FIFTHS_FMT_SUB_ONE_SHARP[] PROGMEM = "{0}-Dur (1 Kreuz)";
    const char DE_CIRCLE_OF_FIFTHS_FMT_SUB_ONE_FLAT[]  PROGMEM = "{0}-Dur (1 b)";
    const char DE_CIRCLE_OF_FIFTHS_FMT_SUB_N_SHARPS[]  PROGMEM = "{0}-Dur ({1:%d} Kreuze)";
    const char DE_CIRCLE_OF_FIFTHS_FMT_SUB_N_FLATS[]   PROGMEM = "{0}-Dur ({1:%d} Bs)";

    // Würfelwerkzeug
    const char DE_DICE_HELP_TITLE[] PROGMEM = "WÜRFEL - ANLEITUNG";
    const char DE_DICE_HELP_TEXT[]  PROGMEM =
        "Echte Zufallszahlen für externe Spiele.\n\n"
        "WERFEN: Tippe den Würfel.\n\n"
        "UMSCHALTEN: Footer-Buttons wechseln zwischen einem und zwei Würfeln. Im 2er-Modus zeigt die Karte die Summe.\n\n"
        "ZUFALL: Die Verteilung der Augenzahlen ist exakt gleichmäßig.\n\n"
        "HISTORIE: Die zehn letzten Würfe. Werden nicht gespeichert.";
    const char DE_DICE_BTN_ONE[]       PROGMEM = "1 Würfel";
    const char DE_DICE_BTN_TWO[]       PROGMEM = "2 Würfel";
    const char DE_DICE_LABEL_RESULT[]  PROGMEM = "ERGEBNIS";
    const char DE_DICE_LABEL_HISTORY[] PROGMEM = "HISTORIE";
    const char DE_DICE_HISTORY_EMPTY[] PROGMEM = "noch keine Würfe";
    const char DE_DICE_SUB_ROLLING[]   PROGMEM = "Würfeln...";
    const char DE_DICE_SUB_ONE_DIE[]   PROGMEM = "Ein Würfel";
    const char DE_DICE_SUB_TWO_DICE[]  PROGMEM = "Zwei Würfel";

    // 2048-Tool
    const char DE_GAME_2048_HELP_TITLE[]  PROGMEM = "2048 - SPIELANLEITUNG";
    const char DE_GAME_2048_HELP_TEXT[]   PROGMEM =
        "Ziel: Zwei gleiche Kacheln verschmelzen zur doppelten Summe. "
        "Erreiche die 2048!\n\n"
        "Steuerung: Wische nach oben, unten, links oder rechts. Alle "
        "Kacheln rutschen gemeinsam in die gewählte Richtung.\n\n"
        "Verschmelzen: Benachbarte Kacheln mit gleichem Wert werden zu "
        "einer Kachel mit doppelter Summe. Jede Kachel verschmilzt pro "
        "Zug nur einmal.\n\n"
        "Neue Kachel: Nach jedem Zug erscheint zufällig eine 2 (meistens) "
        "oder eine 4.\n\n"
        "Ende: Game Over, wenn kein Zug mehr möglich ist.";
    const char DE_GAME_2048_RESET_TITLE[] PROGMEM = "BESTWERT ZURÜCKSETZEN?";
    const char DE_GAME_2048_RESET_TEXT[]  PROGMEM =
        "Der gespeicherte Bestwert wird unwiderruflich auf den aktuellen "
        "Punktestand gesenkt. Die laufende Partie bleibt davon unberührt.";
    const char DE_GAME_2048_SUB_PLAYING[]  PROGMEM = "Wische zum Verschieben der Kacheln";
    const char DE_GAME_2048_SUB_WON[]      PROGMEM = "2048 erreicht - weiter oder neu?";
    const char DE_GAME_2048_SUB_GAMEOVER[] PROGMEM = "Brett voll - neue Runde starten";
    const char DE_GAME_2048_LABEL_SCORE[]  PROGMEM = "Punkte";
    const char DE_GAME_2048_LABEL_BEST[]   PROGMEM = "Bestwert";
    const char DE_GAME_2048_LABEL_HINT[]   PROGMEM = "Wischen";
    const char DE_GAME_2048_OVERLAY_WON_TITLE[]  PROGMEM = "GESCHAFFT!";
    const char DE_GAME_2048_OVERLAY_WON_LINE2[]  PROGMEM = "2048 erreicht";
    const char DE_GAME_2048_OVERLAY_LOST_TITLE[] PROGMEM = "Game Over";
    const char DE_GAME_2048_OVERLAY_LOST_LINE2[] PROGMEM = "Keine Züge mehr";
    const char DE_GAME_2048_BTN_NEW[]      PROGMEM = "neu";
    const char DE_GAME_2048_BTN_RESET[]    PROGMEM = "reset";
    const char DE_GAME_2048_BTN_CONTINUE[] PROGMEM = "weiter";

    // Lights-Out-Tool
    const char DE_LIGHTS_OUT_HELP_TITLE[]  PROGMEM = "LIGHTS OUT - SPIELANLEITUNG";
    const char DE_LIGHTS_OUT_HELP_TEXT[]   PROGMEM =
        "Ziel: Alle 25 Lichter ausschalten.\n\n"
        "TIPPEN: Ein Tap invertiert die Zelle und ihre 4 Nachbarn. Diagonalen bleiben.\n\n"
        "SCHWIERIGKEIT: leicht/mittel/schwer = 3/7/12 zufällige Taps zum Verwürfeln. Stets lösbar.\n\n"
        "BEWERTUNG: Züge und Zeit ab erstem Tap. Pro Stufe werden gelöste Partien und Bestzeit gespeichert.\n\n"
        "NEU: Neue Partie auf aktueller Stufe.\n"
        "RESET: Setzt nach Bestätigung alle Statistiken zurück.";
    const char DE_LIGHTS_OUT_RESET_TITLE[] PROGMEM = "Statistik zurücksetzen";
    const char DE_LIGHTS_OUT_RESET_TEXT[]  PROGMEM = "Sollen die gespeicherten Lösungen und Bestzeiten wirklich gelöscht werden?";
    const char DE_LIGHTS_OUT_DIFF_EASY[]   PROGMEM = "leicht";
    const char DE_LIGHTS_OUT_DIFF_MEDIUM[] PROGMEM = "mittel";
    const char DE_LIGHTS_OUT_DIFF_HARD[]   PROGMEM = "schwer";
    const char DE_LIGHTS_OUT_LABEL_MOVES[]    PROGMEM = "Züge";
    const char DE_LIGHTS_OUT_LABEL_TIME[]     PROGMEM = "Zeit";
    const char DE_LIGHTS_OUT_LABEL_STATS[]    PROGMEM = "Statistik";
    const char DE_LIGHTS_OUT_STATUS_SOLVED[]  PROGMEM = "Gelöst!";
    const char DE_LIGHTS_OUT_STATUS_READY[]   PROGMEM = "Bereit";
    const char DE_LIGHTS_OUT_STATUS_PLAYING[] PROGMEM = "Spielen";
    const char DE_LIGHTS_OUT_SUB_TITLE[]      PROGMEM = "Lights Out";
    const char DE_LIGHTS_OUT_FMT_SUB_DIFFICULTY[] PROGMEM = "Schwierigkeit: {0}";
    const char DE_LIGHTS_OUT_BTN_NEW[]        PROGMEM = "neu";
    const char DE_LIGHTS_OUT_BTN_RESET[]      PROGMEM = "reset";

    // Mühlewerkzeug
    const char DE_MILL_SUB_PLACING[] PROGMEM = "Setzphase - tippe einen freien Punkt";
    const char DE_MILL_SUB_MOVING[]  PROGMEM = "Zugphase - wähle einen Stein und ziehe";
    const char DE_MILL_SUB_FLYING[]  PROGMEM = "Sprungphase - du darfst auf jeden freien Punkt";
    const char DE_MILL_SUB_CAPTURE[] PROGMEM = "Du musst einen schwarzen Stein schlagen";
    const char DE_MILL_SUB_BOT[]     PROGMEM = "Der Bot ist am Zug";
    const char DE_MILL_SUB_WON[]     PROGMEM = "Du hast gewonnen";
    const char DE_MILL_SUB_LOST[]    PROGMEM = "Der Bot hat gewonnen";
    const char DE_MILL_SUB_DRAW[]    PROGMEM = "Remis";

    const char DE_MILL_HELP_TITLE[] PROGMEM = "MÜHLE - SPIELANLEITUNG";
    const char DE_MILL_HELP_TEXT[]  PROGMEM =
        "Jeder Spieler hat neun Steine. Weniger als drei oder kein "
        "Zug möglich = verloren.\n\n"
        "SETZEN: Tippe neun freie Punkte.\n\n"
        "ZIEHEN: Stein wählen (blauer Ring), dann freies Nachbarfeld "
        "(grüner Halo).\n\n"
        "SPRINGEN: Mit nur noch drei Steinen auf jedes freie Feld.\n\n"
        "MÜHLE: Drei Steine in einer Reihe schlagen einen Bot-Stein "
        "(rot markiert). Steine in Mühlen sind tabu.";

    const char DE_MILL_RESET_TITLE[] PROGMEM = "BILANZ ZURÜCKSETZEN?";
    const char DE_MILL_RESET_TEXT[]  PROGMEM =
        "Die gespeicherte Bilanz (Siege, Niederlagen und Remis) wird "
        "unwiderruflich auf null gesetzt. Die laufende Partie bleibt "
        "davon unberührt.";

    const char DE_MILL_LABEL_PHASE[]  PROGMEM = "Phase";
    const char DE_MILL_LABEL_TURN[]   PROGMEM = "Am Zug";
    const char DE_MILL_LABEL_STONES[] PROGMEM = "Steine";
    const char DE_MILL_LABEL_WINS[]   PROGMEM = "Siege";
    const char DE_MILL_LABEL_LOSSES[] PROGMEM = "Niederlagen";
    const char DE_MILL_LABEL_DRAWS[]  PROGMEM = "Remis";

    const char DE_MILL_HINT_DEFAULT[]      PROGMEM = "...";
    const char DE_MILL_HINT_WON[]          PROGMEM = "Du hast gewonnen!";
    const char DE_MILL_HINT_LOST[]         PROGMEM = "Der Bot hat gewonnen.";
    const char DE_MILL_HINT_DRAW[]         PROGMEM = "Unentschieden.";
    const char DE_MILL_HINT_CAPTURE[]      PROGMEM = "Schlage einen Bot-Stein";
    const char DE_MILL_HINT_BOT_THINKING[] PROGMEM = "Bot denkt...";
    const char DE_MILL_HINT_TAP_FREE[]     PROGMEM = "Tippe einen freien Punkt";
    const char DE_MILL_HINT_JUMP_FREE[]    PROGMEM = "Springe auf jeden freien Punkt";
    const char DE_MILL_HINT_PICK_TARGET[]  PROGMEM = "Wähle ein Zielfeld";
    const char DE_MILL_HINT_PICK_OWN[]     PROGMEM = "Wähle einen eigenen Stein";

    const char DE_MILL_PHASE_PLACING[] PROGMEM = "Setzen";
    const char DE_MILL_PHASE_MOVING[]  PROGMEM = "Ziehen";
    const char DE_MILL_PHASE_FLYING[]  PROGMEM = "Springen";

    const char DE_MILL_TURN_YOU[] PROGMEM = "Du";
    const char DE_MILL_TURN_BOT[] PROGMEM = "Bot";

    const char DE_MILL_FMT_STONES_YOU[] PROGMEM = "Du:  Hand {0:%u}  Brett {1:%u}";
    const char DE_MILL_FMT_STONES_BOT[] PROGMEM = "Bot: Hand {0:%u}  Brett {1:%u}";

    const char DE_MILL_OVERLAY_WON_TITLE[]  PROGMEM = "GEWONNEN!";
    const char DE_MILL_OVERLAY_WON_LINE2[]  PROGMEM = "Du hast den Bot besiegt";
    const char DE_MILL_OVERLAY_LOST_TITLE[] PROGMEM = "Verloren";
    const char DE_MILL_OVERLAY_LOST_LINE2[] PROGMEM = "Der Bot war stärker";
    const char DE_MILL_OVERLAY_DRAW_TITLE[] PROGMEM = "Remis";
    const char DE_MILL_OVERLAY_DRAW_LINE2[] PROGMEM = "Keine Entscheidung";

    const char DE_MILL_BTN_EASY[]   PROGMEM = "leicht";
    const char DE_MILL_BTN_MEDIUM[] PROGMEM = "mittel";
    const char DE_MILL_BTN_NEW[]    PROGMEM = "neu";
    const char DE_MILL_BTN_RESET[]  PROGMEM = "reset";

    // PeriodicTable: Kategorie-Namen (lang, Popup)
    const char DE_PERIODIC_TABLE_CAT_ALKALI_LONG[]          PROGMEM = "Alkalimetall";
    const char DE_PERIODIC_TABLE_CAT_ALKALINE_EARTH_LONG[]  PROGMEM = "Erdalkalimetall";
    const char DE_PERIODIC_TABLE_CAT_TRANSITION_LONG[]      PROGMEM = "Übergangsmetall";
    const char DE_PERIODIC_TABLE_CAT_POST_TRANSITION_LONG[] PROGMEM = "Post-Übergangsmetall";
    const char DE_PERIODIC_TABLE_CAT_METALLOID_LONG[]       PROGMEM = "Halbmetall";
    const char DE_PERIODIC_TABLE_CAT_NONMETAL_LONG[]        PROGMEM = "Reaktives Nichtmetall";
    const char DE_PERIODIC_TABLE_CAT_NOBLE_GAS_LONG[]       PROGMEM = "Edelgas";
    const char DE_PERIODIC_TABLE_CAT_LANTHANIDE_LONG[]      PROGMEM = "Lanthanoid";
    const char DE_PERIODIC_TABLE_CAT_ACTINIDE_LONG[]        PROGMEM = "Actinoid";
    const char DE_PERIODIC_TABLE_CAT_UNKNOWN_LONG[]         PROGMEM = "Unbekannte Eigenschaften";

    // PeriodicTable: Kategorie-Namen (kurz, Legende)
    const char DE_PERIODIC_TABLE_CAT_ALKALI_SHORT[]          PROGMEM = "Alkalimetall";
    const char DE_PERIODIC_TABLE_CAT_ALKALINE_EARTH_SHORT[]  PROGMEM = "Erdalkalimetall";
    const char DE_PERIODIC_TABLE_CAT_TRANSITION_SHORT[]      PROGMEM = "Übergangsmetall";
    const char DE_PERIODIC_TABLE_CAT_POST_TRANSITION_SHORT[] PROGMEM = "Post-Übergang";
    const char DE_PERIODIC_TABLE_CAT_METALLOID_SHORT[]       PROGMEM = "Halbmetall";
    const char DE_PERIODIC_TABLE_CAT_NONMETAL_SHORT[]        PROGMEM = "Nichtmetall";
    const char DE_PERIODIC_TABLE_CAT_NOBLE_GAS_SHORT[]       PROGMEM = "Edelgas";
    const char DE_PERIODIC_TABLE_CAT_LANTHANIDE_SHORT[]      PROGMEM = "Lanthanoid";
    const char DE_PERIODIC_TABLE_CAT_ACTINIDE_SHORT[]        PROGMEM = "Actinoid";
    const char DE_PERIODIC_TABLE_CAT_UNKNOWN_SHORT[]         PROGMEM = "Unbekannt";

    // PeriodicTable: Aggregatzustände
    const char DE_PERIODIC_TABLE_STATE_SOLID[]   PROGMEM = "fest";
    const char DE_PERIODIC_TABLE_STATE_LIQUID[]  PROGMEM = "flüssig";
    const char DE_PERIODIC_TABLE_STATE_GAS[]     PROGMEM = "gasförmig";
    const char DE_PERIODIC_TABLE_STATE_UNKNOWN[] PROGMEM = "unbekannt";

    // PeriodicTable: Popup-Labels
    const char DE_PERIODIC_TABLE_LABEL_CATEGORY[]     PROGMEM = "Kategorie:";
    const char DE_PERIODIC_TABLE_LABEL_MASS[]         PROGMEM = "Atommasse:";
    const char DE_PERIODIC_TABLE_LABEL_GROUP_PERIOD[] PROGMEM = "Gr. / Per.:";
    const char DE_PERIODIC_TABLE_LABEL_STATE[]        PROGMEM = "Zustand:";
    const char DE_PERIODIC_TABLE_LABEL_DISCOVERED[]   PROGMEM = "Entdeckt:";
    const char DE_PERIODIC_TABLE_LABEL_DISCOVERER[]   PROGMEM = "Entdecker:";

    // PeriodicTable: Sonderwerte für Entdeckungsjahr
    const char DE_PERIODIC_TABLE_YEAR_ANCIENT[] PROGMEM = "Antike";
    const char DE_PERIODIC_TABLE_YEAR_UNKNOWN[] PROGMEM = "unbekannt";

    // Shut-the-Box-Tool
    const char DE_SHUT_THE_BOX_TITLE[]    PROGMEM = "Shut the Box";
    const char DE_SHUT_THE_BOX_SUBTITLE[] PROGMEM = "Würfelspiel für die Pause";
    const char DE_SHUT_THE_BOX_HELP_TITLE[] PROGMEM = "SHUT THE BOX - SPIELANLEITUNG";
    const char DE_SHUT_THE_BOX_HELP_TEXT[]  PROGMEM =
        "Ziel: Schließe alle Klappen 1 bis 9.\n\n"
        "WURF: Tippe die Wurfkarte oder die Wurf-Taste unten. Solange 7, 8 oder 9 offen sind, wird mit zwei Würfeln gespielt.\n\n"
        "AUSWAHL: Tippe offene Zahlen an, bis ihre Summe dem Wurf entspricht. Mit \"legen\" werden sie geschlossen.\n\n"
        "1W/2W: Sobald 7, 8 und 9 geschlossen sind, kannst du vor dem nächsten Wurf einen oder zwei Würfel wählen.\n\n"
        "ENDE: Ohne passende Kombination endet die Runde. Je niedriger der Restwert, desto besser. Eine Null zählt als Sieg.";
    const char DE_SHUT_THE_BOX_RESET_TITLE[] PROGMEM = "Statistik zurücksetzen";
    const char DE_SHUT_THE_BOX_RESET_TEXT[]  PROGMEM = "Sollen bester Restwert und Siege wirklich gelöscht werden?";
    const char DE_SHUT_THE_BOX_SUB_READY[]   PROGMEM = "Würfeln und passende Summe schließen";
    const char DE_SHUT_THE_BOX_FMT_SUB_PICK[] PROGMEM = "Wähle Summe {0:%u}";
    const char DE_SHUT_THE_BOX_SUB_WON[]      PROGMEM = "Alle Klappen geschlossen";
    const char DE_SHUT_THE_BOX_SUB_GAMEOVER[] PROGMEM = "Keine passende Kombination";
    const char DE_SHUT_THE_BOX_LABEL_ROLL[]   PROGMEM = "WURF";
    const char DE_SHUT_THE_BOX_LABEL_PICK[]   PROGMEM = "AUSWAHL";
    const char DE_SHUT_THE_BOX_LABEL_OPEN[]   PROGMEM = "OFFEN";
    const char DE_SHUT_THE_BOX_LABEL_BEST[]   PROGMEM = "Bester Rest";
    const char DE_SHUT_THE_BOX_LABEL_WINS[]   PROGMEM = "Siege";
    const char DE_SHUT_THE_BOX_HINT_ROLL[]    PROGMEM = "tippe für Wurf";
    const char DE_SHUT_THE_BOX_OVERLAY_WON_TITLE[]  PROGMEM = "GESCHAFFT!";
    const char DE_SHUT_THE_BOX_OVERLAY_WON_LINE2[]  PROGMEM = "Alle Klappen zu";
    const char DE_SHUT_THE_BOX_OVERLAY_LOST_TITLE[] PROGMEM = "Runde vorbei";
    const char DE_SHUT_THE_BOX_OVERLAY_LOST_LINE2[] PROGMEM = "Keine Auswahl passt";
    const char DE_SHUT_THE_BOX_BTN_ONE[]   PROGMEM = "1W";
    const char DE_SHUT_THE_BOX_BTN_TWO[]   PROGMEM = "2W";
    const char DE_SHUT_THE_BOX_BTN_ROLL[]  PROGMEM = "wurf";
    const char DE_SHUT_THE_BOX_BTN_LAY[]   PROGMEM = "legen";
    const char DE_SHUT_THE_BOX_BTN_NEW[]   PROGMEM = "neu";
    const char DE_SHUT_THE_BOX_BTN_RESET[] PROGMEM = "reset";

    // Lotto-Normal-Tool
    const char DE_LOTTO_NORMAL_TITLE[] PROGMEM = "Lotto 6 aus 49";
    const char DE_LOTTO_NORMAL_PAGE_TITLE[] PROGMEM = "Lotto 6 aus 49";
    const char DE_LOTTO_NORMAL_SUBTITLE[] PROGMEM = "Normaler Tipp mit 6 Zahlen";
    const char DE_LOTTO_NORMAL_HELP_TITLE[] PROGMEM = "LOTTO 6 AUS 49";
    const char DE_LOTTO_NORMAL_HELP_TEXT[] PROGMEM =
        "Normaler Lotto-Tipp mit sechs Zahlen und Superzahl.\n\n"
        "Im Tippmodus markierst du genau sechs Zahlen und wählst die Superzahl rechts im Bereich Ziehung.\n\n"
        "Der Würfel füllt den aktuellen Bereich zufällig. Die Mülltonne löscht ihn.\n\n"
        "Mit \"ziehung\" trägst du sechs Gewinnzahlen ein. Mit \"tippen\" wechselst du zurück.\n\n"
        "Die Tabelle zeigt die erreichte Gewinnklasse, keine Gewinnbeträge.";
    const char DE_LOTTO_NORMAL_HINT_DRAW_SUPER_MISSING[] PROGMEM = "Superzahl in Ziehung fehlt";

    // Flugradar-Tool
    const char DE_FLIGHT_RADAR_TITLE[] PROGMEM = "Flugradar";
    const char DE_FLIGHT_RADAR_SUBTITLE[] PROGMEM = "Flugzeuge am Wetterstandort";
    const char DE_FLIGHT_RADAR_FMT_SUBTITLE[] PROGMEM = "Flugzeuge bis {0:%u} km um {1}";
    const char DE_FLIGHT_RADAR_HELP_TITLE[] PROGMEM = "FLUGRADAR";
    const char DE_FLIGHT_RADAR_HELP_TEXT[] PROGMEM =
        "Die Tasten unten legen den Suchradius fest. Die Ringtaste schaltet die Beschriftung im Radar um: Kennung mit Höhe und Tempo, nur Kennung oder nur Symbol.\n\n"
        "Das Radar zeigt Flugzeuge im gewählten Umkreis um den gespeicherten Wetterstandort. Nach einem Wechsel wird der neue Bereich neu geladen. Ein Tipp auf eine Listenzeile öffnet die Details zum Flugzeug. Der schmale Balken am rechten Rand läuft bis zum nächsten Abruf ab.\n\n"
        "Die Flugdaten stammen vom OpenSky Network und sind für die nicht gewerbliche Nutzung frei. Mit OpenSky-Zugangsdaten in den Einstellungen wird häufiger aktualisiert.";
    const char DE_FLIGHT_RADAR_LABEL_COUNT[] PROGMEM = "Flugzeuge";
    const char DE_FLIGHT_RADAR_OFFLINE_TITLE[] PROGMEM = "Keine WLAN-Verbindung";
    const char DE_FLIGHT_RADAR_AUTH_ACCOUNT[] PROGMEM = "Konto";
    const char DE_FLIGHT_RADAR_AUTH_GUEST[] PROGMEM = "Gast";
    const char DE_FLIGHT_RADAR_LABEL_GROUND[] PROGMEM = "am Boden";
    const char DE_FLIGHT_RADAR_FMT_MORE[] PROGMEM = "+{0:%u} weitere";
    const char DE_FLIGHT_RADAR_DETAIL_TITLE_FMT[] PROGMEM = "Flugzeug {0}";
    const char DE_FLIGHT_RADAR_DETAIL_CALLSIGN[] PROGMEM = "Kennung";
    const char DE_FLIGHT_RADAR_DETAIL_ICAO24[] PROGMEM = "ICAO24";
    const char DE_FLIGHT_RADAR_DETAIL_COUNTRY[] PROGMEM = "Herkunftsland";
    const char DE_FLIGHT_RADAR_DETAIL_DISTANCE[] PROGMEM = "Entfernung";
    const char DE_FLIGHT_RADAR_DETAIL_ALTITUDE[] PROGMEM = "Höhe";
    const char DE_FLIGHT_RADAR_DETAIL_SPEED[] PROGMEM = "Geschwindigkeit";
    const char DE_FLIGHT_RADAR_DETAIL_TRACK[] PROGMEM = "Kurs";
    const char DE_FLIGHT_RADAR_DETAIL_LAST_SEEN[] PROGMEM = "Letzte Sichtung";
    const char DE_FLIGHT_RADAR_VALUE_UNKNOWN[] PROGMEM = "unbekannt";
    const char DE_FLIGHT_RADAR_VALUE_JUST_NOW[] PROGMEM = "gerade eben";
    const char DE_FLIGHT_RADAR_FMT_SECONDS_AGO[] PROGMEM = "vor {0:%u} s";
    const char DE_FLIGHT_RADAR_FMT_MINUTES_AGO[] PROGMEM = "vor {0:%u} min";
    const char DE_FLIGHT_RADAR_FMT_TRACK[] PROGMEM = "{0:%.0f} Grad";
    const char DE_FLIGHT_RADAR_LOCATION_UNKNOWN[] PROGMEM = "Standort";
    const char DE_FLIGHT_RADAR_COMPASS_NORTH[] PROGMEM = "N";
    const char DE_FLIGHT_RADAR_COMPASS_EAST[] PROGMEM = "O";
    const char DE_FLIGHT_RADAR_COMPASS_SOUTH[] PROGMEM = "S";
    const char DE_FLIGHT_RADAR_COMPASS_WEST[] PROGMEM = "W";

    // Lotto-System-Tool
    const char DE_LOTTO_TITLE[]    PROGMEM = "Systemlotto 6 aus 49";
    const char DE_LOTTO_PAGE_TITLE[] PROGMEM = "Lotto 6 aus 49 Systemspiel (BER)";
    const char DE_LOTTO_SUBTITLE[] PROGMEM = "Vollsystem 007 bis 011";
    const char DE_LOTTO_HELP_TITLE[] PROGMEM = "LOTTO 6 AUS 49 - VOLLSYSTEM (BER)";
    const char DE_LOTTO_HELP_TEXT[]  PROGMEM =
        "Vollsystem 007 bis 011 für Berlin/Brandenburg.\n\n"
        "Unten wählst du das Vollsystem. Im Tippmodus markierst du 7 bis 11 Tippzahlen.\n\n"
        "Der Würfel füllt den aktuellen Bereich zufällig. Die Mülltonne löscht ihn.\n\n"
        "Mit \"ziehung\" trägst du sechs Gewinnzahlen ein. Mit \"tippen\" wechselst du zurück. Die Superzahl öffnest du rechts im Bereich Ziehung.\n\n"
        "Die Tabelle zeigt Treffer je Gewinnklasse, keine Gewinnbeträge.";
    const char DE_LOTTO_LABEL_TICKET[]    PROGMEM = "Tipp";
    const char DE_LOTTO_LABEL_DRAW[]      PROGMEM = "Ziehung";
    const char DE_LOTTO_LABEL_FIELDS[]    PROGMEM = "Felder";
    const char DE_LOTTO_LABEL_PRICE[]     PROGMEM = "Einsatz";
    const char DE_LOTTO_LABEL_SUPER[]     PROGMEM = "SZ";
    const char DE_LOTTO_LABEL_TICKET_SUPER[] PROGMEM = "Tipp-SZ";
    const char DE_LOTTO_LABEL_DRAW_SUPER[]   PROGMEM = "Zieh-SZ";
    const char DE_LOTTO_DIALOG_SUPER[]    PROGMEM = "Superzahl";
    const char DE_LOTTO_DIALOG_TICKET_SUPER[] PROGMEM = "getippte Superzahl";
    const char DE_LOTTO_DIALOG_DRAW_SUPER[]   PROGMEM = "gezogene Superzahl";
    const char DE_LOTTO_LABEL_WINS[]      PROGMEM = "Gewinnklassen";
    const char DE_LOTTO_PRIZE_6_SUPER[]   PROGMEM = "6 R + SZ";
    const char DE_LOTTO_PRIZE_6[]         PROGMEM = "6 R";
    const char DE_LOTTO_PRIZE_5_SUPER[]   PROGMEM = "5 R + SZ";
    const char DE_LOTTO_PRIZE_5[]         PROGMEM = "5 R";
    const char DE_LOTTO_PRIZE_4_SUPER[]   PROGMEM = "4 R + SZ";
    const char DE_LOTTO_PRIZE_4[]         PROGMEM = "4 R";
    const char DE_LOTTO_PRIZE_3_SUPER[]   PROGMEM = "3 R + SZ";
    const char DE_LOTTO_PRIZE_3[]         PROGMEM = "3 R";
    const char DE_LOTTO_PRIZE_2_SUPER[]   PROGMEM = "2 R + SZ";
    const char DE_LOTTO_EMPTY_TICKET[]    PROGMEM = "noch keine Tippzahlen";
    const char DE_LOTTO_EMPTY_DRAW[]      PROGMEM = "noch keine Ziehung";
    const char DE_LOTTO_HINT_INCOMPLETE[] PROGMEM = "Tipp/Ziehung fehlt";
    const char DE_LOTTO_YES[]             PROGMEM = "ja";
    const char DE_LOTTO_NO[]              PROGMEM = "nein";
    const char DE_LOTTO_FMT_SUB_TICKET[]  PROGMEM = "Tippzahlen {0:%u}/{1:%u}";
    const char DE_LOTTO_FMT_SUB_DRAW[]    PROGMEM = "Gewinnzahlen {0:%u}/6";
    const char DE_LOTTO_SUB_READY[]       PROGMEM = "Gewinnklassen ermittelt";
    const char DE_LOTTO_FMT_SUMMARY_TICKET_SUPER[] PROGMEM = "getippt {0}";
    const char DE_LOTTO_FMT_SUMMARY_DRAW_SUPER[]   PROGMEM = "gezogen {0}";
    const char DE_LOTTO_BTN_RANDOM[]      PROGMEM = "zuf";
    const char DE_LOTTO_BTN_CLEAR[]       PROGMEM = "leer";
    const char DE_LOTTO_BTN_TICKET[]      PROGMEM = "tippen";
    const char DE_LOTTO_BTN_DRAW[]        PROGMEM = "ziehung";

    // SMD-Widerstand
    const char DE_TOOL_SMD_RESISTOR_TITLE[]    PROGMEM = "SMD Widerstand";
    const char DE_TOOL_SMD_RESISTOR_SUBTITLE[] PROGMEM = "3/4 Stellen und R-Code";
    const char DE_SMD_RESISTOR_LABEL_CODE[]     PROGMEM = "Code";
    const char DE_SMD_RESISTOR_LABEL_VALUE[]    PROGMEM = "Wert";
    const char DE_SMD_RESISTOR_ERR_EMPTY[]      PROGMEM = "Code eingeben";
    const char DE_SMD_RESISTOR_ERR_INVALID[]    PROGMEM = "Ungültiger Code";
    const char DE_SMD_RESISTOR_ERR_R_CODE[]     PROGMEM = "R nur einmal und mit Ziffern";
    const char DE_SMD_RESISTOR_NOTE_3_DIGIT[]   PROGMEM = "3-stelliger Code";
    const char DE_SMD_RESISTOR_NOTE_4_DIGIT[]   PROGMEM = "4-stelliger Code";
    const char DE_SMD_RESISTOR_NOTE_R_CODE[]    PROGMEM = "R-Code";
    const char DE_SMD_RESISTOR_NOTE_ZERO[]      PROGMEM = "Null-Ohm-Brücke";
    const char DE_SMD_RESISTOR_MODE_3[]         PROGMEM = "3-stellig";
    const char DE_SMD_RESISTOR_MODE_4[]         PROGMEM = "4-stellig";
    const char DE_SMD_RESISTOR_MODE_R[]         PROGMEM = "R-Code";

    // Website-QR-Dialog
    const char DE_QR_MODAL_TITLE[] PROGMEM = "Website";
    const char DE_QR_MODAL_HINT[]  PROGMEM = "Dieser QR-Code öffnet eine Website zum aktuellen Thema. Für den Inhalt ist nur der Betreiber der Website verantwortlich.";
    const char DE_QR_MODAL_ERROR[] PROGMEM = "QR-Code konnte nicht erzeugt werden.";
}

// ===================================================================
// PROGMEM-Literale (English, British)
// ===================================================================

namespace
{
    // Sonder-Key: leerer Text
    const char EN_EMPTY[] PROGMEM = "";

    // Kategorie-Titel
    const char EN_CAT_MISC_TITLE[]        PROGMEM = "Tools";
    const char EN_CAT_GAMES_TITLE[]       PROGMEM = "Games";
    const char EN_CAT_ELECTRONICS_TITLE[] PROGMEM = "Electronics";
    const char EN_CAT_UNITS_TITLE[]       PROGMEM = "Units";
    const char EN_CAT_DIGITAL_TITLE[]     PROGMEM = "Digital";
    const char EN_CAT_KNITTING_TITLE[]    PROGMEM = "Knitting";
    const char EN_CAT_MUSIC_TITLE[]       PROGMEM = "Music";

    // Kategorie-Kurzbeschreibungen
    const char EN_CAT_MISC_SUMMARY[]        PROGMEM = "References and miscellaneous";
    const char EN_CAT_GAMES_SUMMARY[]       PROGMEM = "Puzzles and board games";
    const char EN_CAT_ELECTRONICS_SUMMARY[] PROGMEM = "Calculators and references";
    const char EN_CAT_UNITS_SUMMARY[]       PROGMEM = "Length, temp., pressure, SI";
    const char EN_CAT_DIGITAL_SUMMARY[]     PROGMEM = "Bit editor, ASCII, binary tools";
    const char EN_CAT_KNITTING_SUMMARY[]    PROGMEM = "Stitches, yarn, gauge";
    const char EN_CAT_MUSIC_SUMMARY[]       PROGMEM = "Tempo, time signature, tuning";

    // Werkzeug-Titel
    const char EN_TOOL_CALCULATOR_TITLE[]         PROGMEM = "Calculator";
    const char EN_TOOL_WORLD_CLOCK_TITLE[]        PROGMEM = "World clock";
    const char EN_TOOL_COLOR_CONVERTER_TITLE[]    PROGMEM = "Colour converter";
    const char EN_TOOL_PERIODIC_TABLE_TITLE[]     PROGMEM = "Periodic table";
    const char EN_TOOL_SCREW_REFERENCE_TITLE[]    PROGMEM = "Screws";
    const char EN_TOOL_BMI_CALCULATOR_TITLE[]     PROGMEM = "BMI calculator";
    const char EN_TOOL_GAME_2048_TITLE[]          PROGMEM = "2048";
    const char EN_TOOL_MILL_GAME_TITLE[]          PROGMEM = "Nine men's morris";
    const char EN_TOOL_LIGHTS_OUT_TITLE[]         PROGMEM = "Lights Out";
    const char EN_TOOL_DICE_TITLE[]               PROGMEM = "Dice";
    const char EN_TOOL_RESISTOR_TITLE[]           PROGMEM = "Resistor colour code";
    const char EN_TOOL_OHM_CALCULATOR_TITLE[]     PROGMEM = "Ohm's law";
    const char EN_TOOL_VOLTAGE_DIVIDER_TITLE[]    PROGMEM = "Voltage divider";
    const char EN_TOOL_LED_CALCULATOR_TITLE[]     PROGMEM = "LED series resistor";
    const char EN_TOOL_RC_FILTER_TITLE[]          PROGMEM = "RC/RL filter";
    const char EN_TOOL_LC_FILTER_TITLE[]          PROGMEM = "LC filter";
    const char EN_TOOL_UKW_RESONATOR_TITLE[]      PROGMEM = "VHF resonator";
    const char EN_TOOL_LM317_TITLE[]              PROGMEM = "LM317 regulator";
    const char EN_TOOL_DECIBEL_TITLE[]            PROGMEM = "Decibel";
    const char EN_TOOL_NE555_TITLE[]              PROGMEM = "NE555 timer";
    const char EN_TOOL_OPAMP_TITLE[]              PROGMEM = "Op-Amp";
    const char EN_TOOL_INCH_MM_TITLE[]            PROGMEM = "Inch <-> mm";
    const char EN_TOOL_TEMP_TITLE[]               PROGMEM = "Temperature";
    const char EN_TOOL_PRESSURE_TITLE[]           PROGMEM = "Pressure";
    const char EN_TOOL_SI_PREFIXES_TITLE[]        PROGMEM = "SI prefixes";
    const char EN_TOOL_ASCII_TABLE_TITLE[]        PROGMEM = "ASCII table";
    const char EN_TOOL_BIT_EDITOR_TITLE[]         PROGMEM = "Bit editor";
    const char EN_TOOL_LOGIC_GATE_TITLE[]         PROGMEM = "Logic gates";
    const char EN_TOOL_STITCH_GAUGE_TITLE[]       PROGMEM = "Stitch gauge";
    const char EN_TOOL_STITCH_DISTRIBUTOR_TITLE[] PROGMEM = "Stitch distribution";
    const char EN_TOOL_SLEEVE_TAPER_TITLE[]       PROGMEM = "Sleeve taper";
    const char EN_TOOL_CIRCLE_OF_FIFTHS_TITLE[]   PROGMEM = "Circle of fifths";
    const char EN_TOOL_METRONOME_TITLE[]          PROGMEM = "Metronome";

    // Werkzeug-Untertitel
    const char EN_TOOL_CALCULATOR_SUBTITLE[]         PROGMEM = "Dec / hex / bin (16-bit)";
    const char EN_TOOL_WORLD_CLOCK_SUBTITLE[]        PROGMEM = "Eight metropolises, standard time";
    const char EN_TOOL_COLOR_CONVERTER_SUBTITLE[]    PROGMEM = "RGB / hex / RGB565";
    const char EN_TOOL_PERIODIC_TABLE_SUBTITLE[]     PROGMEM = "Periodic table of the elements";
    const char EN_TOOL_SCREW_REFERENCE_SUBTITLE[]    PROGMEM = "Metric (DIN 13)";
    const char EN_TOOL_BMI_CALCULATOR_SUBTITLE[]     PROGMEM = "Body Mass Index";
    const char EN_TOOL_GAME_2048_SUBTITLE[]          PROGMEM = "Sliding puzzle for the break";
    const char EN_TOOL_MILL_GAME_SUBTITLE[]          PROGMEM = "Board game for the break";
    const char EN_TOOL_LIGHTS_OUT_SUBTITLE[]         PROGMEM = "Puzzle game for the break";
    const char EN_TOOL_DICE_SUBTITLE[]               PROGMEM = "Random numbers from hardware RNG";
    const char EN_TOOL_RESISTOR_SUBTITLE[]           PROGMEM = "4-band and 5-band";
    const char EN_TOOL_OHM_CALCULATOR_SUBTITLE[]     PROGMEM = "U / I / R / P";
    const char EN_TOOL_VOLTAGE_DIVIDER_SUBTITLE[]    PROGMEM = "R1 / R2 / Uout";
    const char EN_TOOL_LED_CALCULATOR_SUBTITLE[]     PROGMEM = "Resistor, preferred value, power dissipation";
    const char EN_TOOL_RC_FILTER_SUBTITLE[]          PROGMEM = "Low-pass / high-pass, 1st order";
    const char EN_TOOL_LC_FILTER_SUBTITLE[]          PROGMEM = "Low-pass / high-pass, 2nd order";
    const char EN_TOOL_UKW_RESONATOR_SUBTITLE[]      PROGMEM = "Air-core resonant circuit 80...150 MHz";
    const char EN_TOOL_LM317_SUBTITLE[]              PROGMEM = "Vref / R1 / R2 / Vout";
    const char EN_TOOL_DECIBEL_SUBTITLE[]            PROGMEM = "Voltage / power / dB";
    const char EN_TOOL_NE555_SUBTITLE[]              PROGMEM = "Astable multivibrator";
    const char EN_TOOL_OPAMP_SUBTITLE[]              PROGMEM = "Inverting amplifier";
    const char EN_TOOL_INCH_MM_SUBTITLE[]            PROGMEM = "Bidirectional conversion";
    const char EN_TOOL_TEMP_SUBTITLE[]               PROGMEM = "C <-> F <-> K";
    const char EN_TOOL_PRESSURE_SUBTITLE[]           PROGMEM = "bar / psi / atm / Pa";
    const char EN_TOOL_SI_PREFIXES_SUBTITLE[]        PROGMEM = "Pico to tera - symbol, power, example";
    const char EN_TOOL_ASCII_TABLE_SUBTITLE[]        PROGMEM = "Characters 0-127 with DEC / HEX / OCT / BIN";
    const char EN_TOOL_BIT_EDITOR_SUBTITLE[]         PROGMEM = "8-bit binary editor";
    const char EN_TOOL_LOGIC_GATE_SUBTITLE[]         PROGMEM = "AND / OR / XOR / NAND / NOR / NOT";
    const char EN_TOOL_STITCH_GAUGE_SUBTITLE[]       PROGMEM = "Convert tension swatch and target dimensions";
    const char EN_TOOL_STITCH_DISTRIBUTOR_SUBTITLE[] PROGMEM = "Increases and decreases across one round";
    const char EN_TOOL_SLEEVE_TAPER_SUBTITLE[]       PROGMEM = "Increases and decreases across several rows";
    const char EN_TOOL_CIRCLE_OF_FIFTHS_SUBTITLE[]   PROGMEM = "Keys, accidentals, diatonic chords";
    const char EN_TOOL_METRONOME_SUBTITLE[]          PROGMEM = "Tempo and time signature, optional click";

    // Abweichende Menübeschriftung für SCREW_REFERENCE
    const char EN_TOOL_SCREW_REFERENCE_LABEL[] PROGMEM = "Screw reference";

    // Fehlermeldungen
    const char EN_ERR_HEADER[]           PROGMEM = "[ Error ]";
    const char EN_ERR_TOOL_LOAD_FAILED[] PROGMEM = "Tool could not be loaded.";

    // Temp-Tool
    const char EN_TEMP_LABEL_CELSIUS[]    PROGMEM = "Celsius";
    const char EN_TEMP_LABEL_FAHRENHEIT[] PROGMEM = "Fahrenh.";
    const char EN_TEMP_LABEL_KELVIN[]     PROGMEM = "Kelvin";

    const char EN_TEMP_UNIT_CELSIUS[]    PROGMEM = "°C";
    const char EN_TEMP_UNIT_FAHRENHEIT[] PROGMEM = "°F";
    const char EN_TEMP_UNIT_KELVIN[]     PROGMEM = "K";

    const char EN_TEMP_REF_M40[] PROGMEM = "-40  extreme cold";
    const char EN_TEMP_REF_0[]   PROGMEM = "0  freezing point";
    const char EN_TEMP_REF_20[]  PROGMEM = "20  room temp.";
    const char EN_TEMP_REF_37[]  PROGMEM = "37  body temp.";
    const char EN_TEMP_REF_100[] PROGMEM = "100  boiling point";
    const char EN_TEMP_REF_150[] PROGMEM = "150";
    const char EN_TEMP_REF_200[] PROGMEM = "200";

    const char EN_TEMP_FORMULA_C2F[] PROGMEM = "°F = °C x 9/5 + 32";
    const char EN_TEMP_FORMULA_F2C[] PROGMEM = "°C = (°F - 32) x 5/9";
    const char EN_TEMP_FORMULA_K2C[] PROGMEM = "°C = K - 273.15";

    // InchMm-Tool
    const char EN_INCHMM_LABEL_INCH[] PROGMEM = "Inch";
    const char EN_INCHMM_LABEL_MM[]   PROGMEM = "mm";

    const char EN_INCHMM_UNIT_INCH[] PROGMEM = "in";
    const char EN_INCHMM_UNIT_MM[]   PROGMEM = "mm";

    const char EN_INCHMM_FRAC_1_16[]  PROGMEM = "1/16\"";
    const char EN_INCHMM_FRAC_1_8[]   PROGMEM = "1/8\"";
    const char EN_INCHMM_FRAC_3_16[]  PROGMEM = "3/16\"";
    const char EN_INCHMM_FRAC_1_4[]   PROGMEM = "1/4\"";
    const char EN_INCHMM_FRAC_5_16[]  PROGMEM = "5/16\"";
    const char EN_INCHMM_FRAC_3_8[]   PROGMEM = "3/8\"";
    const char EN_INCHMM_FRAC_7_16[]  PROGMEM = "7/16\"";
    const char EN_INCHMM_FRAC_1_2[]   PROGMEM = "1/2\"";
    const char EN_INCHMM_FRAC_9_16[]  PROGMEM = "9/16\"";
    const char EN_INCHMM_FRAC_5_8[]   PROGMEM = "5/8\"";
    const char EN_INCHMM_FRAC_11_16[] PROGMEM = "11/16\"";
    const char EN_INCHMM_FRAC_3_4[]   PROGMEM = "3/4\"";
    const char EN_INCHMM_FRAC_13_16[] PROGMEM = "13/16\"";
    const char EN_INCHMM_FRAC_7_8[]   PROGMEM = "7/8\"";
    const char EN_INCHMM_FRAC_15_16[] PROGMEM = "15/16\"";
    const char EN_INCHMM_FRAC_1[]     PROGMEM = "1\"";

    const char EN_INCHMM_INFO_FACTOR[] PROGMEM = "1 inch = 25.4 mm";

    // Pressure-Tool
    const char EN_PRESSURE_UNIT_BAR[] PROGMEM = "bar";
    const char EN_PRESSURE_UNIT_PSI[] PROGMEM = "psi";
    const char EN_PRESSURE_UNIT_ATM[] PROGMEM = "atm";
    const char EN_PRESSURE_UNIT_PA[]  PROGMEM = "Pa";

    const char EN_PRESSURE_REF_HDR_APPLICATION[] PROGMEM = "Application";

    const char EN_PRESSURE_REF_BLOOD[]      PROGMEM = "Blood pressure (systolic)";
    const char EN_PRESSURE_REF_ATMOSPHERE[] PROGMEM = "Atmosphere (1 atm)";
    const char EN_PRESSURE_REF_TYRE_CAR[]   PROGMEM = "Car tyre pressure";
    const char EN_PRESSURE_REF_TYRE_TRUCK[] PROGMEM = "Lorry tyre pressure";
    const char EN_PRESSURE_REF_PNEUMATIC[]  PROGMEM = "Workshop pneumatics";
    const char EN_PRESSURE_REF_WATER_PIPE[] PROGMEM = "Water mains";
    const char EN_PRESSURE_REF_DIVE_TANK[]  PROGMEM = "Diving cylinder";
    const char EN_PRESSURE_REF_HYDRAULIC[]  PROGMEM = "Hydraulic press";

    const char EN_PRESSURE_INFO_BAR[] PROGMEM = "1 bar = 100,000 Pa";
    const char EN_PRESSURE_INFO_PSI[] PROGMEM = "1 psi = 6,894.757 Pa";
    const char EN_PRESSURE_INFO_ATM[] PROGMEM = "1 atm = 101,325 Pa";
    const char EN_PRESSURE_INFO_PA[]  PROGMEM = "1 Pa = 0.00001 bar";

    // WorldClock-Tool
    const char EN_WORLDCLOCK_CITY_LOS_ANGELES[] PROGMEM = "Los Angeles";
    const char EN_WORLDCLOCK_CITY_NEW_YORK[]    PROGMEM = "New York";
    const char EN_WORLDCLOCK_CITY_LONDON[]      PROGMEM = "London";
    const char EN_WORLDCLOCK_CITY_BERLIN[]      PROGMEM = "Berlin";
    const char EN_WORLDCLOCK_CITY_MOSCOW[]      PROGMEM = "Moscow";
    const char EN_WORLDCLOCK_CITY_DUBAI[]       PROGMEM = "Dubai";
    const char EN_WORLDCLOCK_CITY_TOKYO[]       PROGMEM = "Tokyo";
    const char EN_WORLDCLOCK_CITY_SYDNEY[]      PROGMEM = "Sydney";

    // StitchDistributor-Tool
    const char EN_STITCH_DISTRIBUTOR_LABEL_FROM[]    PROGMEM = "From";
    const char EN_STITCH_DISTRIBUTOR_LABEL_TO[]      PROGMEM = "To";
    const char EN_STITCH_DISTRIBUTOR_UNIT_STITCHES[] PROGMEM = "st";

    const char EN_STITCH_DISTRIBUTOR_TITLE_DISTRIBUTION[] PROGMEM = "Distribution";
    const char EN_STITCH_DISTRIBUTOR_TITLE_NO_CHANGE[]    PROGMEM = "No change";
    const char EN_STITCH_DISTRIBUTOR_TITLE_INCREASE[]     PROGMEM = "Increase";
    const char EN_STITCH_DISTRIBUTOR_TITLE_DECREASE[]     PROGMEM = "Decrease";

    const char EN_STITCH_DISTRIBUTOR_ACTION_INC_WITH_PLAIN[] PROGMEM = "then increase 1 stitch";
    const char EN_STITCH_DISTRIBUTOR_ACTION_DEC_WITH_PLAIN[] PROGMEM = "then knit 2 together";
    const char EN_STITCH_DISTRIBUTOR_ACTION_INC_NO_PLAIN[]   PROGMEM = "increase 1 stitch";
    const char EN_STITCH_DISTRIBUTOR_ACTION_DEC_NO_PLAIN[]   PROGMEM = "knit 2 together";

    const char EN_STITCH_DISTRIBUTOR_HINT_EMPTY_1[]      PROGMEM = "Please enter";
    const char EN_STITCH_DISTRIBUTOR_HINT_EMPTY_2[]      PROGMEM = "stitch counts.";
    const char EN_STITCH_DISTRIBUTOR_HINT_NO_CHANGE_1[]  PROGMEM = "Stitch count stays the same.";
    const char EN_STITCH_DISTRIBUTOR_HINT_NO_CHANGE_2[]  PROGMEM = "Knit the round plain.";
    const char EN_STITCH_DISTRIBUTOR_HINT_INFEASIBLE_1[] PROGMEM = "Difference too large";
    const char EN_STITCH_DISTRIBUTOR_HINT_INFEASIBLE_2[] PROGMEM = "for a single round.";

    const char EN_STITCH_DISTRIBUTOR_FMT_SEG_WITH_PLAIN[] PROGMEM = "{0:%d} times: knit {1:%d},";
    const char EN_STITCH_DISTRIBUTOR_FMT_SEG_NO_PLAIN[]   PROGMEM = "{0:%d} times:";

    // SleeveTaper-Tool
    const char EN_SLEEVE_TAPER_LABEL_FROM[]    PROGMEM = "From";
    const char EN_SLEEVE_TAPER_LABEL_TO[]      PROGMEM = "To";
    const char EN_SLEEVE_TAPER_LABEL_OVER[]    PROGMEM = "Over";
    const char EN_SLEEVE_TAPER_UNIT_STITCHES[] PROGMEM = "st";
    const char EN_SLEEVE_TAPER_UNIT_ROWS[]     PROGMEM = "row";

    const char EN_SLEEVE_TAPER_TITLE_SLEEVE[]   PROGMEM = "Sleeve taper";
    const char EN_SLEEVE_TAPER_TITLE_NO_TAPER[] PROGMEM = "No taper";
    const char EN_SLEEVE_TAPER_TITLE_INCREASE[] PROGMEM = "Increase";
    const char EN_SLEEVE_TAPER_TITLE_DECREASE[] PROGMEM = "Decrease";

    const char EN_SLEEVE_TAPER_ACTION_INC_BOTH[] PROGMEM = "increase 1 st each side";
    const char EN_SLEEVE_TAPER_ACTION_DEC_BOTH[] PROGMEM = "decrease 1 st each side";

    const char EN_SLEEVE_TAPER_HINT_EMPTY_1[]      PROGMEM = "Please enter";
    const char EN_SLEEVE_TAPER_HINT_EMPTY_2[]      PROGMEM = "all values.";
    const char EN_SLEEVE_TAPER_HINT_NO_CHANGE_1[]  PROGMEM = "Stitch count stays the same.";
    const char EN_SLEEVE_TAPER_HINT_NO_CHANGE_2[]  PROGMEM = "No taper needed.";
    const char EN_SLEEVE_TAPER_HINT_ODD_1[]        PROGMEM = "Difference is odd.";
    const char EN_SLEEVE_TAPER_HINT_ODD_2[]        PROGMEM = "Please adjust start or end";
    const char EN_SLEEVE_TAPER_HINT_ODD_3[]        PROGMEM = "by 1.";
    const char EN_SLEEVE_TAPER_HINT_TOO_MANY_INC[] PROGMEM = "Too many increases";
    const char EN_SLEEVE_TAPER_HINT_TOO_MANY_DEC[] PROGMEM = "Too many decreases";
    const char EN_SLEEVE_TAPER_HINT_TOO_MANY_2[]   PROGMEM = "for this row count.";

    const char EN_SLEEVE_TAPER_FMT_ROWS[]     PROGMEM = "{0:%d} times every {1:%d}th row";
    const char EN_SLEEVE_TAPER_FMT_ROWS_AND[] PROGMEM = "{0:%d} times every {1:%d}th row and";

    // StitchGauge-Tool
    const char EN_STITCH_GAUGE_LABEL_STITCHES_PER_10CM[] PROGMEM = "st / 10 cm";
    const char EN_STITCH_GAUGE_LABEL_WIDTH[]             PROGMEM = "Width";
    const char EN_STITCH_GAUGE_LABEL_STITCHES[]          PROGMEM = "Stitches";
    const char EN_STITCH_GAUGE_LABEL_ROWS_PER_10CM[]     PROGMEM = "row / 10 cm";
    const char EN_STITCH_GAUGE_LABEL_HEIGHT[]            PROGMEM = "Height";
    const char EN_STITCH_GAUGE_LABEL_ROWS[]              PROGMEM = "Rows";

    const char EN_STITCH_GAUGE_UNIT_STITCHES[] PROGMEM = "st";
    const char EN_STITCH_GAUGE_UNIT_CM[]       PROGMEM = "cm";
    const char EN_STITCH_GAUGE_UNIT_ROWS[]     PROGMEM = "row";

    const char EN_STITCH_GAUGE_VIS_CM[]              PROGMEM = "{0:%.4g} cm";
    const char EN_STITCH_GAUGE_VIS_STITCHES[]        PROGMEM = "{0:%.4g} st";
    const char EN_STITCH_GAUGE_VIS_ROWS[]            PROGMEM = "{0:%.4g} row";
    const char EN_STITCH_GAUGE_VIS_STITCHES_PER_10[] PROGMEM = "{0:%.4g} st / 10 cm";
    const char EN_STITCH_GAUGE_VIS_ROWS_PER_10[]     PROGMEM = "{0:%.4g} row / 10 cm";

    // BmiCalc-Tool
    const char EN_BMI_ZONE_SEVERE_UNDER[] PROGMEM = "Severely underweight";
    const char EN_BMI_ZONE_UNDER[]        PROGMEM = "Underweight";
    const char EN_BMI_ZONE_NORMAL[]       PROGMEM = "Normal weight";
    const char EN_BMI_ZONE_OVER[]         PROGMEM = "Overweight";
    const char EN_BMI_ZONE_OBESE_1[]      PROGMEM = "Obese class I";
    const char EN_BMI_ZONE_OBESE_2[]      PROGMEM = "Obese class II";
    const char EN_BMI_ZONE_OBESE_3[]      PROGMEM = "Obese class III";

    const char EN_BMI_LABEL_HEIGHT[] PROGMEM = "Height";
    const char EN_BMI_LABEL_WEIGHT[] PROGMEM = "Weight";

    const char EN_BMI_UNIT_CM[] PROGMEM = "cm";
    const char EN_BMI_UNIT_KG[] PROGMEM = "kg";

    const char EN_BMI_NOTE_ADULTS[] PROGMEM = "Valid for adults aged 19 to 65.";

    const char EN_BMI_FMT_RESULT[] PROGMEM = "BMI = {0}";

    // OhmCalculator-Tool
    const char EN_OHM_NAME_VOLTAGE[]    PROGMEM = "(Voltage)";
    const char EN_OHM_NAME_CURRENT[]    PROGMEM = "(Current)";
    const char EN_OHM_NAME_RESISTANCE[] PROGMEM = "(Resistance)";
    const char EN_OHM_NAME_POWER[]      PROGMEM = "(Power)";

    // Decibel-Tool
    const char EN_DECIBEL_SECTION_VOLTAGE[] PROGMEM = "Voltage";
    const char EN_DECIBEL_SECTION_POWER[]   PROGMEM = "Power";

    const char EN_DECIBEL_NAME_INPUT[]  PROGMEM = "(Input)";
    const char EN_DECIBEL_NAME_OUTPUT[] PROGMEM = "(Output)";
    const char EN_DECIBEL_NAME_LEVEL[]  PROGMEM = "(Level)";

    // LogicGate-Tool
    const char EN_LOGIC_GATE_DESC_AND[]  PROGMEM = "AND - Conjunction";
    const char EN_LOGIC_GATE_DESC_OR[]   PROGMEM = "OR - Disjunction";
    const char EN_LOGIC_GATE_DESC_XOR[]  PROGMEM = "XOR - Exclusive OR";
    const char EN_LOGIC_GATE_DESC_NAND[] PROGMEM = "NAND - Negated conjunction";
    const char EN_LOGIC_GATE_DESC_NOR[]  PROGMEM = "NOR - Negated disjunction";
    const char EN_LOGIC_GATE_DESC_NOT[]  PROGMEM = "NOT - Inverter";

    // RcFilter-Tool
    const char EN_RC_FILTER_BTN_LOW_PASS[]  PROGMEM = "low-pass";
    const char EN_RC_FILTER_BTN_HIGH_PASS[] PROGMEM = "high-pass";

    const char EN_RC_FILTER_SUB_LP_RC[] PROGMEM = "Low-pass RC, 1st order";
    const char EN_RC_FILTER_SUB_LP_RL[] PROGMEM = "Low-pass RL, 1st order";
    const char EN_RC_FILTER_SUB_HP_RC[] PROGMEM = "High-pass RC, 1st order";
    const char EN_RC_FILTER_SUB_HP_RL[] PROGMEM = "High-pass RL, 1st order";

    const char EN_RC_FILTER_SLOPE_LP[] PROGMEM = "-20 dB/dec";
    const char EN_RC_FILTER_SLOPE_HP[] PROGMEM = "+20 dB/dec";

    // LcFilter-Tool
    const char EN_LC_FILTER_BTN_LOW_PASS[]  PROGMEM = "low-pass";
    const char EN_LC_FILTER_BTN_HIGH_PASS[] PROGMEM = "high-pass";

    const char EN_LC_FILTER_SUB_LP[] PROGMEM = "Low-pass LC, 2nd order";
    const char EN_LC_FILTER_SUB_HP[] PROGMEM = "High-pass LC, 2nd order";

    const char EN_LC_FILTER_SLOPE_LP[] PROGMEM = "-40 dB/dec";
    const char EN_LC_FILTER_SLOPE_HP[] PROGMEM = "+40 dB/dec";

    // UkwResonator-Tool
    const char EN_UKW_RESONATOR_SUB_D05[] PROGMEM = "Air-core coil, d = 0.5 mm Cu";
    const char EN_UKW_RESONATOR_SUB_D08[] PROGMEM = "Air-core coil, d = 0.8 mm Cu";
    const char EN_UKW_RESONATOR_SUB_D10[] PROGMEM = "Air-core coil, d = 1.0 mm Cu";
    const char EN_UKW_RESONATOR_SUB_D15[] PROGMEM = "Air-core coil, d = 1.5 mm Cu";

    const char EN_UKW_RESONATOR_FMT_TURNS[]         PROGMEM = "N = {0} turns";
    const char EN_UKW_RESONATOR_FMT_Q_APPROX[]      PROGMEM = "Q approx. {0:%d}";
    const char EN_UKW_RESONATOR_WARN_OVERLAP[]      PROGMEM = "Turns overlap!";
    const char EN_UKW_RESONATOR_WARN_OUT_OF_RANGE[] PROGMEM = "out of range!";

    // Ne555-Tool
    const char EN_NE555_BTN_ASTABLE[]    PROGMEM = "astable";
    const char EN_NE555_BTN_MONOSTABLE[] PROGMEM = "monostable";

    const char EN_NE555_SUB_ASTABLE[]    PROGMEM = "Astable multivibrator";
    const char EN_NE555_SUB_MONOSTABLE[] PROGMEM = "Monostable multivibrator";

    // OpAmp-Tool
    const char EN_OPAMP_BTN_INVERTING[]     PROGMEM = "inverting";
    const char EN_OPAMP_BTN_NON_INVERTING[] PROGMEM = "non-inverting";

    const char EN_OPAMP_SUB_INVERTING[]     PROGMEM = "Inverting amplifier";
    const char EN_OPAMP_SUB_NON_INVERTING[] PROGMEM = "Non-inverting amplifier";

    // SiPrefixes-Tool
    const char EN_SI_PREFIXES_HDR_PREFIX[]  PROGMEM = "Prefix";
    const char EN_SI_PREFIXES_HDR_SYMBOL[]  PROGMEM = "Symbol";
    const char EN_SI_PREFIXES_HDR_POWER[]   PROGMEM = "Power";
    const char EN_SI_PREFIXES_HDR_EXAMPLE[] PROGMEM = "Example";

    const char EN_SI_PREFIXES_HINT_TAP[] PROGMEM = "Tap a prefix for conversion hint";

    const char EN_SI_PREFIXES_NAME_TERA[]  PROGMEM = "Tera";
    const char EN_SI_PREFIXES_NAME_GIGA[]  PROGMEM = "Giga";
    const char EN_SI_PREFIXES_NAME_MEGA[]  PROGMEM = "Mega";
    const char EN_SI_PREFIXES_NAME_KILO[]  PROGMEM = "Kilo";
    const char EN_SI_PREFIXES_NAME_BASE[]  PROGMEM = "Base";
    const char EN_SI_PREFIXES_NAME_MILLI[] PROGMEM = "Milli";
    const char EN_SI_PREFIXES_NAME_MICRO[] PROGMEM = "Micro";
    const char EN_SI_PREFIXES_NAME_NANO[]  PROGMEM = "Nano";
    const char EN_SI_PREFIXES_NAME_PICO[]  PROGMEM = "Pico";

    // ScrewRef-Tool
    const char EN_SCREW_REF_BTN_COARSE[] PROGMEM = "metric";
    const char EN_SCREW_REF_BTN_FINE[]   PROGMEM = "fine";

    const char EN_SCREW_REF_SUB_COARSE[] PROGMEM = "Metric (DIN 13)";
    const char EN_SCREW_REF_SUB_FINE[]   PROGMEM = "Fine thread (DIN 13)";

    const char EN_SCREW_REF_FMT_TITLE_COARSE[] PROGMEM = "{0} x {1:%.4g}";
    const char EN_SCREW_REF_FMT_TITLE_FINE[]   PROGMEM = "{0} x {1:%.4g} (fine)";

    const char EN_SCREW_REF_NO_FINE_THREAD[] PROGMEM = "no fine thread";

    const char EN_SCREW_REF_LABEL_PITCH[]       PROGMEM = "Pitch";
    const char EN_SCREW_REF_LABEL_TAPPING[]     PROGMEM = "Tapping size";
    const char EN_SCREW_REF_LABEL_CLEARANCE[]   PROGMEM = "Clearance hole";
    const char EN_SCREW_REF_LABEL_AF_SOCKET[]   PROGMEM = "AF hex socket (DIN 912)";
    const char EN_SCREW_REF_LABEL_AF_HEX[]      PROGMEM = "AF hex head (DIN 934)";
    const char EN_SCREW_REF_LABEL_HEAD_HEIGHT[] PROGMEM = "Head height (DIN 912)";
    const char EN_SCREW_REF_LABEL_HEAD_DIA[]    PROGMEM = "Head diameter (DIN 912)";
    const char EN_SCREW_REF_LABEL_TORQUE[]      PROGMEM = "Torque";

    const char EN_SCREW_REF_FMT_DIM_SW[]    PROGMEM = "AF {0:%.4g}";
    const char EN_SCREW_REF_FMT_DIM_DRILL[] PROGMEM = "drill {0:%.4g}";

    // ColorConverter-Tool
    const char EN_COLOR_CONVERTER_RGB_HINT[] PROGMEM = "Display RGB after RGB565 quantisation";

    // AsciiTable-Tool
    const char EN_ASCII_TABLE_SUB_EXTENDED[] PROGMEM = "Characters 128-255 with DEC / HEX / OCT / BIN";
    const char EN_ASCII_TABLE_HINT_TAP[]     PROGMEM = "Tap on a character";
    const char EN_ASCII_TABLE_NOT_DEFINED[]  PROGMEM = "not defined";

    // BitEditor-Tool
    const char EN_BIT_EDITOR_SUB_8[]  PROGMEM = "8-bit binary editor";
    const char EN_BIT_EDITOR_SUB_16[] PROGMEM = "16-bit binary editor";
    const char EN_BIT_EDITOR_SUB_32[] PROGMEM = "32-bit binary editor";

    // Calculator-Tool
    const char EN_CALCULATOR_ERROR[] PROGMEM = "Error";

    // Gemeinsame Dialogschaltflächen
    const char EN_DIALOG_CLOSE[]  PROGMEM = "close";
    const char EN_DIALOG_OK[]     PROGMEM = "ok";
    const char EN_DIALOG_CANCEL[] PROGMEM = "cancel";

    // Metronom-Tool
    const char EN_METRONOME_HELP_TITLE[] PROGMEM = "METRONOME - GUIDE";
    const char EN_METRONOME_HELP_TEXT[]  PROGMEM =
        "DISPLAY: The dots represent the beats in the bar. The current beat lights up, the first beat is highlighted. The center shows the tempo in beats per minute and the Italian tempo marking.\n\n"
        "TEMPO: Use - and + to change the tempo. Tap 'tap' repeatedly at the desired tempo and the metronome calculates the value automatically.\n\n"
        "TIME: The time signature toggles between 2/4, 3/4, 4/4 and 6/8.\n\n"
        "START: Use 'start' to start the metronome, 'stop' to halt it. Tapping the display also starts and stops it.";
    const char EN_METRONOME_HINT_TAP_TO_START[] PROGMEM = "Tap to start";

    // Quintenzirkel-Tool
    const char EN_CIRCLE_OF_FIFTHS_FUNC_TONIC[]       PROGMEM = "Tonic";
    const char EN_CIRCLE_OF_FIFTHS_FUNC_SUBDOMINANT[] PROGMEM = "Subdominant";
    const char EN_CIRCLE_OF_FIFTHS_FUNC_DOMINANT[]    PROGMEM = "Dominant";
    const char EN_CIRCLE_OF_FIFTHS_FUNC_PARALLEL[]    PROGMEM = "Relative minor";

    const char EN_CIRCLE_OF_FIFTHS_SUFFIX_MAJOR[]   PROGMEM = "major";
    const char EN_CIRCLE_OF_FIFTHS_HEADER_SCALE[]   PROGMEM = "Scale";
    const char EN_CIRCLE_OF_FIFTHS_HEADER_DEGREES[] PROGMEM = "Degrees";

    const char EN_CIRCLE_OF_FIFTHS_FMT_HEADER_KEY[] PROGMEM = "{0} major";

    const char EN_CIRCLE_OF_FIFTHS_NO_ACCIDENTALS[] PROGMEM = "no accidentals";
    const char EN_CIRCLE_OF_FIFTHS_UNIT_SHARP_SG[]  PROGMEM = "sharp";
    const char EN_CIRCLE_OF_FIFTHS_UNIT_SHARP_PL[]  PROGMEM = "sharps";
    const char EN_CIRCLE_OF_FIFTHS_UNIT_FLAT_SG[]   PROGMEM = "flat";
    const char EN_CIRCLE_OF_FIFTHS_UNIT_FLAT_PL[]   PROGMEM = "flats";

    const char EN_CIRCLE_OF_FIFTHS_FMT_SUB_NONE[]      PROGMEM = "{0} major (no accidentals)";
    const char EN_CIRCLE_OF_FIFTHS_FMT_SUB_ONE_SHARP[] PROGMEM = "{0} major (1 sharp)";
    const char EN_CIRCLE_OF_FIFTHS_FMT_SUB_ONE_FLAT[]  PROGMEM = "{0} major (1 flat)";
    const char EN_CIRCLE_OF_FIFTHS_FMT_SUB_N_SHARPS[]  PROGMEM = "{0} major ({1:%d} sharps)";
    const char EN_CIRCLE_OF_FIFTHS_FMT_SUB_N_FLATS[]   PROGMEM = "{0} major ({1:%d} flats)";

    // Würfelwerkzeug
    const char EN_DICE_HELP_TITLE[] PROGMEM = "DICE - GUIDE";
    const char EN_DICE_HELP_TEXT[]  PROGMEM =
        "Genuine random numbers for external games.\n\n"
        "ROLL: Tap the die.\n\n"
        "MODE: The footer buttons switch between one and two dice. In two-dice mode the card shows the sum.\n\n"
        "RANDOMNESS: The distribution of values is perfectly even.\n\n"
        "HISTORY: The ten most recent rolls. Not saved.";
    const char EN_DICE_BTN_ONE[]       PROGMEM = "1 die";
    const char EN_DICE_BTN_TWO[]       PROGMEM = "2 dice";
    const char EN_DICE_LABEL_RESULT[]  PROGMEM = "RESULT";
    const char EN_DICE_LABEL_HISTORY[] PROGMEM = "HISTORY";
    const char EN_DICE_HISTORY_EMPTY[] PROGMEM = "no rolls yet";
    const char EN_DICE_SUB_ROLLING[]   PROGMEM = "Rolling...";
    const char EN_DICE_SUB_ONE_DIE[]   PROGMEM = "One die";
    const char EN_DICE_SUB_TWO_DICE[]  PROGMEM = "Two dice";

    // 2048-Tool
    const char EN_GAME_2048_HELP_TITLE[]  PROGMEM = "2048 - GUIDE";
    const char EN_GAME_2048_HELP_TEXT[]   PROGMEM =
        "Goal: Two identical tiles merge into one with the doubled sum. "
        "Reach 2048!\n\n"
        "Controls: Swipe up, down, left or right. All tiles slide together "
        "in the chosen direction.\n\n"
        "Merging: Adjacent tiles with the same value merge into one tile "
        "with the doubled sum. Each tile merges only once per move.\n\n"
        "New tile: After each move a new 2 (usually) or 4 appears at "
        "random.\n\n"
        "End: Game Over when no move is possible.";
    const char EN_GAME_2048_RESET_TITLE[] PROGMEM = "RESET BEST SCORE?";
    const char EN_GAME_2048_RESET_TEXT[]  PROGMEM =
        "The stored best score will be irreversibly reduced to the current "
        "score. The ongoing game is not affected.";
    const char EN_GAME_2048_SUB_PLAYING[]  PROGMEM = "Swipe to slide the tiles";
    const char EN_GAME_2048_SUB_WON[]      PROGMEM = "2048 reached - continue or restart?";
    const char EN_GAME_2048_SUB_GAMEOVER[] PROGMEM = "Board full - start a new round";
    const char EN_GAME_2048_LABEL_SCORE[]  PROGMEM = "Score";
    const char EN_GAME_2048_LABEL_BEST[]   PROGMEM = "Best";
    const char EN_GAME_2048_LABEL_HINT[]   PROGMEM = "Swipe";
    const char EN_GAME_2048_OVERLAY_WON_TITLE[]  PROGMEM = "YOU WIN!";
    const char EN_GAME_2048_OVERLAY_WON_LINE2[]  PROGMEM = "2048 reached";
    const char EN_GAME_2048_OVERLAY_LOST_TITLE[] PROGMEM = "Game Over";
    const char EN_GAME_2048_OVERLAY_LOST_LINE2[] PROGMEM = "No moves left";
    const char EN_GAME_2048_BTN_NEW[]      PROGMEM = "new";
    const char EN_GAME_2048_BTN_RESET[]    PROGMEM = "reset";
    const char EN_GAME_2048_BTN_CONTINUE[] PROGMEM = "continue";

    // Lights-Out-Tool
    const char EN_LIGHTS_OUT_HELP_TITLE[]  PROGMEM = "LIGHTS OUT - GUIDE";
    const char EN_LIGHTS_OUT_HELP_TEXT[]   PROGMEM =
        "Goal: Turn off all 25 lights.\n\n"
        "TAP: A tap inverts the cell and its 4 neighbours. Diagonals stay.\n\n"
        "DIFFICULTY: easy/medium/hard = 3/7/12 random taps to scramble. Always solvable.\n\n"
        "SCORING: Moves and time start with the first tap. Each level stores solved games and best time.\n\n"
        "NEW: New game on the current level.\n"
        "RESET: After confirmation, clears all statistics.";
    const char EN_LIGHTS_OUT_RESET_TITLE[] PROGMEM = "Reset statistics";
    const char EN_LIGHTS_OUT_RESET_TEXT[]  PROGMEM = "Really delete the stored solved counts and best times?";
    const char EN_LIGHTS_OUT_DIFF_EASY[]   PROGMEM = "easy";
    const char EN_LIGHTS_OUT_DIFF_MEDIUM[] PROGMEM = "medium";
    const char EN_LIGHTS_OUT_DIFF_HARD[]   PROGMEM = "hard";
    const char EN_LIGHTS_OUT_LABEL_MOVES[]    PROGMEM = "Moves";
    const char EN_LIGHTS_OUT_LABEL_TIME[]     PROGMEM = "Time";
    const char EN_LIGHTS_OUT_LABEL_STATS[]    PROGMEM = "Statistics";
    const char EN_LIGHTS_OUT_STATUS_SOLVED[]  PROGMEM = "Solved!";
    const char EN_LIGHTS_OUT_STATUS_READY[]   PROGMEM = "Ready";
    const char EN_LIGHTS_OUT_STATUS_PLAYING[] PROGMEM = "Playing";
    const char EN_LIGHTS_OUT_SUB_TITLE[]      PROGMEM = "Lights Out";
    const char EN_LIGHTS_OUT_FMT_SUB_DIFFICULTY[] PROGMEM = "Difficulty: {0}";
    const char EN_LIGHTS_OUT_BTN_NEW[]        PROGMEM = "new";
    const char EN_LIGHTS_OUT_BTN_RESET[]      PROGMEM = "reset";

    // Mühlewerkzeug
    const char EN_MILL_SUB_PLACING[] PROGMEM = "Placement phase - tap a free point";
    const char EN_MILL_SUB_MOVING[]  PROGMEM = "Movement phase - select a piece and move it";
    const char EN_MILL_SUB_FLYING[]  PROGMEM = "Flying phase - jump to any free point";
    const char EN_MILL_SUB_CAPTURE[] PROGMEM = "You must capture a black piece";
    const char EN_MILL_SUB_BOT[]     PROGMEM = "The bot is to move";
    const char EN_MILL_SUB_WON[]     PROGMEM = "You won";
    const char EN_MILL_SUB_LOST[]    PROGMEM = "The bot won";
    const char EN_MILL_SUB_DRAW[]    PROGMEM = "Draw";

    const char EN_MILL_HELP_TITLE[] PROGMEM = "NINE MEN'S MORRIS - GUIDE";
    const char EN_MILL_HELP_TEXT[]  PROGMEM =
        "Each player has nine pieces. Fewer than three or no possible move "
        "= you lose.\n\n"
        "PLACE: Tap nine free points.\n\n"
        "MOVE: Select a piece (blue ring), then a free adjacent point "
        "(green halo).\n\n"
        "FLY: With only three pieces left, jump to any free point.\n\n"
        "MILL: Three pieces in a row capture a bot piece (marked red). "
        "Pieces inside mills are protected.";

    const char EN_MILL_RESET_TITLE[] PROGMEM = "RESET RECORD?";
    const char EN_MILL_RESET_TEXT[]  PROGMEM =
        "The stored record (wins, losses and draws) will be irreversibly "
        "cleared. The ongoing game is not affected.";

    const char EN_MILL_LABEL_PHASE[]  PROGMEM = "Phase";
    const char EN_MILL_LABEL_TURN[]   PROGMEM = "To move";
    const char EN_MILL_LABEL_STONES[] PROGMEM = "Pieces";
    const char EN_MILL_LABEL_WINS[]   PROGMEM = "Wins";
    const char EN_MILL_LABEL_LOSSES[] PROGMEM = "Losses";
    const char EN_MILL_LABEL_DRAWS[]  PROGMEM = "Draws";

    const char EN_MILL_HINT_DEFAULT[]      PROGMEM = "...";
    const char EN_MILL_HINT_WON[]          PROGMEM = "You won!";
    const char EN_MILL_HINT_LOST[]         PROGMEM = "The bot won.";
    const char EN_MILL_HINT_DRAW[]         PROGMEM = "A draw.";
    const char EN_MILL_HINT_CAPTURE[]      PROGMEM = "Capture a bot piece";
    const char EN_MILL_HINT_BOT_THINKING[] PROGMEM = "Bot is thinking...";
    const char EN_MILL_HINT_TAP_FREE[]     PROGMEM = "Tap a free point";
    const char EN_MILL_HINT_JUMP_FREE[]    PROGMEM = "Jump to any free point";
    const char EN_MILL_HINT_PICK_TARGET[]  PROGMEM = "Choose a target square";
    const char EN_MILL_HINT_PICK_OWN[]     PROGMEM = "Choose one of your pieces";

    const char EN_MILL_PHASE_PLACING[] PROGMEM = "Placing";
    const char EN_MILL_PHASE_MOVING[]  PROGMEM = "Moving";
    const char EN_MILL_PHASE_FLYING[]  PROGMEM = "Flying";

    const char EN_MILL_TURN_YOU[] PROGMEM = "You";
    const char EN_MILL_TURN_BOT[] PROGMEM = "Bot";

    const char EN_MILL_FMT_STONES_YOU[] PROGMEM = "You: hand {0:%u}  board {1:%u}";
    const char EN_MILL_FMT_STONES_BOT[] PROGMEM = "Bot: hand {0:%u}  board {1:%u}";

    const char EN_MILL_OVERLAY_WON_TITLE[]  PROGMEM = "YOU WIN!";
    const char EN_MILL_OVERLAY_WON_LINE2[]  PROGMEM = "You defeated the bot";
    const char EN_MILL_OVERLAY_LOST_TITLE[] PROGMEM = "You lost";
    const char EN_MILL_OVERLAY_LOST_LINE2[] PROGMEM = "The bot was stronger";
    const char EN_MILL_OVERLAY_DRAW_TITLE[] PROGMEM = "Draw";
    const char EN_MILL_OVERLAY_DRAW_LINE2[] PROGMEM = "No decision";

    const char EN_MILL_BTN_EASY[]   PROGMEM = "easy";
    const char EN_MILL_BTN_MEDIUM[] PROGMEM = "medium";
    const char EN_MILL_BTN_NEW[]    PROGMEM = "new";
    const char EN_MILL_BTN_RESET[]  PROGMEM = "reset";

    // PeriodicTable: Kategorie-Namen (lang, Popup)
    const char EN_PERIODIC_TABLE_CAT_ALKALI_LONG[]          PROGMEM = "alkali metal";
    const char EN_PERIODIC_TABLE_CAT_ALKALINE_EARTH_LONG[]  PROGMEM = "alkaline earth metal";
    const char EN_PERIODIC_TABLE_CAT_TRANSITION_LONG[]      PROGMEM = "transition metal";
    const char EN_PERIODIC_TABLE_CAT_POST_TRANSITION_LONG[] PROGMEM = "post-transition metal";
    const char EN_PERIODIC_TABLE_CAT_METALLOID_LONG[]       PROGMEM = "metalloid";
    const char EN_PERIODIC_TABLE_CAT_NONMETAL_LONG[]        PROGMEM = "reactive non-metal";
    const char EN_PERIODIC_TABLE_CAT_NOBLE_GAS_LONG[]       PROGMEM = "noble gas";
    const char EN_PERIODIC_TABLE_CAT_LANTHANIDE_LONG[]      PROGMEM = "lanthanide";
    const char EN_PERIODIC_TABLE_CAT_ACTINIDE_LONG[]        PROGMEM = "actinide";
    const char EN_PERIODIC_TABLE_CAT_UNKNOWN_LONG[]         PROGMEM = "unknown properties";

    // PeriodicTable: Kategorie-Namen (kurz, Legende)
    const char EN_PERIODIC_TABLE_CAT_ALKALI_SHORT[]          PROGMEM = "alkali metal";
    const char EN_PERIODIC_TABLE_CAT_ALKALINE_EARTH_SHORT[]  PROGMEM = "alkaline earth";
    const char EN_PERIODIC_TABLE_CAT_TRANSITION_SHORT[]      PROGMEM = "transition metal";
    const char EN_PERIODIC_TABLE_CAT_POST_TRANSITION_SHORT[] PROGMEM = "post-transition";
    const char EN_PERIODIC_TABLE_CAT_METALLOID_SHORT[]       PROGMEM = "metalloid";
    const char EN_PERIODIC_TABLE_CAT_NONMETAL_SHORT[]        PROGMEM = "non-metal";
    const char EN_PERIODIC_TABLE_CAT_NOBLE_GAS_SHORT[]       PROGMEM = "noble gas";
    const char EN_PERIODIC_TABLE_CAT_LANTHANIDE_SHORT[]      PROGMEM = "lanthanide";
    const char EN_PERIODIC_TABLE_CAT_ACTINIDE_SHORT[]        PROGMEM = "actinide";
    const char EN_PERIODIC_TABLE_CAT_UNKNOWN_SHORT[]         PROGMEM = "unknown";

    // PeriodicTable: Aggregatzustände
    const char EN_PERIODIC_TABLE_STATE_SOLID[]   PROGMEM = "solid";
    const char EN_PERIODIC_TABLE_STATE_LIQUID[]  PROGMEM = "liquid";
    const char EN_PERIODIC_TABLE_STATE_GAS[]     PROGMEM = "gas";
    const char EN_PERIODIC_TABLE_STATE_UNKNOWN[] PROGMEM = "unknown";

    // PeriodicTable: Popup-Labels
    const char EN_PERIODIC_TABLE_LABEL_CATEGORY[]     PROGMEM = "Category:";
    const char EN_PERIODIC_TABLE_LABEL_MASS[]         PROGMEM = "Atomic mass:";
    const char EN_PERIODIC_TABLE_LABEL_GROUP_PERIOD[] PROGMEM = "Group / period:";
    const char EN_PERIODIC_TABLE_LABEL_STATE[]        PROGMEM = "State:";
    const char EN_PERIODIC_TABLE_LABEL_DISCOVERED[]   PROGMEM = "Discovered:";
    const char EN_PERIODIC_TABLE_LABEL_DISCOVERER[]   PROGMEM = "Discovered by:";

    // PeriodicTable: Sonderwerte für Entdeckungsjahr
    const char EN_PERIODIC_TABLE_YEAR_ANCIENT[] PROGMEM = "Antiquity";
    const char EN_PERIODIC_TABLE_YEAR_UNKNOWN[] PROGMEM = "unknown";

    // Shut-the-Box-Tool
    const char EN_SHUT_THE_BOX_TITLE[]    PROGMEM = "Shut the Box";
    const char EN_SHUT_THE_BOX_SUBTITLE[] PROGMEM = "Dice game for the break";
    const char EN_SHUT_THE_BOX_HELP_TITLE[] PROGMEM = "SHUT THE BOX - GUIDE";
    const char EN_SHUT_THE_BOX_HELP_TEXT[]  PROGMEM =
        "Goal: close all boxes from 1 to 9.\n\n"
        "ROLL: Tap the roll card or the roll button at the bottom. As long as 7, 8 or 9 are open, the game uses two dice.\n\n"
        "PICK: Tap open numbers until their sum matches the roll. \"lay\" closes them.\n\n"
        "1D/2D: Once 7, 8 and 9 are closed, choose one or two dice before the next roll.\n\n"
        "END: If no matching combination exists, the round ends. Lower remaining score is better. Zero is a win.";
    const char EN_SHUT_THE_BOX_RESET_TITLE[] PROGMEM = "Reset statistics";
    const char EN_SHUT_THE_BOX_RESET_TEXT[]  PROGMEM = "Really delete best remaining score and wins?";
    const char EN_SHUT_THE_BOX_SUB_READY[]   PROGMEM = "Roll and close the matching sum";
    const char EN_SHUT_THE_BOX_FMT_SUB_PICK[] PROGMEM = "Choose sum {0:%u}";
    const char EN_SHUT_THE_BOX_SUB_WON[]      PROGMEM = "All boxes closed";
    const char EN_SHUT_THE_BOX_SUB_GAMEOVER[] PROGMEM = "No matching combination";
    const char EN_SHUT_THE_BOX_LABEL_ROLL[]   PROGMEM = "ROLL";
    const char EN_SHUT_THE_BOX_LABEL_PICK[]   PROGMEM = "PICK";
    const char EN_SHUT_THE_BOX_LABEL_OPEN[]   PROGMEM = "OPEN";
    const char EN_SHUT_THE_BOX_LABEL_BEST[]   PROGMEM = "Best rest";
    const char EN_SHUT_THE_BOX_LABEL_WINS[]   PROGMEM = "Wins";
    const char EN_SHUT_THE_BOX_HINT_ROLL[]    PROGMEM = "tap to roll";
    const char EN_SHUT_THE_BOX_OVERLAY_WON_TITLE[]  PROGMEM = "YOU WIN!";
    const char EN_SHUT_THE_BOX_OVERLAY_WON_LINE2[]  PROGMEM = "All boxes closed";
    const char EN_SHUT_THE_BOX_OVERLAY_LOST_TITLE[] PROGMEM = "Round over";
    const char EN_SHUT_THE_BOX_OVERLAY_LOST_LINE2[] PROGMEM = "No pick fits";
    const char EN_SHUT_THE_BOX_BTN_ONE[]   PROGMEM = "1D";
    const char EN_SHUT_THE_BOX_BTN_TWO[]   PROGMEM = "2D";
    const char EN_SHUT_THE_BOX_BTN_ROLL[]  PROGMEM = "roll";
    const char EN_SHUT_THE_BOX_BTN_LAY[]   PROGMEM = "lay";
    const char EN_SHUT_THE_BOX_BTN_NEW[]   PROGMEM = "new";
    const char EN_SHUT_THE_BOX_BTN_RESET[] PROGMEM = "reset";

    // Normal lotto tool
    const char EN_LOTTO_NORMAL_TITLE[] PROGMEM = "Lotto 6 from 49";
    const char EN_LOTTO_NORMAL_PAGE_TITLE[] PROGMEM = "Lotto 6 from 49";
    const char EN_LOTTO_NORMAL_SUBTITLE[] PROGMEM = "Standard ticket with 6 numbers";
    const char EN_LOTTO_NORMAL_HELP_TITLE[] PROGMEM = "LOTTO 6 FROM 49";
    const char EN_LOTTO_NORMAL_HELP_TEXT[] PROGMEM =
        "Standard lotto ticket with six numbers and super number.\n\n"
        "In ticket mode, mark exactly six numbers and choose the super number in the draw panel on the right.\n\n"
        "The die fills the current area randomly. The trash can clears it.\n\n"
        "Use \"draw\" for six drawn numbers and \"ticket\" to switch back.\n\n"
        "The table shows the reached prize class, no prize amounts.";
    const char EN_LOTTO_NORMAL_HINT_DRAW_SUPER_MISSING[] PROGMEM = "Draw super number missing";

    // Flight radar tool
    const char EN_FLIGHT_RADAR_TITLE[] PROGMEM = "Flight radar";
    const char EN_FLIGHT_RADAR_SUBTITLE[] PROGMEM = "Aircraft near weather location";
    const char EN_FLIGHT_RADAR_FMT_SUBTITLE[] PROGMEM = "Aircraft within {0:%u} km of {1}";
    const char EN_FLIGHT_RADAR_HELP_TITLE[] PROGMEM = "FLIGHT RADAR";
    const char EN_FLIGHT_RADAR_HELP_TEXT[] PROGMEM =
        "The buttons at the bottom select the search radius. The ring button switches the labels on the radar: callsign with altitude and speed, callsign only, or symbol only.\n\n"
        "The radar shows aircraft within the selected radius around the saved weather location. After a change, the new area is loaded again. Tapping a row in the list opens the aircraft details. The thin bar at the right edge runs down until the next update.\n\n"
        "Flight data by the OpenSky Network, free for non-commercial use. With OpenSky credentials in the settings, data is updated more often.";
    const char EN_FLIGHT_RADAR_LABEL_COUNT[] PROGMEM = "Aircraft";
    const char EN_FLIGHT_RADAR_OFFLINE_TITLE[] PROGMEM = "No Wi-Fi connection";
    const char EN_FLIGHT_RADAR_AUTH_ACCOUNT[] PROGMEM = "Account";
    const char EN_FLIGHT_RADAR_AUTH_GUEST[] PROGMEM = "Guest";
    const char EN_FLIGHT_RADAR_LABEL_GROUND[] PROGMEM = "on ground";
    const char EN_FLIGHT_RADAR_FMT_MORE[] PROGMEM = "+{0:%u} more";
    const char EN_FLIGHT_RADAR_DETAIL_TITLE_FMT[] PROGMEM = "Aircraft {0}";
    const char EN_FLIGHT_RADAR_DETAIL_CALLSIGN[] PROGMEM = "Callsign";
    const char EN_FLIGHT_RADAR_DETAIL_ICAO24[] PROGMEM = "ICAO24";
    const char EN_FLIGHT_RADAR_DETAIL_COUNTRY[] PROGMEM = "Origin country";
    const char EN_FLIGHT_RADAR_DETAIL_DISTANCE[] PROGMEM = "Distance";
    const char EN_FLIGHT_RADAR_DETAIL_ALTITUDE[] PROGMEM = "Altitude";
    const char EN_FLIGHT_RADAR_DETAIL_SPEED[] PROGMEM = "Speed";
    const char EN_FLIGHT_RADAR_DETAIL_TRACK[] PROGMEM = "Track";
    const char EN_FLIGHT_RADAR_DETAIL_LAST_SEEN[] PROGMEM = "Last seen";
    const char EN_FLIGHT_RADAR_VALUE_UNKNOWN[] PROGMEM = "unknown";
    const char EN_FLIGHT_RADAR_VALUE_JUST_NOW[] PROGMEM = "just now";
    const char EN_FLIGHT_RADAR_FMT_SECONDS_AGO[] PROGMEM = "{0:%u} s ago";
    const char EN_FLIGHT_RADAR_FMT_MINUTES_AGO[] PROGMEM = "{0:%u} min ago";
    const char EN_FLIGHT_RADAR_FMT_TRACK[] PROGMEM = "{0:%.0f} deg";
    const char EN_FLIGHT_RADAR_LOCATION_UNKNOWN[] PROGMEM = "location";
    const char EN_FLIGHT_RADAR_COMPASS_NORTH[] PROGMEM = "N";
    const char EN_FLIGHT_RADAR_COMPASS_EAST[] PROGMEM = "E";
    const char EN_FLIGHT_RADAR_COMPASS_SOUTH[] PROGMEM = "S";
    const char EN_FLIGHT_RADAR_COMPASS_WEST[] PROGMEM = "W";

    // Lotto system tool
    const char EN_LOTTO_TITLE[]    PROGMEM = "System lotto 6 from 49";
    const char EN_LOTTO_PAGE_TITLE[] PROGMEM = "Lotto 6 from 49 system play (BER)";
    const char EN_LOTTO_SUBTITLE[] PROGMEM = "Full system 007 to 011";
    const char EN_LOTTO_HELP_TITLE[] PROGMEM = "LOTTO 6 FROM 49 - FULL SYSTEM (BER)";
    const char EN_LOTTO_HELP_TEXT[]  PROGMEM =
        "Full system 007 to 011 for Berlin/Brandenburg.\n\n"
        "Choose the system at the bottom. In ticket mode, mark 7 to 11 ticket numbers.\n\n"
        "The die fills the current area randomly. The trash can clears it.\n\n"
        "Use \"draw\" for six drawn numbers and \"ticket\" to switch back. Open the super number in the draw panel on the right.\n\n"
        "The table shows hits per prize class, no prize amounts.";
    const char EN_LOTTO_LABEL_TICKET[]    PROGMEM = "Ticket";
    const char EN_LOTTO_LABEL_DRAW[]      PROGMEM = "Draw";
    const char EN_LOTTO_LABEL_FIELDS[]    PROGMEM = "Fields";
    const char EN_LOTTO_LABEL_PRICE[]     PROGMEM = "Stake";
    const char EN_LOTTO_LABEL_SUPER[]     PROGMEM = "SN";
    const char EN_LOTTO_LABEL_TICKET_SUPER[] PROGMEM = "Tip SN";
    const char EN_LOTTO_LABEL_DRAW_SUPER[]   PROGMEM = "Draw SN";
    const char EN_LOTTO_DIALOG_SUPER[]    PROGMEM = "Super number";
    const char EN_LOTTO_DIALOG_TICKET_SUPER[] PROGMEM = "picked super number";
    const char EN_LOTTO_DIALOG_DRAW_SUPER[]   PROGMEM = "drawn super number";
    const char EN_LOTTO_LABEL_WINS[]      PROGMEM = "Prize classes";
    const char EN_LOTTO_PRIZE_6_SUPER[]   PROGMEM = "6 correct + SN";
    const char EN_LOTTO_PRIZE_6[]         PROGMEM = "6 correct";
    const char EN_LOTTO_PRIZE_5_SUPER[]   PROGMEM = "5 correct + SN";
    const char EN_LOTTO_PRIZE_5[]         PROGMEM = "5 correct";
    const char EN_LOTTO_PRIZE_4_SUPER[]   PROGMEM = "4 correct + SN";
    const char EN_LOTTO_PRIZE_4[]         PROGMEM = "4 correct";
    const char EN_LOTTO_PRIZE_3_SUPER[]   PROGMEM = "3 correct + SN";
    const char EN_LOTTO_PRIZE_3[]         PROGMEM = "3 correct";
    const char EN_LOTTO_PRIZE_2_SUPER[]   PROGMEM = "2 correct + SN";
    const char EN_LOTTO_EMPTY_TICKET[]    PROGMEM = "no ticket numbers";
    const char EN_LOTTO_EMPTY_DRAW[]      PROGMEM = "no draw numbers";
    const char EN_LOTTO_HINT_INCOMPLETE[] PROGMEM = "Ticket/draw missing";
    const char EN_LOTTO_YES[]             PROGMEM = "yes";
    const char EN_LOTTO_NO[]              PROGMEM = "no";
    const char EN_LOTTO_FMT_SUB_TICKET[]  PROGMEM = "Ticket numbers {0:%u}/{1:%u}";
    const char EN_LOTTO_FMT_SUB_DRAW[]    PROGMEM = "Draw numbers {0:%u}/6";
    const char EN_LOTTO_SUB_READY[]       PROGMEM = "Prize classes counted";
    const char EN_LOTTO_FMT_SUMMARY_TICKET_SUPER[] PROGMEM = "picked {0}";
    const char EN_LOTTO_FMT_SUMMARY_DRAW_SUPER[]   PROGMEM = "drawn {0}";
    const char EN_LOTTO_BTN_RANDOM[]      PROGMEM = "rnd";
    const char EN_LOTTO_BTN_CLEAR[]       PROGMEM = "clear";
    const char EN_LOTTO_BTN_TICKET[]      PROGMEM = "ticket";
    const char EN_LOTTO_BTN_DRAW[]        PROGMEM = "draw";

    // SMD resistor code
    const char EN_TOOL_SMD_RESISTOR_TITLE[]    PROGMEM = "SMD resistor";
    const char EN_TOOL_SMD_RESISTOR_SUBTITLE[] PROGMEM = "3/4 digits and R code";
    const char EN_SMD_RESISTOR_LABEL_CODE[]     PROGMEM = "Code";
    const char EN_SMD_RESISTOR_LABEL_VALUE[]    PROGMEM = "Value";
    const char EN_SMD_RESISTOR_ERR_EMPTY[]      PROGMEM = "Enter code";
    const char EN_SMD_RESISTOR_ERR_INVALID[]    PROGMEM = "Invalid code";
    const char EN_SMD_RESISTOR_ERR_R_CODE[]     PROGMEM = "Use R once, with digits";
    const char EN_SMD_RESISTOR_NOTE_3_DIGIT[]   PROGMEM = "3-digit code";
    const char EN_SMD_RESISTOR_NOTE_4_DIGIT[]   PROGMEM = "4-digit code";
    const char EN_SMD_RESISTOR_NOTE_R_CODE[]    PROGMEM = "R code";
    const char EN_SMD_RESISTOR_NOTE_ZERO[]      PROGMEM = "Zero-ohm link";
    const char EN_SMD_RESISTOR_MODE_3[]         PROGMEM = "3-digit";
    const char EN_SMD_RESISTOR_MODE_4[]         PROGMEM = "4-digit";
    const char EN_SMD_RESISTOR_MODE_R[]         PROGMEM = "R code";

    // Website QR dialog
    const char EN_QR_MODAL_TITLE[] PROGMEM = "Website";
    const char EN_QR_MODAL_HINT[]  PROGMEM = "This QR code opens a website for the current topic. Only the website operator is responsible for its content.";
    const char EN_QR_MODAL_ERROR[] PROGMEM = "QR code could not be created.";
}

// ===================================================================
// Pointer-Tabelle (DE, EN)
// ===================================================================

// Reihenfolge muss exakt zu WorkshopKey passen. Der static_assert weiter unten schlägt zu, falls Drift entsteht.

namespace
{
    const char *const WORKSHOP_TABLE[][2] PROGMEM = {
        // Sonder-Key
        { DE_EMPTY, EN_EMPTY },

        // Kategorie-Titel
        { DE_CAT_MISC_TITLE,        EN_CAT_MISC_TITLE        },
        { DE_CAT_GAMES_TITLE,       EN_CAT_GAMES_TITLE       },
        { DE_CAT_ELECTRONICS_TITLE, EN_CAT_ELECTRONICS_TITLE },
        { DE_CAT_UNITS_TITLE,       EN_CAT_UNITS_TITLE       },
        { DE_CAT_DIGITAL_TITLE,     EN_CAT_DIGITAL_TITLE     },
        { DE_CAT_KNITTING_TITLE,    EN_CAT_KNITTING_TITLE    },
        { DE_CAT_MUSIC_TITLE,       EN_CAT_MUSIC_TITLE       },

        // Kategorie-Kurzbeschreibungen
        { DE_CAT_MISC_SUMMARY,        EN_CAT_MISC_SUMMARY        },
        { DE_CAT_GAMES_SUMMARY,       EN_CAT_GAMES_SUMMARY       },
        { DE_CAT_ELECTRONICS_SUMMARY, EN_CAT_ELECTRONICS_SUMMARY },
        { DE_CAT_UNITS_SUMMARY,       EN_CAT_UNITS_SUMMARY       },
        { DE_CAT_DIGITAL_SUMMARY,     EN_CAT_DIGITAL_SUMMARY     },
        { DE_CAT_KNITTING_SUMMARY,    EN_CAT_KNITTING_SUMMARY    },
        { DE_CAT_MUSIC_SUMMARY,       EN_CAT_MUSIC_SUMMARY       },

        // Werkzeug-Titel
        { DE_TOOL_CALCULATOR_TITLE,         EN_TOOL_CALCULATOR_TITLE         },
        { DE_TOOL_WORLD_CLOCK_TITLE,        EN_TOOL_WORLD_CLOCK_TITLE        },
        { DE_TOOL_COLOR_CONVERTER_TITLE,    EN_TOOL_COLOR_CONVERTER_TITLE    },
        { DE_TOOL_PERIODIC_TABLE_TITLE,     EN_TOOL_PERIODIC_TABLE_TITLE     },
        { DE_TOOL_SCREW_REFERENCE_TITLE,    EN_TOOL_SCREW_REFERENCE_TITLE    },
        { DE_TOOL_BMI_CALCULATOR_TITLE,     EN_TOOL_BMI_CALCULATOR_TITLE     },
        { DE_TOOL_GAME_2048_TITLE,          EN_TOOL_GAME_2048_TITLE          },
        { DE_TOOL_MILL_GAME_TITLE,          EN_TOOL_MILL_GAME_TITLE          },
        { DE_TOOL_LIGHTS_OUT_TITLE,         EN_TOOL_LIGHTS_OUT_TITLE         },
        { DE_TOOL_DICE_TITLE,               EN_TOOL_DICE_TITLE               },
        { DE_TOOL_RESISTOR_TITLE,           EN_TOOL_RESISTOR_TITLE           },
        { DE_TOOL_OHM_CALCULATOR_TITLE,     EN_TOOL_OHM_CALCULATOR_TITLE     },
        { DE_TOOL_VOLTAGE_DIVIDER_TITLE,    EN_TOOL_VOLTAGE_DIVIDER_TITLE    },
        { DE_TOOL_LED_CALCULATOR_TITLE,     EN_TOOL_LED_CALCULATOR_TITLE     },
        { DE_TOOL_RC_FILTER_TITLE,          EN_TOOL_RC_FILTER_TITLE          },
        { DE_TOOL_LC_FILTER_TITLE,          EN_TOOL_LC_FILTER_TITLE          },
        { DE_TOOL_UKW_RESONATOR_TITLE,      EN_TOOL_UKW_RESONATOR_TITLE      },
        { DE_TOOL_LM317_TITLE,              EN_TOOL_LM317_TITLE              },
        { DE_TOOL_DECIBEL_TITLE,            EN_TOOL_DECIBEL_TITLE            },
        { DE_TOOL_NE555_TITLE,              EN_TOOL_NE555_TITLE              },
        { DE_TOOL_OPAMP_TITLE,              EN_TOOL_OPAMP_TITLE              },
        { DE_TOOL_INCH_MM_TITLE,            EN_TOOL_INCH_MM_TITLE            },
        { DE_TOOL_TEMP_TITLE,               EN_TOOL_TEMP_TITLE               },
        { DE_TOOL_PRESSURE_TITLE,           EN_TOOL_PRESSURE_TITLE           },
        { DE_TOOL_SI_PREFIXES_TITLE,        EN_TOOL_SI_PREFIXES_TITLE        },
        { DE_TOOL_ASCII_TABLE_TITLE,        EN_TOOL_ASCII_TABLE_TITLE        },
        { DE_TOOL_BIT_EDITOR_TITLE,         EN_TOOL_BIT_EDITOR_TITLE         },
        { DE_TOOL_LOGIC_GATE_TITLE,         EN_TOOL_LOGIC_GATE_TITLE         },
        { DE_TOOL_STITCH_GAUGE_TITLE,       EN_TOOL_STITCH_GAUGE_TITLE       },
        { DE_TOOL_STITCH_DISTRIBUTOR_TITLE, EN_TOOL_STITCH_DISTRIBUTOR_TITLE },
        { DE_TOOL_SLEEVE_TAPER_TITLE,       EN_TOOL_SLEEVE_TAPER_TITLE       },
        { DE_TOOL_CIRCLE_OF_FIFTHS_TITLE,   EN_TOOL_CIRCLE_OF_FIFTHS_TITLE   },
        { DE_TOOL_METRONOME_TITLE,          EN_TOOL_METRONOME_TITLE          },

        // Werkzeug-Untertitel
        { DE_TOOL_CALCULATOR_SUBTITLE,         EN_TOOL_CALCULATOR_SUBTITLE         },
        { DE_TOOL_WORLD_CLOCK_SUBTITLE,        EN_TOOL_WORLD_CLOCK_SUBTITLE        },
        { DE_TOOL_COLOR_CONVERTER_SUBTITLE,    EN_TOOL_COLOR_CONVERTER_SUBTITLE    },
        { DE_TOOL_PERIODIC_TABLE_SUBTITLE,     EN_TOOL_PERIODIC_TABLE_SUBTITLE     },
        { DE_TOOL_SCREW_REFERENCE_SUBTITLE,    EN_TOOL_SCREW_REFERENCE_SUBTITLE    },
        { DE_TOOL_BMI_CALCULATOR_SUBTITLE,     EN_TOOL_BMI_CALCULATOR_SUBTITLE     },
        { DE_TOOL_GAME_2048_SUBTITLE,          EN_TOOL_GAME_2048_SUBTITLE          },
        { DE_TOOL_MILL_GAME_SUBTITLE,          EN_TOOL_MILL_GAME_SUBTITLE          },
        { DE_TOOL_LIGHTS_OUT_SUBTITLE,         EN_TOOL_LIGHTS_OUT_SUBTITLE         },
        { DE_TOOL_DICE_SUBTITLE,               EN_TOOL_DICE_SUBTITLE               },
        { DE_TOOL_RESISTOR_SUBTITLE,           EN_TOOL_RESISTOR_SUBTITLE           },
        { DE_TOOL_OHM_CALCULATOR_SUBTITLE,     EN_TOOL_OHM_CALCULATOR_SUBTITLE     },
        { DE_TOOL_VOLTAGE_DIVIDER_SUBTITLE,    EN_TOOL_VOLTAGE_DIVIDER_SUBTITLE    },
        { DE_TOOL_LED_CALCULATOR_SUBTITLE,     EN_TOOL_LED_CALCULATOR_SUBTITLE     },
        { DE_TOOL_RC_FILTER_SUBTITLE,          EN_TOOL_RC_FILTER_SUBTITLE          },
        { DE_TOOL_LC_FILTER_SUBTITLE,          EN_TOOL_LC_FILTER_SUBTITLE          },
        { DE_TOOL_UKW_RESONATOR_SUBTITLE,      EN_TOOL_UKW_RESONATOR_SUBTITLE      },
        { DE_TOOL_LM317_SUBTITLE,              EN_TOOL_LM317_SUBTITLE              },
        { DE_TOOL_DECIBEL_SUBTITLE,            EN_TOOL_DECIBEL_SUBTITLE            },
        { DE_TOOL_NE555_SUBTITLE,              EN_TOOL_NE555_SUBTITLE              },
        { DE_TOOL_OPAMP_SUBTITLE,              EN_TOOL_OPAMP_SUBTITLE              },
        { DE_TOOL_INCH_MM_SUBTITLE,            EN_TOOL_INCH_MM_SUBTITLE            },
        { DE_TOOL_TEMP_SUBTITLE,               EN_TOOL_TEMP_SUBTITLE               },
        { DE_TOOL_PRESSURE_SUBTITLE,           EN_TOOL_PRESSURE_SUBTITLE           },
        { DE_TOOL_SI_PREFIXES_SUBTITLE,        EN_TOOL_SI_PREFIXES_SUBTITLE        },
        { DE_TOOL_ASCII_TABLE_SUBTITLE,        EN_TOOL_ASCII_TABLE_SUBTITLE        },
        { DE_TOOL_BIT_EDITOR_SUBTITLE,         EN_TOOL_BIT_EDITOR_SUBTITLE         },
        { DE_TOOL_LOGIC_GATE_SUBTITLE,         EN_TOOL_LOGIC_GATE_SUBTITLE         },
        { DE_TOOL_STITCH_GAUGE_SUBTITLE,       EN_TOOL_STITCH_GAUGE_SUBTITLE       },
        { DE_TOOL_STITCH_DISTRIBUTOR_SUBTITLE, EN_TOOL_STITCH_DISTRIBUTOR_SUBTITLE },
        { DE_TOOL_SLEEVE_TAPER_SUBTITLE,       EN_TOOL_SLEEVE_TAPER_SUBTITLE       },
        { DE_TOOL_CIRCLE_OF_FIFTHS_SUBTITLE,   EN_TOOL_CIRCLE_OF_FIFTHS_SUBTITLE   },
        { DE_TOOL_METRONOME_SUBTITLE,          EN_TOOL_METRONOME_SUBTITLE          },

        // Abweichende Menübeschriftung für SCREW_REFERENCE
        { DE_TOOL_SCREW_REFERENCE_LABEL, EN_TOOL_SCREW_REFERENCE_LABEL },

        // Fehlermeldungen
        { DE_ERR_HEADER,           EN_ERR_HEADER           },
        { DE_ERR_TOOL_LOAD_FAILED, EN_ERR_TOOL_LOAD_FAILED },

        // Temp-Tool
        { DE_TEMP_LABEL_CELSIUS,    EN_TEMP_LABEL_CELSIUS    },
        { DE_TEMP_LABEL_FAHRENHEIT, EN_TEMP_LABEL_FAHRENHEIT },
        { DE_TEMP_LABEL_KELVIN,     EN_TEMP_LABEL_KELVIN     },

        { DE_TEMP_UNIT_CELSIUS,    EN_TEMP_UNIT_CELSIUS    },
        { DE_TEMP_UNIT_FAHRENHEIT, EN_TEMP_UNIT_FAHRENHEIT },
        { DE_TEMP_UNIT_KELVIN,     EN_TEMP_UNIT_KELVIN     },

        { DE_TEMP_REF_M40, EN_TEMP_REF_M40 },
        { DE_TEMP_REF_0,   EN_TEMP_REF_0   },
        { DE_TEMP_REF_20,  EN_TEMP_REF_20  },
        { DE_TEMP_REF_37,  EN_TEMP_REF_37  },
        { DE_TEMP_REF_100, EN_TEMP_REF_100 },
        { DE_TEMP_REF_150, EN_TEMP_REF_150 },
        { DE_TEMP_REF_200, EN_TEMP_REF_200 },

        { DE_TEMP_FORMULA_C2F, EN_TEMP_FORMULA_C2F },
        { DE_TEMP_FORMULA_F2C, EN_TEMP_FORMULA_F2C },
        { DE_TEMP_FORMULA_K2C, EN_TEMP_FORMULA_K2C },

        // InchMm-Tool
        { DE_INCHMM_LABEL_INCH, EN_INCHMM_LABEL_INCH },
        { DE_INCHMM_LABEL_MM,   EN_INCHMM_LABEL_MM   },

        { DE_INCHMM_UNIT_INCH, EN_INCHMM_UNIT_INCH },
        { DE_INCHMM_UNIT_MM,   EN_INCHMM_UNIT_MM   },

        { DE_INCHMM_FRAC_1_16,  EN_INCHMM_FRAC_1_16  },
        { DE_INCHMM_FRAC_1_8,   EN_INCHMM_FRAC_1_8   },
        { DE_INCHMM_FRAC_3_16,  EN_INCHMM_FRAC_3_16  },
        { DE_INCHMM_FRAC_1_4,   EN_INCHMM_FRAC_1_4   },
        { DE_INCHMM_FRAC_5_16,  EN_INCHMM_FRAC_5_16  },
        { DE_INCHMM_FRAC_3_8,   EN_INCHMM_FRAC_3_8   },
        { DE_INCHMM_FRAC_7_16,  EN_INCHMM_FRAC_7_16  },
        { DE_INCHMM_FRAC_1_2,   EN_INCHMM_FRAC_1_2   },
        { DE_INCHMM_FRAC_9_16,  EN_INCHMM_FRAC_9_16  },
        { DE_INCHMM_FRAC_5_8,   EN_INCHMM_FRAC_5_8   },
        { DE_INCHMM_FRAC_11_16, EN_INCHMM_FRAC_11_16 },
        { DE_INCHMM_FRAC_3_4,   EN_INCHMM_FRAC_3_4   },
        { DE_INCHMM_FRAC_13_16, EN_INCHMM_FRAC_13_16 },
        { DE_INCHMM_FRAC_7_8,   EN_INCHMM_FRAC_7_8   },
        { DE_INCHMM_FRAC_15_16, EN_INCHMM_FRAC_15_16 },
        { DE_INCHMM_FRAC_1,     EN_INCHMM_FRAC_1     },

        { DE_INCHMM_INFO_FACTOR, EN_INCHMM_INFO_FACTOR },

        // Pressure-Tool
        { DE_PRESSURE_UNIT_BAR, EN_PRESSURE_UNIT_BAR },
        { DE_PRESSURE_UNIT_PSI, EN_PRESSURE_UNIT_PSI },
        { DE_PRESSURE_UNIT_ATM, EN_PRESSURE_UNIT_ATM },
        { DE_PRESSURE_UNIT_PA,  EN_PRESSURE_UNIT_PA  },

        { DE_PRESSURE_REF_HDR_APPLICATION, EN_PRESSURE_REF_HDR_APPLICATION },

        { DE_PRESSURE_REF_BLOOD,      EN_PRESSURE_REF_BLOOD      },
        { DE_PRESSURE_REF_ATMOSPHERE, EN_PRESSURE_REF_ATMOSPHERE },
        { DE_PRESSURE_REF_TYRE_CAR,   EN_PRESSURE_REF_TYRE_CAR   },
        { DE_PRESSURE_REF_TYRE_TRUCK, EN_PRESSURE_REF_TYRE_TRUCK },
        { DE_PRESSURE_REF_PNEUMATIC,  EN_PRESSURE_REF_PNEUMATIC  },
        { DE_PRESSURE_REF_WATER_PIPE, EN_PRESSURE_REF_WATER_PIPE },
        { DE_PRESSURE_REF_DIVE_TANK,  EN_PRESSURE_REF_DIVE_TANK  },
        { DE_PRESSURE_REF_HYDRAULIC,  EN_PRESSURE_REF_HYDRAULIC  },

        { DE_PRESSURE_INFO_BAR, EN_PRESSURE_INFO_BAR },
        { DE_PRESSURE_INFO_PSI, EN_PRESSURE_INFO_PSI },
        { DE_PRESSURE_INFO_ATM, EN_PRESSURE_INFO_ATM },
        { DE_PRESSURE_INFO_PA,  EN_PRESSURE_INFO_PA  },

        // WorldClock-Tool
        { DE_WORLDCLOCK_CITY_LOS_ANGELES, EN_WORLDCLOCK_CITY_LOS_ANGELES },
        { DE_WORLDCLOCK_CITY_NEW_YORK,    EN_WORLDCLOCK_CITY_NEW_YORK    },
        { DE_WORLDCLOCK_CITY_LONDON,      EN_WORLDCLOCK_CITY_LONDON      },
        { DE_WORLDCLOCK_CITY_BERLIN,      EN_WORLDCLOCK_CITY_BERLIN      },
        { DE_WORLDCLOCK_CITY_MOSCOW,      EN_WORLDCLOCK_CITY_MOSCOW      },
        { DE_WORLDCLOCK_CITY_DUBAI,       EN_WORLDCLOCK_CITY_DUBAI       },
        { DE_WORLDCLOCK_CITY_TOKYO,       EN_WORLDCLOCK_CITY_TOKYO       },
        { DE_WORLDCLOCK_CITY_SYDNEY,      EN_WORLDCLOCK_CITY_SYDNEY      },

        // StitchDistributor-Tool
        { DE_STITCH_DISTRIBUTOR_LABEL_FROM,    EN_STITCH_DISTRIBUTOR_LABEL_FROM    },
        { DE_STITCH_DISTRIBUTOR_LABEL_TO,      EN_STITCH_DISTRIBUTOR_LABEL_TO      },
        { DE_STITCH_DISTRIBUTOR_UNIT_STITCHES, EN_STITCH_DISTRIBUTOR_UNIT_STITCHES },

        { DE_STITCH_DISTRIBUTOR_TITLE_DISTRIBUTION, EN_STITCH_DISTRIBUTOR_TITLE_DISTRIBUTION },
        { DE_STITCH_DISTRIBUTOR_TITLE_NO_CHANGE,    EN_STITCH_DISTRIBUTOR_TITLE_NO_CHANGE    },
        { DE_STITCH_DISTRIBUTOR_TITLE_INCREASE,     EN_STITCH_DISTRIBUTOR_TITLE_INCREASE     },
        { DE_STITCH_DISTRIBUTOR_TITLE_DECREASE,     EN_STITCH_DISTRIBUTOR_TITLE_DECREASE     },

        { DE_STITCH_DISTRIBUTOR_ACTION_INC_WITH_PLAIN, EN_STITCH_DISTRIBUTOR_ACTION_INC_WITH_PLAIN },
        { DE_STITCH_DISTRIBUTOR_ACTION_DEC_WITH_PLAIN, EN_STITCH_DISTRIBUTOR_ACTION_DEC_WITH_PLAIN },
        { DE_STITCH_DISTRIBUTOR_ACTION_INC_NO_PLAIN,   EN_STITCH_DISTRIBUTOR_ACTION_INC_NO_PLAIN   },
        { DE_STITCH_DISTRIBUTOR_ACTION_DEC_NO_PLAIN,   EN_STITCH_DISTRIBUTOR_ACTION_DEC_NO_PLAIN   },

        { DE_STITCH_DISTRIBUTOR_HINT_EMPTY_1,      EN_STITCH_DISTRIBUTOR_HINT_EMPTY_1      },
        { DE_STITCH_DISTRIBUTOR_HINT_EMPTY_2,      EN_STITCH_DISTRIBUTOR_HINT_EMPTY_2      },
        { DE_STITCH_DISTRIBUTOR_HINT_NO_CHANGE_1,  EN_STITCH_DISTRIBUTOR_HINT_NO_CHANGE_1  },
        { DE_STITCH_DISTRIBUTOR_HINT_NO_CHANGE_2,  EN_STITCH_DISTRIBUTOR_HINT_NO_CHANGE_2  },
        { DE_STITCH_DISTRIBUTOR_HINT_INFEASIBLE_1, EN_STITCH_DISTRIBUTOR_HINT_INFEASIBLE_1 },
        { DE_STITCH_DISTRIBUTOR_HINT_INFEASIBLE_2, EN_STITCH_DISTRIBUTOR_HINT_INFEASIBLE_2 },

        { DE_STITCH_DISTRIBUTOR_FMT_SEG_WITH_PLAIN, EN_STITCH_DISTRIBUTOR_FMT_SEG_WITH_PLAIN },
        { DE_STITCH_DISTRIBUTOR_FMT_SEG_NO_PLAIN,   EN_STITCH_DISTRIBUTOR_FMT_SEG_NO_PLAIN   },

        // SleeveTaper-Tool
        { DE_SLEEVE_TAPER_LABEL_FROM,    EN_SLEEVE_TAPER_LABEL_FROM    },
        { DE_SLEEVE_TAPER_LABEL_TO,      EN_SLEEVE_TAPER_LABEL_TO      },
        { DE_SLEEVE_TAPER_LABEL_OVER,    EN_SLEEVE_TAPER_LABEL_OVER    },
        { DE_SLEEVE_TAPER_UNIT_STITCHES, EN_SLEEVE_TAPER_UNIT_STITCHES },
        { DE_SLEEVE_TAPER_UNIT_ROWS,     EN_SLEEVE_TAPER_UNIT_ROWS     },

        { DE_SLEEVE_TAPER_TITLE_SLEEVE,   EN_SLEEVE_TAPER_TITLE_SLEEVE   },
        { DE_SLEEVE_TAPER_TITLE_NO_TAPER, EN_SLEEVE_TAPER_TITLE_NO_TAPER },
        { DE_SLEEVE_TAPER_TITLE_INCREASE, EN_SLEEVE_TAPER_TITLE_INCREASE },
        { DE_SLEEVE_TAPER_TITLE_DECREASE, EN_SLEEVE_TAPER_TITLE_DECREASE },

        { DE_SLEEVE_TAPER_ACTION_INC_BOTH, EN_SLEEVE_TAPER_ACTION_INC_BOTH },
        { DE_SLEEVE_TAPER_ACTION_DEC_BOTH, EN_SLEEVE_TAPER_ACTION_DEC_BOTH },

        { DE_SLEEVE_TAPER_HINT_EMPTY_1,      EN_SLEEVE_TAPER_HINT_EMPTY_1      },
        { DE_SLEEVE_TAPER_HINT_EMPTY_2,      EN_SLEEVE_TAPER_HINT_EMPTY_2      },
        { DE_SLEEVE_TAPER_HINT_NO_CHANGE_1,  EN_SLEEVE_TAPER_HINT_NO_CHANGE_1  },
        { DE_SLEEVE_TAPER_HINT_NO_CHANGE_2,  EN_SLEEVE_TAPER_HINT_NO_CHANGE_2  },
        { DE_SLEEVE_TAPER_HINT_ODD_1,        EN_SLEEVE_TAPER_HINT_ODD_1        },
        { DE_SLEEVE_TAPER_HINT_ODD_2,        EN_SLEEVE_TAPER_HINT_ODD_2        },
        { DE_SLEEVE_TAPER_HINT_ODD_3,        EN_SLEEVE_TAPER_HINT_ODD_3        },
        { DE_SLEEVE_TAPER_HINT_TOO_MANY_INC, EN_SLEEVE_TAPER_HINT_TOO_MANY_INC },
        { DE_SLEEVE_TAPER_HINT_TOO_MANY_DEC, EN_SLEEVE_TAPER_HINT_TOO_MANY_DEC },
        { DE_SLEEVE_TAPER_HINT_TOO_MANY_2,   EN_SLEEVE_TAPER_HINT_TOO_MANY_2   },

        { DE_SLEEVE_TAPER_FMT_ROWS,     EN_SLEEVE_TAPER_FMT_ROWS     },
        { DE_SLEEVE_TAPER_FMT_ROWS_AND, EN_SLEEVE_TAPER_FMT_ROWS_AND },

        // StitchGauge-Tool
        { DE_STITCH_GAUGE_LABEL_STITCHES_PER_10CM, EN_STITCH_GAUGE_LABEL_STITCHES_PER_10CM },
        { DE_STITCH_GAUGE_LABEL_WIDTH,             EN_STITCH_GAUGE_LABEL_WIDTH             },
        { DE_STITCH_GAUGE_LABEL_STITCHES,          EN_STITCH_GAUGE_LABEL_STITCHES          },
        { DE_STITCH_GAUGE_LABEL_ROWS_PER_10CM,     EN_STITCH_GAUGE_LABEL_ROWS_PER_10CM     },
        { DE_STITCH_GAUGE_LABEL_HEIGHT,            EN_STITCH_GAUGE_LABEL_HEIGHT            },
        { DE_STITCH_GAUGE_LABEL_ROWS,              EN_STITCH_GAUGE_LABEL_ROWS              },

        { DE_STITCH_GAUGE_UNIT_STITCHES, EN_STITCH_GAUGE_UNIT_STITCHES },
        { DE_STITCH_GAUGE_UNIT_CM,       EN_STITCH_GAUGE_UNIT_CM       },
        { DE_STITCH_GAUGE_UNIT_ROWS,     EN_STITCH_GAUGE_UNIT_ROWS     },

        { DE_STITCH_GAUGE_VIS_CM,              EN_STITCH_GAUGE_VIS_CM              },
        { DE_STITCH_GAUGE_VIS_STITCHES,        EN_STITCH_GAUGE_VIS_STITCHES        },
        { DE_STITCH_GAUGE_VIS_ROWS,            EN_STITCH_GAUGE_VIS_ROWS            },
        { DE_STITCH_GAUGE_VIS_STITCHES_PER_10, EN_STITCH_GAUGE_VIS_STITCHES_PER_10 },
        { DE_STITCH_GAUGE_VIS_ROWS_PER_10,     EN_STITCH_GAUGE_VIS_ROWS_PER_10     },

        // BmiCalc-Tool
        { DE_BMI_ZONE_SEVERE_UNDER, EN_BMI_ZONE_SEVERE_UNDER },
        { DE_BMI_ZONE_UNDER,        EN_BMI_ZONE_UNDER        },
        { DE_BMI_ZONE_NORMAL,       EN_BMI_ZONE_NORMAL       },
        { DE_BMI_ZONE_OVER,         EN_BMI_ZONE_OVER         },
        { DE_BMI_ZONE_OBESE_1,      EN_BMI_ZONE_OBESE_1      },
        { DE_BMI_ZONE_OBESE_2,      EN_BMI_ZONE_OBESE_2      },
        { DE_BMI_ZONE_OBESE_3,      EN_BMI_ZONE_OBESE_3      },

        { DE_BMI_LABEL_HEIGHT, EN_BMI_LABEL_HEIGHT },
        { DE_BMI_LABEL_WEIGHT, EN_BMI_LABEL_WEIGHT },

        { DE_BMI_UNIT_CM, EN_BMI_UNIT_CM },
        { DE_BMI_UNIT_KG, EN_BMI_UNIT_KG },

        { DE_BMI_NOTE_ADULTS, EN_BMI_NOTE_ADULTS },

        { DE_BMI_FMT_RESULT, EN_BMI_FMT_RESULT },

        // OhmCalculator-Tool
        { DE_OHM_NAME_VOLTAGE,    EN_OHM_NAME_VOLTAGE    },
        { DE_OHM_NAME_CURRENT,    EN_OHM_NAME_CURRENT    },
        { DE_OHM_NAME_RESISTANCE, EN_OHM_NAME_RESISTANCE },
        { DE_OHM_NAME_POWER,      EN_OHM_NAME_POWER      },

        // Decibel-Tool
        { DE_DECIBEL_SECTION_VOLTAGE, EN_DECIBEL_SECTION_VOLTAGE },
        { DE_DECIBEL_SECTION_POWER,   EN_DECIBEL_SECTION_POWER   },

        { DE_DECIBEL_NAME_INPUT,  EN_DECIBEL_NAME_INPUT  },
        { DE_DECIBEL_NAME_OUTPUT, EN_DECIBEL_NAME_OUTPUT },
        { DE_DECIBEL_NAME_LEVEL,  EN_DECIBEL_NAME_LEVEL  },

        // LogicGate-Tool
        { DE_LOGIC_GATE_DESC_AND,  EN_LOGIC_GATE_DESC_AND  },
        { DE_LOGIC_GATE_DESC_OR,   EN_LOGIC_GATE_DESC_OR   },
        { DE_LOGIC_GATE_DESC_XOR,  EN_LOGIC_GATE_DESC_XOR  },
        { DE_LOGIC_GATE_DESC_NAND, EN_LOGIC_GATE_DESC_NAND },
        { DE_LOGIC_GATE_DESC_NOR,  EN_LOGIC_GATE_DESC_NOR  },
        { DE_LOGIC_GATE_DESC_NOT,  EN_LOGIC_GATE_DESC_NOT  },

        // RcFilter-Tool
        { DE_RC_FILTER_BTN_LOW_PASS,  EN_RC_FILTER_BTN_LOW_PASS  },
        { DE_RC_FILTER_BTN_HIGH_PASS, EN_RC_FILTER_BTN_HIGH_PASS },

        { DE_RC_FILTER_SUB_LP_RC, EN_RC_FILTER_SUB_LP_RC },
        { DE_RC_FILTER_SUB_LP_RL, EN_RC_FILTER_SUB_LP_RL },
        { DE_RC_FILTER_SUB_HP_RC, EN_RC_FILTER_SUB_HP_RC },
        { DE_RC_FILTER_SUB_HP_RL, EN_RC_FILTER_SUB_HP_RL },

        { DE_RC_FILTER_SLOPE_LP, EN_RC_FILTER_SLOPE_LP },
        { DE_RC_FILTER_SLOPE_HP, EN_RC_FILTER_SLOPE_HP },

        // LcFilter-Tool
        { DE_LC_FILTER_BTN_LOW_PASS,  EN_LC_FILTER_BTN_LOW_PASS  },
        { DE_LC_FILTER_BTN_HIGH_PASS, EN_LC_FILTER_BTN_HIGH_PASS },

        { DE_LC_FILTER_SUB_LP, EN_LC_FILTER_SUB_LP },
        { DE_LC_FILTER_SUB_HP, EN_LC_FILTER_SUB_HP },

        { DE_LC_FILTER_SLOPE_LP, EN_LC_FILTER_SLOPE_LP },
        { DE_LC_FILTER_SLOPE_HP, EN_LC_FILTER_SLOPE_HP },

        // UkwResonator-Tool
        { DE_UKW_RESONATOR_SUB_D05, EN_UKW_RESONATOR_SUB_D05 },
        { DE_UKW_RESONATOR_SUB_D08, EN_UKW_RESONATOR_SUB_D08 },
        { DE_UKW_RESONATOR_SUB_D10, EN_UKW_RESONATOR_SUB_D10 },
        { DE_UKW_RESONATOR_SUB_D15, EN_UKW_RESONATOR_SUB_D15 },

        { DE_UKW_RESONATOR_FMT_TURNS,         EN_UKW_RESONATOR_FMT_TURNS         },
        { DE_UKW_RESONATOR_FMT_Q_APPROX,      EN_UKW_RESONATOR_FMT_Q_APPROX      },
        { DE_UKW_RESONATOR_WARN_OVERLAP,      EN_UKW_RESONATOR_WARN_OVERLAP      },
        { DE_UKW_RESONATOR_WARN_OUT_OF_RANGE, EN_UKW_RESONATOR_WARN_OUT_OF_RANGE },

        // Ne555-Tool
        { DE_NE555_BTN_ASTABLE,    EN_NE555_BTN_ASTABLE    },
        { DE_NE555_BTN_MONOSTABLE, EN_NE555_BTN_MONOSTABLE },

        { DE_NE555_SUB_ASTABLE,    EN_NE555_SUB_ASTABLE    },
        { DE_NE555_SUB_MONOSTABLE, EN_NE555_SUB_MONOSTABLE },

        // OpAmp-Tool
        { DE_OPAMP_BTN_INVERTING,     EN_OPAMP_BTN_INVERTING     },
        { DE_OPAMP_BTN_NON_INVERTING, EN_OPAMP_BTN_NON_INVERTING },

        { DE_OPAMP_SUB_INVERTING,     EN_OPAMP_SUB_INVERTING     },
        { DE_OPAMP_SUB_NON_INVERTING, EN_OPAMP_SUB_NON_INVERTING },

        // SiPrefixes-Tool
        { DE_SI_PREFIXES_HDR_PREFIX,  EN_SI_PREFIXES_HDR_PREFIX  },
        { DE_SI_PREFIXES_HDR_SYMBOL,  EN_SI_PREFIXES_HDR_SYMBOL  },
        { DE_SI_PREFIXES_HDR_POWER,   EN_SI_PREFIXES_HDR_POWER   },
        { DE_SI_PREFIXES_HDR_EXAMPLE, EN_SI_PREFIXES_HDR_EXAMPLE },

        { DE_SI_PREFIXES_HINT_TAP, EN_SI_PREFIXES_HINT_TAP },

        { DE_SI_PREFIXES_NAME_TERA,  EN_SI_PREFIXES_NAME_TERA  },
        { DE_SI_PREFIXES_NAME_GIGA,  EN_SI_PREFIXES_NAME_GIGA  },
        { DE_SI_PREFIXES_NAME_MEGA,  EN_SI_PREFIXES_NAME_MEGA  },
        { DE_SI_PREFIXES_NAME_KILO,  EN_SI_PREFIXES_NAME_KILO  },
        { DE_SI_PREFIXES_NAME_BASE,  EN_SI_PREFIXES_NAME_BASE  },
        { DE_SI_PREFIXES_NAME_MILLI, EN_SI_PREFIXES_NAME_MILLI },
        { DE_SI_PREFIXES_NAME_MICRO, EN_SI_PREFIXES_NAME_MICRO },
        { DE_SI_PREFIXES_NAME_NANO,  EN_SI_PREFIXES_NAME_NANO  },
        { DE_SI_PREFIXES_NAME_PICO,  EN_SI_PREFIXES_NAME_PICO  },

        // ScrewRef-Tool
        { DE_SCREW_REF_BTN_COARSE, EN_SCREW_REF_BTN_COARSE },
        { DE_SCREW_REF_BTN_FINE,   EN_SCREW_REF_BTN_FINE   },

        { DE_SCREW_REF_SUB_COARSE, EN_SCREW_REF_SUB_COARSE },
        { DE_SCREW_REF_SUB_FINE,   EN_SCREW_REF_SUB_FINE   },

        { DE_SCREW_REF_FMT_TITLE_COARSE, EN_SCREW_REF_FMT_TITLE_COARSE },
        { DE_SCREW_REF_FMT_TITLE_FINE,   EN_SCREW_REF_FMT_TITLE_FINE   },

        { DE_SCREW_REF_NO_FINE_THREAD, EN_SCREW_REF_NO_FINE_THREAD },

        { DE_SCREW_REF_LABEL_PITCH,       EN_SCREW_REF_LABEL_PITCH       },
        { DE_SCREW_REF_LABEL_TAPPING,     EN_SCREW_REF_LABEL_TAPPING     },
        { DE_SCREW_REF_LABEL_CLEARANCE,   EN_SCREW_REF_LABEL_CLEARANCE   },
        { DE_SCREW_REF_LABEL_AF_SOCKET,   EN_SCREW_REF_LABEL_AF_SOCKET   },
        { DE_SCREW_REF_LABEL_AF_HEX,      EN_SCREW_REF_LABEL_AF_HEX      },
        { DE_SCREW_REF_LABEL_HEAD_HEIGHT, EN_SCREW_REF_LABEL_HEAD_HEIGHT },
        { DE_SCREW_REF_LABEL_HEAD_DIA,    EN_SCREW_REF_LABEL_HEAD_DIA    },
        { DE_SCREW_REF_LABEL_TORQUE,      EN_SCREW_REF_LABEL_TORQUE      },

        { DE_SCREW_REF_FMT_DIM_SW,    EN_SCREW_REF_FMT_DIM_SW    },
        { DE_SCREW_REF_FMT_DIM_DRILL, EN_SCREW_REF_FMT_DIM_DRILL },

        // ColorConverter-Tool
        { DE_COLOR_CONVERTER_RGB_HINT, EN_COLOR_CONVERTER_RGB_HINT },

        // AsciiTable-Tool
        { DE_ASCII_TABLE_SUB_EXTENDED, EN_ASCII_TABLE_SUB_EXTENDED },
        { DE_ASCII_TABLE_HINT_TAP,     EN_ASCII_TABLE_HINT_TAP     },
        { DE_ASCII_TABLE_NOT_DEFINED,  EN_ASCII_TABLE_NOT_DEFINED  },

        // BitEditor-Tool
        { DE_BIT_EDITOR_SUB_8,  EN_BIT_EDITOR_SUB_8  },
        { DE_BIT_EDITOR_SUB_16, EN_BIT_EDITOR_SUB_16 },
        { DE_BIT_EDITOR_SUB_32, EN_BIT_EDITOR_SUB_32 },

        // Calculator-Tool
        { DE_CALCULATOR_ERROR, EN_CALCULATOR_ERROR },

        // Gemeinsame Dialogschaltflächen
        { DE_DIALOG_CLOSE,  EN_DIALOG_CLOSE  },
        { DE_DIALOG_OK,     EN_DIALOG_OK     },
        { DE_DIALOG_CANCEL, EN_DIALOG_CANCEL },

        // Metronom-Tool
        { DE_METRONOME_HELP_TITLE,        EN_METRONOME_HELP_TITLE        },
        { DE_METRONOME_HELP_TEXT,         EN_METRONOME_HELP_TEXT         },
        { DE_METRONOME_HINT_TAP_TO_START, EN_METRONOME_HINT_TAP_TO_START },

        // Quintenzirkel-Tool
        { DE_CIRCLE_OF_FIFTHS_FUNC_TONIC,       EN_CIRCLE_OF_FIFTHS_FUNC_TONIC       },
        { DE_CIRCLE_OF_FIFTHS_FUNC_SUBDOMINANT, EN_CIRCLE_OF_FIFTHS_FUNC_SUBDOMINANT },
        { DE_CIRCLE_OF_FIFTHS_FUNC_DOMINANT,    EN_CIRCLE_OF_FIFTHS_FUNC_DOMINANT    },
        { DE_CIRCLE_OF_FIFTHS_FUNC_PARALLEL,    EN_CIRCLE_OF_FIFTHS_FUNC_PARALLEL    },

        { DE_CIRCLE_OF_FIFTHS_SUFFIX_MAJOR,   EN_CIRCLE_OF_FIFTHS_SUFFIX_MAJOR   },
        { DE_CIRCLE_OF_FIFTHS_HEADER_SCALE,   EN_CIRCLE_OF_FIFTHS_HEADER_SCALE   },
        { DE_CIRCLE_OF_FIFTHS_HEADER_DEGREES, EN_CIRCLE_OF_FIFTHS_HEADER_DEGREES },

        { DE_CIRCLE_OF_FIFTHS_FMT_HEADER_KEY, EN_CIRCLE_OF_FIFTHS_FMT_HEADER_KEY },

        { DE_CIRCLE_OF_FIFTHS_NO_ACCIDENTALS, EN_CIRCLE_OF_FIFTHS_NO_ACCIDENTALS },
        { DE_CIRCLE_OF_FIFTHS_UNIT_SHARP_SG,  EN_CIRCLE_OF_FIFTHS_UNIT_SHARP_SG  },
        { DE_CIRCLE_OF_FIFTHS_UNIT_SHARP_PL,  EN_CIRCLE_OF_FIFTHS_UNIT_SHARP_PL  },
        { DE_CIRCLE_OF_FIFTHS_UNIT_FLAT_SG,   EN_CIRCLE_OF_FIFTHS_UNIT_FLAT_SG   },
        { DE_CIRCLE_OF_FIFTHS_UNIT_FLAT_PL,   EN_CIRCLE_OF_FIFTHS_UNIT_FLAT_PL   },

        { DE_CIRCLE_OF_FIFTHS_FMT_SUB_NONE,      EN_CIRCLE_OF_FIFTHS_FMT_SUB_NONE      },
        { DE_CIRCLE_OF_FIFTHS_FMT_SUB_ONE_SHARP, EN_CIRCLE_OF_FIFTHS_FMT_SUB_ONE_SHARP },
        { DE_CIRCLE_OF_FIFTHS_FMT_SUB_ONE_FLAT,  EN_CIRCLE_OF_FIFTHS_FMT_SUB_ONE_FLAT  },
        { DE_CIRCLE_OF_FIFTHS_FMT_SUB_N_SHARPS,  EN_CIRCLE_OF_FIFTHS_FMT_SUB_N_SHARPS  },
        { DE_CIRCLE_OF_FIFTHS_FMT_SUB_N_FLATS,   EN_CIRCLE_OF_FIFTHS_FMT_SUB_N_FLATS   },

        // Würfelwerkzeug
        { DE_DICE_HELP_TITLE,     EN_DICE_HELP_TITLE     },
        { DE_DICE_HELP_TEXT,      EN_DICE_HELP_TEXT      },
        { DE_DICE_BTN_ONE,        EN_DICE_BTN_ONE        },
        { DE_DICE_BTN_TWO,        EN_DICE_BTN_TWO        },
        { DE_DICE_LABEL_RESULT,   EN_DICE_LABEL_RESULT   },
        { DE_DICE_LABEL_HISTORY,  EN_DICE_LABEL_HISTORY  },
        { DE_DICE_HISTORY_EMPTY,  EN_DICE_HISTORY_EMPTY  },
        { DE_DICE_SUB_ROLLING,    EN_DICE_SUB_ROLLING    },
        { DE_DICE_SUB_ONE_DIE,    EN_DICE_SUB_ONE_DIE    },
        { DE_DICE_SUB_TWO_DICE,   EN_DICE_SUB_TWO_DICE   },

        // 2048-Tool
        { DE_GAME_2048_HELP_TITLE,           EN_GAME_2048_HELP_TITLE           },
        { DE_GAME_2048_HELP_TEXT,            EN_GAME_2048_HELP_TEXT            },
        { DE_GAME_2048_RESET_TITLE,          EN_GAME_2048_RESET_TITLE          },
        { DE_GAME_2048_RESET_TEXT,           EN_GAME_2048_RESET_TEXT           },
        { DE_GAME_2048_SUB_PLAYING,          EN_GAME_2048_SUB_PLAYING          },
        { DE_GAME_2048_SUB_WON,              EN_GAME_2048_SUB_WON              },
        { DE_GAME_2048_SUB_GAMEOVER,         EN_GAME_2048_SUB_GAMEOVER         },
        { DE_GAME_2048_LABEL_SCORE,          EN_GAME_2048_LABEL_SCORE          },
        { DE_GAME_2048_LABEL_BEST,           EN_GAME_2048_LABEL_BEST           },
        { DE_GAME_2048_LABEL_HINT,           EN_GAME_2048_LABEL_HINT           },
        { DE_GAME_2048_OVERLAY_WON_TITLE,    EN_GAME_2048_OVERLAY_WON_TITLE    },
        { DE_GAME_2048_OVERLAY_WON_LINE2,    EN_GAME_2048_OVERLAY_WON_LINE2    },
        { DE_GAME_2048_OVERLAY_LOST_TITLE,   EN_GAME_2048_OVERLAY_LOST_TITLE   },
        { DE_GAME_2048_OVERLAY_LOST_LINE2,   EN_GAME_2048_OVERLAY_LOST_LINE2   },
        { DE_GAME_2048_BTN_NEW,              EN_GAME_2048_BTN_NEW              },
        { DE_GAME_2048_BTN_RESET,            EN_GAME_2048_BTN_RESET            },
        { DE_GAME_2048_BTN_CONTINUE,         EN_GAME_2048_BTN_CONTINUE         },

        // Lights-Out-Tool
        { DE_LIGHTS_OUT_HELP_TITLE,        EN_LIGHTS_OUT_HELP_TITLE        },
        { DE_LIGHTS_OUT_HELP_TEXT,         EN_LIGHTS_OUT_HELP_TEXT         },
        { DE_LIGHTS_OUT_RESET_TITLE,       EN_LIGHTS_OUT_RESET_TITLE       },
        { DE_LIGHTS_OUT_RESET_TEXT,        EN_LIGHTS_OUT_RESET_TEXT        },
        { DE_LIGHTS_OUT_DIFF_EASY,         EN_LIGHTS_OUT_DIFF_EASY         },
        { DE_LIGHTS_OUT_DIFF_MEDIUM,       EN_LIGHTS_OUT_DIFF_MEDIUM       },
        { DE_LIGHTS_OUT_DIFF_HARD,         EN_LIGHTS_OUT_DIFF_HARD         },
        { DE_LIGHTS_OUT_LABEL_MOVES,       EN_LIGHTS_OUT_LABEL_MOVES       },
        { DE_LIGHTS_OUT_LABEL_TIME,        EN_LIGHTS_OUT_LABEL_TIME        },
        { DE_LIGHTS_OUT_LABEL_STATS,       EN_LIGHTS_OUT_LABEL_STATS       },
        { DE_LIGHTS_OUT_STATUS_SOLVED,     EN_LIGHTS_OUT_STATUS_SOLVED     },
        { DE_LIGHTS_OUT_STATUS_READY,      EN_LIGHTS_OUT_STATUS_READY      },
        { DE_LIGHTS_OUT_STATUS_PLAYING,    EN_LIGHTS_OUT_STATUS_PLAYING    },
        { DE_LIGHTS_OUT_SUB_TITLE,         EN_LIGHTS_OUT_SUB_TITLE         },
        { DE_LIGHTS_OUT_FMT_SUB_DIFFICULTY, EN_LIGHTS_OUT_FMT_SUB_DIFFICULTY },
        { DE_LIGHTS_OUT_BTN_NEW,           EN_LIGHTS_OUT_BTN_NEW           },
        { DE_LIGHTS_OUT_BTN_RESET,         EN_LIGHTS_OUT_BTN_RESET         },

        // Mühlewerkzeug
        { DE_MILL_SUB_PLACING, EN_MILL_SUB_PLACING },
        { DE_MILL_SUB_MOVING,  EN_MILL_SUB_MOVING  },
        { DE_MILL_SUB_FLYING,  EN_MILL_SUB_FLYING  },
        { DE_MILL_SUB_CAPTURE, EN_MILL_SUB_CAPTURE },
        { DE_MILL_SUB_BOT,     EN_MILL_SUB_BOT     },
        { DE_MILL_SUB_WON,     EN_MILL_SUB_WON     },
        { DE_MILL_SUB_LOST,    EN_MILL_SUB_LOST    },
        { DE_MILL_SUB_DRAW,    EN_MILL_SUB_DRAW    },

        { DE_MILL_HELP_TITLE, EN_MILL_HELP_TITLE },
        { DE_MILL_HELP_TEXT,  EN_MILL_HELP_TEXT  },

        { DE_MILL_RESET_TITLE, EN_MILL_RESET_TITLE },
        { DE_MILL_RESET_TEXT,  EN_MILL_RESET_TEXT  },

        { DE_MILL_LABEL_PHASE,  EN_MILL_LABEL_PHASE  },
        { DE_MILL_LABEL_TURN,   EN_MILL_LABEL_TURN   },
        { DE_MILL_LABEL_STONES, EN_MILL_LABEL_STONES },
        { DE_MILL_LABEL_WINS,   EN_MILL_LABEL_WINS   },
        { DE_MILL_LABEL_LOSSES, EN_MILL_LABEL_LOSSES },
        { DE_MILL_LABEL_DRAWS,  EN_MILL_LABEL_DRAWS  },

        { DE_MILL_HINT_DEFAULT,      EN_MILL_HINT_DEFAULT      },
        { DE_MILL_HINT_WON,          EN_MILL_HINT_WON          },
        { DE_MILL_HINT_LOST,         EN_MILL_HINT_LOST         },
        { DE_MILL_HINT_DRAW,         EN_MILL_HINT_DRAW         },
        { DE_MILL_HINT_CAPTURE,      EN_MILL_HINT_CAPTURE      },
        { DE_MILL_HINT_BOT_THINKING, EN_MILL_HINT_BOT_THINKING },
        { DE_MILL_HINT_TAP_FREE,     EN_MILL_HINT_TAP_FREE     },
        { DE_MILL_HINT_JUMP_FREE,    EN_MILL_HINT_JUMP_FREE    },
        { DE_MILL_HINT_PICK_TARGET,  EN_MILL_HINT_PICK_TARGET  },
        { DE_MILL_HINT_PICK_OWN,     EN_MILL_HINT_PICK_OWN     },

        { DE_MILL_PHASE_PLACING, EN_MILL_PHASE_PLACING },
        { DE_MILL_PHASE_MOVING,  EN_MILL_PHASE_MOVING  },
        { DE_MILL_PHASE_FLYING,  EN_MILL_PHASE_FLYING  },

        { DE_MILL_TURN_YOU, EN_MILL_TURN_YOU },
        { DE_MILL_TURN_BOT, EN_MILL_TURN_BOT },

        { DE_MILL_FMT_STONES_YOU, EN_MILL_FMT_STONES_YOU },
        { DE_MILL_FMT_STONES_BOT, EN_MILL_FMT_STONES_BOT },

        { DE_MILL_OVERLAY_WON_TITLE,  EN_MILL_OVERLAY_WON_TITLE  },
        { DE_MILL_OVERLAY_WON_LINE2,  EN_MILL_OVERLAY_WON_LINE2  },
        { DE_MILL_OVERLAY_LOST_TITLE, EN_MILL_OVERLAY_LOST_TITLE },
        { DE_MILL_OVERLAY_LOST_LINE2, EN_MILL_OVERLAY_LOST_LINE2 },
        { DE_MILL_OVERLAY_DRAW_TITLE, EN_MILL_OVERLAY_DRAW_TITLE },
        { DE_MILL_OVERLAY_DRAW_LINE2, EN_MILL_OVERLAY_DRAW_LINE2 },

        { DE_MILL_BTN_EASY,   EN_MILL_BTN_EASY   },
        { DE_MILL_BTN_MEDIUM, EN_MILL_BTN_MEDIUM },
        { DE_MILL_BTN_NEW,    EN_MILL_BTN_NEW    },
        { DE_MILL_BTN_RESET,  EN_MILL_BTN_RESET  },

        // PeriodicTable: Kategorie-Namen (lang, Popup)
        { DE_PERIODIC_TABLE_CAT_ALKALI_LONG,          EN_PERIODIC_TABLE_CAT_ALKALI_LONG          },
        { DE_PERIODIC_TABLE_CAT_ALKALINE_EARTH_LONG,  EN_PERIODIC_TABLE_CAT_ALKALINE_EARTH_LONG  },
        { DE_PERIODIC_TABLE_CAT_TRANSITION_LONG,      EN_PERIODIC_TABLE_CAT_TRANSITION_LONG      },
        { DE_PERIODIC_TABLE_CAT_POST_TRANSITION_LONG, EN_PERIODIC_TABLE_CAT_POST_TRANSITION_LONG },
        { DE_PERIODIC_TABLE_CAT_METALLOID_LONG,       EN_PERIODIC_TABLE_CAT_METALLOID_LONG       },
        { DE_PERIODIC_TABLE_CAT_NONMETAL_LONG,        EN_PERIODIC_TABLE_CAT_NONMETAL_LONG        },
        { DE_PERIODIC_TABLE_CAT_NOBLE_GAS_LONG,       EN_PERIODIC_TABLE_CAT_NOBLE_GAS_LONG       },
        { DE_PERIODIC_TABLE_CAT_LANTHANIDE_LONG,      EN_PERIODIC_TABLE_CAT_LANTHANIDE_LONG      },
        { DE_PERIODIC_TABLE_CAT_ACTINIDE_LONG,        EN_PERIODIC_TABLE_CAT_ACTINIDE_LONG        },
        { DE_PERIODIC_TABLE_CAT_UNKNOWN_LONG,         EN_PERIODIC_TABLE_CAT_UNKNOWN_LONG         },

        // PeriodicTable: Kategorie-Namen (kurz, Legende)
        { DE_PERIODIC_TABLE_CAT_ALKALI_SHORT,          EN_PERIODIC_TABLE_CAT_ALKALI_SHORT          },
        { DE_PERIODIC_TABLE_CAT_ALKALINE_EARTH_SHORT,  EN_PERIODIC_TABLE_CAT_ALKALINE_EARTH_SHORT  },
        { DE_PERIODIC_TABLE_CAT_TRANSITION_SHORT,      EN_PERIODIC_TABLE_CAT_TRANSITION_SHORT      },
        { DE_PERIODIC_TABLE_CAT_POST_TRANSITION_SHORT, EN_PERIODIC_TABLE_CAT_POST_TRANSITION_SHORT },
        { DE_PERIODIC_TABLE_CAT_METALLOID_SHORT,       EN_PERIODIC_TABLE_CAT_METALLOID_SHORT       },
        { DE_PERIODIC_TABLE_CAT_NONMETAL_SHORT,        EN_PERIODIC_TABLE_CAT_NONMETAL_SHORT        },
        { DE_PERIODIC_TABLE_CAT_NOBLE_GAS_SHORT,       EN_PERIODIC_TABLE_CAT_NOBLE_GAS_SHORT       },
        { DE_PERIODIC_TABLE_CAT_LANTHANIDE_SHORT,      EN_PERIODIC_TABLE_CAT_LANTHANIDE_SHORT      },
        { DE_PERIODIC_TABLE_CAT_ACTINIDE_SHORT,        EN_PERIODIC_TABLE_CAT_ACTINIDE_SHORT        },
        { DE_PERIODIC_TABLE_CAT_UNKNOWN_SHORT,         EN_PERIODIC_TABLE_CAT_UNKNOWN_SHORT         },

        // PeriodicTable: Aggregatzustände
        { DE_PERIODIC_TABLE_STATE_SOLID,   EN_PERIODIC_TABLE_STATE_SOLID   },
        { DE_PERIODIC_TABLE_STATE_LIQUID,  EN_PERIODIC_TABLE_STATE_LIQUID  },
        { DE_PERIODIC_TABLE_STATE_GAS,     EN_PERIODIC_TABLE_STATE_GAS     },
        { DE_PERIODIC_TABLE_STATE_UNKNOWN, EN_PERIODIC_TABLE_STATE_UNKNOWN },

        // PeriodicTable: Popup-Labels
        { DE_PERIODIC_TABLE_LABEL_CATEGORY,     EN_PERIODIC_TABLE_LABEL_CATEGORY     },
        { DE_PERIODIC_TABLE_LABEL_MASS,         EN_PERIODIC_TABLE_LABEL_MASS         },
        { DE_PERIODIC_TABLE_LABEL_GROUP_PERIOD, EN_PERIODIC_TABLE_LABEL_GROUP_PERIOD },
        { DE_PERIODIC_TABLE_LABEL_STATE,        EN_PERIODIC_TABLE_LABEL_STATE        },
        { DE_PERIODIC_TABLE_LABEL_DISCOVERED,   EN_PERIODIC_TABLE_LABEL_DISCOVERED   },
        { DE_PERIODIC_TABLE_LABEL_DISCOVERER,   EN_PERIODIC_TABLE_LABEL_DISCOVERER   },

        // PeriodicTable: Sonderwerte für Entdeckungsjahr
        { DE_PERIODIC_TABLE_YEAR_ANCIENT, EN_PERIODIC_TABLE_YEAR_ANCIENT },
        { DE_PERIODIC_TABLE_YEAR_UNKNOWN, EN_PERIODIC_TABLE_YEAR_UNKNOWN },

        // Shut-the-Box-Tool
        { DE_SHUT_THE_BOX_TITLE,              EN_SHUT_THE_BOX_TITLE              },
        { DE_SHUT_THE_BOX_SUBTITLE,           EN_SHUT_THE_BOX_SUBTITLE           },
        { DE_SHUT_THE_BOX_HELP_TITLE,         EN_SHUT_THE_BOX_HELP_TITLE         },
        { DE_SHUT_THE_BOX_HELP_TEXT,          EN_SHUT_THE_BOX_HELP_TEXT          },
        { DE_SHUT_THE_BOX_RESET_TITLE,        EN_SHUT_THE_BOX_RESET_TITLE        },
        { DE_SHUT_THE_BOX_RESET_TEXT,         EN_SHUT_THE_BOX_RESET_TEXT         },
        { DE_SHUT_THE_BOX_SUB_READY,          EN_SHUT_THE_BOX_SUB_READY          },
        { DE_SHUT_THE_BOX_FMT_SUB_PICK,       EN_SHUT_THE_BOX_FMT_SUB_PICK       },
        { DE_SHUT_THE_BOX_SUB_WON,            EN_SHUT_THE_BOX_SUB_WON            },
        { DE_SHUT_THE_BOX_SUB_GAMEOVER,       EN_SHUT_THE_BOX_SUB_GAMEOVER       },
        { DE_SHUT_THE_BOX_LABEL_ROLL,         EN_SHUT_THE_BOX_LABEL_ROLL         },
        { DE_SHUT_THE_BOX_LABEL_PICK,         EN_SHUT_THE_BOX_LABEL_PICK         },
        { DE_SHUT_THE_BOX_LABEL_OPEN,         EN_SHUT_THE_BOX_LABEL_OPEN         },
        { DE_SHUT_THE_BOX_LABEL_BEST,         EN_SHUT_THE_BOX_LABEL_BEST         },
        { DE_SHUT_THE_BOX_LABEL_WINS,         EN_SHUT_THE_BOX_LABEL_WINS         },
        { DE_SHUT_THE_BOX_HINT_ROLL,          EN_SHUT_THE_BOX_HINT_ROLL          },
        { DE_SHUT_THE_BOX_OVERLAY_WON_TITLE,  EN_SHUT_THE_BOX_OVERLAY_WON_TITLE  },
        { DE_SHUT_THE_BOX_OVERLAY_WON_LINE2,  EN_SHUT_THE_BOX_OVERLAY_WON_LINE2  },
        { DE_SHUT_THE_BOX_OVERLAY_LOST_TITLE, EN_SHUT_THE_BOX_OVERLAY_LOST_TITLE },
        { DE_SHUT_THE_BOX_OVERLAY_LOST_LINE2, EN_SHUT_THE_BOX_OVERLAY_LOST_LINE2 },
        { DE_SHUT_THE_BOX_BTN_ONE,            EN_SHUT_THE_BOX_BTN_ONE            },
        { DE_SHUT_THE_BOX_BTN_TWO,            EN_SHUT_THE_BOX_BTN_TWO            },
        { DE_SHUT_THE_BOX_BTN_ROLL,           EN_SHUT_THE_BOX_BTN_ROLL           },
        { DE_SHUT_THE_BOX_BTN_LAY,            EN_SHUT_THE_BOX_BTN_LAY            },
        { DE_SHUT_THE_BOX_BTN_NEW,            EN_SHUT_THE_BOX_BTN_NEW            },
        { DE_SHUT_THE_BOX_BTN_RESET,          EN_SHUT_THE_BOX_BTN_RESET          },

        // Lotto-System-Tool
        { DE_LOTTO_TITLE,           EN_LOTTO_TITLE           },
        { DE_LOTTO_PAGE_TITLE,      EN_LOTTO_PAGE_TITLE      },
        { DE_LOTTO_SUBTITLE,        EN_LOTTO_SUBTITLE        },
        { DE_LOTTO_HELP_TITLE,      EN_LOTTO_HELP_TITLE      },
        { DE_LOTTO_HELP_TEXT,       EN_LOTTO_HELP_TEXT       },
        { DE_LOTTO_LABEL_TICKET,    EN_LOTTO_LABEL_TICKET    },
        { DE_LOTTO_LABEL_DRAW,      EN_LOTTO_LABEL_DRAW      },
        { DE_LOTTO_LABEL_FIELDS,    EN_LOTTO_LABEL_FIELDS    },
        { DE_LOTTO_LABEL_PRICE,     EN_LOTTO_LABEL_PRICE     },
        { DE_LOTTO_LABEL_SUPER,     EN_LOTTO_LABEL_SUPER     },
        { DE_LOTTO_LABEL_TICKET_SUPER, EN_LOTTO_LABEL_TICKET_SUPER },
        { DE_LOTTO_LABEL_DRAW_SUPER,   EN_LOTTO_LABEL_DRAW_SUPER   },
        { DE_LOTTO_DIALOG_SUPER,    EN_LOTTO_DIALOG_SUPER    },
        { DE_LOTTO_DIALOG_TICKET_SUPER, EN_LOTTO_DIALOG_TICKET_SUPER },
        { DE_LOTTO_DIALOG_DRAW_SUPER,   EN_LOTTO_DIALOG_DRAW_SUPER   },
        { DE_LOTTO_LABEL_WINS,      EN_LOTTO_LABEL_WINS      },
        { DE_LOTTO_PRIZE_6_SUPER,   EN_LOTTO_PRIZE_6_SUPER   },
        { DE_LOTTO_PRIZE_6,         EN_LOTTO_PRIZE_6         },
        { DE_LOTTO_PRIZE_5_SUPER,   EN_LOTTO_PRIZE_5_SUPER   },
        { DE_LOTTO_PRIZE_5,         EN_LOTTO_PRIZE_5         },
        { DE_LOTTO_PRIZE_4_SUPER,   EN_LOTTO_PRIZE_4_SUPER   },
        { DE_LOTTO_PRIZE_4,         EN_LOTTO_PRIZE_4         },
        { DE_LOTTO_PRIZE_3_SUPER,   EN_LOTTO_PRIZE_3_SUPER   },
        { DE_LOTTO_PRIZE_3,         EN_LOTTO_PRIZE_3         },
        { DE_LOTTO_PRIZE_2_SUPER,   EN_LOTTO_PRIZE_2_SUPER   },
        { DE_LOTTO_EMPTY_TICKET,    EN_LOTTO_EMPTY_TICKET    },
        { DE_LOTTO_EMPTY_DRAW,      EN_LOTTO_EMPTY_DRAW      },
        { DE_LOTTO_HINT_INCOMPLETE, EN_LOTTO_HINT_INCOMPLETE },
        { DE_LOTTO_YES,             EN_LOTTO_YES             },
        { DE_LOTTO_NO,              EN_LOTTO_NO              },
        { DE_LOTTO_FMT_SUB_TICKET,  EN_LOTTO_FMT_SUB_TICKET  },
        { DE_LOTTO_FMT_SUB_DRAW,    EN_LOTTO_FMT_SUB_DRAW    },
        { DE_LOTTO_SUB_READY,       EN_LOTTO_SUB_READY       },
        { DE_LOTTO_FMT_SUMMARY_TICKET_SUPER, EN_LOTTO_FMT_SUMMARY_TICKET_SUPER },
        { DE_LOTTO_FMT_SUMMARY_DRAW_SUPER,   EN_LOTTO_FMT_SUMMARY_DRAW_SUPER   },
        { DE_LOTTO_BTN_RANDOM,      EN_LOTTO_BTN_RANDOM      },
        { DE_LOTTO_BTN_CLEAR,       EN_LOTTO_BTN_CLEAR       },
        { DE_LOTTO_BTN_TICKET,      EN_LOTTO_BTN_TICKET      },
        { DE_LOTTO_BTN_DRAW,        EN_LOTTO_BTN_DRAW        },

        // SMD-Widerstand
        { DE_TOOL_SMD_RESISTOR_TITLE,    EN_TOOL_SMD_RESISTOR_TITLE    },
        { DE_TOOL_SMD_RESISTOR_SUBTITLE, EN_TOOL_SMD_RESISTOR_SUBTITLE },
        { DE_SMD_RESISTOR_LABEL_CODE,    EN_SMD_RESISTOR_LABEL_CODE    },
        { DE_SMD_RESISTOR_LABEL_VALUE,   EN_SMD_RESISTOR_LABEL_VALUE   },
        { DE_SMD_RESISTOR_ERR_EMPTY,     EN_SMD_RESISTOR_ERR_EMPTY     },
        { DE_SMD_RESISTOR_ERR_INVALID,   EN_SMD_RESISTOR_ERR_INVALID   },
        { DE_SMD_RESISTOR_ERR_R_CODE,    EN_SMD_RESISTOR_ERR_R_CODE    },
        { DE_SMD_RESISTOR_NOTE_3_DIGIT,  EN_SMD_RESISTOR_NOTE_3_DIGIT  },
        { DE_SMD_RESISTOR_NOTE_4_DIGIT,  EN_SMD_RESISTOR_NOTE_4_DIGIT  },
        { DE_SMD_RESISTOR_NOTE_R_CODE,   EN_SMD_RESISTOR_NOTE_R_CODE   },
        { DE_SMD_RESISTOR_NOTE_ZERO,     EN_SMD_RESISTOR_NOTE_ZERO     },
        { DE_SMD_RESISTOR_MODE_3,        EN_SMD_RESISTOR_MODE_3        },
        { DE_SMD_RESISTOR_MODE_4,        EN_SMD_RESISTOR_MODE_4        },
        { DE_SMD_RESISTOR_MODE_R,        EN_SMD_RESISTOR_MODE_R        },

        // Website-QR-Dialog
        { DE_QR_MODAL_TITLE, EN_QR_MODAL_TITLE },
        { DE_QR_MODAL_HINT,  EN_QR_MODAL_HINT  },
        { DE_QR_MODAL_ERROR, EN_QR_MODAL_ERROR },

        // Lotto-Normal-Tool
        { DE_LOTTO_NORMAL_TITLE,      EN_LOTTO_NORMAL_TITLE      },
        { DE_LOTTO_NORMAL_PAGE_TITLE, EN_LOTTO_NORMAL_PAGE_TITLE },
        { DE_LOTTO_NORMAL_SUBTITLE,   EN_LOTTO_NORMAL_SUBTITLE   },
        { DE_LOTTO_NORMAL_HELP_TITLE, EN_LOTTO_NORMAL_HELP_TITLE },
        { DE_LOTTO_NORMAL_HELP_TEXT,  EN_LOTTO_NORMAL_HELP_TEXT  },
        { DE_LOTTO_NORMAL_HINT_DRAW_SUPER_MISSING, EN_LOTTO_NORMAL_HINT_DRAW_SUPER_MISSING },

        // Flugradar-Tool
        { DE_FLIGHT_RADAR_TITLE,          EN_FLIGHT_RADAR_TITLE          },
        { DE_FLIGHT_RADAR_SUBTITLE,       EN_FLIGHT_RADAR_SUBTITLE       },
        { DE_FLIGHT_RADAR_FMT_SUBTITLE,   EN_FLIGHT_RADAR_FMT_SUBTITLE   },
        { DE_FLIGHT_RADAR_HELP_TITLE,      EN_FLIGHT_RADAR_HELP_TITLE     },
        { DE_FLIGHT_RADAR_HELP_TEXT,       EN_FLIGHT_RADAR_HELP_TEXT      },
        { DE_FLIGHT_RADAR_LABEL_COUNT,    EN_FLIGHT_RADAR_LABEL_COUNT    },
        { DE_FLIGHT_RADAR_OFFLINE_TITLE,  EN_FLIGHT_RADAR_OFFLINE_TITLE  },
        { DE_FLIGHT_RADAR_LABEL_GROUND,   EN_FLIGHT_RADAR_LABEL_GROUND   },
        { DE_FLIGHT_RADAR_FMT_MORE,       EN_FLIGHT_RADAR_FMT_MORE       },
        { DE_FLIGHT_RADAR_DETAIL_TITLE_FMT, EN_FLIGHT_RADAR_DETAIL_TITLE_FMT },
        { DE_FLIGHT_RADAR_DETAIL_CALLSIGN, EN_FLIGHT_RADAR_DETAIL_CALLSIGN },
        { DE_FLIGHT_RADAR_DETAIL_ICAO24,  EN_FLIGHT_RADAR_DETAIL_ICAO24  },
        { DE_FLIGHT_RADAR_DETAIL_COUNTRY, EN_FLIGHT_RADAR_DETAIL_COUNTRY },
        { DE_FLIGHT_RADAR_DETAIL_DISTANCE, EN_FLIGHT_RADAR_DETAIL_DISTANCE },
        { DE_FLIGHT_RADAR_DETAIL_ALTITUDE, EN_FLIGHT_RADAR_DETAIL_ALTITUDE },
        { DE_FLIGHT_RADAR_DETAIL_SPEED,   EN_FLIGHT_RADAR_DETAIL_SPEED   },
        { DE_FLIGHT_RADAR_DETAIL_TRACK,   EN_FLIGHT_RADAR_DETAIL_TRACK   },
        { DE_FLIGHT_RADAR_DETAIL_LAST_SEEN, EN_FLIGHT_RADAR_DETAIL_LAST_SEEN },
        { DE_FLIGHT_RADAR_VALUE_UNKNOWN,  EN_FLIGHT_RADAR_VALUE_UNKNOWN  },
        { DE_FLIGHT_RADAR_VALUE_JUST_NOW, EN_FLIGHT_RADAR_VALUE_JUST_NOW },
        { DE_FLIGHT_RADAR_FMT_SECONDS_AGO, EN_FLIGHT_RADAR_FMT_SECONDS_AGO },
        { DE_FLIGHT_RADAR_FMT_MINUTES_AGO, EN_FLIGHT_RADAR_FMT_MINUTES_AGO },
        { DE_FLIGHT_RADAR_FMT_TRACK,      EN_FLIGHT_RADAR_FMT_TRACK      },
        { DE_FLIGHT_RADAR_LOCATION_UNKNOWN, EN_FLIGHT_RADAR_LOCATION_UNKNOWN },
        { DE_FLIGHT_RADAR_AUTH_ACCOUNT,   EN_FLIGHT_RADAR_AUTH_ACCOUNT   },
        { DE_FLIGHT_RADAR_AUTH_GUEST,     EN_FLIGHT_RADAR_AUTH_GUEST     },
        { DE_FLIGHT_RADAR_COMPASS_NORTH,  EN_FLIGHT_RADAR_COMPASS_NORTH  },
        { DE_FLIGHT_RADAR_COMPASS_EAST,   EN_FLIGHT_RADAR_COMPASS_EAST   },
        { DE_FLIGHT_RADAR_COMPASS_SOUTH,  EN_FLIGHT_RADAR_COMPASS_SOUTH  },
        { DE_FLIGHT_RADAR_COMPASS_WEST,   EN_FLIGHT_RADAR_COMPASS_WEST   }
    };

    static_assert(sizeof(WORKSHOP_TABLE) / sizeof(WORKSHOP_TABLE[0])
                  == static_cast<size_t>(WorkshopKey::WORKSHOP_KEY_COUNT),
                  "WORKSHOP_TABLE / WorkshopKey out of sync");

    // Rückfalltext für einen fehlenden Eintrag
    const char EMPTY_STR[] PROGMEM = "";
}

// ===================================================================
// Lookup-Funktion (wird von I18n.cpp aufgerufen)
// ===================================================================

/**
 * @brief Liefert den lokalisierten Werkstatttext für Schlüssel und Sprache.
 */
const __FlashStringHelper *i18n_getWorkshop(WorkshopKey key, LangIndex lang)
{
    const auto idx = static_cast<size_t>(key);
    if (idx >= static_cast<size_t>(WorkshopKey::WORKSHOP_KEY_COUNT))
        return reinterpret_cast<const __FlashStringHelper *>(EMPTY_STR);

    auto li = static_cast<size_t>(lang);
    if (li >= static_cast<size_t>(LangIndex::COUNT))
        li = 0;

    PGM_P p = reinterpret_cast<PGM_P>(pgm_read_ptr(&WORKSHOP_TABLE[idx][li]));
    if (!p && li != 0)
        p = reinterpret_cast<PGM_P>(pgm_read_ptr(&WORKSHOP_TABLE[idx][0]));
    if (!p)
        return reinterpret_cast<const __FlashStringHelper *>(EMPTY_STR);
    return reinterpret_cast<const __FlashStringHelper *>(p);
}

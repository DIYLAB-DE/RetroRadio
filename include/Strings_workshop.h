#ifndef STRINGS_WORKSHOP_H
#define STRINGS_WORKSHOP_H

#include <stdint.h>

/**
 * @brief Schlüssel für alle anwender-sichtbaren Texte des Workshop-Moduls
 *
 * Diese Datei enthält die Keys für den Workshop-Rahmen und für alle Werkzeuge. Sub-Komponenten (Werkzeuge) trennen sich über Key-Präfixe `WORKSHOP_<TOOL>_*`, der Rahmen nutzt `WORKSHOP_CAT_*`, `WORKSHOP_TOOL_*`, `WORKSHOP_ERR_*` und `WORKSHOP_EMPTY`.
 *
 * Die numerischen Werte bestehender Schlüssel bleiben stabil. Neue Schlüssel werden unmittelbar vor `WORKSHOP_KEY_COUNT` eingefügt.
 *
 * Beim Hinzufügen eines Keys muss auch die Pointer-Tabelle in `Strings_workshop.cpp` erweitert werden. Der `static_assert` dort verhindert eine Drift.
 */
enum class WorkshopKey : uint16_t
{
    // ========================================================================
    // RAHMEN: Sonder-Key für leeren Text (z. B. NONE-Page-Eintrag)
    // ========================================================================
    WORKSHOP_EMPTY = 0, ///< Leerer String, beide Sprachen

    // ========================================================================
    // RAHMEN: Kategorie-Titel
    //
    // Diese Schlüssel dienen als WorkshopCategoryDef::titleKey und als WorkshopListEntry::labelKey für SECTION_HEADER-Einträge.
    // ========================================================================
    WORKSHOP_CAT_MISC_TITLE,        ///< "Werkzeuge" / "Tools"
    WORKSHOP_CAT_GAMES_TITLE,       ///< "Spiele" / "Games"
    WORKSHOP_CAT_ELECTRONICS_TITLE, ///< "Elektronik" / "Electronics"
    WORKSHOP_CAT_UNITS_TITLE,       ///< "Einheiten" / "Units"
    WORKSHOP_CAT_DIGITAL_TITLE,     ///< "Digital" / "Digital"
    WORKSHOP_CAT_KNITTING_TITLE,    ///< "Stricken" / "Knitting"
    WORKSHOP_CAT_MUSIC_TITLE,       ///< "Musik" / "Music"

    // ========================================================================
    // RAHMEN: Kategorie-Kurzbeschreibungen
    // ========================================================================
    WORKSHOP_CAT_MISC_SUMMARY,        ///< "Referenzen und Sonstiges" / "References and miscellaneous"
    WORKSHOP_CAT_GAMES_SUMMARY,       ///< "Knobel- und Brettspiele" / "Puzzles and board games"
    WORKSHOP_CAT_ELECTRONICS_SUMMARY, ///< "Rechner und Referenzen" / "Calculators and references"
    WORKSHOP_CAT_UNITS_SUMMARY,       ///< "Länge, Temp., Druck, SI" / "Length, temp., pressure, SI"
    WORKSHOP_CAT_DIGITAL_SUMMARY,     ///< "Bit-Editor, ASCII, Binärwerkzeuge" / "Bit editor, ASCII, binary tools"
    WORKSHOP_CAT_KNITTING_SUMMARY,    ///< "Maschen, Garn, Strickfestigkeit" / "Stitches, yarn, gauge"
    WORKSHOP_CAT_MUSIC_SUMMARY,       ///< "Tempo, Takt, Stimmung" / "Tempo, time signature, tuning"

    // ========================================================================
    // RAHMEN: Werkzeug-Titel
    //
    // Diese Schlüssel liegen auf WorkshopPageDef::titleKey. Die meisten Werkzeuge verwenden sie zusätzlich als WorkshopListEntry::labelKey in der Menüliste. SCREW_REFERENCE besitzt dort einen abweichenden Label-Key.
    // ========================================================================
    WORKSHOP_TOOL_CALCULATOR_TITLE,         ///< "Rechner" / "Calculator"
    WORKSHOP_TOOL_WORLD_CLOCK_TITLE,        ///< "Weltzeit-Uhr" / "World clock"
    WORKSHOP_TOOL_COLOR_CONVERTER_TITLE,    ///< "Farb-Konverter" / "Colour converter"
    WORKSHOP_TOOL_PERIODIC_TABLE_TITLE,     ///< "Periodensystem" / "Periodic table"
    WORKSHOP_TOOL_SCREW_REFERENCE_TITLE,    ///< "Schrauben" / "Screws"
    WORKSHOP_TOOL_BMI_CALCULATOR_TITLE,     ///< "BMI-Rechner" / "BMI calculator"
    WORKSHOP_TOOL_GAME_2048_TITLE,          ///< "2048" / "2048"
    WORKSHOP_TOOL_MILL_GAME_TITLE,          ///< "Mühle" / "Nine men's morris"
    WORKSHOP_TOOL_LIGHTS_OUT_TITLE,         ///< "Lights Out" / "Lights Out"
    WORKSHOP_TOOL_DICE_TITLE,               ///< "Würfel" / "Dice"
    WORKSHOP_TOOL_RESISTOR_TITLE,           ///< "Widerstandsfarbcode" / "Resistor colour code"
    WORKSHOP_TOOL_OHM_CALCULATOR_TITLE,     ///< "Ohmsches Gesetz" / "Ohm's law"
    WORKSHOP_TOOL_VOLTAGE_DIVIDER_TITLE,    ///< "Spannungsteiler" / "Voltage divider"
    WORKSHOP_TOOL_LED_CALCULATOR_TITLE,     ///< "LED-Vorwiderstand" / "LED series resistor"
    WORKSHOP_TOOL_RC_FILTER_TITLE,          ///< "RC/RL-Filter" / "RC/RL filter"
    WORKSHOP_TOOL_LC_FILTER_TITLE,          ///< "LC-Filter" / "LC filter"
    WORKSHOP_TOOL_UKW_RESONATOR_TITLE,      ///< "UKW-Schwingkreis" / "VHF resonator"
    WORKSHOP_TOOL_LM317_TITLE,              ///< "LM317-Regler" / "LM317 regulator"
    WORKSHOP_TOOL_DECIBEL_TITLE,            ///< "Dezibel" / "Decibel"
    WORKSHOP_TOOL_NE555_TITLE,              ///< "NE555-Timer" / "NE555 timer"
    WORKSHOP_TOOL_OPAMP_TITLE,              ///< "Op-Amp" / "Op-Amp"
    WORKSHOP_TOOL_INCH_MM_TITLE,            ///< "Zoll <-> mm" / "Inch <-> mm"
    WORKSHOP_TOOL_TEMP_TITLE,               ///< "Temperatur" / "Temperature"
    WORKSHOP_TOOL_PRESSURE_TITLE,           ///< "Druck" / "Pressure"
    WORKSHOP_TOOL_SI_PREFIXES_TITLE,        ///< "SI-Präfixe" / "SI prefixes"
    WORKSHOP_TOOL_ASCII_TABLE_TITLE,        ///< "ASCII-Tabelle" / "ASCII table"
    WORKSHOP_TOOL_BIT_EDITOR_TITLE,         ///< "Bit-Editor" / "Bit editor"
    WORKSHOP_TOOL_LOGIC_GATE_TITLE,         ///< "Logikgatter" / "Logic gates"
    WORKSHOP_TOOL_STITCH_GAUGE_TITLE,       ///< "Maschenprobe" / "Stitch gauge"
    WORKSHOP_TOOL_STITCH_DISTRIBUTOR_TITLE, ///< "Maschenverteilung" / "Stitch distribution"
    WORKSHOP_TOOL_SLEEVE_TAPER_TITLE,       ///< "Ärmelschräge" / "Sleeve taper"
    WORKSHOP_TOOL_CIRCLE_OF_FIFTHS_TITLE,   ///< "Quintenzirkel" / "Circle of fifths"
    WORKSHOP_TOOL_METRONOME_TITLE,          ///< "Metronom" / "Metronome"

    // ========================================================================
    // RAHMEN: Werkzeug-Untertitel
    // ========================================================================
    WORKSHOP_TOOL_CALCULATOR_SUBTITLE,         ///< "Dez / Hex / Bin (16 Bit)" / "Dec / hex / bin (16-bit)"
    WORKSHOP_TOOL_WORLD_CLOCK_SUBTITLE,        ///< "Acht Metropolen, Standardzeit" / "Eight metropolises, standard time"
    WORKSHOP_TOOL_COLOR_CONVERTER_SUBTITLE,    ///< "RGB / HEX / RGB565" / "RGB / hex / RGB565"
    WORKSHOP_TOOL_PERIODIC_TABLE_SUBTITLE,     ///< "Periodensystem der Elemente" / "Periodic table of the elements"
    WORKSHOP_TOOL_SCREW_REFERENCE_SUBTITLE,    ///< "Metrisch (DIN 13)" / "Metric (DIN 13)"
    WORKSHOP_TOOL_BMI_CALCULATOR_SUBTITLE,     ///< "Body Mass Index" / "Body Mass Index"
    WORKSHOP_TOOL_GAME_2048_SUBTITLE,          ///< "Schiebepuzzle für die Pause" / "Sliding puzzle for the break"
    WORKSHOP_TOOL_MILL_GAME_SUBTITLE,          ///< "Brettspiel für die Pause" / "Board game for the break"
    WORKSHOP_TOOL_LIGHTS_OUT_SUBTITLE,         ///< "Knobelspiel für die Pause" / "Puzzle game for the break"
    WORKSHOP_TOOL_DICE_SUBTITLE,               ///< "Zufallszahlen aus Hardware-RNG" / "Random numbers from hardware RNG"
    WORKSHOP_TOOL_RESISTOR_SUBTITLE,           ///< "4- und 5-Band" / "4-band and 5-band"
    WORKSHOP_TOOL_OHM_CALCULATOR_SUBTITLE,     ///< "U / I / R / P" / "U / I / R / P"
    WORKSHOP_TOOL_VOLTAGE_DIVIDER_SUBTITLE,    ///< "R1 / R2 / Uout" / "R1 / R2 / Uout"
    WORKSHOP_TOOL_LED_CALCULATOR_SUBTITLE,     ///< "Widerstand, Normwert, Verlustleistung" / "Resistor, preferred value, power dissipation"
    WORKSHOP_TOOL_RC_FILTER_SUBTITLE,          ///< "Tiefpass / Hochpass, 1. Ordnung" / "Low-pass / high-pass, 1st order"
    WORKSHOP_TOOL_LC_FILTER_SUBTITLE,          ///< "Tiefpass / Hochpass, 2. Ordnung" / "Low-pass / high-pass, 2nd order"
    WORKSHOP_TOOL_UKW_RESONATOR_SUBTITLE,      ///< "Luftspulen-Resonanzkreis 80...150 MHz" / "Air-core resonant circuit 80...150 MHz"
    WORKSHOP_TOOL_LM317_SUBTITLE,              ///< "Vref / R1 / R2 / Vout" / "Vref / R1 / R2 / Vout"
    WORKSHOP_TOOL_DECIBEL_SUBTITLE,            ///< "Spannung / Leistung / dB" / "Voltage / power / dB"
    WORKSHOP_TOOL_NE555_SUBTITLE,              ///< "Astabile Kippstufe" / "Astable multivibrator"
    WORKSHOP_TOOL_OPAMP_SUBTITLE,              ///< "Invertierender Verstärker" / "Inverting amplifier"
    WORKSHOP_TOOL_INCH_MM_SUBTITLE,            ///< "Bidirektionale Umrechnung" / "Bidirectional conversion"
    WORKSHOP_TOOL_TEMP_SUBTITLE,               ///< "C <-> F <-> K" / "C <-> F <-> K"
    WORKSHOP_TOOL_PRESSURE_SUBTITLE,           ///< "bar / psi / atm / Pa" / "bar / psi / atm / Pa"
    WORKSHOP_TOOL_SI_PREFIXES_SUBTITLE,        ///< "Piko bis Tera - Symbol, Potenz, Beispiel" / "Pico to tera - symbol, power, example"
    WORKSHOP_TOOL_ASCII_TABLE_SUBTITLE,        ///< "Zeichen 0-127 mit DEC / HEX / OCT / BIN" / "Characters 0-127 with DEC / HEX / OCT / BIN"
    WORKSHOP_TOOL_BIT_EDITOR_SUBTITLE,         ///< "8-Bit Binär-Editor" / "8-bit binary editor"
    WORKSHOP_TOOL_LOGIC_GATE_SUBTITLE,         ///< "AND / OR / XOR / NAND / NOR / NOT" / "AND / OR / XOR / NAND / NOR / NOT"
    WORKSHOP_TOOL_STITCH_GAUGE_SUBTITLE,       ///< "Strickprobe und Zielmaße umrechnen" / "Convert tension swatch and target dimensions"
    WORKSHOP_TOOL_STITCH_DISTRIBUTOR_SUBTITLE, ///< "Zu- und Abnahmen über eine Runde" / "Increases and decreases across one round"
    WORKSHOP_TOOL_SLEEVE_TAPER_SUBTITLE,       ///< "Zunahmen und Abnahmen über mehrere Reihen" / "Increases and decreases across several rows"
    WORKSHOP_TOOL_CIRCLE_OF_FIFTHS_SUBTITLE,   ///< "Tonarten, Vorzeichen, Stufenharmonik" / "Keys, accidentals, diatonic chords"
    WORKSHOP_TOOL_METRONOME_SUBTITLE,          ///< "Tempo und Taktart, optionaler Klick" / "Tempo and time signature, optional click"

    // ========================================================================
    // RAHMEN: Abweichende Menübeschriftung für SCREW_REFERENCE
    //
    // page.title enthält "Schrauben", während die Menüliste "Schrauben-Referenz" anzeigt.
    // ========================================================================
    WORKSHOP_TOOL_SCREW_REFERENCE_LABEL, ///< "Schrauben-Referenz" / "Screw reference"

    // ========================================================================
    // RAHMEN: Fehlermeldungen
    // ========================================================================
    WORKSHOP_ERR_HEADER,           ///< "[ Fehler ]" / "[ Error ]"
    WORKSHOP_ERR_TOOL_LOAD_FAILED, ///< "Werkzeug konnte nicht geladen werden." / "Tool could not be loaded."

    // ========================================================================
    // WERKZEUG: Temperatur-Umrechner
    // ========================================================================
    WORKSHOP_TEMP_LABEL_CELSIUS,    ///< "Celsius" / "Celsius"
    WORKSHOP_TEMP_LABEL_FAHRENHEIT, ///< "Fahrenh." / "Fahrenh."
    WORKSHOP_TEMP_LABEL_KELVIN,     ///< "Kelvin" / "Kelvin"

    WORKSHOP_TEMP_UNIT_CELSIUS,    ///< "°C" / "°C"
    WORKSHOP_TEMP_UNIT_FAHRENHEIT, ///< "°F" / "°F"
    WORKSHOP_TEMP_UNIT_KELVIN,     ///< "K" / "K"

    WORKSHOP_TEMP_REF_M40, ///< "-40  Extremkälte" / "-40  extreme cold"
    WORKSHOP_TEMP_REF_0,   ///< "0  Gefrierpunkt" / "0  freezing point"
    WORKSHOP_TEMP_REF_20,  ///< "20  Raumtemp." / "20  room temp."
    WORKSHOP_TEMP_REF_37,  ///< "37  Körpertemp." / "37  body temp."
    WORKSHOP_TEMP_REF_100, ///< "100  Siedepunkt" / "100  boiling point"
    WORKSHOP_TEMP_REF_150, ///< "150" / "150"
    WORKSHOP_TEMP_REF_200, ///< "200" / "200"

    WORKSHOP_TEMP_FORMULA_C2F, ///< "°F = °C x 9/5 + 32" / "°F = °C x 9/5 + 32"
    WORKSHOP_TEMP_FORMULA_F2C, ///< "°C = (°F - 32) x 5/9" / "°C = (°F - 32) x 5/9"
    WORKSHOP_TEMP_FORMULA_K2C, ///< "°C = K - 273,15" / "°C = K - 273.15"

    // ========================================================================
    // WERKZEUG: Zoll/mm-Umrechner
    // ========================================================================
    WORKSHOP_INCHMM_LABEL_INCH, ///< "Zoll" / "Inch"
    WORKSHOP_INCHMM_LABEL_MM,   ///< "mm" / "mm"

    WORKSHOP_INCHMM_UNIT_INCH, ///< "in" / "in"
    WORKSHOP_INCHMM_UNIT_MM,   ///< "mm" / "mm"

    WORKSHOP_INCHMM_FRAC_1_16,  ///< "1/16\"" / "1/16\""
    WORKSHOP_INCHMM_FRAC_1_8,   ///< "1/8\"" / "1/8\""
    WORKSHOP_INCHMM_FRAC_3_16,  ///< "3/16\"" / "3/16\""
    WORKSHOP_INCHMM_FRAC_1_4,   ///< "1/4\"" / "1/4\""
    WORKSHOP_INCHMM_FRAC_5_16,  ///< "5/16\"" / "5/16\""
    WORKSHOP_INCHMM_FRAC_3_8,   ///< "3/8\"" / "3/8\""
    WORKSHOP_INCHMM_FRAC_7_16,  ///< "7/16\"" / "7/16\""
    WORKSHOP_INCHMM_FRAC_1_2,   ///< "1/2\"" / "1/2\""
    WORKSHOP_INCHMM_FRAC_9_16,  ///< "9/16\"" / "9/16\""
    WORKSHOP_INCHMM_FRAC_5_8,   ///< "5/8\"" / "5/8\""
    WORKSHOP_INCHMM_FRAC_11_16, ///< "11/16\"" / "11/16\""
    WORKSHOP_INCHMM_FRAC_3_4,   ///< "3/4\"" / "3/4\""
    WORKSHOP_INCHMM_FRAC_13_16, ///< "13/16\"" / "13/16\""
    WORKSHOP_INCHMM_FRAC_7_8,   ///< "7/8\"" / "7/8\""
    WORKSHOP_INCHMM_FRAC_15_16, ///< "15/16\"" / "15/16\""
    WORKSHOP_INCHMM_FRAC_1,     ///< "1\"" / "1\""

    WORKSHOP_INCHMM_INFO_FACTOR, ///< "1 Zoll = 25,4 mm" / "1 inch = 25.4 mm"

    // ========================================================================
    // WERKZEUG: Druck-Umrechner
    // ========================================================================
    WORKSHOP_PRESSURE_UNIT_BAR, ///< "bar" / "bar"
    WORKSHOP_PRESSURE_UNIT_PSI, ///< "psi" / "psi"
    WORKSHOP_PRESSURE_UNIT_ATM, ///< "atm" / "atm"
    WORKSHOP_PRESSURE_UNIT_PA,  ///< "Pa" / "Pa"

    WORKSHOP_PRESSURE_REF_HDR_APPLICATION, ///< "Anwendung" / "Application"

    WORKSHOP_PRESSURE_REF_BLOOD,      ///< "Blutdruck (systol.)" / "Blood pressure (systolic)"
    WORKSHOP_PRESSURE_REF_ATMOSPHERE, ///< "Atmosphäre (1 atm)" / "Atmosphere (1 atm)"
    WORKSHOP_PRESSURE_REF_TYRE_CAR,   ///< "Reifendruck PKW" / "Car tyre pressure"
    WORKSHOP_PRESSURE_REF_TYRE_TRUCK, ///< "Reifendruck LKW" / "Lorry tyre pressure"
    WORKSHOP_PRESSURE_REF_PNEUMATIC,  ///< "Pneumatik Werkstatt" / "Workshop pneumatics"
    WORKSHOP_PRESSURE_REF_WATER_PIPE, ///< "Wasserleitung" / "Water mains"
    WORKSHOP_PRESSURE_REF_DIVE_TANK,  ///< "Tauchflasche" / "Diving cylinder"
    WORKSHOP_PRESSURE_REF_HYDRAULIC,  ///< "Hydraulikpresse" / "Hydraulic press"

    WORKSHOP_PRESSURE_INFO_BAR, ///< "1 bar = 100.000 Pa" / "1 bar = 100,000 Pa"
    WORKSHOP_PRESSURE_INFO_PSI, ///< "1 psi = 6.894,757 Pa" / "1 psi = 6,894.757 Pa"
    WORKSHOP_PRESSURE_INFO_ATM, ///< "1 atm = 101.325 Pa" / "1 atm = 101,325 Pa"
    WORKSHOP_PRESSURE_INFO_PA,  ///< "1 Pa = 0,00001 bar" / "1 Pa = 0.00001 bar"

    // ========================================================================
    // WERKZEUG: Weltzeit-Uhr (Stadtnamen)
    // ========================================================================
    WORKSHOP_WORLDCLOCK_CITY_LOS_ANGELES, ///< "Los Angeles" / "Los Angeles"
    WORKSHOP_WORLDCLOCK_CITY_NEW_YORK,    ///< "New York" / "New York"
    WORKSHOP_WORLDCLOCK_CITY_LONDON,      ///< "London" / "London"
    WORKSHOP_WORLDCLOCK_CITY_BERLIN,      ///< "Berlin" / "Berlin"
    WORKSHOP_WORLDCLOCK_CITY_MOSCOW,      ///< "Moskau" / "Moscow"
    WORKSHOP_WORLDCLOCK_CITY_DUBAI,       ///< "Dubai" / "Dubai"
    WORKSHOP_WORLDCLOCK_CITY_TOKYO,       ///< "Tokio" / "Tokyo"
    WORKSHOP_WORLDCLOCK_CITY_SYDNEY,      ///< "Sydney" / "Sydney"

    // ========================================================================
    // WERKZEUG: Maschenverteilung (StitchDistributor)
    // ========================================================================
    WORKSHOP_STITCH_DISTRIBUTOR_LABEL_FROM,    ///< "Von" / "From"
    WORKSHOP_STITCH_DISTRIBUTOR_LABEL_TO,      ///< "Auf" / "To"
    WORKSHOP_STITCH_DISTRIBUTOR_UNIT_STITCHES, ///< "M" / "st"

    WORKSHOP_STITCH_DISTRIBUTOR_TITLE_DISTRIBUTION, ///< "Verteilung" / "Distribution"
    WORKSHOP_STITCH_DISTRIBUTOR_TITLE_NO_CHANGE,    ///< "Keine Änderung" / "No change"
    WORKSHOP_STITCH_DISTRIBUTOR_TITLE_INCREASE,     ///< "Zunahme" / "Increase"
    WORKSHOP_STITCH_DISTRIBUTOR_TITLE_DECREASE,     ///< "Abnahme" / "Decrease"

    WORKSHOP_STITCH_DISTRIBUTOR_ACTION_INC_WITH_PLAIN, ///< "dann 1 Masche zunehmen" / "then increase 1 stitch"
    WORKSHOP_STITCH_DISTRIBUTOR_ACTION_DEC_WITH_PLAIN, ///< "dann 2 Maschen zusammen" / "then knit 2 together"
    WORKSHOP_STITCH_DISTRIBUTOR_ACTION_INC_NO_PLAIN,   ///< "1 Masche zunehmen" / "increase 1 stitch"
    WORKSHOP_STITCH_DISTRIBUTOR_ACTION_DEC_NO_PLAIN,   ///< "2 Maschen zusammen" / "knit 2 together"

    WORKSHOP_STITCH_DISTRIBUTOR_HINT_EMPTY_1,      ///< "Bitte Maschenzahlen" / "Please enter"
    WORKSHOP_STITCH_DISTRIBUTOR_HINT_EMPTY_2,      ///< "eingeben." / "stitch counts."
    WORKSHOP_STITCH_DISTRIBUTOR_HINT_NO_CHANGE_1,  ///< "Maschenzahl bleibt gleich." / "Stitch count stays the same."
    WORKSHOP_STITCH_DISTRIBUTOR_HINT_NO_CHANGE_2,  ///< "Runde gerade stricken." / "Knit the round plain."
    WORKSHOP_STITCH_DISTRIBUTOR_HINT_INFEASIBLE_1, ///< "Differenz zu groß" / "Difference too large"
    WORKSHOP_STITCH_DISTRIBUTOR_HINT_INFEASIBLE_2, ///< "für eine einzelne Runde." / "for a single round."

    WORKSHOP_STITCH_DISTRIBUTOR_FMT_SEG_WITH_PLAIN, ///< "{0:%d} mal Strick {1:%d}," / "{0:%d} times: knit {1:%d},"
    WORKSHOP_STITCH_DISTRIBUTOR_FMT_SEG_NO_PLAIN,   ///< "{0:%d} mal:" / "{0:%d} times:"

    // ========================================================================
    // WERKZEUG: Ärmelschräge (SleeveTaper)
    // ========================================================================
    WORKSHOP_SLEEVE_TAPER_LABEL_FROM,    ///< "Von" / "From"
    WORKSHOP_SLEEVE_TAPER_LABEL_TO,      ///< "Auf" / "To"
    WORKSHOP_SLEEVE_TAPER_LABEL_OVER,    ///< "Über" / "Over"
    WORKSHOP_SLEEVE_TAPER_UNIT_STITCHES, ///< "M" / "st"
    WORKSHOP_SLEEVE_TAPER_UNIT_ROWS,     ///< "R" / "row"

    WORKSHOP_SLEEVE_TAPER_TITLE_SLEEVE,   ///< "Ärmelschräge" / "Sleeve taper"
    WORKSHOP_SLEEVE_TAPER_TITLE_NO_TAPER, ///< "Keine Schräge" / "No taper"
    WORKSHOP_SLEEVE_TAPER_TITLE_INCREASE, ///< "Zunahme" / "Increase"
    WORKSHOP_SLEEVE_TAPER_TITLE_DECREASE, ///< "Abnahme" / "Decrease"

    WORKSHOP_SLEEVE_TAPER_ACTION_INC_BOTH, ///< "beidseitig 1 M zunehmen" / "increase 1 st each side"
    WORKSHOP_SLEEVE_TAPER_ACTION_DEC_BOTH, ///< "beidseitig 1 M abnehmen" / "decrease 1 st each side"

    WORKSHOP_SLEEVE_TAPER_HINT_EMPTY_1,      ///< "Bitte alle Werte" / "Please enter"
    WORKSHOP_SLEEVE_TAPER_HINT_EMPTY_2,      ///< "eingeben." / "all values."
    WORKSHOP_SLEEVE_TAPER_HINT_NO_CHANGE_1,  ///< "Maschenzahl bleibt gleich." / "Stitch count stays the same."
    WORKSHOP_SLEEVE_TAPER_HINT_NO_CHANGE_2,  ///< "Keine Schräge nötig." / "No taper needed."
    WORKSHOP_SLEEVE_TAPER_HINT_ODD_1,        ///< "Differenz ist ungerade." / "Difference is odd."
    WORKSHOP_SLEEVE_TAPER_HINT_ODD_2,        ///< "Bitte Anfang oder Ende" / "Please adjust start or end"
    WORKSHOP_SLEEVE_TAPER_HINT_ODD_3,        ///< "um 1 anpassen." / "by 1."
    WORKSHOP_SLEEVE_TAPER_HINT_TOO_MANY_INC, ///< "Zu viele Zunahmen" / "Too many increases"
    WORKSHOP_SLEEVE_TAPER_HINT_TOO_MANY_DEC, ///< "Zu viele Abnahmen" / "Too many decreases"
    WORKSHOP_SLEEVE_TAPER_HINT_TOO_MANY_2,   ///< "für diese Reihen-Anzahl." / "for this row count."

    WORKSHOP_SLEEVE_TAPER_FMT_ROWS,     ///< "{0:%d} mal alle {1:%d}. Reihe" / "{0:%d} times every {1:%d}th row"
    WORKSHOP_SLEEVE_TAPER_FMT_ROWS_AND, ///< "{0:%d} mal alle {1:%d}. Reihe und" / "{0:%d} times every {1:%d}th row and"

    // ========================================================================
    // WERKZEUG: Maschenprobe (StitchGauge)
    // ========================================================================
    WORKSHOP_STITCH_GAUGE_LABEL_STITCHES_PER_10CM, ///< "M / 10 cm" / "st / 10 cm"
    WORKSHOP_STITCH_GAUGE_LABEL_WIDTH,             ///< "Breite" / "Width"
    WORKSHOP_STITCH_GAUGE_LABEL_STITCHES,          ///< "Maschen" / "Stitches"
    WORKSHOP_STITCH_GAUGE_LABEL_ROWS_PER_10CM,     ///< "R / 10 cm" / "row / 10 cm"
    WORKSHOP_STITCH_GAUGE_LABEL_HEIGHT,            ///< "Höhe" / "Height"
    WORKSHOP_STITCH_GAUGE_LABEL_ROWS,              ///< "Reihen" / "Rows"

    WORKSHOP_STITCH_GAUGE_UNIT_STITCHES, ///< "M" / "st"
    WORKSHOP_STITCH_GAUGE_UNIT_CM,       ///< "cm" / "cm"
    WORKSHOP_STITCH_GAUGE_UNIT_ROWS,     ///< "R" / "row"

    WORKSHOP_STITCH_GAUGE_VIS_CM,              ///< "{0:%.4g} cm" / "{0:%.4g} cm"
    WORKSHOP_STITCH_GAUGE_VIS_STITCHES,        ///< "{0:%.4g} M" / "{0:%.4g} st"
    WORKSHOP_STITCH_GAUGE_VIS_ROWS,            ///< "{0:%.4g} R" / "{0:%.4g} row"
    WORKSHOP_STITCH_GAUGE_VIS_STITCHES_PER_10, ///< "{0:%.4g} M / 10 cm" / "{0:%.4g} st / 10 cm"
    WORKSHOP_STITCH_GAUGE_VIS_ROWS_PER_10,     ///< "{0:%.4g} R / 10 cm" / "{0:%.4g} row / 10 cm"

    // ========================================================================
    // WERKZEUG: BMI-Rechner
    // ========================================================================
    WORKSHOP_BMI_ZONE_SEVERE_UNDER, ///< "Starkes Untergewicht" / "Severely underweight"
    WORKSHOP_BMI_ZONE_UNDER,        ///< "Untergewicht" / "Underweight"
    WORKSHOP_BMI_ZONE_NORMAL,       ///< "Normalgewicht" / "Normal weight"
    WORKSHOP_BMI_ZONE_OVER,         ///< "Übergewicht" / "Overweight"
    WORKSHOP_BMI_ZONE_OBESE_1,      ///< "Adipositas Grad I" / "Obese class I"
    WORKSHOP_BMI_ZONE_OBESE_2,      ///< "Adipositas Grad II" / "Obese class II"
    WORKSHOP_BMI_ZONE_OBESE_3,      ///< "Adipositas Grad III" / "Obese class III"

    WORKSHOP_BMI_LABEL_HEIGHT, ///< "Größe" / "Height"
    WORKSHOP_BMI_LABEL_WEIGHT, ///< "Gewicht" / "Weight"

    WORKSHOP_BMI_UNIT_CM, ///< "cm" / "cm"
    WORKSHOP_BMI_UNIT_KG, ///< "kg" / "kg"

    WORKSHOP_BMI_NOTE_ADULTS, ///< "Gilt für Erwachsene von 19 bis 65." / "Valid for adults aged 19 to 65."

    WORKSHOP_BMI_FMT_RESULT, ///< "BMI = {0}" / "BMI = {0}"

    // ========================================================================
    // WERKZEUG: Ohmsches Gesetz (OhmCalculator)
    // ========================================================================
    WORKSHOP_OHM_NAME_VOLTAGE,    ///< "(Spannung)" / "(Voltage)"
    WORKSHOP_OHM_NAME_CURRENT,    ///< "(Strom)" / "(Current)"
    WORKSHOP_OHM_NAME_RESISTANCE, ///< "(Widerstand)" / "(Resistance)"
    WORKSHOP_OHM_NAME_POWER,      ///< "(Leistung)" / "(Power)"

    // ========================================================================
    // WERKZEUG: Dezibel-Rechner
    // ========================================================================
    WORKSHOP_DECIBEL_SECTION_VOLTAGE, ///< "Spannung" / "Voltage"
    WORKSHOP_DECIBEL_SECTION_POWER,   ///< "Leistung" / "Power"

    WORKSHOP_DECIBEL_NAME_INPUT,  ///< "(Eingang)" / "(Input)"
    WORKSHOP_DECIBEL_NAME_OUTPUT, ///< "(Ausgang)" / "(Output)"
    WORKSHOP_DECIBEL_NAME_LEVEL,  ///< "(Pegel)" / "(Level)"

    // ========================================================================
    // WERKZEUG: Logikgatter (LogicGate)
    //
    // Dynamischer Subtitle pro Gattertyp. Reihenfolge entspricht GateType-Enum-Werten (AND=0, OR=1, XOR=2, NAND=3, NOR=4, NOT=5).
    // ========================================================================
    WORKSHOP_LOGIC_GATE_DESC_AND,  ///< "AND - Konjunktion" / "AND - Conjunction"
    WORKSHOP_LOGIC_GATE_DESC_OR,   ///< "OR - Disjunktion" / "OR - Disjunction"
    WORKSHOP_LOGIC_GATE_DESC_XOR,  ///< "XOR - Exklusiv-Oder" / "XOR - Exclusive OR"
    WORKSHOP_LOGIC_GATE_DESC_NAND, ///< "NAND - Negierte Konjunktion" / "NAND - Negated conjunction"
    WORKSHOP_LOGIC_GATE_DESC_NOR,  ///< "NOR - Negierte Disjunktion" / "NOR - Negated disjunction"
    WORKSHOP_LOGIC_GATE_DESC_NOT,  ///< "NOT - Inverter" / "NOT - Inverter"

    // ========================================================================
    // WERKZEUG: RC/RL-Filter (RcFilter)
    //
    // Footer-Toggles "tiefpass"/"hochpass" und vier dynamische Subtitle-Strings (Filterart x Topologie). Die Slope-Anzeige in der Info-Zeile wechselt mit der Filterart das Vorzeichen.
    // ========================================================================
    WORKSHOP_RC_FILTER_BTN_LOW_PASS,  ///< "tiefpass" / "low-pass"
    WORKSHOP_RC_FILTER_BTN_HIGH_PASS, ///< "hochpass" / "high-pass"

    WORKSHOP_RC_FILTER_SUB_LP_RC, ///< "Tiefpass RC, 1. Ordnung" / "Low-pass RC, 1st order"
    WORKSHOP_RC_FILTER_SUB_LP_RL, ///< "Tiefpass RL, 1. Ordnung" / "Low-pass RL, 1st order"
    WORKSHOP_RC_FILTER_SUB_HP_RC, ///< "Hochpass RC, 1. Ordnung" / "High-pass RC, 1st order"
    WORKSHOP_RC_FILTER_SUB_HP_RL, ///< "Hochpass RL, 1. Ordnung" / "High-pass RL, 1st order"

    WORKSHOP_RC_FILTER_SLOPE_LP, ///< "-20 dB/Dek" / "-20 dB/dec"
    WORKSHOP_RC_FILTER_SLOPE_HP, ///< "+20 dB/Dek" / "+20 dB/dec"

    // ========================================================================
    // WERKZEUG: LC-Filter (LcFilter)
    //
    // Footer-Toggles "tiefpass"/"hochpass" und zwei dynamische Subtitle-Strings (LP/HP). Slope-Anzeige zweiter Ordnung (40 dB/Dek).
    // ========================================================================
    WORKSHOP_LC_FILTER_BTN_LOW_PASS,  ///< "tiefpass" / "low-pass"
    WORKSHOP_LC_FILTER_BTN_HIGH_PASS, ///< "hochpass" / "high-pass"

    WORKSHOP_LC_FILTER_SUB_LP, ///< "Tiefpass LC, 2. Ordnung" / "Low-pass LC, 2nd order"
    WORKSHOP_LC_FILTER_SUB_HP, ///< "Hochpass LC, 2. Ordnung" / "High-pass LC, 2nd order"

    WORKSHOP_LC_FILTER_SLOPE_LP, ///< "-40 dB/Dek" / "-40 dB/dec"
    WORKSHOP_LC_FILTER_SLOPE_HP, ///< "+40 dB/Dek" / "+40 dB/dec"

    // ========================================================================
    // WERKZEUG: UKW-Schwingkreis (UkwResonator)
    //
    // Vier dynamische Untertitel je Drahtdurchmesser, zwei Warnmeldungen zu Wicklungsüberlappung und Frequenzbereich sowie Formatzeichenfolgen für Windungszahl und Q-Faktor.
    // ========================================================================
    WORKSHOP_UKW_RESONATOR_SUB_D05, ///< "Luftspule, d = 0,5 mm Cu" / "Air-core coil, d = 0.5 mm Cu"
    WORKSHOP_UKW_RESONATOR_SUB_D08, ///< "Luftspule, d = 0,8 mm Cu" / "Air-core coil, d = 0.8 mm Cu"
    WORKSHOP_UKW_RESONATOR_SUB_D10, ///< "Luftspule, d = 1,0 mm Cu" / "Air-core coil, d = 1.0 mm Cu"
    WORKSHOP_UKW_RESONATOR_SUB_D15, ///< "Luftspule, d = 1,5 mm Cu" / "Air-core coil, d = 1.5 mm Cu"

    WORKSHOP_UKW_RESONATOR_FMT_TURNS,        ///< "N = {0} Wdg." / "N = {0} turns"
    WORKSHOP_UKW_RESONATOR_FMT_Q_APPROX,     ///< "Q ca. {0:%d}" / "Q approx. {0:%d}"
    WORKSHOP_UKW_RESONATOR_WARN_OVERLAP,     ///< "Wdg. überlappen!" / "Turns overlap!"
    WORKSHOP_UKW_RESONATOR_WARN_OUT_OF_RANGE,///< "außerhalb!" / "out of range!"

    // ========================================================================
    // WERKZEUG: NE555-Timer (Ne555)
    //
    // Footer-Toggles DE gross ("Astabil"/"Monostabil"), UK klein ("astable"/"monostable"). Zwei dynamische Subtitle-Strings.
    // ========================================================================
    WORKSHOP_NE555_BTN_ASTABLE,    ///< "Astabil" / "astable"
    WORKSHOP_NE555_BTN_MONOSTABLE, ///< "Monostabil" / "monostable"

    WORKSHOP_NE555_SUB_ASTABLE,    ///< "Astabile Kippstufe" / "Astable multivibrator"
    WORKSHOP_NE555_SUB_MONOSTABLE, ///< "Monostabile Kippstufe" / "Monostable multivibrator"

    // ========================================================================
    // WERKZEUG: Op-Amp (OpAmp)
    //
    // Footer-Toggles und zwei dynamische Subtitle-Strings.
    // ========================================================================
    WORKSHOP_OPAMP_BTN_INVERTING,     ///< "invertierend" / "inverting"
    WORKSHOP_OPAMP_BTN_NON_INVERTING, ///< "nicht invertierend" / "non-inverting"

    WORKSHOP_OPAMP_SUB_INVERTING,     ///< "Invertierender Verstärker" / "Inverting amplifier"
    WORKSHOP_OPAMP_SUB_NON_INVERTING, ///< "Nicht invertierender Verstärker" / "Non-inverting amplifier"

    // ========================================================================
    // WERKZEUG: SI-Präfixe (SiPrefixes)
    //
    // Vier Spalten-Header, ein Hinweistext im Info-Balken bei fehlender Auswahl und neun Präfixnamen von Tera bis Piko. Symbole, Beispiele und Zehnerpotenz-Format sind sprachneutral und bleiben in der Toolquelle.
    // ========================================================================
    WORKSHOP_SI_PREFIXES_HDR_PREFIX,  ///< "Präfix" / "Prefix"
    WORKSHOP_SI_PREFIXES_HDR_SYMBOL,  ///< "Symbol" / "Symbol"
    WORKSHOP_SI_PREFIXES_HDR_POWER,   ///< "Potenz" / "Power"
    WORKSHOP_SI_PREFIXES_HDR_EXAMPLE, ///< "Beispiel" / "Example"

    WORKSHOP_SI_PREFIXES_HINT_TAP, ///< "Präfix antippen für Umrechnungshinweis" / "Tap a prefix for conversion hint"

    WORKSHOP_SI_PREFIXES_NAME_TERA,  ///< "Tera" / "Tera"
    WORKSHOP_SI_PREFIXES_NAME_GIGA,  ///< "Giga" / "Giga"
    WORKSHOP_SI_PREFIXES_NAME_MEGA,  ///< "Mega" / "Mega"
    WORKSHOP_SI_PREFIXES_NAME_KILO,  ///< "Kilo" / "Kilo"
    WORKSHOP_SI_PREFIXES_NAME_BASE,  ///< "Basis" / "Base"
    WORKSHOP_SI_PREFIXES_NAME_MILLI, ///< "Milli" / "Milli"
    WORKSHOP_SI_PREFIXES_NAME_MICRO, ///< "Mikro" / "Micro"
    WORKSHOP_SI_PREFIXES_NAME_NANO,  ///< "Nano" / "Nano"
    WORKSHOP_SI_PREFIXES_NAME_PICO,  ///< "Piko" / "Pico"

    // ========================================================================
    // WERKZEUG: Schrauben-Referenz (ScrewRef)
    //
    // Fußzeilenumschaltung "metrisch"/"fein", dynamischer Untertitel je Gewindeart, Titelformat mit Schraubenname und Steigung, Tabellenbeschriftungen und zwei Bemaßungsformate für die Seitenansicht.
    // ========================================================================
    WORKSHOP_SCREW_REF_BTN_COARSE, ///< "metrisch" / "metric"
    WORKSHOP_SCREW_REF_BTN_FINE,   ///< "fein" / "fine"

    WORKSHOP_SCREW_REF_SUB_COARSE, ///< "Metrisch (DIN 13)" / "Metric (DIN 13)"
    WORKSHOP_SCREW_REF_SUB_FINE,   ///< "Feingewinde (DIN 13)" / "Fine thread (DIN 13)"

    WORKSHOP_SCREW_REF_FMT_TITLE_COARSE, ///< "{0} x {1:%.4g}" / "{0} x {1:%.4g}"
    WORKSHOP_SCREW_REF_FMT_TITLE_FINE,   ///< "{0} x {1:%.4g} (fein)" / "{0} x {1:%.4g} (fine)"

    WORKSHOP_SCREW_REF_NO_FINE_THREAD, ///< "kein Feingewinde" / "no fine thread"

    WORKSHOP_SCREW_REF_LABEL_PITCH,       ///< "Steigung" / "Pitch"
    WORKSHOP_SCREW_REF_LABEL_TAPPING,     ///< "Kernloch-Bohrung" / "Tapping size"
    WORKSHOP_SCREW_REF_LABEL_CLEARANCE,   ///< "Durchgangsbohrung" / "Clearance hole"
    WORKSHOP_SCREW_REF_LABEL_AF_SOCKET,   ///< "SW Inbus (DIN 912)" / "AF hex socket (DIN 912)"
    WORKSHOP_SCREW_REF_LABEL_AF_HEX,      ///< "SW Sechskant (DIN 934)" / "AF hex head (DIN 934)"
    WORKSHOP_SCREW_REF_LABEL_HEAD_HEIGHT, ///< "Kopfhöhe (DIN 912)" / "Head height (DIN 912)"
    WORKSHOP_SCREW_REF_LABEL_HEAD_DIA,    ///< "Kopf-Ø (DIN 912)" / "Head diameter (DIN 912)"
    WORKSHOP_SCREW_REF_LABEL_TORQUE,      ///< "Drehmoment" / "Torque"

    WORKSHOP_SCREW_REF_FMT_DIM_SW,    ///< "SW {0:%.4g}" / "AF {0:%.4g}"
    WORKSHOP_SCREW_REF_FMT_DIM_DRILL, ///< "Bohrung {0:%.4g}" / "drill {0:%.4g}"

    // ========================================================================
    // WERKZEUG: Farb-Konverter (ColorConverter)
    //
    // Der Hinweistext steht über den drei quantisierten RGB-Werten. R/G/B-Reglerbeschriftungen, HEX/RGB565 und die Formatzeichenfolgen sind sprachneutral.
    // ========================================================================
    WORKSHOP_COLOR_CONVERTER_RGB_HINT, ///< "Display-RGB nach der RGB565-Quantisierung" / "Display RGB after RGB565 quantisation"

    // ========================================================================
    // WERKZEUG: ASCII-Tabelle (AsciiTable)
    //
    // Der dynamische Untertitel gilt nur für den erweiterten Bereich von 128 bis 255. Im Standardbereich liefert getSubtitle() einen leeren Puffer und Workshop verwendet den statischen Untertitel aus PageDef. Hinzu kommen der Antipp-Hinweis und die Markierung für nicht definierte Windows-1252-Positionen.
    // ========================================================================
    WORKSHOP_ASCII_TABLE_SUB_EXTENDED, ///< "Zeichen 128-255 mit DEC / HEX / OCT / BIN" / "Characters 128-255 with DEC / HEX / OCT / BIN"
    WORKSHOP_ASCII_TABLE_HINT_TAP,     ///< "Tippe auf ein Zeichen" / "Tap on a character"
    WORKSHOP_ASCII_TABLE_NOT_DEFINED,  ///< "nicht definiert" / "not defined"

    // ========================================================================
    // WERKZEUG: Bit-Editor (BitEditor)
    //
    // Drei dynamische Subtitle-Strings (8/16/32 Bit). Footer-Toggle-Beschriftungen sind reine Ziffern und damit sprachneutral.
    // ========================================================================
    WORKSHOP_BIT_EDITOR_SUB_8,  ///< "8-Bit Binär-Editor" / "8-bit binary editor"
    WORKSHOP_BIT_EDITOR_SUB_16, ///< "16-Bit Binär-Editor" / "16-bit binary editor"
    WORKSHOP_BIT_EDITOR_SUB_32, ///< "32-Bit Binär-Editor" / "32-bit binary editor"

    // ========================================================================
    // WERKZEUG: Rechner (Calculator)
    //
    // Ein Anzeigetext für den Fehlerfall NaN/Inf. Tasten, Operatoren und Anzeige-Labels sind sprachneutral.
    // ========================================================================
    WORKSHOP_CALCULATOR_ERROR, ///< "Fehler" / "Error"

    // ========================================================================
    // GEMEINSAM: Dialogschaltflächen für mehrere Werkzeuge
    //
    // Drei Beschriftungen für Hilfe- und Bestätigungsdialoge mehrerer Werkzeuge. MessageBox::setButtonLabel(...) speichert den Zeiger auf den Member-Puffer, daher müssen die Puffer während der Dialognutzung gültig bleiben.
    // ========================================================================
    WORKSHOP_DIALOG_CLOSE,  ///< "schließen" / "close"
    WORKSHOP_DIALOG_OK,     ///< "ok" / "ok"
    WORKSHOP_DIALOG_CANCEL, ///< "abbruch" / "cancel"

    // ========================================================================
    // WERKZEUG: Metronom (Metronome)
    //
    // Footer-Buttons (start/stop/-/+/tap/?) und Taktart-Labels (2/4 ...) sind sprachneutral. Der Untertitel "120 BPM - Moderato" bleibt sprachneutral, weil die Tempo-Bezeichnungen italienisch sind.
    // ========================================================================
    WORKSHOP_METRONOME_HELP_TITLE,         ///< Titel des Hilfe-Dialogs
    WORKSHOP_METRONOME_HELP_TEXT,          ///< Mehrzeiliger Hilfe-Text
    WORKSHOP_METRONOME_HINT_TAP_TO_START,  ///< "Tippen zum Starten" / "Tap to start"

    // ========================================================================
    // WERKZEUG: Quintenzirkel (CircleOfFifths) - Minimalmigration
    //
    // Tonartnamen, Tonleitertöne und Akkordsymbole verwenden auch im englischen Modus die deutsche Notation H, B, Fis und Es. Lokalisiert werden Funktionsbezeichnungen, Spaltenüberschriften, Dur-Suffix, Vorzeichendetails und Untertitelformate.
    // ========================================================================
    WORKSHOP_CIRCLE_OF_FIFTHS_FUNC_TONIC,        ///< "Tonika" / "Tonic"
    WORKSHOP_CIRCLE_OF_FIFTHS_FUNC_SUBDOMINANT,  ///< "Subdominante" / "Subdominant"
    WORKSHOP_CIRCLE_OF_FIFTHS_FUNC_DOMINANT,     ///< "Dominante" / "Dominant"
    WORKSHOP_CIRCLE_OF_FIFTHS_FUNC_PARALLEL,     ///< "T-Parallele" / "Relative minor"

    WORKSHOP_CIRCLE_OF_FIFTHS_SUFFIX_MAJOR,      ///< "Dur" / "major"
    WORKSHOP_CIRCLE_OF_FIFTHS_HEADER_SCALE,      ///< "Tonleiter" / "Scale"
    WORKSHOP_CIRCLE_OF_FIFTHS_HEADER_DEGREES,    ///< "Stufen" / "Degrees"

    WORKSHOP_CIRCLE_OF_FIFTHS_FMT_HEADER_KEY,    ///< "{0}-Dur" / "{0} major"

    WORKSHOP_CIRCLE_OF_FIFTHS_NO_ACCIDENTALS,    ///< "ohne Vorzeichen" / "no accidentals"
    WORKSHOP_CIRCLE_OF_FIFTHS_UNIT_SHARP_SG,     ///< "Kreuz" / "sharp"
    WORKSHOP_CIRCLE_OF_FIFTHS_UNIT_SHARP_PL,     ///< "Kreuze" / "sharps"
    WORKSHOP_CIRCLE_OF_FIFTHS_UNIT_FLAT_SG,      ///< "b" / "flat"
    WORKSHOP_CIRCLE_OF_FIFTHS_UNIT_FLAT_PL,      ///< "Bs" / "flats"

    WORKSHOP_CIRCLE_OF_FIFTHS_FMT_SUB_NONE,      ///< "{0}-Dur (keine Vorzeichen)" / "{0} major (no accidentals)"
    WORKSHOP_CIRCLE_OF_FIFTHS_FMT_SUB_ONE_SHARP, ///< "{0}-Dur (1 Kreuz)" / "{0} major (1 sharp)"
    WORKSHOP_CIRCLE_OF_FIFTHS_FMT_SUB_ONE_FLAT,  ///< "{0}-Dur (1 b)" / "{0} major (1 flat)"
    WORKSHOP_CIRCLE_OF_FIFTHS_FMT_SUB_N_SHARPS,  ///< "{0}-Dur ({1:%d} Kreuze)" / "{0} major ({1:%d} sharps)"
    WORKSHOP_CIRCLE_OF_FIFTHS_FMT_SUB_N_FLATS,   ///< "{0}-Dur ({1:%d} Bs)" / "{0} major ({1:%d} flats)"

    // ========================================================================
    // WERKZEUG: Würfel (Dice)
    //
    // Die Beschriftungen "1 Würfel" und "2 Würfel" sind sprachabhängig und liegen in Member-Puffern. Die Kartenbeschriftungen ERGEBNIS/HISTORIE und der Platzhalter "noch keine Würfe" erscheinen im Inhaltsbereich. Für jeden Modus existiert eine Untertitelvariante.
    // ========================================================================
    WORKSHOP_DICE_HELP_TITLE,       ///< Titel des Hilfe-Dialogs
    WORKSHOP_DICE_HELP_TEXT,        ///< Mehrzeiliger Hilfe-Text
    WORKSHOP_DICE_BTN_ONE,          ///< "1 Würfel" / "1 die"
    WORKSHOP_DICE_BTN_TWO,          ///< "2 Würfel" / "2 dice"
    WORKSHOP_DICE_LABEL_RESULT,     ///< "ERGEBNIS" / "RESULT"
    WORKSHOP_DICE_LABEL_HISTORY,    ///< "HISTORIE" / "HISTORY"
    WORKSHOP_DICE_HISTORY_EMPTY,    ///< "noch keine Würfe" / "no rolls yet"
    WORKSHOP_DICE_SUB_ROLLING,      ///< "Würfeln..." / "Rolling..."
    WORKSHOP_DICE_SUB_ONE_DIE,      ///< "Ein Würfel" / "One die"
    WORKSHOP_DICE_SUB_TWO_DICE,     ///< "Zwei Würfel" / "Two dice"

    // ========================================================================
    // WERKZEUG: 2048 (Game2048)
    //
    // Hilfe- und Rücksetzdialog, drei Untertitelvarianten, Seitenleistenbeschriftungen, je zwei Einblendungszeilen für Sieg und Spielende sowie die Schaltflächenbeschriftungen "neu", "reset" und "weiter".
    // ========================================================================
    WORKSHOP_GAME_2048_HELP_TITLE,           ///< Titel des Hilfe-Dialogs
    WORKSHOP_GAME_2048_HELP_TEXT,            ///< Mehrzeiliger Hilfe-Text
    WORKSHOP_GAME_2048_RESET_TITLE,          ///< Titel des Rücksetzdialogs
    WORKSHOP_GAME_2048_RESET_TEXT,           ///< Erläuternder Text im Rücksetzdialog
    WORKSHOP_GAME_2048_SUB_PLAYING,          ///< Untertitel während des Spiels
    WORKSHOP_GAME_2048_SUB_WON,              ///< Untertitel nach Erreichen der 2048-Kachel
    WORKSHOP_GAME_2048_SUB_GAMEOVER,         ///< Untertitel bei vollem Brett ohne Zug
    WORKSHOP_GAME_2048_LABEL_SCORE,          ///< "Punkte" / "Score"
    WORKSHOP_GAME_2048_LABEL_BEST,           ///< "Bestwert" / "Best"
    WORKSHOP_GAME_2048_LABEL_HINT,           ///< "Wischen" / "Swipe"
    WORKSHOP_GAME_2048_OVERLAY_WON_TITLE,    ///< Großer Titel in der Siegeinblendung
    WORKSHOP_GAME_2048_OVERLAY_WON_LINE2,    ///< Zweite Zeile im Sieg-Overlay
    WORKSHOP_GAME_2048_OVERLAY_LOST_TITLE,   ///< Großer Titel in der Einblendung zum Spielende
    WORKSHOP_GAME_2048_OVERLAY_LOST_LINE2,   ///< Zweite Zeile im Game-Over-Overlay
    WORKSHOP_GAME_2048_BTN_NEW,              ///< "neu" / "new"
    WORKSHOP_GAME_2048_BTN_RESET,            ///< "reset" / "reset"
    WORKSHOP_GAME_2048_BTN_CONTINUE,         ///< "weiter" / "continue"

    // ========================================================================
    // WERKZEUG: Lights Out
    //
    // Drei Schwierigkeits-Beschriftungen (leicht/mittel/schwer) werden zweifach genutzt: Footer-Toggle-Buttons (Member-Label-Puffer) und Stats-Tabelle. Der Subtitle ist dynamisch: "Schwierigkeit: <stufe>" oder "Gelöst!" oder "Lights Out" als Fallback. Drei Status-Karten im Side-Panel.
    // ========================================================================
    WORKSHOP_LIGHTS_OUT_HELP_TITLE,        ///< Titel des Hilfe-Dialogs
    WORKSHOP_LIGHTS_OUT_HELP_TEXT,         ///< Mehrzeiliger Hilfe-Text
    WORKSHOP_LIGHTS_OUT_RESET_TITLE,       ///< Titel des Reset-Dialogs
    WORKSHOP_LIGHTS_OUT_RESET_TEXT,        ///< Erläuternder Text im Rücksetzdialog
    WORKSHOP_LIGHTS_OUT_DIFF_EASY,         ///< "leicht" / "easy"
    WORKSHOP_LIGHTS_OUT_DIFF_MEDIUM,       ///< "mittel" / "medium"
    WORKSHOP_LIGHTS_OUT_DIFF_HARD,         ///< "schwer" / "hard"
    WORKSHOP_LIGHTS_OUT_LABEL_MOVES,       ///< "Züge" / "Moves"
    WORKSHOP_LIGHTS_OUT_LABEL_TIME,        ///< "Zeit" / "Time"
    WORKSHOP_LIGHTS_OUT_LABEL_STATS,       ///< "Statistik" / "Statistics"
    WORKSHOP_LIGHTS_OUT_STATUS_SOLVED,     ///< "Gelöst!" / "Solved!"
    WORKSHOP_LIGHTS_OUT_STATUS_READY,      ///< "Bereit" / "Ready"
    WORKSHOP_LIGHTS_OUT_STATUS_PLAYING,    ///< "Spielen" / "Playing"
    WORKSHOP_LIGHTS_OUT_SUB_TITLE,         ///< "Lights Out" / "Lights Out" (Fallback im Subtitle)
    WORKSHOP_LIGHTS_OUT_FMT_SUB_DIFFICULTY,///< "Schwierigkeit: {0}" / "Difficulty: {0}"
    WORKSHOP_LIGHTS_OUT_BTN_NEW,           ///< "neu" / "new"
    WORKSHOP_LIGHTS_OUT_BTN_RESET,         ///< "reset" / "reset"

    // ========================================================================
    // WERKZEUG: Mühle (Mill)
    //
    // Acht Untertitelzustände, ein großer Hilfetext mit fünf Absätzen, Rücksetzdialog, Seitenleistenbeschriftungen, Hinweiszeilen, drei Phasenanzeigen, zwei Spielermarkierungen, zwei Formate für die Steinkarte und je zwei Einblendungszeilen für drei Endzustände.
    // ========================================================================
    WORKSHOP_MILL_SUB_PLACING, ///< Untertitel während der Setzphase
    WORKSHOP_MILL_SUB_MOVING,  ///< Untertitel während der Zugphase
    WORKSHOP_MILL_SUB_FLYING,  ///< Untertitel während der Sprungphase
    WORKSHOP_MILL_SUB_CAPTURE, ///< Untertitel während des Steinschlagens
    WORKSHOP_MILL_SUB_BOT,     ///< Untertitel während des Bot-Zugs
    WORKSHOP_MILL_SUB_WON,     ///< Untertitel nach Sieg
    WORKSHOP_MILL_SUB_LOST,    ///< Untertitel nach Niederlage
    WORKSHOP_MILL_SUB_DRAW,    ///< Untertitel nach Remis

    WORKSHOP_MILL_HELP_TITLE, ///< Titel des Hilfe-Dialogs
    WORKSHOP_MILL_HELP_TEXT,  ///< Mehrzeiliger Hilfetext mit fünf Absätzen

    WORKSHOP_MILL_RESET_TITLE, ///< Titel des Rücksetzdialogs
    WORKSHOP_MILL_RESET_TEXT,  ///< Erläuternder Text im Rücksetzdialog

    WORKSHOP_MILL_LABEL_PHASE,  ///< "Phase" / "Phase"
    WORKSHOP_MILL_LABEL_TURN,   ///< "Am Zug" / "To move"
    WORKSHOP_MILL_LABEL_STONES, ///< "Steine" / "Pieces"
    WORKSHOP_MILL_LABEL_WINS,   ///< "Siege" / "Wins"
    WORKSHOP_MILL_LABEL_LOSSES, ///< "Niederlagen" / "Losses"
    WORKSHOP_MILL_LABEL_DRAWS,  ///< "Remis" / "Draws"

    WORKSHOP_MILL_HINT_DEFAULT,     ///< "..." (Platzhalter)
    WORKSHOP_MILL_HINT_WON,         ///< "Du hast gewonnen!" / "You won!"
    WORKSHOP_MILL_HINT_LOST,        ///< "Der Bot hat gewonnen." / "The bot won."
    WORKSHOP_MILL_HINT_DRAW,        ///< "Unentschieden." / "A draw."
    WORKSHOP_MILL_HINT_CAPTURE,     ///< "Schlage einen Bot-Stein" / "Capture a bot piece"
    WORKSHOP_MILL_HINT_BOT_THINKING,///< "Bot denkt..." / "Bot is thinking..."
    WORKSHOP_MILL_HINT_TAP_FREE,    ///< "Tippe einen freien Punkt" / "Tap a free point"
    WORKSHOP_MILL_HINT_JUMP_FREE,   ///< "Springe auf jeden freien Punkt" / "Jump to any free point"
    WORKSHOP_MILL_HINT_PICK_TARGET, ///< "Wähle ein Zielfeld" / "Choose a target square"
    WORKSHOP_MILL_HINT_PICK_OWN,    ///< "Wähle einen eigenen Stein" / "Choose one of your pieces"

    WORKSHOP_MILL_PHASE_PLACING, ///< "Setzen" / "Placing"
    WORKSHOP_MILL_PHASE_MOVING,  ///< "Ziehen" / "Moving"
    WORKSHOP_MILL_PHASE_FLYING,  ///< "Springen" / "Flying"

    WORKSHOP_MILL_TURN_YOU, ///< "Du" / "You"
    WORKSHOP_MILL_TURN_BOT, ///< "Bot" / "Bot"

    WORKSHOP_MILL_FMT_STONES_YOU, ///< "Du:  Hand {0:%u}  Brett {1:%u}" / "You:  hand {0:%u}  board {1:%u}"
    WORKSHOP_MILL_FMT_STONES_BOT, ///< "Bot: Hand {0:%u}  Brett {1:%u}" / "Bot: hand {0:%u}  board {1:%u}"

    WORKSHOP_MILL_OVERLAY_WON_TITLE,   ///< Großer Titel in der Siegeinblendung
    WORKSHOP_MILL_OVERLAY_WON_LINE2,   ///< Zweite Zeile im Sieg-Overlay
    WORKSHOP_MILL_OVERLAY_LOST_TITLE,  ///< Großer Titel in der Niederlageneinblendung
    WORKSHOP_MILL_OVERLAY_LOST_LINE2,  ///< Zweite Zeile im Niederlage-Overlay
    WORKSHOP_MILL_OVERLAY_DRAW_TITLE,  ///< Großer Titel in der Remiseinblendung
    WORKSHOP_MILL_OVERLAY_DRAW_LINE2,  ///< Zweite Zeile im Remis-Overlay

    WORKSHOP_MILL_BTN_EASY,   ///< "leicht" / "easy"
    WORKSHOP_MILL_BTN_MEDIUM, ///< "mittel" / "medium"
    WORKSHOP_MILL_BTN_NEW,    ///< "neu" / "new"
    WORKSHOP_MILL_BTN_RESET,  ///< "reset" / "reset"

    // ========================================================================
    // PERIODIC TABLE: Kategorie-Namen (lang, für das Detail-Popup)
    // ========================================================================
    WORKSHOP_PERIODIC_TABLE_CAT_ALKALI_LONG,          ///< "Alkalimetall" / "alkali metal"
    WORKSHOP_PERIODIC_TABLE_CAT_ALKALINE_EARTH_LONG,  ///< "Erdalkalimetall" / "alkaline earth metal"
    WORKSHOP_PERIODIC_TABLE_CAT_TRANSITION_LONG,      ///< "Übergangsmetall" / "transition metal"
    WORKSHOP_PERIODIC_TABLE_CAT_POST_TRANSITION_LONG, ///< "Post-Übergangsmetall" / "post-transition metal"
    WORKSHOP_PERIODIC_TABLE_CAT_METALLOID_LONG,       ///< "Halbmetall" / "metalloid"
    WORKSHOP_PERIODIC_TABLE_CAT_NONMETAL_LONG,        ///< "Reaktives Nichtmetall" / "reactive non-metal"
    WORKSHOP_PERIODIC_TABLE_CAT_NOBLE_GAS_LONG,       ///< "Edelgas" / "noble gas"
    WORKSHOP_PERIODIC_TABLE_CAT_LANTHANIDE_LONG,      ///< "Lanthanoid" / "lanthanide"
    WORKSHOP_PERIODIC_TABLE_CAT_ACTINIDE_LONG,        ///< "Actinoid" / "actinide"
    WORKSHOP_PERIODIC_TABLE_CAT_UNKNOWN_LONG,         ///< "Unbekannte Eigenschaften" / "unknown properties"

    // ========================================================================
    // PERIODIC TABLE: Kategorie-Namen (kurz, für die Farblegende)
    // ========================================================================
    WORKSHOP_PERIODIC_TABLE_CAT_ALKALI_SHORT,          ///< "Alkalimetall" / "alkali metal"
    WORKSHOP_PERIODIC_TABLE_CAT_ALKALINE_EARTH_SHORT,  ///< "Erdalkalimetall" / "alkaline earth"
    WORKSHOP_PERIODIC_TABLE_CAT_TRANSITION_SHORT,      ///< "Übergangsmetall" / "transition metal"
    WORKSHOP_PERIODIC_TABLE_CAT_POST_TRANSITION_SHORT, ///< "Post-Übergang" / "post-transition"
    WORKSHOP_PERIODIC_TABLE_CAT_METALLOID_SHORT,       ///< "Halbmetall" / "metalloid"
    WORKSHOP_PERIODIC_TABLE_CAT_NONMETAL_SHORT,        ///< "Nichtmetall" / "non-metal"
    WORKSHOP_PERIODIC_TABLE_CAT_NOBLE_GAS_SHORT,       ///< "Edelgas" / "noble gas"
    WORKSHOP_PERIODIC_TABLE_CAT_LANTHANIDE_SHORT,      ///< "Lanthanoid" / "lanthanide"
    WORKSHOP_PERIODIC_TABLE_CAT_ACTINIDE_SHORT,        ///< "Actinoid" / "actinide"
    WORKSHOP_PERIODIC_TABLE_CAT_UNKNOWN_SHORT,         ///< "Unbekannt" / "unknown"

    // ========================================================================
    // PERIODIC TABLE: Aggregatzustände
    // ========================================================================
    WORKSHOP_PERIODIC_TABLE_STATE_SOLID,   ///< "fest" / "solid"
    WORKSHOP_PERIODIC_TABLE_STATE_LIQUID,  ///< "flüssig" / "liquid"
    WORKSHOP_PERIODIC_TABLE_STATE_GAS,     ///< "gasförmig" / "gas"
    WORKSHOP_PERIODIC_TABLE_STATE_UNKNOWN, ///< "unbekannt" / "unknown"

    // ========================================================================
    // PERIODIC TABLE: Popup-Labels (linke Spalte)
    // ========================================================================
    WORKSHOP_PERIODIC_TABLE_LABEL_CATEGORY,     ///< "Kategorie:" / "Category:"
    WORKSHOP_PERIODIC_TABLE_LABEL_MASS,         ///< "Atommasse:" / "Atomic mass:"
    WORKSHOP_PERIODIC_TABLE_LABEL_GROUP_PERIOD, ///< "Gr. / Per.:" / "Group / period:"
    WORKSHOP_PERIODIC_TABLE_LABEL_STATE,        ///< "Zustand:" / "State:"
    WORKSHOP_PERIODIC_TABLE_LABEL_DISCOVERED,   ///< "Entdeckt:" / "Discovered:"
    WORKSHOP_PERIODIC_TABLE_LABEL_DISCOVERER,   ///< "Entdecker:" / "Discovered by:"

    // ========================================================================
    // PERIODIC TABLE: Sonderwerte für Entdeckungsjahr
    // ========================================================================
    WORKSHOP_PERIODIC_TABLE_YEAR_ANCIENT, ///< "Antike" / "Antiquity"
    WORKSHOP_PERIODIC_TABLE_YEAR_UNKNOWN, ///< "unbekannt" / "unknown"

    // ========================================================================
    // WERKZEUG: Shut the Box
    //
    // Titel, statischer Untertitel, Hilfe-/Reset-Dialog, dynamische Untertitel, Statuskarten, Overlay-Zeilen und Footer-Beschriftungen.
    // ========================================================================
    WORKSHOP_SHUT_THE_BOX_TITLE,               ///< "Shut the Box" / "Shut the Box"
    WORKSHOP_SHUT_THE_BOX_SUBTITLE,            ///< "Würfelspiel für die Pause" / "Dice game for the break"
    WORKSHOP_SHUT_THE_BOX_HELP_TITLE,          ///< Titel des Hilfe-Dialogs
    WORKSHOP_SHUT_THE_BOX_HELP_TEXT,           ///< Mehrzeiliger Hilfe-Text
    WORKSHOP_SHUT_THE_BOX_RESET_TITLE,         ///< Titel des Reset-Dialogs
    WORKSHOP_SHUT_THE_BOX_RESET_TEXT,          ///< Erläuternder Text im Reset-Dialog
    WORKSHOP_SHUT_THE_BOX_SUB_READY,           ///< Untertitel vor dem Wurf
    WORKSHOP_SHUT_THE_BOX_FMT_SUB_PICK,        ///< "Wähle Summe {0:%u}" / "Choose sum {0:%u}"
    WORKSHOP_SHUT_THE_BOX_SUB_WON,             ///< Untertitel bei Sieg
    WORKSHOP_SHUT_THE_BOX_SUB_GAMEOVER,        ///< Untertitel bei Spielende
    WORKSHOP_SHUT_THE_BOX_LABEL_ROLL,          ///< "WURF" / "ROLL"
    WORKSHOP_SHUT_THE_BOX_LABEL_PICK,          ///< "AUSWAHL" / "PICK"
    WORKSHOP_SHUT_THE_BOX_LABEL_OPEN,          ///< "OFFEN" / "OPEN"
    WORKSHOP_SHUT_THE_BOX_LABEL_BEST,          ///< "Bester Rest" / "Best rest"
    WORKSHOP_SHUT_THE_BOX_LABEL_WINS,          ///< "Siege" / "Wins"
    WORKSHOP_SHUT_THE_BOX_HINT_ROLL,           ///< "tippe für Wurf" / "tap to roll"
    WORKSHOP_SHUT_THE_BOX_OVERLAY_WON_TITLE,   ///< Großer Titel im Sieg-Overlay
    WORKSHOP_SHUT_THE_BOX_OVERLAY_WON_LINE2,   ///< Zweite Zeile im Sieg-Overlay
    WORKSHOP_SHUT_THE_BOX_OVERLAY_LOST_TITLE,  ///< Großer Titel im Ende-Overlay
    WORKSHOP_SHUT_THE_BOX_OVERLAY_LOST_LINE2,  ///< Zweite Zeile im Ende-Overlay
    WORKSHOP_SHUT_THE_BOX_BTN_ONE,             ///< "1W" / "1D"
    WORKSHOP_SHUT_THE_BOX_BTN_TWO,             ///< "2W" / "2D"
    WORKSHOP_SHUT_THE_BOX_BTN_ROLL,            ///< "wurf" / "roll"
    WORKSHOP_SHUT_THE_BOX_BTN_LAY,             ///< "legen" / "lay"
    WORKSHOP_SHUT_THE_BOX_BTN_NEW,             ///< "neu" / "new"
    WORKSHOP_SHUT_THE_BOX_BTN_RESET,           ///< "reset" / "reset"

    // ========================================================================
    // WERKZEUG: Lotto 6 aus 49
    // ========================================================================
    WORKSHOP_LOTTO_TITLE,           ///< "Systemlotto 6 aus 49" / "System lotto 6 from 49"
    WORKSHOP_LOTTO_PAGE_TITLE,      ///< "Lotto 6 aus 49 Systemspiel (BER)" / "Lotto 6 from 49 system play (BER)"
    WORKSHOP_LOTTO_SUBTITLE,        ///< "Vollsystem 007 bis 011" / "Full system 007 to 011"
    WORKSHOP_LOTTO_HELP_TITLE,      ///< Titel des Hilfe-Dialogs
    WORKSHOP_LOTTO_HELP_TEXT,       ///< Mehrzeiliger Hilfe-Text
    WORKSHOP_LOTTO_LABEL_TICKET,    ///< "Tipp" / "Ticket"
    WORKSHOP_LOTTO_LABEL_DRAW,      ///< "Ziehung" / "Draw"
    WORKSHOP_LOTTO_LABEL_FIELDS,    ///< "Felder" / "Fields"
    WORKSHOP_LOTTO_LABEL_PRICE,     ///< "Einsatz" / "Stake"
    WORKSHOP_LOTTO_LABEL_SUPER,     ///< "SZ" / "SN"
    WORKSHOP_LOTTO_LABEL_TICKET_SUPER, ///< "Tipp-SZ" / "Tip SN"
    WORKSHOP_LOTTO_LABEL_DRAW_SUPER, ///< "Zieh-SZ" / "Draw SN"
    WORKSHOP_LOTTO_DIALOG_SUPER,    ///< "Superzahl" / "Super number"
    WORKSHOP_LOTTO_DIALOG_TICKET_SUPER, ///< "getippte Superzahl" / "picked super number"
    WORKSHOP_LOTTO_DIALOG_DRAW_SUPER, ///< "gezogene Superzahl" / "drawn super number"
    WORKSHOP_LOTTO_LABEL_WINS,      ///< "Gewinnklassen" / "Prize classes"
    WORKSHOP_LOTTO_PRIZE_6_SUPER,   ///< "6 R + SZ" / "6 correct + SN"
    WORKSHOP_LOTTO_PRIZE_6,         ///< "6 R" / "6 correct"
    WORKSHOP_LOTTO_PRIZE_5_SUPER,   ///< "5 R + SZ" / "5 correct + SN"
    WORKSHOP_LOTTO_PRIZE_5,         ///< "5 R" / "5 correct"
    WORKSHOP_LOTTO_PRIZE_4_SUPER,   ///< "4 R + SZ" / "4 correct + SN"
    WORKSHOP_LOTTO_PRIZE_4,         ///< "4 R" / "4 correct"
    WORKSHOP_LOTTO_PRIZE_3_SUPER,   ///< "3 R + SZ" / "3 correct + SN"
    WORKSHOP_LOTTO_PRIZE_3,         ///< "3 R" / "3 correct"
    WORKSHOP_LOTTO_PRIZE_2_SUPER,   ///< "2 R + SZ" / "2 correct + SN"
    WORKSHOP_LOTTO_EMPTY_TICKET,    ///< "noch keine Tippzahlen" / "no ticket numbers"
    WORKSHOP_LOTTO_EMPTY_DRAW,      ///< "noch keine Ziehung" / "no draw numbers"
    WORKSHOP_LOTTO_HINT_INCOMPLETE, ///< "Tipp/Ziehung fehlt" / "Ticket/draw missing"
    WORKSHOP_LOTTO_YES,             ///< "ja" / "yes"
    WORKSHOP_LOTTO_NO,              ///< "nein" / "no"
    WORKSHOP_LOTTO_FMT_SUB_TICKET,  ///< "Tippzahlen {0:%u}/{1:%u}" / "Ticket numbers {0:%u}/{1:%u}"
    WORKSHOP_LOTTO_FMT_SUB_DRAW,    ///< "Gewinnzahlen {0:%u}/6" / "Draw numbers {0:%u}/6"
    WORKSHOP_LOTTO_SUB_READY,       ///< "Gewinnklassen ermittelt" / "Prize classes counted"
    WORKSHOP_LOTTO_FMT_SUMMARY_TICKET_SUPER, ///< "getippt {0}" / "picked {0}"
    WORKSHOP_LOTTO_FMT_SUMMARY_DRAW_SUPER, ///< "gezogen {0}" / "drawn {0}"
    WORKSHOP_LOTTO_BTN_RANDOM,      ///< "zuf" / "rnd"
    WORKSHOP_LOTTO_BTN_CLEAR,       ///< "leer" / "clear"
    WORKSHOP_LOTTO_BTN_TICKET,      ///< "tippen" / "ticket"
    WORKSHOP_LOTTO_BTN_DRAW,        ///< "ziehung" / "draw"

    // ========================================================================
    // WERKZEUG: SMD-Widerstand
    // ========================================================================
    WORKSHOP_TOOL_SMD_RESISTOR_TITLE,     ///< "SMD Widerstand" / "SMD resistor"
    WORKSHOP_TOOL_SMD_RESISTOR_SUBTITLE,  ///< "3/4 Stellen und R-Code" / "3/4 digits and R code"
    WORKSHOP_SMD_RESISTOR_LABEL_CODE,     ///< "Code" / "Code"
    WORKSHOP_SMD_RESISTOR_LABEL_VALUE,    ///< "Wert" / "Value"
    WORKSHOP_SMD_RESISTOR_ERR_EMPTY,      ///< "Code eingeben" / "Enter code"
    WORKSHOP_SMD_RESISTOR_ERR_INVALID,    ///< "Ungültiger Code" / "Invalid code"
    WORKSHOP_SMD_RESISTOR_ERR_R_CODE,     ///< "R nur einmal und mit Ziffern" / "Use R once, with digits"
    WORKSHOP_SMD_RESISTOR_NOTE_3_DIGIT,   ///< "3-stelliger Code" / "3-digit code"
    WORKSHOP_SMD_RESISTOR_NOTE_4_DIGIT,   ///< "4-stelliger Code" / "4-digit code"
    WORKSHOP_SMD_RESISTOR_NOTE_R_CODE,    ///< "R-Code" / "R code"
    WORKSHOP_SMD_RESISTOR_NOTE_ZERO,      ///< "Null-Ohm-Brücke" / "Zero-ohm link"
    WORKSHOP_SMD_RESISTOR_MODE_3,         ///< "3-stellig" / "3-digit"
    WORKSHOP_SMD_RESISTOR_MODE_4,         ///< "4-stellig" / "4-digit"
    WORKSHOP_SMD_RESISTOR_MODE_R,         ///< "R-Code" / "R code"

    // ========================================================================
    // RAHMEN: Website-QR-Dialog
    // ========================================================================
    WORKSHOP_QR_MODAL_TITLE, ///< Titel des Website-QR-Dialogs
    WORKSHOP_QR_MODAL_HINT,  ///< Hinweistext unter dem Website-QR-Code
    WORKSHOP_QR_MODAL_ERROR, ///< Fehlermeldung, wenn der QR-Code nicht erzeugt werden konnte

    // ========================================================================
    // WERKZEUG: Normales Lotto 6 aus 49
    // ========================================================================
    WORKSHOP_LOTTO_NORMAL_TITLE,      ///< "Lotto 6 aus 49" / "Lotto 6 from 49"
    WORKSHOP_LOTTO_NORMAL_PAGE_TITLE, ///< "Lotto 6 aus 49" / "Lotto 6 from 49"
    WORKSHOP_LOTTO_NORMAL_SUBTITLE,   ///< "Normaler Tipp mit 6 Zahlen" / "Standard ticket with 6 numbers"
    WORKSHOP_LOTTO_NORMAL_HELP_TITLE, ///< Titel des Hilfe-Dialogs
    WORKSHOP_LOTTO_NORMAL_HELP_TEXT,  ///< Mehrzeiliger Hilfe-Text
    WORKSHOP_LOTTO_NORMAL_HINT_DRAW_SUPER_MISSING, ///< "Superzahl in Ziehung fehlt" / "Draw super number missing"

    // ========================================================================
    // WERKZEUG: Flugradar
    // ========================================================================
    WORKSHOP_FLIGHT_RADAR_TITLE,          ///< "Flugradar" / "Flight radar"
    WORKSHOP_FLIGHT_RADAR_SUBTITLE,       ///< Statischer Untertitel
    WORKSHOP_FLIGHT_RADAR_FMT_SUBTITLE,   ///< Dynamischer Untertitel mit Radius und Ort
    WORKSHOP_FLIGHT_RADAR_HELP_TITLE,      ///< Titel des Hilfe-Dialogs
    WORKSHOP_FLIGHT_RADAR_HELP_TEXT,       ///< Mehrzeiliger Hilfe-Text
    WORKSHOP_FLIGHT_RADAR_LABEL_COUNT,    ///< "Flugzeuge" / "Aircraft"
    WORKSHOP_FLIGHT_RADAR_OFFLINE_TITLE,  ///< "Keine WLAN-Verbindung" / "No Wi-Fi connection"
    WORKSHOP_FLIGHT_RADAR_LABEL_GROUND,   ///< "am Boden" / "on ground"
    WORKSHOP_FLIGHT_RADAR_FMT_MORE,       ///< "+{0:%u} weitere" / "+{0:%u} more"
    WORKSHOP_FLIGHT_RADAR_DETAIL_TITLE_FMT, ///< Titel des Flugzeugdetail-Dialogs
    WORKSHOP_FLIGHT_RADAR_DETAIL_CALLSIGN, ///< Label für Kennung
    WORKSHOP_FLIGHT_RADAR_DETAIL_ICAO24, ///< Label für ICAO24
    WORKSHOP_FLIGHT_RADAR_DETAIL_COUNTRY, ///< Label für Herkunftsland
    WORKSHOP_FLIGHT_RADAR_DETAIL_DISTANCE, ///< Label für Entfernung
    WORKSHOP_FLIGHT_RADAR_DETAIL_ALTITUDE, ///< Label für Höhe
    WORKSHOP_FLIGHT_RADAR_DETAIL_SPEED, ///< Label für Geschwindigkeit
    WORKSHOP_FLIGHT_RADAR_DETAIL_TRACK, ///< Label für Kurs
    WORKSHOP_FLIGHT_RADAR_DETAIL_LAST_SEEN, ///< Label für letzte Sichtung
    WORKSHOP_FLIGHT_RADAR_VALUE_UNKNOWN, ///< "unbekannt" / "unknown"
    WORKSHOP_FLIGHT_RADAR_VALUE_JUST_NOW, ///< "gerade eben" / "just now"
    WORKSHOP_FLIGHT_RADAR_FMT_SECONDS_AGO, ///< Alter in Sekunden
    WORKSHOP_FLIGHT_RADAR_FMT_MINUTES_AGO, ///< Alter in Minuten
    WORKSHOP_FLIGHT_RADAR_FMT_TRACK, ///< Kurswert
    WORKSHOP_FLIGHT_RADAR_LOCATION_UNKNOWN, ///< Standort-Platzhalter
    WORKSHOP_FLIGHT_RADAR_AUTH_ACCOUNT,   ///< "Konto" / "Account"
    WORKSHOP_FLIGHT_RADAR_AUTH_GUEST,     ///< "Gast" / "Guest"
    WORKSHOP_FLIGHT_RADAR_COMPASS_NORTH,  ///< Richtungszeichen Norden im Radar
    WORKSHOP_FLIGHT_RADAR_COMPASS_EAST,   ///< Richtungszeichen Osten im Radar, "O" / "E"
    WORKSHOP_FLIGHT_RADAR_COMPASS_SOUTH,  ///< Richtungszeichen Süden im Radar
    WORKSHOP_FLIGHT_RADAR_COMPASS_WEST,   ///< Richtungszeichen Westen im Radar

    WORKSHOP_KEY_COUNT ///< Endmarker für Plausibilitätsprüfungen
};

#endif // STRINGS_WORKSHOP_H

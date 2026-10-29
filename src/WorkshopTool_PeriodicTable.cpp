// ===================================================================
// WorkshopTool_PeriodicTable.cpp - Interaktives Periodensystem der Elemente
// ===================================================================

#include "WorkshopTool_PeriodicTable.h"

#include "I18n.h"
#include "Workshop.h"
#include "fonts/ComfortaaBold10.h"
#include "fonts/ComfortaaBold14.h"
#include "fonts/ComfortaaBold18.h"
#include "fonts/LeagueGothicCond34.h"

#include <stdio.h>
#include <string.h>

// Statische Member-Definitionen
int8_t WorkshopTool_PeriodicTable::s_cellLookup[9][18];
bool   WorkshopTool_PeriodicTable::s_lookupBuilt = false;

// ===================================================================
// STATISCHE DATENBASIS
// ===================================================================

// Kürzel für ElementCategory-Werte - nur innerhalb dieser Übersetzungseinheit.
namespace {

constexpr ElementCategory kAM = ElementCategory::ALKALI_METAL;
constexpr ElementCategory kAE = ElementCategory::ALKALINE_EARTH;
constexpr ElementCategory kTM = ElementCategory::TRANSITION_METAL;
constexpr ElementCategory kPT = ElementCategory::POST_TRANSITION;
constexpr ElementCategory kML = ElementCategory::METALLOID;
constexpr ElementCategory kRN = ElementCategory::REACTIVE_NONMETAL;
constexpr ElementCategory kNG = ElementCategory::NOBLE_GAS;
constexpr ElementCategory kLN = ElementCategory::LANTHANIDE;
constexpr ElementCategory kAC = ElementCategory::ACTINIDE;
constexpr ElementCategory kUK = ElementCategory::UNKNOWN;

} // namespace (Kürzel)

// Kategorie-Farben (RGB565), orientiert an der Google-Arts-Farbpalette. Index entspricht ElementCategory-Enum (uint8_t-Wert). Werte sind die "Rahmenfarbe"; die Zellfüllung wird per scaleColor(0.55) abgedunkelt.
static const uint16_t s_catColors[] = {
    0x0C52,  // 0 ALKALI_METAL      - Petrol    #0D8A96
    0x6904,  // 1 ALKALINE_EARTH    - Weinrot   #6B2020
    0x3AD1,  // 2 TRANSITION_METAL  - Stahlblau #3D5A8A
    0x2BCD,  // 3 POST_TRANSITION   - Jade      #2E7A68
    0x69D1,  // 4 METALLOID         - Violett   #6A3A8A
    0x9A84,  // 5 REACTIVE_NONMETAL - Rost      #9A5020
    0x194C,  // 6 NOBLE_GAS         - Dunkelblau#1A2866
    0xAC04,  // 7 LANTHANIDE        - Gold      #AA8020
    0xA304,  // 8 ACTINIDE          - Amber     #A06020
    0x42AC,  // 9 UNKNOWN           - Grau      #445560
};

// Schlüssel der ausführlichen Kategorienamen für das Popup Index entspricht ElementCategory-Enum (uint8_t-Wert).
static const WorkshopKey s_catNameKeys[] = {
    WorkshopKey::WORKSHOP_PERIODIC_TABLE_CAT_ALKALI_LONG,          // 0 ALKALI_METAL
    WorkshopKey::WORKSHOP_PERIODIC_TABLE_CAT_ALKALINE_EARTH_LONG,  // 1 ALKALINE_EARTH
    WorkshopKey::WORKSHOP_PERIODIC_TABLE_CAT_TRANSITION_LONG,      // 2 TRANSITION_METAL
    WorkshopKey::WORKSHOP_PERIODIC_TABLE_CAT_POST_TRANSITION_LONG, // 3 POST_TRANSITION
    WorkshopKey::WORKSHOP_PERIODIC_TABLE_CAT_METALLOID_LONG,       // 4 METALLOID
    WorkshopKey::WORKSHOP_PERIODIC_TABLE_CAT_NONMETAL_LONG,        // 5 REACTIVE_NONMETAL
    WorkshopKey::WORKSHOP_PERIODIC_TABLE_CAT_NOBLE_GAS_LONG,       // 6 NOBLE_GAS
    WorkshopKey::WORKSHOP_PERIODIC_TABLE_CAT_LANTHANIDE_LONG,      // 7 LANTHANIDE
    WorkshopKey::WORKSHOP_PERIODIC_TABLE_CAT_ACTINIDE_LONG,        // 8 ACTINIDE
    WorkshopKey::WORKSHOP_PERIODIC_TABLE_CAT_UNKNOWN_LONG,         // 9 UNKNOWN
};

// Aggregatzustand-Namen-Keys (Index = e.state: 0=fest, 1=flüssig, 2=gasförmig).
static const WorkshopKey s_stateNameKeys[] = {
    WorkshopKey::WORKSHOP_PERIODIC_TABLE_STATE_SOLID,
    WorkshopKey::WORKSHOP_PERIODIC_TABLE_STATE_LIQUID,
    WorkshopKey::WORKSHOP_PERIODIC_TABLE_STATE_GAS,
};

// Anzahl Einträge in den Kategorie- und Zustandsarrays (für static_assert und Bounds-Check).
static constexpr size_t CATEGORY_COUNT = 10;
static constexpr size_t STATE_COUNT    = 3;

// Absicherung: Array-Größen müssen zur Anzahl der Enum-Werte passen.
static_assert(sizeof(s_catColors) / sizeof(s_catColors[0]) == CATEGORY_COUNT,
              "s_catColors muss genau CATEGORY_COUNT Eintraege haben");
static_assert(sizeof(s_catNameKeys) / sizeof(s_catNameKeys[0]) == CATEGORY_COUNT,
              "s_catNameKeys muss genau CATEGORY_COUNT Eintraege haben");
static_assert(sizeof(s_stateNameKeys) / sizeof(s_stateNameKeys[0]) == STATE_COUNT,
              "s_stateNameKeys muss genau STATE_COUNT Eintraege haben");

// Hilfsfunktionen und Text-Clipping - greifen auf die Arrays oben zu.
namespace {

// ===================================================================
// Sichere Accessor-Funktionen (kapseln den static_cast + Bounds-Check)
// ===================================================================

/**
 * @brief Liefert einen gültigen Kategorieindex für das Element.
 */
uint8_t getSafeCategoryIndex(const ElementData &e)
{
    const uint8_t idx = static_cast<uint8_t>(e.category);
    return (idx < CATEGORY_COUNT) ? idx : static_cast<uint8_t>(ElementCategory::UNKNOWN);
}

/**
 * @brief Liefert die Farbe der Elementkategorie.
 */
uint16_t getCategoryColor(const ElementData &e)
{
    return s_catColors[getSafeCategoryIndex(e)];
}

// ===================================================================
// Text-Clipping mit Ellipsis ("..."), UTF-8-sicher.
// ===================================================================

// Der gewünschte Font muss vor dem Aufruf in spr geladen sein.

/**
 * @brief Kürzt einen UTF-8-Text passend zur verfügbaren Breite.
 */
void makeEllipsizedText(char *dst, size_t dstSize,
                        Gfx_Sprite *spr, const char *text, int maxWidth)
{
    if (!dst || dstSize == 0)
        return;

    dst[0] = '\0';

    if (!spr || !text || maxWidth <= 0)
        return;

    stringutils::safeCopy(dst, dstSize, text);
    if (spr->textWidth(dst) <= maxWidth)
        return;

    static const char kEllipsis[] = "...";
    if (spr->textWidth(kEllipsis) >= maxWidth)
    {
        dst[0] = '\0';
        return;
    }

    size_t len = strlen(dst);
    while (len > 0)
    {
        // UTF-8-sicher: Continuation Bytes (0x80-0xBF) am Ende überspringen
        len--;
        while (len > 0 && (static_cast<uint8_t>(dst[len]) & 0xC0) == 0x80)
            len--;
        dst[len] = '\0';

        char trial[64];
        stringutils::safeFormat(trial, "%s%s", dst, kEllipsis);
        if (spr->textWidth(trial) <= maxWidth)
        {
            stringutils::safeCopy(dst, dstSize, trial);
            return;
        }
    }

    stringutils::safeCopy(dst, dstSize, kEllipsis);
}

/**
 * @brief Zeichnet einen UTF-8-Text gekürzt auf die verfügbare Breite.
 */
void drawTextEllipsized(Gfx_Sprite *spr, const char *text,
                        int x, int y, int maxWidth,
                        uint16_t fg, uint16_t bg)
{
    char buf[64];
    makeEllipsizedText(buf, sizeof(buf), spr, text, maxWidth);
    spr->setTextColor(fg, bg);
    spr->drawString(buf, x, y);
}

} // namespace (Hilfsfunktionen)

// ===================================================================
// ELEMENTDATEN-ARRAY (118 Elemente)
// ===================================================================

// Felder: {mass, year, number, col, row, category, state, pad, symbol, nameDe, discoverer}
// row=8 -> Lanthanoid-Zeile, row=9 -> Actinoid-Zeile
// year=0 -> seit der Antike bekannt, year>0 -> Entdeckungsjahr
// discoverer: max. 22 Zeichen, reines ASCII

const ElementData WorkshopTool_PeriodicTable::s_elements[118] = {
    // Periode 1
    {1.008f,  1766,  1,  1, 1, kRN, 2, 0, "H",  "Wasserstoff",   "Henry Cavendish"},
    {4.003f,  1868,  2, 18, 1, kNG, 2, 0, "He", "Helium",        "Janssen / Lockyer"},
    // Periode 2
    {6.941f,  1817,  3,  1, 2, kAM, 0, 0, "Li", "Lithium",       "Johan A. Arfvedson"},
    {9.012f,  1798,  4,  2, 2, kAE, 0, 0, "Be", "Beryllium",     "Louis N. Vauquelin"},
    {10.81f,  1808,  5, 13, 2, kML, 0, 0, "B",  "Bor",           "Davy / Gay-Lussac"},
    {12.01f,     0,  6, 14, 2, kRN, 0, 0, "C",  "Kohlenstoff",   "Seit der Antike"},
    {14.01f,  1772,  7, 15, 2, kRN, 2, 0, "N",  "Stickstoff",    "Daniel Rutherford"},
    {16.00f,  1774,  8, 16, 2, kRN, 2, 0, "O",  "Sauerstoff",    "Scheele / Priestley"},
    {19.00f,  1886,  9, 17, 2, kRN, 2, 0, "F",  "Fluor",         "Henri Moissan"},
    {20.18f,  1898, 10, 18, 2, kNG, 2, 0, "Ne", "Neon",          "William Ramsay"},
    // Periode 3
    {22.99f,  1807, 11,  1, 3, kAM, 0, 0, "Na", "Natrium",       "Humphry Davy"},
    {24.31f,  1808, 12,  2, 3, kAE, 0, 0, "Mg", "Magnesium",     "Humphry Davy"},
    {26.98f,  1825, 13, 13, 3, kPT, 0, 0, "Al", "Aluminium",     "Hans C. Orsted"},
    {28.09f,  1824, 14, 14, 3, kML, 0, 0, "Si", "Silicium",      "Jons J. Berzelius"},
    {30.97f,  1669, 15, 15, 3, kRN, 0, 0, "P",  "Phosphor",      "Hennig Brand"},
    {32.07f,     0, 16, 16, 3, kRN, 0, 0, "S",  "Schwefel",      "Seit der Antike"},
    {35.45f,  1774, 17, 17, 3, kRN, 2, 0, "Cl", "Chlor",         "Carl W. Scheele"},
    {39.95f,  1894, 18, 18, 3, kNG, 2, 0, "Ar", "Argon",         "Rayleigh / Ramsay"},
    // Periode 4
    {39.10f,  1807, 19,  1, 4, kAM, 0, 0, "K",  "Kalium",        "Humphry Davy"},
    {40.08f,  1808, 20,  2, 4, kAE, 0, 0, "Ca", "Calcium",       "Humphry Davy"},
    {44.96f,  1879, 21,  3, 4, kTM, 0, 0, "Sc", "Scandium",      "Lars F. Nilson"},
    {47.87f,  1791, 22,  4, 4, kTM, 0, 0, "Ti", "Titan",         "William Gregor"},
    {50.94f,  1801, 23,  5, 4, kTM, 0, 0, "V",  "Vanadium",      "del Rio / Sefstrom"},
    {52.00f,  1798, 24,  6, 4, kTM, 0, 0, "Cr", "Chrom",         "Louis N. Vauquelin"},
    {54.94f,  1774, 25,  7, 4, kTM, 0, 0, "Mn", "Mangan",        "Johann G. Gahn"},
    {55.85f,     0, 26,  8, 4, kTM, 0, 0, "Fe", "Eisen",         "Seit der Antike"},
    {58.93f,  1735, 27,  9, 4, kTM, 0, 0, "Co", "Cobalt",        "Georg Brandt"},
    {58.69f,  1751, 28, 10, 4, kTM, 0, 0, "Ni", "Nickel",        "Axel F. Cronstedt"},
    {63.55f,     0, 29, 11, 4, kTM, 0, 0, "Cu", "Kupfer",        "Seit der Antike"},
    {65.38f,     0, 30, 12, 4, kTM, 0, 0, "Zn", "Zink",          "Seit der Antike"},
    {69.72f,  1875, 31, 13, 4, kPT, 0, 0, "Ga", "Gallium",       "P. E. de Boisbaudran"},
    {72.63f,  1886, 32, 14, 4, kML, 0, 0, "Ge", "Germanium",     "Clemens A. Winkler"},
    {74.92f,     0, 33, 15, 4, kML, 0, 0, "As", "Arsen",         "Seit der Antike"},
    {78.97f,  1817, 34, 16, 4, kRN, 0, 0, "Se", "Selen",         "Jons J. Berzelius"},
    {79.90f,  1826, 35, 17, 4, kRN, 1, 0, "Br", "Brom",          "Balard / Lowig"},
    {83.80f,  1898, 36, 18, 4, kNG, 2, 0, "Kr", "Krypton",       "William Ramsay"},
    // Periode 5
    {85.47f,  1861, 37,  1, 5, kAM, 0, 0, "Rb", "Rubidium",      "Bunsen / Kirchhoff"},
    {87.62f,  1790, 38,  2, 5, kAE, 0, 0, "Sr", "Strontium",     "William Cruickshank"},
    {88.91f,  1794, 39,  3, 5, kTM, 0, 0, "Y",  "Yttrium",       "Johan Gadolin"},
    {91.22f,  1789, 40,  4, 5, kTM, 0, 0, "Zr", "Zirconium",     "Martin H. Klaproth"},
    {92.91f,  1801, 41,  5, 5, kTM, 0, 0, "Nb", "Niob",          "Charles Hatchett"},
    {95.96f,  1781, 42,  6, 5, kTM, 0, 0, "Mo", "Molybdän",      "Carl W. Scheele"},
    {98.00f,  1937, 43,  7, 5, kTM, 0, 0, "Tc", "Technetium",    "Perrier / Segre"},
    {101.1f,  1844, 44,  8, 5, kTM, 0, 0, "Ru", "Ruthenium",     "Karl K. Klaus"},
    {102.9f,  1803, 45,  9, 5, kTM, 0, 0, "Rh", "Rhodium",       "W. H. Wollaston"},
    {106.4f,  1803, 46, 10, 5, kTM, 0, 0, "Pd", "Palladium",     "W. H. Wollaston"},
    {107.9f,     0, 47, 11, 5, kTM, 0, 0, "Ag", "Silber",        "Seit der Antike"},
    {112.4f,  1817, 48, 12, 5, kTM, 0, 0, "Cd", "Cadmium",       "Friedrich Stromeyer"},
    {114.8f,  1863, 49, 13, 5, kPT, 0, 0, "In", "Indium",        "Reich / Richter"},
    {118.7f,     0, 50, 14, 5, kPT, 0, 0, "Sn", "Zinn",          "Seit der Antike"},
    {121.8f,     0, 51, 15, 5, kML, 0, 0, "Sb", "Antimon",       "Seit der Antike"},
    {127.6f,  1782, 52, 16, 5, kML, 0, 0, "Te", "Tellur",        "Franz J. Muller"},
    {126.9f,  1811, 53, 17, 5, kRN, 0, 0, "I",  "Iod",           "Bernard Courtois"},
    {131.3f,  1898, 54, 18, 5, kNG, 2, 0, "Xe", "Xenon",         "William Ramsay"},
    // Periode 6
    {132.9f,  1860, 55,  1, 6, kAM, 0, 0, "Cs", "Caesium",       "Bunsen / Kirchhoff"},
    {137.3f,  1808, 56,  2, 6, kAE, 0, 0, "Ba", "Barium",        "Humphry Davy"},
    {138.9f,  1839, 57,  3, 6, kLN, 0, 0, "La", "Lanthan",       "Carl G. Mosander"},
    {178.5f,  1923, 72,  4, 6, kTM, 0, 0, "Hf", "Hafnium",       "Coster / de Hevesy"},
    {180.9f,  1802, 73,  5, 6, kTM, 0, 0, "Ta", "Tantal",        "Anders G. Ekeberg"},
    {183.8f,  1783, 74,  6, 6, kTM, 0, 0, "W",  "Wolfram",       "J. J. / F. Elhuyar"},
    {186.2f,  1925, 75,  7, 6, kTM, 0, 0, "Re", "Rhenium",       "Noddack / Berg"},
    {190.2f,  1803, 76,  8, 6, kTM, 0, 0, "Os", "Osmium",        "Smithson Tennant"},
    {192.2f,  1803, 77,  9, 6, kTM, 0, 0, "Ir", "Iridium",       "Smithson Tennant"},
    {195.1f,  1735, 78, 10, 6, kTM, 0, 0, "Pt", "Platin",        "Antonio de Ulloa"},
    {197.0f,     0, 79, 11, 6, kTM, 0, 0, "Au", "Gold",          "Seit der Antike"},
    {200.6f,     0, 80, 12, 6, kTM, 1, 0, "Hg", "Quecksilber",   "Seit der Antike"},
    {204.4f,  1861, 81, 13, 6, kPT, 0, 0, "Tl", "Thallium",      "William Crookes"},
    {207.2f,     0, 82, 14, 6, kPT, 0, 0, "Pb", "Blei",          "Seit der Antike"},
    {209.0f,  1753, 83, 15, 6, kPT, 0, 0, "Bi", "Bismut",        "C. F. Geoffroy"},
    {209.0f,  1898, 84, 16, 6, kPT, 0, 0, "Po", "Polonium",      "Marie Curie"},
    {210.0f,  1940, 85, 17, 6, kML, 0, 0, "At", "Astat",         "Corson / MacKenzie"},
    {222.0f,  1900, 86, 18, 6, kNG, 2, 0, "Rn", "Radon",         "Friedrich E. Dorn"},
    // Periode 7
    {223.0f,  1939,  87,  1, 7, kAM, 0, 0, "Fr", "Francium",     "Marguerite Perey"},
    {226.0f,  1898,  88,  2, 7, kAE, 0, 0, "Ra", "Radium",       "Pierre / Marie Curie"},
    {227.0f,  1899,  89,  3, 7, kAC, 0, 0, "Ac", "Actinium",     "Andre-L. Debierne"},
    {267.0f,  1964, 104,  4, 7, kUK, 0, 0, "Rf", "Rutherfordium","Dubna / Berkeley"},
    {268.0f,  1967, 105,  5, 7, kUK, 0, 0, "Db", "Dubnium",      "Dubna / Berkeley"},
    {271.0f,  1974, 106,  6, 7, kUK, 0, 0, "Sg", "Seaborgium",   "Berkeley-Team"},
    {272.0f,  1981, 107,  7, 7, kUK, 0, 0, "Bh", "Bohrium",      "GSI Darmstadt"},
    {270.0f,  1984, 108,  8, 7, kUK, 0, 0, "Hs", "Hassium",      "GSI Darmstadt"},
    {278.0f,  1982, 109,  9, 7, kUK, 0, 0, "Mt", "Meitnerium",   "GSI Darmstadt"},
    {281.0f,  1994, 110, 10, 7, kUK, 0, 0, "Ds", "Darmstadtium", "GSI Darmstadt"},
    {282.0f,  1994, 111, 11, 7, kUK, 0, 0, "Rg", "Röntgenium",  "GSI Darmstadt"},
    {285.0f,  1996, 112, 12, 7, kUK, 0, 0, "Cn", "Copernicium",  "GSI Darmstadt"},
    {286.0f,  2004, 113, 13, 7, kUK, 0, 0, "Nh", "Nihonium",     "RIKEN Japan"},
    {289.0f,  1999, 114, 14, 7, kUK, 0, 0, "Fl", "Flerovium",    "Dubna-Team"},
    {290.0f,  2003, 115, 15, 7, kUK, 0, 0, "Mc", "Moscovium",    "Dubna-Team"},
    {293.0f,  2000, 116, 16, 7, kUK, 0, 0, "Lv", "Livermorium",  "Dubna-Team"},
    {294.0f,  2010, 117, 17, 7, kUK, 0, 0, "Ts", "Tennessin",    "Dubna / Oak Ridge"},
    {294.0f,  2002, 118, 18, 7, kNG, 2, 0, "Og", "Oganesson",    "Dubna-Team"},
    // Lanthanoid-Zeile (row=8, Spalten 4-17)
    {140.1f,  1803,  58,  4, 8, kLN, 0, 0, "Ce", "Cer",          "Berzelius / Hisinger"},
    {140.9f,  1885,  59,  5, 8, kLN, 0, 0, "Pr", "Praseodym",    "C. A. von Welsbach"},
    {144.2f,  1885,  60,  6, 8, kLN, 0, 0, "Nd", "Neodym",       "C. A. von Welsbach"},
    {145.0f,  1945,  61,  7, 8, kLN, 0, 0, "Pm", "Promethium",   "Marinsky / Glendenin"},
    {150.4f,  1879,  62,  8, 8, kLN, 0, 0, "Sm", "Samarium",     "P. E. de Boisbaudran"},
    {152.0f,  1901,  63,  9, 8, kLN, 0, 0, "Eu", "Europium",     "Eugene A. Demarcay"},
    {157.3f,  1880,  64, 10, 8, kLN, 0, 0, "Gd", "Gadolinium",   "J. C. G. de Marignac"},
    {158.9f,  1843,  65, 11, 8, kLN, 0, 0, "Tb", "Terbium",      "Carl G. Mosander"},
    {162.5f,  1886,  66, 12, 8, kLN, 0, 0, "Dy", "Dysprosium",   "P. E. de Boisbaudran"},
    {164.9f,  1878,  67, 13, 8, kLN, 0, 0, "Ho", "Holmium",      "Cleve / Soret"},
    {167.3f,  1843,  68, 14, 8, kLN, 0, 0, "Er", "Erbium",       "Carl G. Mosander"},
    {168.9f,  1879,  69, 15, 8, kLN, 0, 0, "Tm", "Thulium",      "Per T. Cleve"},
    {173.0f,  1878,  70, 16, 8, kLN, 0, 0, "Yb", "Ytterbium",    "J. C. G. de Marignac"},
    {175.0f,  1907,  71, 17, 8, kLN, 0, 0, "Lu", "Lutetium",     "Georges Urbain"},
    // Actinoid-Zeile (row=9, Spalten 4-17)
    {232.0f,  1829,  90,  4, 9, kAC, 0, 0, "Th", "Thorium",      "Jons J. Berzelius"},
    {231.0f,  1913,  91,  5, 9, kAC, 0, 0, "Pa", "Protactinium", "Fajans / Gohring"},
    {238.0f,  1789,  92,  6, 9, kAC, 0, 0, "U",  "Uran",         "Martin H. Klaproth"},
    {237.0f,  1940,  93,  7, 9, kAC, 0, 0, "Np", "Neptunium",    "McMillan / Abelson"},
    {244.0f,  1940,  94,  8, 9, kAC, 0, 0, "Pu", "Plutonium",    "Seaborg / McMillan"},
    {243.0f,  1944,  95,  9, 9, kAC, 0, 0, "Am", "Americium",    "Glenn T. Seaborg"},
    {247.0f,  1944,  96, 10, 9, kAC, 0, 0, "Cm", "Curium",       "Seaborg / James"},
    {247.0f,  1949,  97, 11, 9, kAC, 0, 0, "Bk", "Berkelium",    "Thompson / Ghiorso"},
    {251.0f,  1950,  98, 12, 9, kAC, 0, 0, "Cf", "Californium",  "Thompson / Ghiorso"},
    {252.0f,  1952,  99, 13, 9, kAC, 0, 0, "Es", "Einsteinium",  "Ghiorso et al."},
    {257.0f,  1952, 100, 14, 9, kAC, 0, 0, "Fm", "Fermium",      "Ghiorso et al."},
    {258.0f,  1955, 101, 15, 9, kAC, 0, 0, "Md", "Mendelevium",  "Ghiorso et al."},
    {259.0f,  1958, 102, 16, 9, kAC, 0, 0, "No", "Nobelium",     "Ghiorso et al."},
    {262.0f,  1961, 103, 17, 9, kAC, 0, 0, "Lr", "Lawrencium",   "Ghiorso et al."},
};


// ===================================================================
// KONSTRUKTOR / LIFECYCLE
// ===================================================================

/**
 * @brief Konstruktor - setzt alle Member auf Standardwerte.
 */
WorkshopTool_PeriodicTable::WorkshopTool_PeriodicTable()
    : m_originX(0), m_originY(0), m_contentW(0), m_contentH(0),
      m_layoutValid(false), m_tableX(0), m_tableY(0),
      m_popupX(0), m_popupY(0),
      m_selectedElem(-1), m_wasPressed(false), m_pressedOnOk(false),
      m_tableDirty(true), m_popupDirty(false),
      m_lastTouchX(0), m_lastTouchY(0)
{
}

/**
 * @brief Setzt Zustand zurück und erzwingt vollständiges Neuzeichnen.
 */
void WorkshopTool_PeriodicTable::onEnter(LGFX *tft)
{
    m_tft          = tft;
    m_selectedElem = -1;
    m_wasPressed   = false;
    m_pressedOnOk  = false;
    m_tableDirty   = true;
    m_popupDirty   = false;
    m_layoutValid  = false;
    m_lastTouchX   = 0;
    m_lastTouchY   = 0;
    initLookup();
}

/**
 * @brief Schließt ein offenes Popup beim Verlassen.
 */
void WorkshopTool_PeriodicTable::onExit()
{
    m_selectedElem = -1;
    m_wasPressed   = false;
    m_pressedOnOk  = false;
    m_popupDirty   = false;
}

// ===================================================================
// LAYOUT
// ===================================================================

/**
 * @brief Berechnet und cached Tabellen- und Popup-Ursprung.
 */
void WorkshopTool_PeriodicTable::computeLayout(int w, int h)
{
    if (m_layoutValid && m_contentW == w && m_contentH == h)
        return;

    m_contentW = w;
    m_contentH = h;

    // Tabelle linksbündig, vertikal nah am oberen Rand
    m_tableX = 0;
    m_tableY = 3;

    // Popup in der Mitte des Inhaltsbereichs zentrieren, aber nie negativ beginnen
    m_popupX = (w > POPUP_W) ? ((w - POPUP_W) / 2) : 0;
    m_popupY = (h > POPUP_H) ? ((h - POPUP_H) / 2) : 0;

    m_layoutValid = true;
}

// ===================================================================
// ZEICHNUNG
// ===================================================================

/**
 * @brief Zeichnet Tabelle und ggf. Detailpopup in den Work-Sprite.
 */
void WorkshopTool_PeriodicTable::draw(Gfx_Sprite *sprite,
                                      int originX, int originY,
                                      int w, int h)
{
    // Geometrieänderung erzwingt vollständiges Neuzeichnen
    const bool geometryChanged = (!m_layoutValid     ||
                                  m_originX != originX ||
                                  m_originY != originY  ||
                                  m_contentW != w       ||
                                  m_contentH != h);

    m_originX = originX;
    m_originY = originY;

    if (geometryChanged)
    {
        m_tableDirty = true;
        if (m_selectedElem >= 0)
            m_popupDirty = true;
    }

    computeLayout(w, h);

    // Tabelle nur bei Bedarf neu zeichnen (nach Öffnen/Schließen oder erstem Aufruf)
    if (m_tableDirty)
    {
        drawTable(sprite);
        drawLegend(sprite);
        m_tableDirty = false;
    }

    // Popup nur bei Bedarf neu zeichnen (Öffnen, Elementwechsel, Layoutänderung)
    if (m_selectedElem >= 0 && m_popupDirty)
    {
        drawPopup(sprite);
        m_popupDirty = false;
    }
}

/**
 * @brief Zeichnet die Kategorie-Farblegende unterhalb der Tabelle.
 */
void WorkshopTool_PeriodicTable::drawLegend(Gfx_Sprite *spr)
{
    static const WorkshopKey s_legendNameKeys[] = {
        WorkshopKey::WORKSHOP_PERIODIC_TABLE_CAT_ALKALI_SHORT,
        WorkshopKey::WORKSHOP_PERIODIC_TABLE_CAT_ALKALINE_EARTH_SHORT,
        WorkshopKey::WORKSHOP_PERIODIC_TABLE_CAT_TRANSITION_SHORT,
        WorkshopKey::WORKSHOP_PERIODIC_TABLE_CAT_POST_TRANSITION_SHORT,
        WorkshopKey::WORKSHOP_PERIODIC_TABLE_CAT_METALLOID_SHORT,
        WorkshopKey::WORKSHOP_PERIODIC_TABLE_CAT_NONMETAL_SHORT,
        WorkshopKey::WORKSHOP_PERIODIC_TABLE_CAT_NOBLE_GAS_SHORT,
        WorkshopKey::WORKSHOP_PERIODIC_TABLE_CAT_LANTHANIDE_SHORT,
        WorkshopKey::WORKSHOP_PERIODIC_TABLE_CAT_ACTINIDE_SHORT,
        WorkshopKey::WORKSHOP_PERIODIC_TABLE_CAT_UNKNOWN_SHORT,
    };
    static constexpr int NUM_CATS = 10;

    const int legendH         = 2 * LEGEND_ROW_H;
    const int legendFromTable = m_originY + m_tableY + TOTAL_TABLE_H + LEGEND_TOP_GAP;

    // Oberkante so, dass die Legende 10 px über dem unteren Rand endet
    const int legendFromBot   = m_originY + m_contentH - 10 - legendH;

    // Möglichst unten platzieren, aber nicht über die Tabelle schieben
    const int legendY = (legendFromBot >= legendFromTable)
                        ? legendFromBot
                        : legendFromTable;

    const int colW = m_contentW / LEGEND_COLS;

    spr->loadFont(ComfortaaBold10);
    spr->setTextDatum(TL_DATUM);

    char legBuf[24];
    for (int i = 0; i < NUM_CATS; ++i)
    {
        const int col = i % LEGEND_COLS;
        const int row = i / LEGEND_COLS;
        const int ix  = m_originX + col * colW;
        const int iy  = legendY + row * LEGEND_ROW_H;

        const uint16_t catColor  = s_catColors[i];
        const uint16_t fillColor = scaleColor(catColor, 0.55f);

        spr->fillRect(ix, iy, LEGEND_SQ, LEGEND_SQ, catColor);
        spr->fillRect(ix + 1, iy + 1, LEGEND_SQ - 2, LEGEND_SQ - 2, fillColor);

        I18n::format(legBuf, sizeof(legBuf), s_legendNameKeys[i]);
        spr->setTextColor(C_LEGEND_TXT, C_WORK_BG);
        spr->drawString(legBuf, ix + LEGEND_SQ + LEGEND_TXT_GAP, iy);
    }

    spr->unloadFont();
}

/**
 * @brief Zeichnet alle Elementzellen in den Sprite.
 */
void WorkshopTool_PeriodicTable::drawTable(Gfx_Sprite *spr)
{
    // Hintergrund löschen
    spr->fillRect(m_originX, m_originY, m_contentW, m_contentH, C_WORK_BG);

    // Pass 1: Alle Zellen füllen (keine Font-Operationen)
    for (int i = 0; i < 118; ++i)
    {
        int sx, sy;
        getCellSpritePos(s_elements[i], sx, sy);
        const uint16_t catColor  = getCategoryColor(s_elements[i]);
        const uint16_t fillColor = scaleColor(catColor, 0.55f);
        spr->fillRect(sx, sy, CELL_W, CELL_H, catColor);
        spr->fillRect(sx + 1, sy + 1, CELL_W - 2, CELL_H - 2, fillColor);
    }

    // Pass 2: Ordnungszahlen - Font einmalig laden
    spr->loadFont(ComfortaaBold10);
    spr->setTextDatum(TL_DATUM);
    for (int i = 0; i < 118; ++i)
    {
        int sx, sy;
        getCellSpritePos(s_elements[i], sx, sy);
        const uint16_t fillColor = scaleColor(getCategoryColor(s_elements[i]), 0.55f);
        char numBuf[5];
        snprintf(numBuf, sizeof(numBuf), "%d", (int)s_elements[i].number);
        spr->setTextColor(C_LEGEND_TXT, fillColor);
        spr->drawString(numBuf, sx + 2, sy + 2);
    }
    spr->unloadFont();

    // Pass 3: Elementsymbole - Font einmalig laden
    spr->loadFont(ComfortaaBold14);
    spr->setTextDatum(TC_DATUM);
    for (int i = 0; i < 118; ++i)
    {
        int sx, sy;
        getCellSpritePos(s_elements[i], sx, sy);
        const uint16_t fillColor = scaleColor(getCategoryColor(s_elements[i]), 0.55f);
        spr->setTextColor(0xFFFF, fillColor);
        spr->drawString(s_elements[i].symbol, sx + CELL_W / 2, sy + 13);
    }
    spr->unloadFont();
}

/**
 * @brief Zeichnet das Element-Detailpopup im MessageBox-Stil als Overlay.
 */
void WorkshopTool_PeriodicTable::drawPopup(Gfx_Sprite *spr)
{
    if (m_selectedElem < 0 || m_selectedElem >= 118)
        return;

    const ElementData &e     = s_elements[m_selectedElem];
    const uint16_t catColor  = getCategoryColor(e);
    const uint16_t fillColor = scaleColor(catColor, 0.55f);

    const int px = m_originX + m_popupX;
    const int py = m_originY + m_popupY;

    // ===================================================================
    // Rahmen + Hintergrund (wie MessageBox::drawFrame)
    // ===================================================================
    spr->fillRect(px, py, POPUP_W, POPUP_H, C_POPUP_BORDER);
    spr->fillSmoothRoundRect(px, py, POPUP_W, POPUP_H, POPUP_RADIUS, C_POPUP_BORDER);
    spr->fillRect(px + 1, py + 1, POPUP_W - 2, POPUP_H - 2, C_POPUP_BG);
    spr->fillSmoothRoundRect(px + 1, py + 1, POPUP_W - 2, POPUP_H - 2, POPUP_RADIUS, C_POPUP_BG);

    // ===================================================================
    // Header: Symbol-Icon links, Elementname rechts daneben
    // ===================================================================
    const int symX = px + POPUP_PAD;
    const int symY = py + (POPUP_SEP_Y - POPUP_HDR_SYM_H) / 2;  // vertikal zentriert

    // Symbol-Kästchen (exakt wie Tabellenzelle: 29×30, 1px catColor-Rahmen)
    spr->fillRect(symX, symY, POPUP_HDR_SYM, POPUP_HDR_SYM_H, catColor);
    spr->fillRect(symX + 1, symY + 1, POPUP_HDR_SYM - 2, POPUP_HDR_SYM_H - 2, fillColor);

    // Ordnungszahl oben links (CB10, wie drawCell)
    char numBuf[6];
    snprintf(numBuf, sizeof(numBuf), "%d", (int)e.number);
    spr->loadFont(ComfortaaBold10);
    spr->setTextDatum(TL_DATUM);
    spr->setTextColor(C_LEGEND_TXT, fillColor);
    spr->drawString(numBuf, symX + 2, symY + 2);
    spr->unloadFont();

    // Symbol zentriert (CB14, wie drawCell: TC_DATUM, sy+13)
    spr->loadFont(ComfortaaBold14);
    spr->setTextDatum(TC_DATUM);
    spr->setTextColor(0xFFFF, fillColor);
    spr->drawString(e.symbol, symX + POPUP_HDR_SYM / 2, symY + 13);
    spr->unloadFont();

    // Elementname rechts neben dem Icon (CB18, vertikal zentriert mit Icon)
    const int nameX    = symX + POPUP_HDR_SYM + 10;
    const int nameY    = symY + (POPUP_HDR_SYM_H - 18) / 2;
    const int nameMaxW = px + POPUP_W - POPUP_PAD - nameX;
    spr->loadFont(ComfortaaBold18);
    spr->setTextDatum(TL_DATUM);
    drawTextEllipsized(spr, e.nameDe, nameX, nameY, nameMaxW, C_POPUP_TITLE, C_POPUP_BG);
    spr->unloadFont();

    // ===================================================================
    // Trennlinie (wie MessageBox::drawSeparator)
    // ===================================================================
    spr->fillRect(px + POPUP_PAD, py + POPUP_SEP_Y, POPUP_W - 2 * POPUP_PAD, 2, C_POPUP_SEP);

    // ===================================================================
    // Detailzeilen - alle linksbündig, Label gedämpft + ":", Wert hell
    // ===================================================================

    // Feste Label-Spalte POPUP_LABEL_W, Werte beginnen dahinter
    const int lx        = px + POPUP_PAD;         // Label-X
    const int vx        = lx + POPUP_LABEL_W;     // Wert-X
    const int valueMaxW = px + POPUP_W - POPUP_PAD - vx; // Maximale Wert-Breite (laufzeitberechnet)
    int       ly        = py + POPUP_TXT_Y;       // aktuelle Zeilen-Y

    char valueBuf[32];  ///< Lokaler Puffer für übersetzte Werttexte zu Kategorie und Zustand

    spr->loadFont(ComfortaaBold14);
    spr->setTextDatum(TL_DATUM);

    // Zeile 1: Kategorie
    spr->setTextColor(C_POPUP_DIM, C_POPUP_BG);
    spr->drawString(I18n::str(WorkshopKey::WORKSHOP_PERIODIC_TABLE_LABEL_CATEGORY), lx, ly);
    I18n::format(valueBuf, sizeof(valueBuf), s_catNameKeys[getSafeCategoryIndex(e)]);
    drawTextEllipsized(spr, valueBuf, vx, ly, valueMaxW, C_POPUP_TXT, C_POPUP_BG);
    ly += POPUP_LINE_H;

    // Zeile 2: Atommasse
    char massBuf[16];
    snprintf(massBuf, sizeof(massBuf), "%.3f u", e.mass);
    spr->setTextColor(C_POPUP_DIM, C_POPUP_BG);
    spr->drawString(I18n::str(WorkshopKey::WORKSHOP_PERIODIC_TABLE_LABEL_MASS), lx, ly);
    drawTextEllipsized(spr, massBuf, vx, ly, valueMaxW, C_POPUP_TXT, C_POPUP_BG);
    ly += POPUP_LINE_H;

    // Zeile 3: Gruppe / Periode
    char groupBuf[20];
    if (e.row <= 7)
        snprintf(groupBuf, sizeof(groupBuf), "%d / %d", (int)e.col, (int)e.row);
    else
        snprintf(groupBuf, sizeof(groupBuf), "%d (f-Block)", (e.row == 8) ? 6 : 7);
    spr->setTextColor(C_POPUP_DIM, C_POPUP_BG);
    spr->drawString(I18n::str(WorkshopKey::WORKSHOP_PERIODIC_TABLE_LABEL_GROUP_PERIOD), lx, ly);
    drawTextEllipsized(spr, groupBuf, vx, ly, valueMaxW, C_POPUP_TXT, C_POPUP_BG);
    ly += POPUP_LINE_H;

    // Zeile 4: Aggregatzustand
    spr->setTextColor(C_POPUP_DIM, C_POPUP_BG);
    spr->drawString(I18n::str(WorkshopKey::WORKSHOP_PERIODIC_TABLE_LABEL_STATE), lx, ly);
    const WorkshopKey stateKey = (e.state < STATE_COUNT)
        ? s_stateNameKeys[e.state]
        : WorkshopKey::WORKSHOP_PERIODIC_TABLE_STATE_UNKNOWN;
    I18n::format(valueBuf, sizeof(valueBuf), stateKey);
    drawTextEllipsized(spr, valueBuf, vx, ly, valueMaxW, C_POPUP_TXT, C_POPUP_BG);
    ly += POPUP_LINE_H;

    // Zeile 5: Entdeckungsjahr (0=Antike, -1=unbekannt, sonst Jahreszahl)
    char yearBuf[20];
    if (e.year == 0)
        I18n::format(yearBuf, sizeof(yearBuf), WorkshopKey::WORKSHOP_PERIODIC_TABLE_YEAR_ANCIENT);
    else if (e.year < 0)
        I18n::format(yearBuf, sizeof(yearBuf), WorkshopKey::WORKSHOP_PERIODIC_TABLE_YEAR_UNKNOWN);
    else
        snprintf(yearBuf, sizeof(yearBuf), "%d", (int)e.year);
    spr->setTextColor(C_POPUP_DIM, C_POPUP_BG);
    spr->drawString(I18n::str(WorkshopKey::WORKSHOP_PERIODIC_TABLE_LABEL_DISCOVERED), lx, ly);
    drawTextEllipsized(spr, yearBuf, vx, ly, valueMaxW, C_POPUP_TXT, C_POPUP_BG);
    ly += POPUP_LINE_H;

    // Zeile 6: Entdecker
    spr->setTextColor(C_POPUP_DIM, C_POPUP_BG);
    spr->drawString(I18n::str(WorkshopKey::WORKSHOP_PERIODIC_TABLE_LABEL_DISCOVERER), lx, ly);
    drawTextEllipsized(spr, e.discoverer, vx, ly, valueMaxW, C_POPUP_TXT, C_POPUP_BG);

    spr->unloadFont();

    // ===================================================================
    // OK-Button zentriert am unteren Rand
    // ===================================================================
    const int btnCX = px + POPUP_W / 2;
    const int btnCY = py + POPUP_H - POPUP_PAD - POPUP_BTN_H / 2;
    // Der Knopf läuft über dieselbe Klasse wie alle anderen, damit er auch gleich aussieht. Er zeichnet in das Arbeits-Sprite, deshalb bekommt er dessen Lage auf dem Schirm mit.
    m_okButton.initButton(m_tft, WorkshopCfg::WORK_X + btnCX, WorkshopCfg::WORK_Y + btnCY,
                          POPUP_BTN_W, POPUP_BTN_H, POPUP_RADIUS,
                          Theme::ColorButtonBorder, Theme::ColorButtonFill, Theme::ColorButtonTextOk,
                          reinterpret_cast<PGM_P>(I18n::str(WorkshopKey::WORKSHOP_DIALOG_OK)), nullptr);
    m_okButton.setFont(LeagueGothicCond34);
    m_okButton.setDrawTarget(spr, WorkshopCfg::WORK_X, WorkshopCfg::WORK_Y);
    m_okButton.drawButton(false);
}

// ===================================================================
// HILFSMETHODEN
// ===================================================================

/**
 * @brief Berechnet Sprite-relative Position einer Elementzelle.
 */
void WorkshopTool_PeriodicTable::getCellSpritePos(const ElementData &e,
                                                  int &outX, int &outY) const
{
    const int cellStep = CELL_W + CELL_GAP;

    if (e.row <= 7)
    {
        // Haupttabelle: Zeile 1..7
        outX = m_originX + m_tableX + (e.col - 1) * cellStep;
        outY = m_originY + m_tableY + (e.row - 1) * (CELL_H + CELL_GAP);
    }
    else
    {
        // Lanthanoid (row=8) oder Actinoid (row=9)
        const int lantRow  = e.row - 8;  // 0=Lanthanoid, 1=Actinoid
        const int lantYOff = TABLE_MAIN_H + LANT_GAP;
        outX = m_originX + m_tableX + (e.col - 1) * cellStep;
        outY = m_originY + m_tableY + lantYOff + lantRow * (CELL_H + CELL_GAP);
    }
}

/**
 * @brief Treffertest: liefert Element-Index an content-relativer Position.
 */
int WorkshopTool_PeriodicTable::hitTestCell(int cx, int cy) const
{
    const int relX    = cx - m_tableX;
    const int relY    = cy - m_tableY;
    const int stepX   = CELL_W + CELL_GAP;
    const int stepY   = CELL_H + CELL_GAP;
    const int lantOff = TABLE_MAIN_H + LANT_GAP;

    if (relX < 0)
        return -1;

    const int c = relX / stepX;
    if (c < 0 || c >= 18 || (relX % stepX) >= CELL_W)
        return -1;

    int r;
    if (relY >= 0 && relY < TABLE_MAIN_H)
    {
        // Haupttabelle
        r = relY / stepY;
        if ((relY % stepY) >= CELL_H)
            return -1;
    }
    else if (relY >= lantOff && relY < lantOff + LANT_H)
    {
        // Lanthanoid / Actinoid-Zeilen
        const int lr = (relY - lantOff) / stepY;
        if (((relY - lantOff) % stepY) >= CELL_H)
            return -1;
        r = 7 + lr;  // row-1: 7=Lanthanoid, 8=Actinoid
    }
    else
    {
        return -1;
    }

    if (r < 0 || r >= 9)
        return -1;

    return static_cast<int>(s_cellLookup[r][c]);  // -1 wenn Zelle leer
}

/**
 * @brief Baut die Lookup-Tabelle einmalig auf.
 */
void WorkshopTool_PeriodicTable::initLookup()
{
    if (s_lookupBuilt)
        return;

    for (int r = 0; r < 9; ++r)
        for (int c = 0; c < 18; ++c)
            s_cellLookup[r][c] = -1;

    for (int i = 0; i < 118; ++i)
    {
        const int r = s_elements[i].row - 1;   // 0-basiert
        const int c = s_elements[i].col - 1;   // 0-basiert
        if (r >= 0 && r < 9 && c >= 0 && c < 18)
            s_cellLookup[r][c] = static_cast<int8_t>(i);
    }

    s_lookupBuilt = true;
}

/**
 * @brief Prüft ob content-relative Position innerhalb des Popups liegt.
 */
bool WorkshopTool_PeriodicTable::hitTestPopup(int cx, int cy) const
{
    return (cx >= m_popupX && cx < m_popupX + POPUP_W &&
            cy >= m_popupY && cy < m_popupY + POPUP_H);
}

/**
 * @brief Prüft ob content-relative Position den OK-Button trifft.
 */
bool WorkshopTool_PeriodicTable::hitTestOkButton(int cx, int cy) const
{
    // Die Trefferprüfung verwendet die Geometrie der gezeichneten Popup-Schaltfläche
    const int btnCX = m_popupX + POPUP_W / 2;
    const int btnCY = m_popupY + POPUP_H - POPUP_PAD - POPUP_BTN_H / 2;
    const int btnX  = btnCX - POPUP_BTN_W / 2;
    const int btnY  = btnCY - POPUP_BTN_H / 2;
    return (cx >= btnX && cx < btnX + POPUP_BTN_W &&
            cy >= btnY && cy < btnY + POPUP_BTN_H);
}

// ===================================================================
// TOUCH-HANDLING
// ===================================================================

/**
 * @brief Verarbeitet Touch-Events für Elementauswahl und Popup-Steuerung.
 */
bool WorkshopTool_PeriodicTable::handleTouch(int32_t tx, int32_t ty, bool touched)
{
    if (touched)
    {
        m_lastTouchX = tx;
        m_lastTouchY = ty;

        if (!m_wasPressed)
        {
            m_wasPressed = true;
            m_pressedOnOk = false;

            const int cx = static_cast<int>(tx) - WorkshopCfg::WORK_X - m_originX;
            const int cy = static_cast<int>(ty) - WorkshopCfg::WORK_Y - m_originY;

            if (m_selectedElem >= 0)
            {
                if (hitTestPopup(cx, cy))
                {
                    // Touch-down im Popup: nur OK-Button registrieren
                    m_pressedOnOk = hitTestOkButton(cx, cy);
                }
                else
                {
                    // Touch-down außerhalb Popup: sofort schließen
                    m_selectedElem = -1;
                    m_tableDirty   = true;
                    return true;
                }
                return false;
            }

            // Kein Popup: Element-Treffertest beim Touch-down -> sofort öffnen
            const int hit = hitTestCell(cx, cy);
            if (hit >= 0)
            {
                m_selectedElem = hit;
                m_popupDirty   = true;
                return true;
            }
        }
        return false;
    }

    // Touch-up
    if (!m_wasPressed)
        return false;

    m_wasPressed = false;

    // OK-Button: nur schließen wenn Finger beim Loslassen noch auf dem Button lag
    if (m_selectedElem >= 0 && m_pressedOnOk)
    {
        const int cx = static_cast<int>(m_lastTouchX) - WorkshopCfg::WORK_X - m_originX;
        const int cy = static_cast<int>(m_lastTouchY) - WorkshopCfg::WORK_Y - m_originY;
        if (hitTestOkButton(cx, cy))
        {
            m_pressedOnOk  = false;
            m_selectedElem = -1;
            m_tableDirty   = true;
            return true;
        }
    }

    m_pressedOnOk = false;
    return false;
}

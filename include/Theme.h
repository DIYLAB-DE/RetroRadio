#ifndef THEME_H
#define THEME_H

#include <cstdint> // für uint16_t

namespace Theme
{
    constexpr uint16_t ColorDialogBorder = 0x24F2;    ///< Rahmenfarbe der Dialoge
    constexpr uint16_t ColorMainBackGround = 0x0000;  ///< Hintergrundfarbe
    constexpr uint16_t ColorDialogSeparator = 0xd206; ///< Farbe für Trennlinien
    constexpr uint16_t ColorMainText = 0xD69A;        ///< Farbe für Titel
    constexpr uint16_t ColorTitle = 0xEC84;           ///< Farbe für Titel

    constexpr uint16_t ColorButtonBorder = 0x39E8;     ///< Farbe Button Umrandung
    constexpr uint16_t ColorButtonFill = 0x0000;       ///< Farbe Button Füllung
    constexpr uint16_t ColorButtonText = 0x7BEF;       ///< Farbe Button Text
    constexpr uint16_t ColorButtonTextOk = 0xBDF7;     ///< Farbe Button Text für "Ok"
    constexpr uint16_t ColorButtonTextCancel = 0x7BEF; ///< Farbe Button Text für "Abbruch"

    constexpr uint16_t ColorSelectedRow = 0x328a; ///< Ausgewählte Zeile

    constexpr uint16_t ColorGreen = 0x3666;  ///< grün
    constexpr uint16_t ColorCyan = 0x8dfa;   ///< cyan
    constexpr uint16_t ColorMarker = 0x5370; ///< dark cyan
    constexpr uint16_t ColorAmber = 0xed86;  ///< amber

    constexpr uint16_t ColorRetroRed = 0xd206; ///< retro red

    // =================================================================
    // MainScreen-Farbpalette
    // =================================================================

    /**
     * @brief Farbschema für den MainScreen (Uhr, Status, Datum, Buttons)
     *
     * Alle Farben im RGB565-Format. Um das Theme zu wechseln, einfach die Referenz `ActiveMainScreen` am Ende umbiegen.
     */
    struct MainScreenColors
    {
        // Analoge Uhr
        uint16_t clockMarks;        ///< Stunden-Markierungen (12, 3, 6, 9)
        uint16_t clockMinorMarks;   ///< Minuten- und sekundäre Stunden-Markierungen
        uint16_t clockHands;        ///< Stunden- und Minutenzeiger
        uint16_t clockSecondHand;   ///< Sekundenzeiger
        uint16_t clockCenter;       ///< Mittelpunkt / Achse
        uint16_t clockCenterAccent; ///< Sekundenzeiger-Achse (innerer Kreis)

        // Statusanzeigen in der Uhr
        uint16_t statusNtpActive;  ///< NTP-Sync aktiv (11-Uhr-Marker)
        uint16_t statusDstActive;  ///< Sommerzeit aktiv (1-Uhr-Marker)
        uint16_t statusWifiActive; ///< WiFi verbunden (Signalbalken)
        uint16_t statusOff;        ///< Inaktiver Status (alle Indikatoren)
        uint16_t statusLabels;     ///< NTP/DST-Beschriftung

        // Datum
        uint16_t dateText; ///< Datumsanzeige unter der Uhr

        // SSID-Anzeige
        uint16_t ssidText; ///< SSID-Text unter den WiFi-Balken
        uint16_t ipText;   ///< IP-Adresse unter dem SSID-Text

        // Buttons
        uint16_t buttonBorder; ///< Button-Rahmen
        uint16_t buttonFill;   ///< Button-Füllung
        uint16_t buttonText;   ///< Button-Text (z.B. Kalender-Tag)

        // Individuell konfigurierbare Icon-Farben
        uint16_t iconTints[12]; ///< Tintfarbe je Icon (Reihenfolge: siehe IconIndex)
    };

    /// Index-Zuordnung für iconTints[] entsprechend der Reihenfolge in initButtons()
    enum IconIndex : uint8_t
    {
        IC_RADIO = 0,     ///< Radio
        IC_PLAYER = 1,    ///< SD-Player
        IC_CALENDAR = 2,  ///< Kalender
        IC_TODO = 3,      ///< ToDo
        IC_CONTACTS = 4,  ///< Kontakte
        IC_WEATHER = 5,   ///< Wetter
        IC_CONFIG = 6,    ///< Einstellungen
        IC_WORKSHOP = 7,  ///< Werkstatt
        IC_OFF = 8,       ///< Ausschalten
        IC_BREATHER = 9,  ///< Breather
        IC_POMODORO = 10, ///< Pomodoro
        IC_DAYVIEW = 11,  ///< Tagesansicht
    };

    // -----------------------------------------------------------------
    // Theme 1: "Standard"
    // -----------------------------------------------------------------
    constexpr MainScreenColors Standard = {
        .clockMarks = 0xC618,        // Stundenmarken
        .clockMinorMarks = 0x528A,   // Minutenmarken
        .clockHands = 0xC618,        // Zeiger
        .clockSecondHand = 0xF800,   // Sekundenzeiger
        .clockCenter = 0xC618,       // Achse
        .clockCenterAccent = 0xF800, // Sekundenzeiger Mitte

        .statusNtpActive = 0x5C44,  // NTP Zeit geholt
        .statusDstActive = 0xFBC0,  // DST Sommerzeit
        .statusWifiActive = 0x5C44, // WiFi-Balken
        .statusOff = 0x31A6,        // OFF-Zustand der Statuselemente
        .statusLabels = 0xC618,     // Textfarbe der Beschriftung NTP/DST

        .dateText = 0xC618, // Textfarbe für Datum
        .ssidText = 0xC618, // SSID
        .ipText = 0xC618,   // IP-Adresse

        .buttonBorder = 0x78E3, // Button-Rahmen
        .buttonFill = 0x0000,   // Button-Hintergrund
        .buttonText = 0x8A86,   // Kalender-Tag

        // Icon-Farben
        .iconTints = {
            0x3ACF, // [0] Radio
            0x8A86, // [1] SD-Player
            0x8A86, // [2] Kalender
            0x8A86, // [3] ToDo
            0x8A86, // [4] Kontakte
            0x8A86, // [5] Wetter
            0x8A86, // [6] Settings
            0x8A86, // [7] Werkstatt
            0xA944, // [8] Aus
            0x8A86, // [9] Breather
            0xC52E, // [10] Pomodoro (Brass)
            0xC52E, // [11] Tagesansicht (Brass)
        },
    };

    // -----------------------------------------------------------------
    // Theme 2: "PosterVintage"
    // -----------------------------------------------------------------
    constexpr MainScreenColors PosterVintage = {
        .clockMarks = 0xEF5B,        // Stundenmarken (Cream dark)
        .clockMinorMarks = 0x42CA,   // Minutenmarken (TealSlate)
        .clockHands = 0xDE54,        // Zeiger (Sand)
        .clockSecondHand = 0xC104,   // Sekundenzeiger (BrickLight)
        .clockCenter = 0xDE54,       // Achse (Sand)
        .clockCenterAccent = 0xC104, // Sekundenzeiger Mitte (BrickLight)

        .statusNtpActive = 0x74D0,  // NTP Zeit geholt (TealMid)
        .statusDstActive = 0xC52E,  // DST Sommerzeit (Brass)
        .statusWifiActive = 0x74D0, // WiFi-Balken (TealMid)
        .statusOff = 0x2124,        // OFF-Zustand der Statuselemente (Charcoal)
        .statusLabels = 0xDE54,     // Textfarbe der Beschriftung NTP/DST (Sand)

        .dateText = 0xEF5B, // Textfarbe für Datum (Cream)
        .ssidText = 0xDE54, // SSID (Sand)
        .ipText = 0xDE54,   // IP-Adresse (Sand)

        .buttonBorder = 0x2AA9, // Button-Rahmen (TealDark)
        .buttonFill = 0x0000,   // Button-Hintergrund (Schwarz)
        .buttonText = 0xEF5B,   // Kalender-Tag (Cream)

        // Icon-Farben
        .iconTints = {
            0x74D0, // [0] Radio     (TealMid)
            0xDE54, // [1] SD-Player (Sand)
            0xC52E, // [2] Kalender  (Brass)
            0x42CA, // [3] ToDo      (TealSlate)
            0x7309, // [4] Kontakte  (WarmBrown)
            0xEF5B, // [5] Wetter    (Cream)
            0x2AA9, // [6] Settings  (TealDark)
            0x2124, // [7] Werkstatt    (Charcoal)
            0x98E3, // [8] Aus       (Brick)
            0x42CA, // [9] Breather (TealSlate)
            0xC52E, // [10] Pomodoro (Brass)
            0xC52E, // [11] Tagesansicht (Brass)
        },
    };

    // -----------------------------------------------------------------
    // Theme 2: "PosterVintage Minimal" (2 Farben + Akzent)
    // -----------------------------------------------------------------
    constexpr MainScreenColors PosterVintageMinimal = {
        .clockMarks = 0xEF5B,        // Stundenmarken (Cream dark)
        .clockMinorMarks = 0x2AA9,   // Minutenmarken (TealDark)
        .clockHands = 0xEF5B,        // Zeiger (Cream dark)
        .clockSecondHand = 0xC104,   // Sekundenzeiger (BrickLight)
        .clockCenter = 0xDE54,       // Achse (Sand)
        .clockCenterAccent = 0xC104, // Sekundenzeiger Mitte (BrickLight)

        .statusNtpActive = 0x74D0,  // NTP Zeit geholt (TealMid)
        .statusDstActive = 0xEF5B,  // DST Sommerzeit (Cream dark)
        .statusWifiActive = 0x74D0, // WiFi-Balken (TealMid)
        .statusOff = 0x2124,        // OFF-Zustand der Statuselemente (Charcoal)
        .statusLabels = 0xEF5B,     // Textfarbe der Beschriftung NTP/DST (Cream dark)

        .dateText = 0xEF5B, // Textfarbe für Datum (Cream)
        .ssidText = 0xEF5B, // SSID (Cream)
        .ipText = 0xEF5B,   // IP-Adresse (Cream)

        .buttonBorder = 0x2AA9, // Button-Rahmen (TealDark)
        .buttonFill = 0x0000,   // Button-Hintergrund (Schwarz)
        .buttonText = 0xEF5B,   // Kalender-Tag (Cream)

        // Icon-Farben
        .iconTints = {
            0x74D0, // [0] Radio     (TealMid)
            0xEF5B, // [1] SD-Player (Cream dark)
            0xEF5B, // [2] Kalender  (Cream dark)
            0x74D0, // [3] ToDo      (TealMid)
            0xEF5B, // [4] Kontakte  (Cream dark)
            0x74D0, // [5] Wetter    (TealMid)
            0x2AA9, // [6] Settings  (TealDark)
            0x2124, // [7] Werkstatt (Charcoal)
            0x98E3, // [8] Aus       (Brick)
            0x74D0, // [9] Breather (TealMid)
            0xC52E, // [10] Pomodoro (Brass)
            0xC52E, // [11] Tagesansicht (Brass)
        },
    };

    // -----------------------------------------------------------------
    // Theme 2: "Brown Atelier" (Dieter-Rams / Braun Studio Style)
    // -----------------------------------------------------------------
    constexpr MainScreenColors BrownAtelier = {
        .clockMarks = 0xDE54,        // Stundenmarken (Sand)
        .clockMinorMarks = 0x4A08,   // Minutenmarken (Chocolate)
        .clockHands = 0xF718,        // Zeiger (Cream)
        .clockSecondHand = 0xF800,   // Sekundenzeiger (Rot)
        .clockCenter = 0x0000,       // Achse (Schwarz)
        .clockCenterAccent = 0xF800, // Sekundenzeiger Mitte (Rot)

        .statusNtpActive = 0x5C44,  // NTP Zeit geholt (Avocado)
        .statusDstActive = 0xA285,  // DST Sommerzeit (Sienna)
        .statusWifiActive = 0xDE54, // WiFi-Balken (Sand)
        .statusOff = 0x4A08,        // OFF-Zustand der Statuselemente (Chocolate)
        .statusLabels = 0xDE54,     // Textfarbe der Beschriftung NTP/DST (Sand)

        .dateText = 0x9C2B, // Textfarbe für Datum (Khaki)
        .ssidText = 0xDE54, // SSID (Sand)
        .ipText = 0x7D7A,   // IP-Adresse (Helles Nordischblau)

        .buttonBorder = 0x5A89, // Button-Rahmen (Chocolate)
        .buttonFill = 0x0000,   // Button-Hintergrund (Schwarz)
        .buttonText = 0xDE54,   // Kalender-Tag (Sand)

        // Icon-Farben "PosterVintage"
        .iconTints = {
            0x9C2B,              // [0] Radio     (Khaki)
            0x9C2B,              // [1] SD-Player (Khaki)
            0x9C2B,              // [2] Kalender  (Khaki)
            0x9C2B,              // [3] ToDo      (Khaki)
            0x9C2B /* 0x7309 */, // [4] Kontakte  (Khaki) /* (WarmBrown) */
            0x9C2B,              // [5] Wetter    (Khaki)
            0x42CA,              // [6] Settings  (TealSlate)
            0x9C2B,              // [7] Werkstatt (Khaki)
            0x98E3,              // [8] Aus       (Brick)
            0x9C2B,              // [9] Breather  (Khaki)
            0x9C2B,              // [10] Pomodoro (Khaki)
            0x9C2B,              // [11] Tagesansicht (Khaki)
        },
    };

    // -----------------------------------------------------------------
    // Theme 2: "VU Meter Optimized" (Kontrast + klare Akzente, bleibt Vintage)
    // -----------------------------------------------------------------
    constexpr MainScreenColors VUMeterOptimized = {
        .clockMarks = 0xF718,        // Stundenmarken (Cream)
        .clockMinorMarks = 0x42CA,   // Minutenmarken (TealSlate)
        .clockHands = 0xF718,        // Zeiger (Cream)
        .clockSecondHand = 0xC104,   // Sekundenzeiger (BrickLight)
        .clockCenter = 0xC52E,       // Achse (Brass)
        .clockCenterAccent = 0xC104, // Sekundenzeiger Mitte (BrickLight)

        .statusNtpActive = 0xDCC4,  // Aktiver NTP-Status (Harvest)
        .statusDstActive = 0xC52E,  // DST Sommerzeit (Brass)
        .statusWifiActive = 0x74D0, // WiFi-Balken (TealMid)
        .statusOff = 0x2124,        // OFF-Zustand der Statuselemente (Charcoal)
        .statusLabels = 0xDE54,     // Textfarbe der Beschriftung NTP/DST (Sand)

        .dateText = 0xF718, // Textfarbe für Datum (Cream)
        .ssidText = 0xDE54, // SSID (Sand)
        .ipText = 0xDE54,   // IP-Adresse (Sand)

        .buttonBorder = 0x2AA9, // Button-Rahmen (TealDark)
        .buttonFill = 0x0000,   // Button-Hintergrund (Schwarz)
        .buttonText = 0xF718,   // Kalender-Tag (Cream)

        // Kontrastreiche Icon-Farben im Vintage-Farbschema
        .iconTints = {
            0x74D0, // [0] Radio     (TealMid)
            0xF718, // [1] SD-Player (Cream)
            0xC52E, // [2] Kalender  (Brass)
            0xDCC4, // [3] ToDo      (Harvest)
            0x7309, // [4] Kontakte  (WarmBrown)
            0xDE54, // [5] Wetter    (Sand)
            0x2AA9, // [6] Settings  (TealDark)
            0x2124, // [7] Werkstatt    (Charcoal)
            0x98E3, // [8] Aus       (Brick)
            0xDCC4, // [9] Breather (Harvest)
            0xC52E, // [10] Pomodoro (Brass)
            0xC52E, // [11] Tagesansicht (Brass)
        },
    };

    // =================================================================
    // Aktives Theme
    // =================================================================
    inline constexpr const MainScreenColors &ActiveMainScreen = BrownAtelier;

} // namespace Theme

// ===================================================================
// Retro/Vintage-Farbpalette
// ===================================================================

/**
 * @brief Retro/Vintage-Farbpalette in RGB565
 *
 * Enthält typische Farben aus den 60er-80er Jahren: Warme Erdtöne, gedeckte Pastelle, Neonakzente, Brauntöne und Retro-Signalfarben.
 *
 * Alle Werte als uint16_t im RGB565-Format:
 *   Bits [15:11] = Rot (0-31)
 *   Bits [10:5]  = Grün (0-63)
 *   Bits [4:0]   = Blau (0-31)
 *
 * Umrechnung von RGB888 (r,g,b) nach RGB565:
 *   ((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3)
 */
namespace RetroColors
{
    // Warme Erdtöne und Braun
    constexpr uint16_t Tan = 0xCCA8;         ///< Helles Beige/Tan           (RGB: 204, 149, 68)
    constexpr uint16_t Sienna = 0xA285;      ///< Gebranntes Sienna          (RGB: 160, 82, 45)
    constexpr uint16_t BurntOrange = 0xCC41; ///< Gebranntes Orange          (RGB: 204, 85, 0)
    constexpr uint16_t Rust = 0xB224;        ///< Rostbraun                  (RGB: 183, 65, 14)
    constexpr uint16_t Chocolate = 0x4A08;   ///< Schokoladenbraun           (RGB: 75, 54, 33)
    constexpr uint16_t Saddle = 0x6A08;      ///< Sattelbraun                (RGB: 109, 64, 36)
    constexpr uint16_t WarmBrown = 0x7309;   ///< WarmBrown                  (RGB: 115, 9, 0)

    // Gedämpfte Rottöne
    constexpr uint16_t Maroon = 0x6800;     ///< Dunkles Weinrot             (RGB: 105, 0, 0)
    constexpr uint16_t Brick = 0xA944;      ///< Ziegelrot                   (RGB: 172, 42, 36)
    constexpr uint16_t Terracotta = 0xD346; ///< Terrakotta                  (RGB: 210, 105, 48)
    constexpr uint16_t Salmon = 0xDC8A;     ///< Gedeckter Lachs             (RGB: 219, 145, 82)

    // Grüntöne
    constexpr uint16_t Avocado = 0x5C44; ///< Avocado                        (RGB: 90, 138, 32)
    constexpr uint16_t Olive = 0x5345;   ///< Olivgrün                       (RGB: 85, 107, 47)
    constexpr uint16_t Forest = 0x2B44;  ///< Waldgrün                       (RGB: 34, 100, 34)
    constexpr uint16_t Mint = 0x7F13;    ///< Gedämpftes Mint                (RGB: 119, 221, 153)
    constexpr uint16_t Sage = 0x8E0C;    ///< Salbeigrün                     (RGB: 140, 193, 98)

    // Blau- und Türkistöne
    constexpr uint16_t Teal = 0x0410;        ///< Petrol/Teal                (RGB: 0, 128, 128)
    constexpr uint16_t Petrol = 0x1B4E;      ///< Dunkles Petrol             (RGB: 24, 104, 112)
    constexpr uint16_t Slate = 0x4A8C;       ///< Schieferblau               (RGB: 72, 84, 96)
    constexpr uint16_t Powder = 0xADB7;      ///< Puderblau (50er-Pastell)   (RGB: 170, 183, 191)
    constexpr uint16_t Ocean = 0x1B51;       ///< Ozeanblau (Navbar-Ton)     (RGB: 25, 105, 140)
    constexpr uint16_t OceanLight = 0x5DDA;  ///< Helles Ozeancyan (Akzent)  (RGB: 90, 186, 214)
    constexpr uint16_t DeepOcean = 0x224B;   ///< Tiefes Ozeanblau           (RGB: 33, 73, 90)
    constexpr uint16_t Nordic = 0x3ACF;      ///< Nordisches Stahlblau       (RGB: 58, 89, 123)
    constexpr uint16_t NordicLight = 0x7D7A; ///< Helles Nordischblau        (RGB: 123, 174, 214)
    constexpr uint16_t DeepNordic = 0x21C9;  ///< Tiefes Nordischblau        (RGB: 33, 57, 74)

    // Gelb- und Goldtöne
    constexpr uint16_t Mustard = 0xD5A0; ///< Senfgelb                       (RGB: 210, 180, 0)
    constexpr uint16_t Gold = 0xD520;    ///< Altgold                        (RGB: 212, 167, 0)
    constexpr uint16_t Harvest = 0xDCC4; ///< Erntegelb                      (RGB: 220, 152, 36)

    // Orange und Koralle
    constexpr uint16_t Tangerine = 0xFBC0; ///< Mandarinorange               (RGB: 255, 119, 0)
    constexpr uint16_t Coral = 0xFB4A;     ///< Retro-Koralle                (RGB: 255, 105, 80)
    constexpr uint16_t Peach = 0xFD93;     ///< Pfirsich                     (RGB: 255, 178, 150)

    // Pastell- und Akzentfarben
    constexpr uint16_t Lavender = 0xBD9B; ///< Gedämpftes Lavendel           (RGB: 186, 179, 219)
    constexpr uint16_t Mauve = 0x9A52;    ///< Mauve/Altrosa                 (RGB: 153, 75, 146)
    constexpr uint16_t Cream = 0xFFF5;    ///< Cremeweiß                     (RGB: 255, 253, 235)

} // namespace RetroColors

// ===================================================================
// RAL-Classic-Farbpalette
// ===================================================================

/**
 * @brief RAL-Classic-Farben in RGB565
*
 * 216 Farben der RAL-Classic-Karte als uint16_t im RGB565-Format. Der Bezeichner ist der deutsche Farbname (z.B. ral::Orientrot), der Kommentar nennt die RAL-Nummer, den RGB888-Hexwert und die RGB888-Komponenten.
 */
namespace ral
{
    // Gelbtöne (RAL 1xxx)
    constexpr uint16_t Gruenbeige = 0xCDD1;    ///< RAL 1000  #CDBA88  (RGB: 205, 186, 136)
    constexpr uint16_t Beige = 0xD590;         ///< RAL 1001  #D0B084  (RGB: 208, 176, 132)
    constexpr uint16_t Sandgelb = 0xD54D;      ///< RAL 1002  #D2AA6D  (RGB: 210, 170, 109)
    constexpr uint16_t Signalgelb = 0xFD40;    ///< RAL 1003  #F9A900  (RGB: 249, 169, 0)
    constexpr uint16_t Goldgelb = 0xE4E0;      ///< RAL 1004  #E49E00  (RGB: 228, 158, 0)
    constexpr uint16_t Honiggelb = 0xCC60;     ///< RAL 1005  #CB8F00  (RGB: 203, 143, 0)
    constexpr uint16_t Maisgelb = 0xE480;      ///< RAL 1006  #E19000  (RGB: 225, 144, 0)
    constexpr uint16_t Narzissengelb = 0xEC60; ///< RAL 1007  #E88C00  (RGB: 232, 140, 0)
    constexpr uint16_t Braunbeige = 0xAC0A;    ///< RAL 1011  #AF8050  (RGB: 175, 128, 80)
    constexpr uint16_t Zitronengelb = 0xDD65;  ///< RAL 1012  #DDAF28  (RGB: 221, 175, 40)
    constexpr uint16_t Perlweiss = 0xE6D8;     ///< RAL 1013  #E3D9C7  (RGB: 227, 217, 199)
    constexpr uint16_t Elfenbein = 0xDE33;     ///< RAL 1014  #DDC49B  (RGB: 221, 196, 155)
    constexpr uint16_t Hellelfenbein = 0xE696; ///< RAL 1015  #E6D2B5  (RGB: 230, 210, 181)
    constexpr uint16_t Schwefelgelb = 0xF6E7;  ///< RAL 1016  #F1DD39  (RGB: 241, 221, 57)
    constexpr uint16_t Safrangelb = 0xF54A;    ///< RAL 1017  #F6A951  (RGB: 246, 169, 81)
    constexpr uint16_t Zinkgelb = 0xFE46;      ///< RAL 1018  #FACA31  (RGB: 250, 202, 49)
    constexpr uint16_t Graubeige = 0xA46F;     ///< RAL 1019  #A48F7A  (RGB: 164, 143, 122)
    constexpr uint16_t Olivgelb = 0xA46C;      ///< RAL 1020  #A08F65  (RGB: 160, 143, 101)
    constexpr uint16_t Rapsgelb = 0xF5A0;      ///< RAL 1021  #F6B600  (RGB: 246, 182, 0)
    constexpr uint16_t Verkehrsgelb = 0xF5A0;  ///< RAL 1023  #F7B500  (RGB: 247, 181, 0)
    constexpr uint16_t Ockergelb = 0xBC69;     ///< RAL 1024  #BA8F4C  (RGB: 186, 143, 76)
    constexpr uint16_t Leuchtgelb = 0xFFE0;    ///< RAL 1026  #FFFF00  (RGB: 255, 255, 0)
    constexpr uint16_t Currygelb = 0xA3E1;     ///< RAL 1027  #A77F0F  (RGB: 167, 127, 15)
    constexpr uint16_t Melonengelb = 0xFCE0;   ///< RAL 1028  #FF9C00  (RGB: 255, 156, 0)
    constexpr uint16_t Ginstergelb = 0xE500;   ///< RAL 1032  #E2A300  (RGB: 226, 163, 0)
    constexpr uint16_t Dahliengelb = 0xFCC3;   ///< RAL 1033  #F99A1D  (RGB: 249, 154, 29)
    constexpr uint16_t Pastellgelb = 0xECEA;   ///< RAL 1034  #EB9C52  (RGB: 235, 156, 82)
    constexpr uint16_t Perlbeige = 0x8C0E;     ///< RAL 1035  #8F8370  (RGB: 143, 131, 112)
    constexpr uint16_t Perlgold = 0x8328;      ///< RAL 1036  #806440  (RGB: 128, 100, 64)
    constexpr uint16_t Sonnengelb = 0xF480;    ///< RAL 1037  #F09200  (RGB: 240, 146, 0)

    // Orangetöne (RAL 2xxx)
    constexpr uint16_t Gelborange = 0xDB60;       ///< RAL 2000  #DA6E00  (RGB: 218, 110, 0)
    constexpr uint16_t Rotorange = 0xBA43;        ///< RAL 2001  #BA481C  (RGB: 186, 72, 28)
    constexpr uint16_t Blutorange = 0xB9C4;       ///< RAL 2002  #BF3922  (RGB: 191, 57, 34)
    constexpr uint16_t Pastellorange = 0xF3C5;    ///< RAL 2003  #F67829  (RGB: 246, 120, 41)
    constexpr uint16_t Reinorange = 0xE280;       ///< RAL 2004  #E25304  (RGB: 226, 83, 4)
    constexpr uint16_t Leuchtorange = 0xFA61;     ///< RAL 2005  #FF4D08  (RGB: 255, 77, 8)
    constexpr uint16_t Leuchthellorange = 0xFD80; ///< RAL 2007  #FFB200  (RGB: 255, 178, 0)
    constexpr uint16_t Hellrotorange = 0xEB44;    ///< RAL 2008  #EC6B22  (RGB: 236, 107, 34)
    constexpr uint16_t Verkehrsorange = 0xDA81;   ///< RAL 2009  #DE5308  (RGB: 222, 83, 8)
    constexpr uint16_t Signalorange = 0xD2E5;     ///< RAL 2010  #D05D29  (RGB: 208, 93, 41)
    constexpr uint16_t Tieforange = 0xE361;       ///< RAL 2011  #E26E0F  (RGB: 226, 110, 15)
    constexpr uint16_t Lachsorange = 0xD329;      ///< RAL 2012  #D5654E  (RGB: 213, 101, 78)
    constexpr uint16_t Perlorange = 0x91E4;       ///< RAL 2013  #923E25  (RGB: 146, 62, 37)
    constexpr uint16_t RALorange = 0xFAA0;        ///< RAL 2017  #FC5500  (RGB: 252, 85, 0)

    // Rottöne (RAL 3xxx)
    constexpr uint16_t Feuerrot = 0xA144;      ///< RAL 3000  #A72920  (RGB: 167, 41, 32)
    constexpr uint16_t Signalrot = 0x9924;     ///< RAL 3001  #9B2423  (RGB: 155, 36, 35)
    constexpr uint16_t Karminrot = 0x9904;     ///< RAL 3002  #9B2321  (RGB: 155, 35, 33)
    constexpr uint16_t Rubinrot = 0x80C4;      ///< RAL 3003  #861A22  (RGB: 134, 26, 34)
    constexpr uint16_t Purpurrot = 0x68E4;     ///< RAL 3004  #6B1C23  (RGB: 107, 28, 35)
    constexpr uint16_t Weinrot = 0x58C3;       ///< RAL 3005  #59191F  (RGB: 89, 25, 31)
    constexpr uint16_t Schwarzrot = 0x3904;    ///< RAL 3007  #3E2022  (RGB: 62, 32, 34)
    constexpr uint16_t Oxidrot = 0x69A5;       ///< RAL 3009  #6D342D  (RGB: 109, 52, 45)
    constexpr uint16_t Braunrot = 0x7924;      ///< RAL 3011  #782423  (RGB: 120, 36, 35)
    constexpr uint16_t Beigerot = 0xC42D;      ///< RAL 3012  #C5856D  (RGB: 197, 133, 109)
    constexpr uint16_t Tomatenrot = 0x9164;    ///< RAL 3013  #972E25  (RGB: 151, 46, 37)
    constexpr uint16_t Altrosa = 0xCB8E;       ///< RAL 3014  #CB7375  (RGB: 203, 115, 117)
    constexpr uint16_t Hellrosa = 0xDD14;      ///< RAL 3015  #D8A0A6  (RGB: 216, 160, 166)
    constexpr uint16_t Korallenrot = 0xA1E6;   ///< RAL 3016  #A63D30  (RGB: 166, 61, 48)
    constexpr uint16_t Rose = 0xCAAB;          ///< RAL 3017  #CA555D  (RGB: 202, 85, 93)
    constexpr uint16_t Erdbeerrot = 0xC1E9;    ///< RAL 3018  #C63F4A  (RGB: 198, 63, 74)
    constexpr uint16_t Verkehrsrot = 0xB8E2;   ///< RAL 3020  #BB1F11  (RGB: 187, 31, 17)
    constexpr uint16_t Lachsrot = 0xCB4A;      ///< RAL 3022  #CF6955  (RGB: 207, 105, 85)
    constexpr uint16_t Leuchtrot = 0xF964;     ///< RAL 3024  #FF2D21  (RGB: 255, 45, 33)
    constexpr uint16_t Leuchthellrot = 0xF943; ///< RAL 3026  #FF2A1C  (RGB: 255, 42, 28)
    constexpr uint16_t Himbeerrot = 0xA927;    ///< RAL 3027  #AB273C  (RGB: 171, 39, 60)
    constexpr uint16_t Reinrot = 0xC964;       ///< RAL 3028  #CC2C24  (RGB: 204, 44, 36)
    constexpr uint16_t Orientrot = 0xA1A6;     ///< RAL 3031  #A63437  (RGB: 166, 52, 55)
    constexpr uint16_t Perlrubinrot = 0x70E4;  ///< RAL 3032  #701D24  (RGB: 112, 29, 36)
    constexpr uint16_t Perlrosa = 0xA1C5;      ///< RAL 3033  #A53A2E  (RGB: 165, 58, 46)

    // Violetttöne (RAL 4xxx)
    constexpr uint16_t Rotlila = 0x8310;         ///< RAL 4001  #816183  (RGB: 129, 97, 131)
    constexpr uint16_t Rotviolett = 0x89E9;      ///< RAL 4002  #8D3C4B  (RGB: 141, 60, 75)
    constexpr uint16_t Erikaviolett = 0xC311;    ///< RAL 4003  #C4618C  (RGB: 196, 97, 140)
    constexpr uint16_t Bordeauxviolett = 0x60E7; ///< RAL 4004  #651E38  (RGB: 101, 30, 56)
    constexpr uint16_t Blaulila = 0x7353;        ///< RAL 4005  #76689A  (RGB: 118, 104, 154)
    constexpr uint16_t Verkehrspurpur = 0x918E;  ///< RAL 4006  #903373  (RGB: 144, 51, 115)
    constexpr uint16_t Purpurviolett = 0x4127;   ///< RAL 4007  #47243C  (RGB: 71, 36, 60)
    constexpr uint16_t Signalviolett = 0x8270;   ///< RAL 4008  #844C82  (RGB: 132, 76, 130)
    constexpr uint16_t Pastellviolett = 0x9C32;  ///< RAL 4009  #9D8692  (RGB: 157, 134, 146)
    constexpr uint16_t Telemagenta = 0xBA0E;     ///< RAL 4010  #BB4077  (RGB: 187, 64, 119)
    constexpr uint16_t Perlviolett = 0x6B10;     ///< RAL 4011  #6E6387  (RGB: 110, 99, 135)
    constexpr uint16_t Perlbrombeer = 0x6B4F;    ///< RAL 4012  #6A6B7F  (RGB: 106, 107, 127)

    // Blautöne (RAL 5xxx)
    constexpr uint16_t Violettblau = 0x326D;    ///< RAL 5000  #304F6E  (RGB: 48, 79, 110)
    constexpr uint16_t Gruenblau = 0x0A6C;      ///< RAL 5001  #0E4C64  (RGB: 14, 76, 100)
    constexpr uint16_t Ultramarinblau = 0x01CF; ///< RAL 5002  #00387A  (RGB: 0, 56, 122)
    constexpr uint16_t Saphirblau = 0x19CA;     ///< RAL 5003  #1F3855  (RGB: 31, 56, 85)
    constexpr uint16_t Schwarzblau = 0x18E5;    ///< RAL 5004  #191E28  (RGB: 25, 30, 40)
    constexpr uint16_t Signalblau = 0x0290;     ///< RAL 5005  #005387  (RGB: 0, 83, 135)
    constexpr uint16_t Brillantblau = 0x3351;   ///< RAL 5007  #376B8C  (RGB: 55, 107, 140)
    constexpr uint16_t Graublau = 0x29C8;       ///< RAL 5008  #2B3A44  (RGB: 43, 58, 68)
    constexpr uint16_t Azurblau = 0x22EF;       ///< RAL 5009  #215F78  (RGB: 33, 95, 120)
    constexpr uint16_t Enzianblau = 0x026F;     ///< RAL 5010  #004F7C  (RGB: 0, 79, 124)
    constexpr uint16_t Stahlblau = 0x1947;      ///< RAL 5011  #1A2B3C  (RGB: 26, 43, 60)
    constexpr uint16_t Lichtblau = 0x0456;      ///< RAL 5012  #0089B6  (RGB: 0, 137, 182)
    constexpr uint16_t Kobaltblau = 0x198A;     ///< RAL 5013  #193153  (RGB: 25, 49, 83)
    constexpr uint16_t Taubenblau = 0x63F2;     ///< RAL 5014  #637D96  (RGB: 99, 125, 150)
    constexpr uint16_t Himmelblau = 0x03F5;     ///< RAL 5015  #007CAF  (RGB: 0, 124, 175)
    constexpr uint16_t Verkehrsblau = 0x02D1;   ///< RAL 5017  #005B8C  (RGB: 0, 91, 140)
    constexpr uint16_t Tuerkisblau = 0x0451;    ///< RAL 5018  #048B8C  (RGB: 4, 139, 140)
    constexpr uint16_t Capriblau = 0x02F0;      ///< RAL 5019  #005E83  (RGB: 0, 94, 131)
    constexpr uint16_t Ozeanblau = 0x0209;      ///< RAL 5020  #00414B  (RGB: 0, 65, 75)
    constexpr uint16_t Wasserblau = 0x03AE;     ///< RAL 5021  #007577  (RGB: 0, 117, 119)
    constexpr uint16_t Nachtblau = 0x216B;      ///< RAL 5022  #222D5A  (RGB: 34, 45, 90)
    constexpr uint16_t Fernblau = 0x4351;       ///< RAL 5023  #41698C  (RGB: 65, 105, 140)
    constexpr uint16_t Pastellblau = 0x6495;    ///< RAL 5024  #6093AC  (RGB: 96, 147, 172)
    constexpr uint16_t Perlenzian = 0x234F;     ///< RAL 5025  #20697C  (RGB: 32, 105, 124)
    constexpr uint16_t Perlnachtblau = 0x098A;  ///< RAL 5026  #0F3052  (RGB: 15, 48, 82)

    // Grüntöne (RAL 6xxx)
    constexpr uint16_t Patinagruen = 0x3BAC;    ///< RAL 6000  #3C7460  (RGB: 60, 116, 96)
    constexpr uint16_t Smaragdgruen = 0x3326;   ///< RAL 6001  #366735  (RGB: 54, 103, 53)
    constexpr uint16_t Laubgruen = 0x32C5;      ///< RAL 6002  #325928  (RGB: 50, 89, 40)
    constexpr uint16_t Olivgruen = 0x5287;      ///< RAL 6003  #50533C  (RGB: 80, 83, 60)
    constexpr uint16_t Blaugruen = 0x0228;      ///< RAL 6004  #024442  (RGB: 2, 68, 66)
    constexpr uint16_t Moosgruen = 0x1206;      ///< RAL 6005  #114232  (RGB: 17, 66, 50)
    constexpr uint16_t Grauoliv = 0x39C5;       ///< RAL 6006  #3C392E  (RGB: 60, 57, 46)
    constexpr uint16_t Flaschengruen = 0x2984;  ///< RAL 6007  #2C3222  (RGB: 44, 50, 34)
    constexpr uint16_t Braungruen = 0x31A5;     ///< RAL 6008  #36342A  (RGB: 54, 52, 42)
    constexpr uint16_t Tannengruen = 0x21A5;    ///< RAL 6009  #27352A  (RGB: 39, 53, 42)
    constexpr uint16_t Grasgruen = 0x4B67;      ///< RAL 6010  #4D6F39  (RGB: 77, 111, 57)
    constexpr uint16_t Resedagruen = 0x6BEB;    ///< RAL 6011  #6B7C59  (RGB: 107, 124, 89)
    constexpr uint16_t Schwarzgruen = 0x29E7;   ///< RAL 6012  #2F3D3A  (RGB: 47, 61, 58)
    constexpr uint16_t Schilfgruen = 0x7BAB;    ///< RAL 6013  #7C765A  (RGB: 124, 118, 90)
    constexpr uint16_t Gelboliv = 0x4206;       ///< RAL 6014  #474135  (RGB: 71, 65, 53)
    constexpr uint16_t Schwarzoliv = 0x39E6;    ///< RAL 6015  #3D3D36  (RGB: 61, 61, 54)
    constexpr uint16_t Tuerkisgruen = 0x0349;   ///< RAL 6016  #00694C  (RGB: 0, 105, 76)
    constexpr uint16_t Maigruen = 0x5BE8;       ///< RAL 6017  #587F40  (RGB: 88, 127, 64)
    constexpr uint16_t Gelbgruen = 0x64C7;      ///< RAL 6018  #60993B  (RGB: 96, 153, 59)
    constexpr uint16_t Weissgruen = 0xBE75;     ///< RAL 6019  #B9CEAC  (RGB: 185, 206, 172)
    constexpr uint16_t Chromoxidgruen = 0x3205; ///< RAL 6020  #37422F  (RGB: 55, 66, 47)
    constexpr uint16_t Blassgruen = 0x8CCE;     ///< RAL 6021  #8A9977  (RGB: 138, 153, 119)
    constexpr uint16_t Braunoliv = 0x3984;      ///< RAL 6022  #3A3327  (RGB: 58, 51, 39)
    constexpr uint16_t Verkehrsgruen = 0x040A;  ///< RAL 6024  #008351  (RGB: 0, 131, 81)
    constexpr uint16_t Farngruen = 0x5B67;      ///< RAL 6025  #5E6E3B  (RGB: 94, 110, 59)
    constexpr uint16_t Opalgruen = 0x02E9;      ///< RAL 6026  #005F4E  (RGB: 0, 95, 78)
    constexpr uint16_t Lichtgruen = 0x7DD6;     ///< RAL 6027  #7EBAB5  (RGB: 126, 186, 181)
    constexpr uint16_t Kieferngruen = 0x32A8;   ///< RAL 6028  #315442  (RGB: 49, 84, 66)
    constexpr uint16_t Minzgruen = 0x0367;      ///< RAL 6029  #006F3D  (RGB: 0, 111, 61)
    constexpr uint16_t Signalgruen = 0x23EA;    ///< RAL 6032  #237F52  (RGB: 35, 127, 82)
    constexpr uint16_t Minttuerkis = 0x442F;    ///< RAL 6033  #45877F  (RGB: 69, 135, 127)
    constexpr uint16_t Pastelltuerkis = 0x7D75; ///< RAL 6034  #7AADAC  (RGB: 122, 173, 172)
    constexpr uint16_t Perlgruen = 0x1A64;      ///< RAL 6035  #194D25  (RGB: 25, 77, 37)
    constexpr uint16_t Perlopalgruen = 0x02A9;  ///< RAL 6036  #04574B  (RGB: 4, 87, 75)
    constexpr uint16_t Reingruen = 0x0445;      ///< RAL 6037  #008B29  (RGB: 0, 139, 41)
    constexpr uint16_t Leuchtgruen = 0x05A3;    ///< RAL 6038  #00B51B  (RGB: 0, 181, 27)
    constexpr uint16_t Fasergruen = 0xB627;     ///< RAL 6039  #B3C43E  (RGB: 179, 196, 62)

    // Grautöne (RAL 7xxx)
    constexpr uint16_t Fehgrau = 0x7C51;       ///< RAL 7000  #7A888E  (RGB: 122, 136, 142)
    constexpr uint16_t Silbergrau = 0x8CB3;    ///< RAL 7001  #8C979C  (RGB: 140, 151, 156)
    constexpr uint16_t Olivgrau = 0x83CC;      ///< RAL 7002  #817863  (RGB: 129, 120, 99)
    constexpr uint16_t Moosgrau = 0x7BAD;      ///< RAL 7003  #797669  (RGB: 121, 118, 105)
    constexpr uint16_t Signalgrau = 0x9CD3;    ///< RAL 7004  #9A9B9B  (RGB: 154, 155, 155)
    constexpr uint16_t Mausgrau = 0x6B6D;      ///< RAL 7005  #6B6E6B  (RGB: 107, 110, 107)
    constexpr uint16_t Beigegrau = 0x734B;     ///< RAL 7006  #766A5E  (RGB: 118, 106, 94)
    constexpr uint16_t Khakigrau = 0x72E7;     ///< RAL 7008  #745F3D  (RGB: 116, 95, 61)
    constexpr uint16_t Gruengrau = 0x5B0B;     ///< RAL 7009  #5D6058  (RGB: 93, 96, 88)
    constexpr uint16_t Zeltgrau = 0x5AEA;      ///< RAL 7010  #585C56  (RGB: 88, 92, 86)
    constexpr uint16_t Eisengrau = 0x52CB;     ///< RAL 7011  #52595D  (RGB: 82, 89, 93)
    constexpr uint16_t Basaltgrau = 0x52EB;    ///< RAL 7012  #575D5E  (RGB: 87, 93, 94)
    constexpr uint16_t Braungrau = 0x5288;     ///< RAL 7013  #575044  (RGB: 87, 80, 68)
    constexpr uint16_t Schiefergrau = 0x4A8B;  ///< RAL 7015  #4F5358  (RGB: 79, 83, 88)
    constexpr uint16_t Anthrazitgrau = 0x39E8; ///< RAL 7016  #383E42  (RGB: 56, 62, 66)
    constexpr uint16_t Schwarzgrau = 0x2986;   ///< RAL 7021  #2F3234  (RGB: 47, 50, 52)
    constexpr uint16_t Umbragrau = 0x4A48;     ///< RAL 7022  #4C4A44  (RGB: 76, 74, 68)
    constexpr uint16_t Betongrau = 0x840E;     ///< RAL 7023  #808076  (RGB: 128, 128, 118)
    constexpr uint16_t Graphitgrau = 0x4249;   ///< RAL 7024  #45494E  (RGB: 69, 73, 78)
    constexpr uint16_t Granitgrau = 0x3208;    ///< RAL 7026  #374345  (RGB: 55, 67, 69)
    constexpr uint16_t Steingrau = 0x9470;     ///< RAL 7030  #928E85  (RGB: 146, 142, 133)
    constexpr uint16_t Blaugrau = 0x5B4D;      ///< RAL 7031  #5B686D  (RGB: 91, 104, 109)
    constexpr uint16_t Kieselgrau = 0xB594;    ///< RAL 7032  #B5B0A1  (RGB: 181, 176, 161)
    constexpr uint16_t Zementgrau = 0x7C0E;    ///< RAL 7033  #7F8274  (RGB: 127, 130, 116)
    constexpr uint16_t Gelbgrau = 0x944D;      ///< RAL 7034  #92886F  (RGB: 146, 136, 111)
    constexpr uint16_t Lichtgrau = 0xC638;     ///< RAL 7035  #C5C7C4  (RGB: 197, 199, 196)
    constexpr uint16_t Platingrau = 0x9492;    ///< RAL 7036  #979392  (RGB: 151, 147, 146)
    constexpr uint16_t Staubgrau = 0x7BCF;     ///< RAL 7037  #7A7B7A  (RGB: 122, 123, 122)
    constexpr uint16_t Achatgrau = 0xB595;     ///< RAL 7038  #B0B0A9  (RGB: 176, 176, 169)
    constexpr uint16_t Quarzgrau = 0x6B2B;     ///< RAL 7039  #6B665E  (RGB: 107, 102, 94)
    constexpr uint16_t Fenstergrau = 0x9CF4;   ///< RAL 7040  #989EA1  (RGB: 152, 158, 161)
    constexpr uint16_t VerkehrsgrauA = 0x8C92; ///< RAL 7042  #8E9291  (RGB: 142, 146, 145)
    constexpr uint16_t VerkehrsgrauB = 0x4A8A; ///< RAL 7043  #4F5250  (RGB: 79, 82, 80)
    constexpr uint16_t Seidengrau = 0xB595;    ///< RAL 7044  #B7B3A8  (RGB: 183, 179, 168)
    constexpr uint16_t Telegrau1 = 0x8C92;     ///< RAL 7045  #8D9295  (RGB: 141, 146, 149)
    constexpr uint16_t Telegrau2 = 0x7C31;     ///< RAL 7046  #7E868A  (RGB: 126, 134, 138)
    constexpr uint16_t Telegrau4 = 0xCE58;     ///< RAL 7047  #C8C8C7  (RGB: 200, 200, 199)
    constexpr uint16_t Perlmausgrau = 0x83CE;  ///< RAL 7048  #817B73  (RGB: 129, 123, 115)

    // Brauntöne (RAL 8xxx)
    constexpr uint16_t Gruenbraun = 0x8B47;       ///< RAL 8000  #89693F  (RGB: 137, 105, 63)
    constexpr uint16_t Ockerbraun = 0x9B05;       ///< RAL 8001  #9D622B  (RGB: 157, 98, 43)
    constexpr uint16_t Signalbraun = 0x7A67;      ///< RAL 8002  #794D3E  (RGB: 121, 77, 62)
    constexpr uint16_t Lehmbraun = 0x7A44;        ///< RAL 8003  #7E4B27  (RGB: 126, 75, 39)
    constexpr uint16_t Kupferbraun = 0x8A46;      ///< RAL 8004  #8D4931  (RGB: 141, 73, 49)
    constexpr uint16_t Rehbraun = 0x7225;         ///< RAL 8007  #70462B  (RGB: 112, 70, 43)
    constexpr uint16_t Olivbraun = 0x7244;        ///< RAL 8008  #724A25  (RGB: 114, 74, 37)
    constexpr uint16_t Nussbraun = 0x59C4;        ///< RAL 8011  #5A3827  (RGB: 90, 56, 39)
    constexpr uint16_t Rotbraun = 0x6185;         ///< RAL 8012  #66332B  (RGB: 102, 51, 43)
    constexpr uint16_t Sepiabraun = 0x49A4;       ///< RAL 8014  #4A3526  (RGB: 74, 53, 38)
    constexpr uint16_t Kastanienbraun = 0x5964;   ///< RAL 8015  #5E2F26  (RGB: 94, 47, 38)
    constexpr uint16_t Mahagonibraun = 0x4944;    ///< RAL 8016  #4C2B20  (RGB: 76, 43, 32)
    constexpr uint16_t Schokoladenbraun = 0x4165; ///< RAL 8017  #442F29  (RGB: 68, 47, 41)
    constexpr uint16_t Graubraun = 0x39A6;        ///< RAL 8019  #3D3635  (RGB: 61, 54, 53)
    constexpr uint16_t Schwarzbraun = 0x18A3;     ///< RAL 8022  #1A1719  (RGB: 26, 23, 25)
    constexpr uint16_t Orangebraun = 0xA2A5;      ///< RAL 8023  #A45729  (RGB: 164, 87, 41)
    constexpr uint16_t Beigebraun = 0x7A87;       ///< RAL 8024  #795038  (RGB: 121, 80, 56)
    constexpr uint16_t Blassbraun = 0x72C8;       ///< RAL 8025  #755847  (RGB: 117, 88, 71)
    constexpr uint16_t Terrabraun = 0x51C5;       ///< RAL 8028  #513A2A  (RGB: 81, 58, 42)
    constexpr uint16_t Perlkupfer = 0x7A06;       ///< RAL 8029  #7F4031  (RGB: 127, 64, 49)

    // Weiß- und Schwarztöne (RAL 9xxx)
    constexpr uint16_t Cremeweiss = 0xEF1A;      ///< RAL 9001  #E9E0D2  (RGB: 233, 224, 210)
    constexpr uint16_t Grauweiss = 0xD6B9;       ///< RAL 9002  #D6D5CB  (RGB: 214, 213, 203)
    constexpr uint16_t Signalweiss = 0xEF7C;     ///< RAL 9003  #ECECE7  (RGB: 236, 236, 231)
    constexpr uint16_t Signalschwarz = 0x2945;   ///< RAL 9004  #2B2B2C  (RGB: 43, 43, 44)
    constexpr uint16_t Tiefschwarz = 0x0862;     ///< RAL 9005  #0E0E10  (RGB: 14, 14, 16)
    constexpr uint16_t Weissaluminium = 0xA514;  ///< RAL 9006  #A1A1A0  (RGB: 161, 161, 160)
    constexpr uint16_t Graualuminium = 0x8430;   ///< RAL 9007  #868581  (RGB: 134, 133, 129)
    constexpr uint16_t Reinweiss = 0xF77C;       ///< RAL 9010  #F1EDE1  (RGB: 241, 237, 225)
    constexpr uint16_t Graphitschwarz = 0x2145;  ///< RAL 9011  #27292B  (RGB: 39, 41, 43)
    constexpr uint16_t Reinraumweiss = 0xFF9C;   ///< RAL 9012  #F8F2E1  (RGB: 248, 242, 225)
    constexpr uint16_t Verkehrsweiss = 0xF79D;   ///< RAL 9016  #F1F1EA  (RGB: 241, 241, 234)
    constexpr uint16_t Verkehrsschwarz = 0x2945; ///< RAL 9017  #29292A  (RGB: 41, 41, 42)
    constexpr uint16_t Papyrusweiss = 0xCE58;    ///< RAL 9018  #C8CBC4  (RGB: 200, 203, 196)
    constexpr uint16_t Perlhellgrau = 0x8430;    ///< RAL 9022  #858583  (RGB: 133, 133, 131)
    constexpr uint16_t Perldunkelgrau = 0x7BCF;  ///< RAL 9023  #787B7A  (RGB: 120, 123, 122)
} // namespace ral

#endif // THEME_H

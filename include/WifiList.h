#ifndef WIFI_LIST_H
#define WIFI_LIST_H

#include <memory>
#include <vector>

#include "Button.h"

class LGFX;
class Gfx_Sprite;

/**
 * @struct WiFiNetwork
 * @brief Speichert Informationen über ein gefundenes WiFi-Netzwerk.
 */
struct WiFiNetwork {
    char ssid[33];    ///< SSID des Netzwerks (max. 32 Zeichen + \0)
    int32_t rssi;     ///< Signalstärke in dBm
    uint8_t channel;  ///< WiFi-Kanal
    bool encrypted;   ///< Verschlüsselung aktiv
};

/**
 * @class WifiList
 * @brief Modales Fenster zur WiFi-Netzwerk-Auswahl mit Touch-Bedienung.
 *
 * @details Zeigt verfügbare WLAN-Netzwerke nach Signalstärke sortiert in einer per Touch bedienbaren Bildlaufliste an.
 *
 * @code
 * // Beispiel-Verwendung:
 * WifiList wifiList(&gfx);
 * wifiList.setPosition();                // zentrieren
 * wifiList.setSize(600, 380);
 *
 * char selectedSSID[33];
 * if (wifiList.show("WiFi-Netzwerk auswählen:", selectedSSID)) {
 *     // OK gedrückt, selectedSSID enthält die Auswahl
 * } else {
 *     // Abgebrochen mit X
 * }
 * @endcode
 */
class WifiList {
   public:
    /**
     * @brief Konstruktor
     * @param gfx Pointer auf das LovyanGFX-Objekt für die Grafikausgabe.
     */
    explicit WifiList(LGFX* gfx);

    /**
     * @brief Destruktor - gibt alle dynamisch allokierten Ressourcen frei.
     */
    ~WifiList();

    /**
     * @brief Zeigt das modale Fenster an und wartet auf eine Benutzereingabe.
     * @param label Beschriftung, die über der Liste angezeigt wird.
     * @param selectedSSID Puffer, der die SSID der Auswahl aufnimmt (min. 33 Bytes).
     * @return true wenn "OK" gedrückt wurde, false bei Abbruch.
     */
    bool show(const char* label, char* selectedSSID);

    /**
     * @brief Setzt die Position des Fensters.
     * @param x X-Koordinate der linken oberen Ecke.
     * @param y Y-Koordinate der linken oberen Ecke.
     */
    void setPosition(int x, int y);

    /**
     * @brief Zentriert das Fenster auf der aktuellen Displaygröße.
     */
    void setPosition();

    /**
     * @brief Setzt die Größe des Fensters.
     * @param width Breite in Pixeln.
     * @param height Höhe in Pixeln.
     */
    void setSize(int width, int height);

    /**
     * @brief Setzt die Schriftart für die Einträge in der Liste.
     * @param font Pointer auf die zu verwendenden Font-Daten.
     */
    void setListFont(const uint8_t* font) { m_listFont = font; }

    /**
     * @brief Setzt die Schriftart für die Überschrift (Label).
     * @param font Pointer auf die zu verwendenden Font-Daten.
     */
    void setLabelFont(const uint8_t* font) { m_labelFont = font; }

    /**
     * @brief Setzt die Farbe der Überschrift (Label).
     * @param color 16-Bit RGB565 Farbwert.
     */
    void setLabelColor(uint16_t color) { m_labelColor = color; }

   private:
    /**
     * @struct UIConfig
     * @brief Zentrale Konfiguration für alle UI-Elemente
     */
    struct UIConfig {
        // Farben
        static constexpr uint16_t COLOR_ITEM_SELECTED = 0x328a;   ///< Ausgewählte Zeile
        static constexpr uint16_t COLOR_TEXT = 0xBDF7;            ///< Textfarbe
        static constexpr uint16_t COLOR_TEXT_SELECTED = 0xffff;   ///< Text ausgewählt
        static constexpr uint16_t COLOR_BUTTON = 0x0000;          ///< Button-Farbe
        static constexpr uint16_t COLOR_KEY_SPECIAL = 0x39E8;     ///< OK-Taste
        static constexpr uint16_t COLOR_BUTTON_PRESSED = 0x632C;  ///< Button gedrückt

        static constexpr uint16_t COLOR_BUTTON_TEXT_OK = 0xBDF7;       ///< OK-Button Textfarbe
        static constexpr uint16_t COLOR_BUTTON_TEXT_REFRESH = 0x7BEF;  ///< REFRESH-Button Textfarbe

        static constexpr uint16_t COLOR_SCROLLBAR = 0x4A49;      ///< Scrollbar-Farbe
        static constexpr uint16_t COLOR_SIGNAL_GOOD = 0x07e0;    ///< Gutes Signal
        static constexpr uint16_t COLOR_SIGNAL_MEDIUM = 0xffe0;  ///< Mittleres Signal
        static constexpr uint16_t COLOR_SIGNAL_WEAK = 0xf800;    ///< Schwaches Signal

        // Geometrie
        static constexpr int BORDER_RADIUS = 4;    ///< Radius für abgerundete Ecken
        static constexpr int BUTTON_RADIUS = 4;    ///< Radius für Buttons
        static constexpr int PADDING = 10;         ///< Innenabstand
        static constexpr int ITEM_HEIGHT = 36;     ///< Höhe einer Zeile
        static constexpr int SCROLLBAR_WIDTH = 8;  ///< Breite der Scrollbar
    };

    // Geometrie-Konstanten
    static constexpr int DEFAULT_WIDTH = 400;   ///< Standard-Breite
    static constexpr int DEFAULT_HEIGHT = 320;  ///< Standard-Höhe
    static constexpr int MIN_WIDTH = 250;       ///< Minimale Breite
    static constexpr int MIN_HEIGHT = 200;      ///< Minimale Höhe
    static constexpr int LABEL_HEIGHT = 40;     ///< Höhe des Label-Bereichs
    static constexpr int BUTTON_HEIGHT = 45;    ///< Höhe der Buttons
    static constexpr int BUTTON_WIDTH = 80;     ///< Breite der Buttons
    static constexpr int CLOSE_BTN_SIZE = 30;   ///< Größe des Schließen-Buttons

    // Private Methoden
    void initUI(const char* label);
    void cleanup();
    void scanNetworks();
    void handleTouch();

    void drawFrame();
    void drawLabel(const char* label);
    void drawList();
    void drawListItem(int index, int y, bool selected);
    void drawScrollbar();
    void showSearchingMessage();

    void backupScreen();
    void restoreScreen();

    int getIndexFromTouch(int touch_y);
    void constrainScrollOffset();
    uint16_t getSignalColor(int32_t rssi);
    void formatSignalStrength(int32_t rssi, char* buffer, size_t bufferSize);

    // Member-Variablen
    LGFX* m_tft;                     ///< LovyanGFX-Instanz
    char* m_selectedSSID = nullptr;  ///< Puffer für ausgewählte SSID
    bool m_running = false;          ///< Läuft das Fenster noch?
    bool m_result = false;           ///< Ergebnis (true = OK, false = Abbruch)

    std::vector<WiFiNetwork> m_networks;  ///< Liste der gefundenen Netzwerke
    int m_selectedIndex = -1;             ///< Index des ausgewählten Netzwerks
    int m_scrollOffset = 0;               ///< Scroll-Offset für die Liste
    int m_maxVisibleItems = 0;            ///< Anzahl sichtbarer Einträge

    std::unique_ptr<Button> m_closeButton;    ///< Schließen-Button (X)
    std::unique_ptr<Button> m_okButton;       ///< OK-Button
    std::unique_ptr<Button> m_refreshButton;  ///< Refresh-Button

    const uint8_t* m_listFont = nullptr;   ///< Schriftart für Liste (Default: ComfortaaBold18)
    const uint8_t* m_labelFont = nullptr;  ///< Schriftart für Label (Default: ComfortaaBold18)
    uint16_t m_labelColor;                 ///< Farbe für Label

    bool m_touchProcessed = false;    ///< Wurde Touch bereits verarbeitet?
    int32_t m_touchStartY = 0;        ///< Y-Position bei Touch-Start
    int32_t m_scrollStartOffset = 0;  ///< Scroll-Offset bei Touch-Start
    bool m_isScrolling = false;       ///< Wird gerade gescrollt?
    uint32_t m_lastScanTime = 0;      ///< Zeit des letzten Scans
    uint32_t m_startTime = 0;         ///< Startzeit (für Touch-Schutz)

    int m_posX;                ///< X-Position des Fensters
    int m_posY;                ///< Y-Position des Fensters
    int m_width;               ///< Breite des Fensters
    int m_height;              ///< Höhe des Fensters
    int m_listAreaHeight = 0;  ///< Höhe des Listen-Bereichs

    lgfx::rgb565_t* m_screenBuffer = nullptr;  ///< Puffer für Hintergrund-Backup
    bool m_backupValid = false;                ///< Ist das Backup gültig?
    Gfx_Sprite* m_listSprite = nullptr;        ///< Sprite für die Liste
};

#endif  // WIFI_LIST_H

#include "WifiList.h"

#include <Arduino.h>
#include <WiFi.h>
#include <esp_heap_caps.h>

#include <algorithm>
#include <cstring>

#include "Gfx.h"
#include "Theme.h"
#include "fonts/ComfortaaBold18.h"
#include "fonts/LeagueGothicCond34.h"

/**
 * @brief Konstruktor der WifiList-Klasse.
 */
WifiList::WifiList(LGFX* gfx)
    : m_tft(gfx),
      m_labelColor(Theme::ColorTitle),
      m_posX((800 - DEFAULT_WIDTH) / 2),  // Start: auf 800x480 zentriert
      m_posY((480 - DEFAULT_HEIGHT) / 2),
      m_width(DEFAULT_WIDTH),
      m_height(DEFAULT_HEIGHT) {
    m_networks.reserve(20);  // Typische Anzahl von Netzwerken
}

/**
 * @brief Destruktor
 */
WifiList::~WifiList() {
    cleanup();
}

/**
 * @brief Setzt die Position des Fensters.
 */
void WifiList::setPosition(int x, int y) {
    m_posX = x;
    m_posY = y;
}

/**
 * @brief Zentriert das Fenster dynamisch auf der aktuellen Displaygröße.
 */
void WifiList::setPosition() {
    // Sicherstellen, dass m_tft existiert und Dimensionen liefert
    int dw = m_tft ? m_tft->width() : 800;
    int dh = m_tft ? m_tft->height() : 480;
    m_posX = (dw - m_width) / 2;
    m_posY = (dh - m_height) / 2;
}

/**
 * @brief Setzt die Größe des Fensters.
 */
void WifiList::setSize(int width, int height) {
    m_width = std::max(width, MIN_WIDTH);
    m_height = std::max(height, MIN_HEIGHT);
}

/**
 * @brief Zeigt das modale Fenster an und startet die Interaktionsschleife.
 */
bool WifiList::show(const char* label, char* selectedSSID) {
    // Touch-Release abwarten
    int32_t tx, ty;
    int timeout = 0;
    while (m_tft->getTouch(&tx, &ty) && timeout < 50) {
        delay(10);
        yield();
        timeout++;
    }
    delay(100);

    // Initialisierung
    m_selectedSSID = selectedSSID;
    if (m_selectedSSID) {
        m_selectedSSID[0] = '\0';
    }
    m_selectedIndex = -1;
    m_scrollOffset = 0;
    m_running = true;
    m_result = false;
    m_touchProcessed = false;
    m_isScrolling = false;
    m_startTime = millis();

    // Listen-Bereich berechnen
    m_listAreaHeight = m_height - LABEL_HEIGHT - BUTTON_HEIGHT - (3 * UIConfig::PADDING);
    m_maxVisibleItems = m_listAreaHeight / UIConfig::ITEM_HEIGHT;

    // UI initialisieren
    initUI(label);

    // "Suche Netzwerke..." anzeigen
    showSearchingMessage();

    // Initialer Scan
    scanNetworks();

    // Ersten Eintrag automatisch markieren
    if (!m_networks.empty()) {
        m_selectedIndex = 0;
    }

    // Liste anzeigen
    drawList();
    drawScrollbar();

    // Hauptschleife
    while (m_running) {
        handleTouch();
        yield();
        delay(10);
    }

    // Aufräumen
    cleanup();

    return m_result;
}

/**
 * @brief Initialisiert die Benutzeroberfläche.
 */
void WifiList::initUI(const char* label) {
    // Hintergrund sichern
    backupScreen();

    // Rahmen zeichnen
    drawFrame();

    // Label zeichnen (vertikal zentriert zum Close-Button)
    drawLabel(label);

    // Close-Button erstellen
    m_closeButton.reset(new Button());
    int close_x = m_posX + m_width - UIConfig::PADDING - CLOSE_BTN_SIZE / 2;
    int close_y = m_posY + UIConfig::PADDING + CLOSE_BTN_SIZE / 2;

    m_closeButton->initButton(m_tft, close_x, close_y,
                              CLOSE_BTN_SIZE, CLOSE_BTN_SIZE,
                              4,
                              (uint16_t)0xF800,  // Rot
                              (uint16_t)0xF800,
                              (uint16_t)TFT_WHITE,
                              "^DEL:20^",
                              nullptr,
                              1.0f, 1.0f);
    m_closeButton->drawButton(false);

    // OK-Button erstellen
    m_okButton.reset(new Button());
    int ok_x = m_posX + m_width - UIConfig::PADDING - BUTTON_WIDTH / 2;
    int ok_y = m_posY + m_height - UIConfig::PADDING - BUTTON_HEIGHT / 2;

    m_okButton->initButton(m_tft, ok_x, ok_y,
                           BUTTON_WIDTH, BUTTON_HEIGHT,
                           UIConfig::BUTTON_RADIUS,
                           UIConfig::COLOR_KEY_SPECIAL,
                           UIConfig::COLOR_BUTTON,
                           UIConfig::COLOR_BUTTON_TEXT_OK,
                           "ok",
                           nullptr,
                           1.0f, 1.0f);
    {
        // Standard-Font erzwingen, wenn nichts gesetzt
        const uint8_t* fontToUse = m_listFont ? m_listFont : LeagueGothicCond34;
        m_okButton->setFont(fontToUse);
    }
    m_okButton->drawButton(false);

    // Refresh-Button erstellen
    m_refreshButton.reset(new Button());
    int refresh_x = ok_x - BUTTON_WIDTH - UIConfig::PADDING;
    int refresh_y = ok_y;

    m_refreshButton->initButton(m_tft, refresh_x, refresh_y,
                                BUTTON_WIDTH, BUTTON_HEIGHT,
                                UIConfig::BUTTON_RADIUS,
                                UIConfig::COLOR_KEY_SPECIAL,
                                UIConfig::COLOR_BUTTON,
                                UIConfig::COLOR_BUTTON_TEXT_REFRESH,
                                "scan",
                                nullptr,
                                1.0f, 1.0f);
    {
        const uint8_t* fontToUse = m_listFont ? m_listFont : LeagueGothicCond34;
        m_refreshButton->setFont(fontToUse);
    }
    m_refreshButton->drawButton(false);

    // Sprite für die Liste erstellen
    int listWidth = m_width - (2 * UIConfig::PADDING);
    m_tft->safeCreateSprite(m_listSprite, listWidth, m_listAreaHeight);
}

/**
 * @brief Räumt alle Ressourcen auf.
 */
void WifiList::cleanup() {
    // Buttons freigeben
    m_closeButton.reset();
    m_okButton.reset();
    m_refreshButton.reset();

    // Screen wiederherstellen
    restoreScreen();

    // Sprite löschen
    m_tft->safeDeleteSprite(m_listSprite);

    // Buffer freigeben
    if (m_screenBuffer) {
        heap_caps_free(m_screenBuffer);
        m_screenBuffer = nullptr;
    }
}

/**
 * @brief Führt einen WiFi-Scan durch und füllt die Netzwerk-Liste.
 */
void WifiList::scanNetworks() {
    // Alte Liste leeren
    m_networks.clear();
    m_selectedIndex = -1;
    m_scrollOffset = 0;

    int n = WiFi.scanNetworks(false, true, true, 300);

    if (n > 0) {
        for (int i = 0; i < n; i++) {
            WiFiNetwork net;
            strncpy(net.ssid, WiFi.SSID(i).c_str(), 32);
            net.ssid[32] = '\0';
            net.rssi = WiFi.RSSI(i);
            net.channel = WiFi.channel(i);
            net.encrypted = (WiFi.encryptionType(i) != WIFI_AUTH_OPEN);

            if (strlen(net.ssid) > 0) {
                m_networks.push_back(net);
            }
        }

        std::sort(m_networks.begin(), m_networks.end(),
                  [](const WiFiNetwork& a, const WiFiNetwork& b) {
                      return a.rssi > b.rssi;
                  });
    }

    WiFi.scanDelete();
    m_lastScanTime = millis();
}

/**
 * @brief Verarbeitet Touch-Eingaben.
 */
void WifiList::handleTouch() {
    if (millis() - m_startTime < 150) {
        return;
    }

    int32_t tx, ty;
    bool touched = m_tft->getTouch(&tx, &ty);

    if (touched) {
        if (!m_touchProcessed && !m_isScrolling) {
            if (m_closeButton && m_closeButton->contains(tx, ty)) {
                m_touchProcessed = true;
                m_closeButton->drawButton(true);
                delay(100);
                m_result = false;
                m_running = false;
                return;
            }

            if (m_okButton && m_okButton->contains(tx, ty)) {
                if (m_selectedIndex >= 0 && m_selectedIndex < (int)m_networks.size()) {
                    m_touchProcessed = true;
                    m_okButton->drawButton(true);
                    strncpy(m_selectedSSID, m_networks[m_selectedIndex].ssid, 32);
                    m_selectedSSID[32] = '\0';
                    delay(100);
                    m_result = true;
                    m_running = false;
                    return;
                }
            }

            if (m_refreshButton && m_refreshButton->contains(tx, ty)) {
                if (millis() - m_lastScanTime > 2000) {
                    m_touchProcessed = true;
                    m_refreshButton->drawButton(true);

                    showSearchingMessage();
                    scanNetworks();

                    if (!m_networks.empty()) {
                        m_selectedIndex = 0;
                    }

                    drawList();
                    drawScrollbar();

                    delay(100);
                    m_refreshButton->drawButton(false);
                    return;
                }
            }

            int list_x = m_posX + UIConfig::PADDING;
            int list_y = m_posY + LABEL_HEIGHT + UIConfig::PADDING;
            int list_width = m_width - (2 * UIConfig::PADDING);

            if (tx >= list_x && tx <= list_x + list_width &&
                ty >= list_y && ty <= list_y + m_listAreaHeight) {
                m_touchStartY = ty;
                m_scrollStartOffset = m_scrollOffset;
                m_touchProcessed = true;

                int idx = getIndexFromTouch(ty);
                if (idx >= 0) {
                    if (idx != m_selectedIndex) {
                        m_selectedIndex = idx;
                        drawList();
                    }
                }
            }
        } else if (m_touchProcessed) {
            int list_y = m_posY + LABEL_HEIGHT + UIConfig::PADDING;
            if (ty >= list_y && ty <= list_y + m_listAreaHeight) {
                int delta_y = ty - m_touchStartY;

                if (abs(delta_y) > 5) {
                    m_isScrolling = true;

                    int item_delta = -delta_y / UIConfig::ITEM_HEIGHT;
                    int new_offset = m_scrollStartOffset + item_delta;

                    if (new_offset != m_scrollOffset) {
                        m_scrollOffset = new_offset;
                        constrainScrollOffset();
                        drawList();
                        drawScrollbar();
                    }
                }
            }
        }
    } else {
        if (m_touchProcessed || m_isScrolling) {
            m_touchProcessed = false;
            m_isScrolling = false;
        }
    }
}

/**
 * @brief Zeichnet den Rahmen des Fensters.
 */
void WifiList::drawFrame() {
    m_tft->fillSmoothRoundRect(m_posX, m_posY, m_width, m_height, UIConfig::BORDER_RADIUS, Theme::ColorDialogBorder);
    m_tft->fillSmoothRoundRect(m_posX + 1, m_posY + 1, m_width - 2, m_height - 2, UIConfig::BORDER_RADIUS, Theme::ColorMainBackGround);
}

/**
 * @brief Zeichnet das Label über der Liste (vertikal am Close-Button ausgerichtet).
 */
void WifiList::drawLabel(const char* label) {
    if (!label || !*label) return;

    const uint8_t* fontToUse = m_labelFont ? m_labelFont : ComfortaaBold18;
    m_tft->loadFont(fontToUse);
    m_tft->setTextColor(m_labelColor, Theme::ColorMainBackGround);

    int label_x = m_posX + UIConfig::PADDING;
    // Vertikale Mitte am Close-Button ausrichten (Größe = CLOSE_BTN_SIZE)
    int close_center_y = m_posY + UIConfig::PADDING + (CLOSE_BTN_SIZE / 2);
    m_tft->setTextDatum(ML_DATUM);  // Middle-Left
    m_tft->drawString(label, label_x, close_center_y);

    m_tft->unloadFont();
    m_tft->setTextDatum(TL_DATUM);  // zurücksetzen
}

/**
 * @brief Zeichnet die Liste der WiFi-Netzwerke.
 */
void WifiList::drawList() {
    if (!m_listSprite) return;

    m_listSprite->clear(Theme::ColorMainBackGround);

    // Font einmal laden (nicht pro Eintrag!)
    const uint8_t* fontToUse = m_listFont ? m_listFont : ComfortaaBold18;
    m_listSprite->loadFont(fontToUse);

    if (m_networks.empty()) {
        m_listSprite->setTextColor(Theme::ColorTitle);
        m_listSprite->setTextDatum(lgfx::middle_center);
        m_listSprite->drawString("Keine Netzwerke gefunden", m_listSprite->width() / 2, m_listAreaHeight / 2);
    } else {
        int startIdx = m_scrollOffset;
        int endIdx = std::min(startIdx + m_maxVisibleItems, (int)m_networks.size());

        for (int i = startIdx; i < endIdx; i++) {
            int y = (i - startIdx) * UIConfig::ITEM_HEIGHT;
            drawListItem(i, y, i == m_selectedIndex);
        }
    }

    // Font einmal entladen (nach allen Einträgen)
    m_listSprite->unloadFont();

    int list_x = m_posX + UIConfig::PADDING;
    int list_y = m_posY + LABEL_HEIGHT + UIConfig::PADDING;
    m_listSprite->pushSprite(list_x, list_y);
}

/**
 * @brief Zeichnet einen einzelnen Listen-Eintrag (Font muss bereits geladen sein!).
 */
void WifiList::drawListItem(int index, int y, bool selected) {
    if (index < 0 || index >= (int)m_networks.size() || !m_listSprite) return;

    const WiFiNetwork& net = m_networks[index];

    // Hintergrund für selektierten Eintrag
    if (selected) {
        m_listSprite->fillRect(0, y, m_listSprite->width(), UIConfig::ITEM_HEIGHT, UIConfig::COLOR_ITEM_SELECTED);
    }

    // SSID zeichnen (Font ist bereits geladen!)
    m_listSprite->setTextColor(selected ? UIConfig::COLOR_TEXT_SELECTED : UIConfig::COLOR_TEXT);
    m_listSprite->setTextDatum(lgfx::middle_left);

    int text_y = y + UIConfig::ITEM_HEIGHT / 2;
    m_listSprite->drawString(net.ssid, 10, text_y);

    // Schloss-Symbol für verschlüsselte Netzwerke
    if (net.encrypted) {
        uint16_t lockColor = selected ? UIConfig::COLOR_TEXT_SELECTED : UIConfig::COLOR_TEXT;
        uint16_t backgroundColor = selected ? UIConfig::COLOR_ITEM_SELECTED : Theme::ColorMainBackGround;

        const int16_t y_offset = -2;
        const int16_t lockSize = 24;

        const int16_t original_left_x = m_listSprite->width() - 80;
        const int16_t lock_center_x = original_left_x + (lockSize / 2);
        const int16_t lock_center_y = (y + UIConfig::ITEM_HEIGHT / 2) + y_offset;

        m_listSprite->drawSmoothCircle(lock_center_x, lock_center_y - 3, 5, 2.0f,
                                       lockColor, backgroundColor);

        const int16_t body_w = 17;
        const int16_t body_h = 12;
        const int16_t body_x = lock_center_x - (body_w / 2);
        const int16_t body_y = lock_center_y - (lockSize / 2) + 10;
        const int16_t corner_radius = 2;

        m_listSprite->fillSmoothRoundRect(body_x, body_y, body_w, body_h,
                                          corner_radius, lockColor);
        m_listSprite->fillSmoothRoundRect(body_x + 1, body_y + 1, body_w - 2, body_h - 2,
                                          corner_radius, backgroundColor);

        m_listSprite->fillSmoothCircle(lock_center_x, lock_center_y + 3, 2, lockColor);
    }

    // Signalstärke rechts anzeigen
    char rssiStr[16];
    formatSignalStrength(net.rssi, rssiStr, sizeof(rssiStr));
    m_listSprite->setTextColor(getSignalColor(net.rssi));
    m_listSprite->setTextDatum(lgfx::middle_right);
    m_listSprite->drawString(rssiStr, m_listSprite->width() - 10, text_y);

    // Trennlinie (außer beim letzten Eintrag)
    if (!selected && index < (int)m_networks.size() - 1) {
        m_listSprite->drawFastHLine(5, y + UIConfig::ITEM_HEIGHT - 1, m_listSprite->width() - 10, 0x2104);
    }
}

/**
 * @brief Zeichnet die Scrollbar (Integer-Arithmetik, keine Floats).
 */
void WifiList::drawScrollbar() {
    int networkCount = static_cast<int>(m_networks.size());
    if (networkCount <= m_maxVisibleItems) return;

    int scrollbar_x = m_posX + m_width - UIConfig::PADDING - UIConfig::SCROLLBAR_WIDTH;
    int scrollbar_y = m_posY + LABEL_HEIGHT + UIConfig::PADDING;
    int scrollbar_height = m_listAreaHeight;

    // Hintergrund löschen
    m_tft->fillRect(scrollbar_x, scrollbar_y, UIConfig::SCROLLBAR_WIDTH,
                    scrollbar_height, Theme::ColorMainBackGround);

    // Scrollbar-Höhe berechnen (Integer-Arithmetik)
    // bar_height = scrollbar_height * m_maxVisibleItems / networkCount
    int bar_height = (scrollbar_height * m_maxVisibleItems) / networkCount;
    if (bar_height < 20) bar_height = 20;  // Minimum

    // Scrollbar-Position berechnen (Integer-Arithmetik)
    // bar_y = scrollbar_y + (scrollbar_height - bar_height) * m_scrollOffset / maxScroll
    int maxScroll = networkCount - m_maxVisibleItems;
    int bar_y = scrollbar_y;
    if (maxScroll > 0) {
        bar_y += ((scrollbar_height - bar_height) * m_scrollOffset) / maxScroll;
    }

    m_tft->fillSmoothRoundRect(scrollbar_x, bar_y, UIConfig::SCROLLBAR_WIDTH, bar_height, 3, UIConfig::COLOR_SCROLLBAR);
}

/**
 * @brief Zeigt "Suche Netzwerke..." Nachricht an.
 */
void WifiList::showSearchingMessage() {
    if (!m_listSprite) return;

    m_listSprite->clear(Theme::ColorMainBackGround);
    const uint8_t* fontToUse = m_listFont ? m_listFont : ComfortaaBold18;
    m_listSprite->loadFont(fontToUse);
    m_listSprite->setTextColor(Theme::ColorTitle);
    m_listSprite->setTextDatum(lgfx::middle_center);
    m_listSprite->drawString("Suche Netzwerke...", m_listSprite->width() / 2, m_listAreaHeight / 2);
    m_listSprite->unloadFont();

    int list_x = m_posX + UIConfig::PADDING;
    int list_y = m_posY + LABEL_HEIGHT + UIConfig::PADDING;
    m_listSprite->pushSprite(list_x, list_y);
}

/**
 * @brief Sichert den Bildschirminhalt unter dem Fenster.
 */
void WifiList::backupScreen() {
    size_t size = m_width * m_height * sizeof(lgfx::rgb565_t);
    if (!m_screenBuffer) {
        m_screenBuffer = (lgfx::rgb565_t*)heap_caps_malloc(size, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    }

    if (m_screenBuffer) {
        m_tft->readRect(m_posX, m_posY, m_width, m_height, m_screenBuffer);
        m_backupValid = true;
    }
}

/**
 * @brief Stellt den gesicherten Bildschirminhalt wieder her.
 */
void WifiList::restoreScreen() {
    if (m_backupValid && m_screenBuffer) {
        m_tft->pushImage(m_posX, m_posY, m_width, m_height, m_screenBuffer);
        m_backupValid = false;
    }
}

/**
 * @brief Berechnet den Index basierend auf Touch-Position.
 */
int WifiList::getIndexFromTouch(int touch_y) {
    int list_y = m_posY + LABEL_HEIGHT + UIConfig::PADDING;
    int relative_y = touch_y - list_y;

    if (relative_y < 0 || relative_y >= m_listAreaHeight) {
        return -1;
    }

    int item_index = relative_y / UIConfig::ITEM_HEIGHT;
    int absolute_index = m_scrollOffset + item_index;

    if (absolute_index >= 0 && absolute_index < (int)m_networks.size()) {
        return absolute_index;
    }

    return -1;
}

/**
 * @brief Begrenzt den Scroll-Offset auf gültige Werte.
 */
void WifiList::constrainScrollOffset() {
    if (m_networks.size() <= (size_t)m_maxVisibleItems) {
        m_scrollOffset = 0;
        return;
    }

    int max_offset = m_networks.size() - m_maxVisibleItems;

    if (m_scrollOffset < 0) {
        m_scrollOffset = 0;
    } else if (m_scrollOffset > max_offset) {
        m_scrollOffset = max_offset;
    }
}

/**
 * @brief Ermittelt die Farbe basierend auf der Signalstärke.
 */
uint16_t WifiList::getSignalColor(int32_t rssi) {
    if (rssi >= -50) {
        return UIConfig::COLOR_SIGNAL_GOOD;  // Sehr gut
    } else if (rssi >= -70) {
        return UIConfig::COLOR_SIGNAL_MEDIUM;  // Mittel
    } else {
        return UIConfig::COLOR_SIGNAL_WEAK;  // Schwach
    }
}

/**
 * @brief Formatiert die Signalstärke als Text.
 */
void WifiList::formatSignalStrength(int32_t rssi, char* buffer, size_t bufferSize) {
    int percent;
    if (rssi >= -50) {
        percent = 100;
    } else if (rssi >= -100) {
        percent = 2 * (rssi + 100);
    } else {
        percent = 0;
    }

    snprintf(buffer, bufferSize, "%d%%", percent);
}
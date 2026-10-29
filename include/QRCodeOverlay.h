#ifndef QR_CODE_OVERLAY_H
#define QR_CODE_OVERLAY_H

#include <stdint.h>
#include "Gfx.h"

/**
 * @class QRCodeOverlay
 * @brief Zeigt einen QR-Code als Overlay auf dem Display an.
 *
 * Sichert den Hintergrund des Anzeigebereichs mit readRect, rendert den QR-Code über QRCodeRenderer und stellt den Hintergrund beim Ausblenden mit pushImage exakt wieder her.
 *
 * Rendering-Engine: QRCodeRenderer.h (Template-Wrapper um nayuki qrcodegen, C-Variante). Der PSRAM-Allocator wird per Makro gesetzt, bevor QRCodeRenderer.h eingebunden wird; dies erfolgt in QRCodeOverlay.cpp.
 *
 * QRCodeRenderer ist auch direkt nutzbar, unabhängig von dieser Klasse:
 * @code
 *   // Direkt auf Display (auto-Skalierung in Box):
 *   QRCodeRenderer::drawCentered(tft, x, y, w, h, url, opt);
 *
 *   // Auf Sprite:
 *   QRCodeRenderer::drawCentered(mySprite, 0, 0, w, h, url, opt);
 * @endcode
 *
 * Verwendung dieses Overlays (z. B. im DayView News-Ticker):
 * @code
 *   // Einblenden - gleiche URL und sichtbar -> keine erneute Darstellung
 *   m_qrCodeOverlay.show(m_tft, articleUrl, QR_X, QR_Y, QR_BOX_SIZE);
 *
 *   // Ausblenden -> Hintergrund exakt wiederherstellen
 *   m_qrCodeOverlay.hide();
 * @endcode
 *
 * @note Benötigt in lib/qrcodegen/:
 *       qrcodegen.h, qrcodegen.c  (nayuki QR-Code-generator, MIT-Lizenz)
 *       QRCodeRenderer.h          (Template-Renderer für LovyanGFX)
 */
class QRCodeOverlay
{
public:
    QRCodeOverlay() = default;
    ~QRCodeOverlay();

    /// @brief Kein Kopieren / Verschieben
    QRCodeOverlay(const QRCodeOverlay &) = delete;
    QRCodeOverlay &operator=(const QRCodeOverlay &) = delete;

    /**
     * @brief Zeigt den QR-Code für eine URL an.
     * @param tft     Zeiger auf das LGFX-Display
     * @param url     Zu kodierende URL
     * @param x       X-Position der linken oberen Ecke auf dem Display
     * @param y       Y-Position der linken oberen Ecke auf dem Display
     * @param boxSize Seitenlänge der Box in Pixeln; QRCodeRenderer wählt
     *                automatisch die größtmögliche ganzzahlige Skalierung
     * @return true wenn der QR-Code erfolgreich angezeigt wurde
     *
     * Bei gleicher URL und bereits sichtbarem QR-Code passiert nichts (Change-Detection). Bei neuer URL wird der vorherige Hintergrund wiederhergestellt, der neue gesichert und der neue Code gerendert.
     */
    bool show(LGFX *tft, const char *url, int x, int y, int boxSize = 120);

    /**
     * @brief Versteckt den QR-Code und stellt den gesicherten Hintergrund exakt wieder her.
     *
     * Gibt den PSRAM-Hintergrundpuffer frei. Ist kein QR-Code sichtbar, ist der Aufruf wirkungslos.
     */
    void hide();

    /**
     * @brief Gibt an ob aktuell ein QR-Code angezeigt wird.
     * @return true wenn sichtbar
     */
    bool isVisible() const { return m_visible; }

    /**
     * @brief Zeichnet den QR-Code erneut, ohne den Hintergrund mit readRect zu sichern.
     * @note redraw() füllt die gesamte Box zunächst weiß und legt anschließend den QR-Code darüber. Der weiße Hintergrund und die weiße Ruhezone sichern die zuverlässige Lesbarkeit. hide() stellt den in m_bgBuffer gesicherten Hintergrund mit pushImage wieder her.
     *
     * Muss nach jedem drawTasksSection()-Aufruf ausgeführt werden, damit das Tasks-Sprite (x=401..790) den QR-Bereich (x=610..790) nicht dauerhaft überschreibt. Ist kein QR-Code sichtbar, beendet sich der Aufruf sofort.
     */
    void redraw();

private:
    LGFX *m_tft = nullptr;                ///< Display-Zeiger (nicht freigeben)
    lgfx::rgb565_t *m_bgBuffer = nullptr; ///< PSRAM-Puffer für gesicherten Hintergrund
    uint8_t *m_qrBitmap = nullptr;        ///< PSRAM-Puffer für kodiertes QR-Bitmap (persistent für redraw)
    int m_x = 0;                          ///< Linke obere Ecke X (Display-Koordinaten)
    int m_y = 0;                          ///< Linke obere Ecke Y (Display-Koordinaten)
    int m_boxSize = 0;                    ///< Gesicherte Box-Seitenlänge in Pixeln
    int m_modules = 0;                    ///< Anzahl QR-Module (gecacht für redraw)
    int m_pixelSize = 0;                  ///< Pixel pro Modul (gecacht für redraw)
    int m_offsetX = 0;                    ///< X-Offset der Quiet-Zone in Pixeln (gecacht)
    int m_offsetY = 0;                    ///< Y-Offset der Quiet-Zone in Pixeln (gecacht)
    bool m_visible = false;               ///< Sichtbarkeitsstatus
    char m_currentUrl[200] = {};          ///< URL des aktuell gezeigten QR-Codes (Change-Detection)
};

#endif // QR_CODE_OVERLAY_H

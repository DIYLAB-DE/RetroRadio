#include "QRCodeOverlay.h"

#include <string.h>
#include <algorithm>
#include "Logger.h"
#include "StringUtils.h"

// nayuki QR-Code-generator
extern "C"
{
#include "qrcodegen.h"
}

// ===================================================================
// Konstanten
// ===================================================================

namespace
{
    constexpr uint16_t COLOR_QR_FG = 0x0000; ///< QR-Module: schwarz
    constexpr uint16_t COLOR_QR_BG = 0xFFFF; ///< Hintergrund + Quiet-Zone: weiß
    constexpr int QUIET_MODULES = 4;         ///< ISO-Norm: 4 Module Quiet-Zone
}

// ===================================================================
// Destruktor
// ===================================================================

/**
 * @brief Gibt Ressourcen frei und stellt den Hintergrund wieder her
 */
QRCodeOverlay::~QRCodeOverlay()
{
    hide();
}

// ===================================================================
// Öffentliche Methoden
// ===================================================================

/**
 * @brief Zeigt den QR-Code für die gegebene URL an
 */
bool QRCodeOverlay::show(LGFX *tft, const char *url, int x, int y, int boxSize)
{
    if (!tft || !url || url[0] == '\0' || boxSize < 10)
        return false;

    // Change-Detection: alle rendering-relevanten Parameter vergleichen. Nur wenn URL, Position, Boxgröße UND Display-Target identisch sind, ist ein Re-Render überflüssig. Jede Abweichung (andere Position, anderer tft-Zeiger, andere Boxgröße) erfordert ein vollständiges Neu-Rendering.
    if (m_visible && tft == m_tft && x == m_x && y == m_y && boxSize == m_boxSize && strcmp(m_currentUrl, url) == 0)
    {
        return true;
    }

    // Vorherigen QR-Code ausblenden (Hintergrund wiederherstellen)
    if (m_visible)
        hide();

    LOG_INFO_F("QRCodeOverlay: Kodiere %d Zeichen: %s", (int)strlen(url), url);

    // Encoding-Puffer im PSRAM allokieren: qrcodegen_BUFFER_LEN_MAX ist immer ausreichend für alle 40 Versionen. Zwei Puffer à 3918 Bytes = ~7.7 KB - nur für die Dauer dieser Funktion. WICHTIG: Kein boostEcl! boostEcl sucht das kleinste Version/ECC-Kombi das irgendwie passt - und landet so bei ECC-Low (nur 7% Fehlerkorrektur) wenn ECC-Medium eine höhere Version bräuchte. ECC-Medium garantiert 15%.

    uint8_t *qrcode = static_cast<uint8_t *>(heap_caps_malloc(qrcodegen_BUFFER_LEN_MAX, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT));
    uint8_t *tempBuffer = static_cast<uint8_t *>(heap_caps_malloc(qrcodegen_BUFFER_LEN_MAX, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT));

    if (!qrcode || !tempBuffer)
    {
        // Fallback auf internen Heap
        free(qrcode);
        free(tempBuffer);
        qrcode = static_cast<uint8_t *>(malloc(qrcodegen_BUFFER_LEN_MAX));
        tempBuffer = static_cast<uint8_t *>(malloc(qrcodegen_BUFFER_LEN_MAX));

        if (!qrcode || !tempBuffer)
        {
            free(qrcode);
            free(tempBuffer);
            LOG_ERROR("QRCodeOverlay: Kein Speicher für Encoding-Puffer");
            return false;
        }

        LOG_WARNING("QRCodeOverlay: Encoding-Puffer im internen Heap (PSRAM erschöpft)");
    }

    // QR-Code generieren:
    // ECC_MEDIUM: 15% Fehlerkorrektur.
    // boostEcl = false: nayuki MUSS ECC-Medium verwenden, keine stille Degradierung auf ECC-Low um eine kleinere Version zu erzwingen.
    // VERSION_MIN..VERSION_MAX: kleinste passende Version wird automatisch gewählt.

    const bool ok = qrcodegen_encodeText(
        url,
        tempBuffer,
        qrcode,
        qrcodegen_Ecc_MEDIUM,
        qrcodegen_VERSION_MIN,
        qrcodegen_VERSION_MAX,
        qrcodegen_Mask_AUTO,
        false); // <- boostEcl = false

    free(tempBuffer);

    if (!ok)
    {
        free(qrcode);
        LOG_WARNING_F("QRCodeOverlay: Encoding fehlgeschlagen (URL evtl. zu lang: %d Zeichen)", (int)strlen(url));
        return false;
    }

    const int modules = qrcodegen_getSize(qrcode);
    const int version = (modules - 17) / 4;
    const int quietPx = QUIET_MODULES; // in Modulen (wird mit pixelSize multipliziert)

    // Größte ganzzahlige Skalierung die in die Box passt
    const int totalMods = modules + 2 * quietPx;
    const int pixelSize = std::max(1, boxSize / totalMods);
    const int renderSize = totalMods * pixelSize;   // tatsächliche Render-Größe ≤ boxSize
    const int offsetX = (boxSize - renderSize) / 2; // zentrieren in Box
    const int offsetY = (boxSize - renderSize) / 2;

    LOG_INFO_F("QRCodeOverlay: Version %d, ECC-Medium, %d Module, Scale %d, %dx%d px (Box %d px)", version, modules, pixelSize, renderSize, renderSize, boxSize);

    // Hintergrund sichern:
    const size_t bgBytes = static_cast<size_t>(boxSize * boxSize) * sizeof(lgfx::rgb565_t);
    m_bgBuffer = static_cast<lgfx::rgb565_t *>(heap_caps_malloc(bgBytes, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT));

    if (!m_bgBuffer)
    {
        // Ersatzweise internen Heap verwenden
        m_bgBuffer = static_cast<lgfx::rgb565_t *>(malloc(bgBytes));
        if (!m_bgBuffer)
        {
            free(qrcode);
            LOG_ERROR_F("QRCodeOverlay: Kein Speicher für Hintergrundpuffer (%d Bytes)", (int)bgBytes);
            return false;
        }

        LOG_WARNING_F("QRCodeOverlay: Hintergrundpuffer im internen Heap (%d Bytes)", (int)bgBytes);
    }

    tft->readRect(x, y, boxSize, boxSize, m_bgBuffer);

    // QR-Code zeichnen:
    tft->fillRect(x, y, boxSize, boxSize, COLOR_QR_BG);

    // Run-Length-Rendering zeilenweise: Aufeinanderfolgende dunkle Module werden zu einem einzigen fillRect zusammengefasst.
    const int drawX = x + offsetX + quietPx * pixelSize;
    const int drawY = y + offsetY + quietPx * pixelSize;

    for (int my = 0; my < modules; my++)
    {
        int runStart = -1;

        for (int mx = 0; mx <= modules; mx++)
        {
            const bool dark = (mx < modules) && qrcodegen_getModule(qrcode, mx, my);

            if (dark)
            {
                if (runStart < 0)
                    runStart = mx;
            }
            else if (runStart >= 0)
            {
                tft->fillRect(drawX + runStart * pixelSize, drawY + my * pixelSize, (mx - runStart) * pixelSize, pixelSize, COLOR_QR_FG);
                runStart = -1;
            }
        }
    }

    // Bitmap & Render-Parameter als Member persistieren. qrcode-Puffer NICHT freigeben - er wird für redraw() benötigt. Alten Bitmap-Puffer vorher freigeben (falls eine neue URL gerendert wird).
    free(m_qrBitmap);
    m_qrBitmap = qrcode; // Eigentumsübergabe - Freigabe in hide()
    m_modules = modules;
    m_pixelSize = pixelSize;
    m_offsetX = offsetX;
    m_offsetY = offsetY;

    // Zustand sichern:
    m_tft = tft;
    m_x = x;
    m_y = y;
    m_boxSize = boxSize;
    m_visible = true;
    stringutils::safeCopy(m_currentUrl, url);
    return true;
}

/**
 * @brief Versteckt den QR-Code und stellt den gesicherten Hintergrund wieder her
 */
void QRCodeOverlay::hide()
{
    // Hintergrund nur wiederherstellen wenn alle Voraussetzungen erfüllt sind. free(nullptr) ist per C-Standard definiert ein No-Op -> bedingungslose Freigabe und Reset funktionieren unabhängig vom aktuellen Teilzustand.
    if (m_visible && m_tft && m_bgBuffer)
        m_tft->pushImage(m_x, m_y, m_boxSize, m_boxSize, m_bgBuffer);

    free(m_bgBuffer);
    free(m_qrBitmap);
    m_bgBuffer = nullptr;
    m_qrBitmap = nullptr;
    m_tft = nullptr;
    m_visible = false;
    m_modules = 0;
    m_pixelSize = 0;
    m_offsetX = 0;
    m_offsetY = 0;
    m_x = 0;
    m_y = 0;
    m_boxSize = 0;
    m_currentUrl[0] = '\0';
}

// ===================================================================
// Redraw (ohne readRect - nur erneutes Rendering des gecachten Bitmaps)
// ===================================================================

/**
 * @brief Zeichnet den QR-Code erneut ohne Hintergrund-Save (weiße Karte, kein transparentes Layer).
 */
void QRCodeOverlay::redraw()
{
    // Ausstieg: kein QR sichtbar oder Bitmap fehlt
    if (!m_visible || !m_tft || !m_qrBitmap || m_modules == 0 || m_pixelSize == 0)
        return;

    constexpr int quietPx = QUIET_MODULES;

    // Gesamtfläche weiß (Quiet-Zone + Zentrierung)
    m_tft->fillRect(m_x, m_y, m_boxSize, m_boxSize, COLOR_QR_BG);

    // Lauflängen-Rendering auf dem zwischengespeicherten Bitmap
    const int drawX = m_x + m_offsetX + quietPx * m_pixelSize;
    const int drawY = m_y + m_offsetY + quietPx * m_pixelSize;

    for (int my = 0; my < m_modules; my++)
    {
        int runStart = -1;

        for (int mx = 0; mx <= m_modules; mx++)
        {
            const bool dark = (mx < m_modules) && qrcodegen_getModule(m_qrBitmap, mx, my);

            if (dark)
            {
                if (runStart < 0)
                    runStart = mx;
            }
            else if (runStart >= 0)
            {
                m_tft->fillRect(drawX + runStart * m_pixelSize, drawY + my * m_pixelSize, (mx - runStart) * m_pixelSize, m_pixelSize, COLOR_QR_FG);
                runStart = -1;
            }
        }
    }
}

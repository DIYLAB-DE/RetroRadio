#pragma once

#include <Arduino.h>
#include <stdlib.h>
#include <string.h>
#include "qrcodegen.h"

/**
 * @brief Kleiner QR-Code-Renderer für LovyanGFX-kompatible Displays/Sprites.
 *
 * Verwendet die C-Variante von Nayuki qrcodegen.
 *
 * Vorteile:
 * - funktioniert mit Display und Sprite (Template-Parameter TGfx)
 * - Run-Length-Rendering
 * - optional zentriertes Rendern in eine Box
 * - Hintergrund/Quiet-Zone werden korrekt gezeichnet
 *
 * Voraussetzungen:
 * - qrcodegen.h / qrcodegen.c  (nayuki QR-Code-generator, MIT-Lizenz)
 *
 * Allocator überschreiben (z. B. PSRAM), bevor dieser Header eingebunden wird:
 *
 *   #define QR_CODE_RENDERER_MALLOC(sz) heap_caps_malloc((sz), MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT)
 *   #define QR_CODE_RENDERER_FREE(ptr)  free((ptr))
 *   #include "QRCodeRenderer.h"
 *
 * @note Ecc, Options und Result sind als eigenständige Typen außerhalb der
 *       Klasse definiert (QRCodeRendererEcc, QRCodeRendererOptions,
 *       QRCodeRendererResult) und werden innerhalb als Aliase bereitgestellt.+-
 *       Hintergrund: C++11 erlaubt keine Default-Member-Initializer in nested
 *       Structs wenn diese als Default-Argumente der äußeren Klasse verwendet
 *       werden; C++14 hebt diese Einschränkung auf. Die Implementierung verwendet den C++11-kompatiblen Weg.
 */

#ifndef QR_CODE_RENDERER_MALLOC
#define QR_CODE_RENDERER_MALLOC(sz) malloc(sz)
#endif

#ifndef QR_CODE_RENDERER_FREE
#define QR_CODE_RENDERER_FREE(ptr) free(ptr)
#endif

// ============================================================================
// Unterstützende Typen
// ============================================================================

/**
 * @brief Error-Correction-Level.
 */
enum class QRCodeRendererEcc : uint8_t
{
    Low,
    Medium,
    Quartile,
    High
};

/**
 * @brief Optionen für QR-Code-Erzeugung und Darstellung.
 */
struct QRCodeRendererOptions
{
    QRCodeRendererEcc ecc = QRCodeRendererEcc::Medium;
    int borderModules = 4;      ///< Quiet-Zone in Modulen (ISO: mind. 4)
    uint16_t fgColor = 0x0000;  ///< Vordergrundfarbe (schwarz)
    uint16_t bgColor = 0xFFFF;  ///< Hintergrundfarbe inkl. Quiet-Zone (weiß)
    bool drawBackground = true; ///< Gesamte Fläche inkl. Quiet-Zone löschen
    bool boostEcl = true;       ///< Internen ECL-Boost erlauben
    int minVersion = qrcodegen_VERSION_MIN;
    int maxVersion = qrcodegen_VERSION_MAX;
};

/**
 * @brief Ergebnis- und Maßdaten des erzeugten QR-Codes.
 */
struct QRCodeRendererResult
{
    bool ok = false;
    int version = 0;
    int qrSize = 0;       ///< Modul-Raster ohne Quiet-Zone
    int totalModules = 0; ///< Modul-Raster inkl. Quiet-Zone
    int scale = 0;        ///< Display-Pixel pro Modul
    int width = 0;        ///< Gesamtbreite in Pixeln
    int height = 0;       ///< Gesamthöhe in Pixeln
};

// ============================================================================
// QRCodeRenderer
// ============================================================================

class QRCodeRenderer
{
public:
    // Aliase damit QRCodeRenderer::Ecc / ::Options / ::Result weiter funktionieren
    using Ecc = QRCodeRendererEcc;
    using Options = QRCodeRendererOptions;
    using Result = QRCodeRendererResult;

    /**
     * @brief Ermittelt die Maße eines QR-Codes für gegebenen Text und feste Skalierung.
     */
    static Result measure(const char *text, int scale, const Options &options = Options())
    {
        Result result;
        if (text == nullptr || *text == '\0' || scale < 1)
            return result;

        Options opt = options;
        sanitizeOptions(opt);

        EncodedQrData data;
        if (!encodeText(text, opt, data))
            return result;

        fillResult(result, data.qrSize, scale, opt.borderModules);
        result.ok = true;
        return result;
    }

    /**
     * @brief Ermittelt automatisch die größte ganzzahlige Skalierung die in die Box passt.
     * @return 0 wenn der QR-Code nicht hineinpasst oder nicht erzeugt werden konnte.
     */
    static int calcScaleForBounds(const char *text, int maxWidth, int maxHeight, const Options &options = Options())
    {
        if (text == nullptr || *text == '\0' || maxWidth <= 0 || maxHeight <= 0)
            return 0;

        Options opt = options;
        sanitizeOptions(opt);

        EncodedQrData data;
        if (!encodeText(text, opt, data))
            return 0;

        return calcScaleForTotalModules(data.totalModules, maxWidth, maxHeight);
    }

    /**
     * @brief Zeichnet einen QR-Code an feste Position mit fester Skalierung.
     * @tparam TGfx LovyanGFX-Display oder LGFX_Sprite
     */
    template <typename TGfx>
    static bool draw(TGfx &gfx, int x, int y, const char *text, int scale, const Options &options = Options(), Result *outResult = nullptr)
    {
        resetResult(outResult);

        if (text == nullptr || *text == '\0' || scale < 1)
            return false;

        Options opt = options;
        sanitizeOptions(opt);

        EncodedQrData data;
        if (!encodeText(text, opt, data))
            return false;

        renderEncoded(gfx, x, y, data.qrBuffer, data.qrSize, scale, opt);

        if (outResult != nullptr)
        {
            fillResult(*outResult, data.qrSize, scale, opt.borderModules);
            outResult->ok = true;
        }

        return true;
    }

    /**
     * @brief Zeichnet einen QR-Code mittig in eine vorgegebene Box.
     * Die größtmögliche ganzzahlige Skalierung wird automatisch ermittelt.
     * @tparam TGfx LovyanGFX-Display oder LGFX_Sprite
     */
    template <typename TGfx>
    static bool drawCentered(TGfx &gfx, int boxX, int boxY, int boxW, int boxH, const char *text, const Options &options = Options(), Result *outResult = nullptr)
    {
        resetResult(outResult);

        if (boxW <= 0 || boxH <= 0 || text == nullptr || *text == '\0')
            return false;

        Options opt = options;
        sanitizeOptions(opt);

        EncodedQrData data;
        if (!encodeText(text, opt, data))
            return false;

        const int scale = calcScaleForTotalModules(data.totalModules, boxW, boxH);
        if (scale < 1)
            return false;

        Result m;
        fillResult(m, data.qrSize, scale, opt.borderModules);
        m.ok = true;

        const int px = boxX + ((boxW - m.width) / 2);
        const int py = boxY + ((boxH - m.height) / 2);

        renderEncoded(gfx, px, py, data.qrBuffer, data.qrSize, scale, opt);

        if (outResult != nullptr)
            *outResult = m;

        return true;
    }

    /**
     * @brief Komfort-Overload für Arduino-String.
     */
    template <typename TGfx>
    static bool draw(TGfx &gfx, int x, int y, const String &text, int scale, const Options &options = Options(), Result *outResult = nullptr)
    {
        return draw(gfx, x, y, text.c_str(), scale, options, outResult);
    }

    /**
     * @brief Komfort-Overload für Arduino-String, zentriert.
     */
    template <typename TGfx>
    static bool drawCentered(TGfx &gfx, int boxX, int boxY, int boxW, int boxH, const String &text, const Options &options = Options(), Result *outResult = nullptr)
    {
        return drawCentered(gfx, boxX, boxY, boxW, boxH, text.c_str(), options, outResult);
    }

private:
    struct EncodedQrData
    {
        uint8_t *tempBuffer = nullptr;
        uint8_t *qrBuffer = nullptr;
        int qrSize = 0;
        int totalModules = 0;

        ~EncodedQrData()
        {
            releaseBuffers(tempBuffer, qrBuffer);
        }

        EncodedQrData() {}

        EncodedQrData(const EncodedQrData &) = delete;
        EncodedQrData &operator=(const EncodedQrData &) = delete;
    };

    static qrcodegen_Ecc mapEcc(Ecc ecc)
    {
        switch (ecc)
        {
        case Ecc::Low:
            return qrcodegen_Ecc_LOW;
        case Ecc::Medium:
            return qrcodegen_Ecc_MEDIUM;
        case Ecc::Quartile:
            return qrcodegen_Ecc_QUARTILE;
        case Ecc::High:
            return qrcodegen_Ecc_HIGH;
        default:
            return qrcodegen_Ecc_MEDIUM;
        }
    }

    static void sanitizeOptions(Options &opt)
    {
        if (opt.borderModules < 0)
            opt.borderModules = 0;

        if (opt.minVersion < qrcodegen_VERSION_MIN)
            opt.minVersion = qrcodegen_VERSION_MIN;

        if (opt.maxVersion > qrcodegen_VERSION_MAX)
            opt.maxVersion = qrcodegen_VERSION_MAX;

        if (opt.maxVersion < opt.minVersion)
            opt.maxVersion = opt.minVersion;
    }

    static int versionFromSize(int qrSize)
    {
        if (qrSize < 21)
            return 0;
        return (qrSize - 17) / 4;
    }

    static int calcScaleForTotalModules(int totalModules, int maxWidth, int maxHeight)
    {
        if (totalModules <= 0 || maxWidth <= 0 || maxHeight <= 0)
            return 0;

        const int scaleX = maxWidth / totalModules;
        const int scaleY = maxHeight / totalModules;
        const int scale = (scaleX < scaleY) ? scaleX : scaleY;

        return (scale >= 1) ? scale : 0;
    }

    static void resetResult(Result *outResult)
    {
        if (outResult != nullptr)
            *outResult = Result();
    }

    static void fillResult(Result &result, int qrSize, int scale, int borderModules)
    {
        const int totalModules = qrSize + (borderModules * 2);
        const int fullSizePx = totalModules * scale;

        result.version = versionFromSize(qrSize);
        result.qrSize = qrSize;
        result.totalModules = totalModules;
        result.scale = scale;
        result.width = fullSizePx;
        result.height = fullSizePx;
    }

    static bool encodeText(const char *text, const Options &options, EncodedQrData &outData)
    {
        outData.qrSize = 0;
        outData.totalModules = 0;

        if (text == nullptr || *text == '\0')
            return false;

        const int bufSize = qrcodegen_BUFFER_LEN_FOR_VERSION(options.maxVersion);
        if (bufSize <= 0)
            return false;

        outData.tempBuffer = static_cast<uint8_t *>(QR_CODE_RENDERER_MALLOC(static_cast<size_t>(bufSize)));
        outData.qrBuffer = static_cast<uint8_t *>(QR_CODE_RENDERER_MALLOC(static_cast<size_t>(bufSize)));

        if (outData.tempBuffer == nullptr || outData.qrBuffer == nullptr)
        {
            releaseBuffers(outData.tempBuffer, outData.qrBuffer);
            return false;
        }

        memset(outData.tempBuffer, 0, static_cast<size_t>(bufSize));
        memset(outData.qrBuffer, 0, static_cast<size_t>(bufSize));

        const bool ok = qrcodegen_encodeText(text, outData.tempBuffer, outData.qrBuffer, mapEcc(options.ecc), options.minVersion, options.maxVersion, qrcodegen_Mask_AUTO, options.boostEcl);
        if (!ok)
        {
            releaseBuffers(outData.tempBuffer, outData.qrBuffer);
            return false;
        }

        outData.qrSize = qrcodegen_getSize(outData.qrBuffer);
        outData.totalModules = outData.qrSize + (options.borderModules * 2);
        return true;
    }

    template <typename TGfx>
    static void renderEncoded(TGfx &gfx, int x, int y, const uint8_t *qrBuffer, int qrSize, int scale, const Options &opt)
    {
        if (qrBuffer == nullptr || qrSize <= 0 || scale < 1)
            return;

        const int totalModules = qrSize + (opt.borderModules * 2);
        const int fullSizePx = totalModules * scale;

        if (opt.drawBackground)
            gfx.fillRect(x, y, fullSizePx, fullSizePx, opt.bgColor);

        // Run-Length-Rendering zeilenweise: fasst aufeinanderfolgende dunkle Module zu einem einzigen fillRect zusammen
        for (int yy = 0; yy < qrSize; ++yy)
        {
            int runStart = -1;

            for (int xx = 0; xx <= qrSize; ++xx)
            {
                const bool dark = (xx < qrSize) ? qrcodegen_getModule(qrBuffer, xx, yy) : false;

                if (dark)
                {
                    if (runStart < 0)
                        runStart = xx;
                }
                else if (runStart >= 0)
                {
                    const int runLen = xx - runStart;
                    gfx.fillRect(x + ((runStart + opt.borderModules) * scale), y + ((yy + opt.borderModules) * scale), runLen * scale, scale, opt.fgColor);
                    runStart = -1;
                }
            }
        }
    }

    static void releaseBuffers(uint8_t *&tempBuffer, uint8_t *&qrBuffer)
    {
        if (tempBuffer != nullptr)
        {
            QR_CODE_RENDERER_FREE(tempBuffer);
            tempBuffer = nullptr;
        }

        if (qrBuffer != nullptr)
        {
            QR_CODE_RENDERER_FREE(qrBuffer);
            qrBuffer = nullptr;
        }
    }
};

#ifndef STRING_UTILS_H
#define STRING_UTILS_H

#include <cctype>   // isalnum
#include <stdarg.h>
#include <stddef.h> // size_t
#include <stdio.h>  // snprintf
#include <string.h> // strncpy, strlen

namespace stringutils
{

    /**
     * sicheres Kopieren (Array-Überladung)
     */
    template <size_t N>
    inline void safeCopy(char (&dst)[N], const char *src)
    {
        if (N == 0)
            return;

        if (!src)
        {
            dst[0] = '\0';
            return;
        }

        strncpy(dst, src, N - 1);
        dst[N - 1] = '\0';
    }

    /**
     * sicheres Kopieren (Pointer, Größe)
     */
    inline void safeCopy(char *dst, size_t dstSize, const char *src)
    {
        if (!dst || dstSize == 0)
            return;

        if (!src)
        {
            dst[0] = '\0';
            return;
        }

        strncpy(dst, src, dstSize - 1);
        dst[dstSize - 1] = '\0';
    }

    /**
     * sicheres Anhängen (Array-Überladung)
     */
    template <size_t N>
    inline void safeAppend(char (&dst)[N], const char *src)
    {
        if (N == 0)
            return;

        if (!src)
            return;

        size_t len = strnlen(dst, N);
        if (len >= N - 1)
            return;

        size_t space = (N - 1) - len;
        strncpy(dst + len, src, space);
        dst[N - 1] = '\0';
    }

    /**
     * sicheres Anhängen (Pointer, Größe)
     */
    inline void safeAppend(char *dst, size_t dstSize, const char *src)
    {
        if (!dst || dstSize == 0 || !src)
            return;

        size_t len = strnlen(dst, dstSize);
        if (len >= dstSize - 1)
            return;

        size_t space = (dstSize - 1) - len;
        strncpy(dst + len, src, space);
        dst[dstSize - 1] = '\0';
    }

    /**
     * sicheres Formatieren (void-Variante)
     */
    template <size_t N>
    inline void safeFormat(char (&dst)[N], const char *fmt, ...)
    {
        if (N == 0)
            return;

        va_list ap;
        va_start(ap, fmt);
        vsnprintf(dst, N, fmt, ap);
        va_end(ap);
    }

    /**
     * sicheres Formatieren (Pointer, Größe) (void-Variante)
     */
    inline void safeFormat(char *dst, size_t dstSize, const char *fmt, ...)
    {
        if (!dst || dstSize == 0)
            return;

        va_list ap;
        va_start(ap, fmt);
        vsnprintf(dst, dstSize, fmt, ap);
        va_end(ap);
    }

    /**
     * sicheres Formatieren mit Rückgabewert (size_t-Variante)
     */
    template <size_t N>
    inline size_t safeFormatReturn(char (&dst)[N], const char *fmt, ...)
    {
        if (N == 0)
            return 0;

        va_list ap;
        va_start(ap, fmt);
        int ret = vsnprintf(dst, N, fmt, ap);
        va_end(ap);
        return (ret < 0) ? 0 : static_cast<size_t>(ret);
    }

    /**
     * sicheres Formatieren (Pointer, Größe) mit Rückgabewert (size_t-Variante)
     */
    inline size_t safeFormatReturn(char *dst, size_t dstSize, const char *fmt, ...)
    {
        if (!dst || dstSize == 0)
            return 0;

        va_list ap;
        va_start(ap, fmt);
        int ret = vsnprintf(dst, dstSize, fmt, ap);
        va_end(ap);
        return (ret < 0) ? 0 : static_cast<size_t>(ret);
    }

    /**
     * @brief URL-Kodierung für HTTP-Parameter
     * @param in Eingabe-String
     * @param out Ausgabe-Buffer
     * @param outSize Größe des Ausgabe-Buffers
     * @details Codiert Sonderzeichen gemäß RFC 3986
     */
    inline void urlEncode(const char *in, char *out, size_t outSize)
    {
        if (!in || !out || outSize == 0)
        {
            if (out && outSize > 0)
                out[0] = '\0';
            return;
        }

        constexpr const char *hex = "0123456789ABCDEF";
        size_t pos = 0;

        while (*in && pos + 1 < outSize)
        {
            const unsigned char c = static_cast<unsigned char>(*in++);

            // Unreserved characters (RFC 3986)
            if (std::isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~')
            {
                out[pos++] = static_cast<char>(c);
            }
            else if (c == ' ')
            {
                // Platz für "%20" prüfen
                if (pos + 3 < outSize)
                {
                    out[pos++] = '%';
                    out[pos++] = '2';
                    out[pos++] = '0';
                }
                else
                {
                    break; // Buffer voll
                }
            }
            else
            {
                // Platz für "%XX" prüfen
                if (pos + 3 < outSize)
                {
                    out[pos++] = '%';
                    out[pos++] = hex[c >> 4];
                    out[pos++] = hex[c & 0x0F];
                }
                else
                {
                    break; // Buffer voll
                }
            }
        }

        out[pos] = '\0';
    }

    /**
     * @brief Ersetzt UTF-8 Umlaute durch ASCII-Äquivalente
     * @param in Eingabe-String (UTF-8)
     * @param out Ausgabe-Buffer (ASCII)
     * @param outsz Größe des Ausgabe-Buffers
     */
    inline void replaceUmlautsUtf8ToAscii(const char *in, char *out, size_t outsz)
    {
        if (!in || !out || outsz == 0)
        {
            if (out && outsz > 0)
                out[0] = '\0';
                
            return;
        }

        size_t pos = 0;
        const unsigned char *p = reinterpret_cast<const unsigned char *>(in);

        while (*p && pos + 1 < outsz)
        {
            // UTF-8 Zwei-Byte-Sequenzen für deutsche Umlaute
            if (p[0] == 0xC3 && p[1])
            {
                switch (p[1])
                {
                case 0xA4: // ä
                    if (pos + 2 < outsz)
                    {
                        out[pos++] = 'a';
                        out[pos++] = 'e';
                    }
                    p += 2;
                    continue;
                case 0x84: // Ä
                    if (pos + 2 < outsz)
                    {
                        out[pos++] = 'A';
                        out[pos++] = 'e';
                    }
                    p += 2;
                    continue;
                case 0xB6: // ö
                    if (pos + 2 < outsz)
                    {
                        out[pos++] = 'o';
                        out[pos++] = 'e';
                    }
                    p += 2;
                    continue;
                case 0x96: // Ö
                    if (pos + 2 < outsz)
                    {
                        out[pos++] = 'O';
                        out[pos++] = 'e';
                    }
                    p += 2;
                    continue;
                case 0xBC: // ü
                    if (pos + 2 < outsz)
                    {
                        out[pos++] = 'u';
                        out[pos++] = 'e';
                    }
                    p += 2;
                    continue;
                case 0x9C: // Ü
                    if (pos + 2 < outsz)
                    {
                        out[pos++] = 'U';
                        out[pos++] = 'e';
                    }
                    p += 2;
                    continue;
                case 0x9F: // ß
                    if (pos + 2 < outsz)
                    {
                        out[pos++] = 's';
                        out[pos++] = 's';
                    }
                    p += 2;
                    continue;
                default:
                    break;
                }
            }

            // Normale ASCII-Zeichen
            if (*p < 128)
            {
                out[pos++] = static_cast<char>(*p);
                ++p;
            }
            else
            {
                // Nicht-ASCII, überspringe oder ersetze mit '?'
                out[pos++] = '?';
                ++p;
            }
        }

        out[pos] = '\0';
    }
}

#endif // STRING_UTILS_H
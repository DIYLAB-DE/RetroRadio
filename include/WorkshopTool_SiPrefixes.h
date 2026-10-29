#ifndef WORKSHOP_TOOL_SI_PREFIXES_H
#define WORKSHOP_TOOL_SI_PREFIXES_H

// ============================================================================
// Referenztabelle der SI-Präfixe
// ============================================================================

#include "Strings_workshop.h"
#include "WorkshopTool.h"

/**
 * @brief Referenztabelle der neun elektronikrelevanten SI-Präfixe von Tera bis Piko.
 *
 * Zeigt Name, Symbol, Zehnerpotenz ("10^X") und ein typisches Anwendungsbeispiel in einer 4-Spalten-Tabelle mit 10 px Abstand zum oberen Rand.
 *
 * Interaktion: Antippen einer Zeile (Auslösung beim Release) markiert sie und zeigt im Info-Balken unten die Umrechnungsrelationen zu den Nachbar-Präfixen:
 *   "1 M = 1000 k   ·   1 G = 1000 M"
 * Zweites Antippen derselben Zeile hebt die Markierung auf.
 */
class WorkshopTool_SiPrefixes : public WorkshopTool
{
public:
    WorkshopTool_SiPrefixes() = default;
    ~WorkshopTool_SiPrefixes() override = default;

    WorkshopTool_SiPrefixes(const WorkshopTool_SiPrefixes &) = delete;
    WorkshopTool_SiPrefixes &operator=(const WorkshopTool_SiPrefixes &) = delete;

    /**
     * @brief Setzt den internen Zustand zurück.
     * @param tft Zeiger auf das Display-Objekt (wird nicht zwischengespeichert)
     */
    void onEnter(LGFX *tft) override;

    /** @brief Ressourcenfreigabe (keine dynamischen Ressourcen vorhanden). */
    void onExit() override;

    /**
     * @brief Zeichnet Tabelle und Info-Balken in den Work-Sprite.
     * @param spr Zeiger auf den Work-Sprite (sprite-relativ)
     * @param ox  X-Ursprung des Werkzeugbereichs im Sprite
     * @param oy  Y-Ursprung des Werkzeugbereichs im Sprite
     * @param w   Breite des Werkzeugbereichs in Pixeln
     * @param h   Höhe des Werkzeugbereichs in Pixeln
     */
    void draw(Gfx_Sprite *spr, int ox, int oy, int w, int h) override;

    /**
     * @brief Verarbeitet Touch-Ereignisse; Selektion wird beim Release ausgelöst.
     *
     * Beim ersten touched=true-Frame wird die getroffene Zeile in m_pressedRow gespeichert; weitere touched=true-Frames werden ignoriert. Die Selektion wechselt erst bei touched=false (Finger-Abheben), sodass kein Flatter-Toggle entsteht.
     *
     * @param tx      Absoluter Touch-X in Display-Koordinaten
     * @param ty      Absoluter Touch-Y in Display-Koordinaten
     * @param touched true bei laufendem Touch, false beim Loslassen
     * @return true wenn ein Neuzeichnen des Inhaltsbereichs erforderlich ist
     */
    bool handleTouch(int32_t tx, int32_t ty, bool touched) override;

private:
    // ========================================================================
    // INNERE STRUKTUREN
    // ========================================================================

    /**
     * @brief Datensatz eines SI-Präfixes.
     */
    struct SiPrefix
    {
        WorkshopKey nameKey;   ///< Schlüssel des Vollnamens in Strings_workshop
        const char *symbol;    ///< Vorsatzzeichen (z. B. "G"), leer bei Basis
        int8_t      exponent;  ///< Zehnerpotenz (z. B. 9 für 10^9)
        const char *example;   ///< Typisches Elektronik-Beispiel
    };

    // ========================================================================
    // HILFSFUNKTIONEN
    // ========================================================================

    /**
     * @brief Zeichnet Hintergründe, Trennlinien und Text der Präfix-Tabelle.
     * @param spr  Ziel-Sprite
     * @param ox   X-Ursprung im Sprite
     * @param tabY Y-Ursprung der Tabelle im Sprite (= oy + TOP_PAD)
     * @param w    Verfügbare Breite
     */
    void drawTable(Gfx_Sprite *spr, int ox, int tabY, int w) const;

    /**
     * @brief Zeichnet den Info-Balken mit Umrechnungsrelationen oder Hinweistext.
     *
     * Ohne Selektion: "Präfix antippen für Umrechnungshinweis". Mit Selektion z. B. für Mega: "1 M = 1000 k  ·  1 G = 1000 M".
     *
     * @param spr    Ziel-Sprite
     * @param ox     X-Ursprung im Sprite
     * @param infoY  Y-Startposition des Balkens im Sprite
     * @param w      Verfügbare Breite
     * @param h      Höhe des Balkens
     */
    void drawInfoBar(Gfx_Sprite *spr, int ox, int infoY, int w, int h) const;

    /**
     * @brief Treffertest: gibt den Datenzeilen-Index für eine Display-Y-Koordinate zurück.
     * @param ty Absoluter Touch-Y in Display-Koordinaten
     * @return Zeilenindex 0 bis PREFIX_COUNT-1 oder -1 bei Kopfzeile beziehungsweise außerhalb
     */
    int hitTestRow(int32_t ty) const;

    // ========================================================================
    // STATISCHE DATENBASIS
    // ========================================================================

    static const SiPrefix s_prefixes[]; ///< Tabelle der neun Präfixe von Tera bis Piko
    static const int       PREFIX_COUNT; ///< Anzahl der Einträge in s_prefixes[]

    // ========================================================================
    // ZUSTAND
    // ========================================================================

    int8_t m_selectedRow = -1; ///< Aktuell markierte Datenzeile (-1 = keine Markierung)
    int8_t m_pressedRow  = -1; ///< Beim Touch-Start getroffene Zeile (für Release-Auswertung)
    bool   m_touchActive = false; ///< true: Touch läuft, m_pressedRow ist gültig

    int m_ox = 0; ///< Gespeicherter ox-Wert des letzten draw()-Aufrufs
    int m_oy = 0; ///< Gespeicherter oy-Wert des letzten draw()-Aufrufs
    int m_w  = 0; ///< Gespeicherte Breite des letzten draw()-Aufrufs
    int m_h  = 0; ///< Gespeicherte Höhe des letzten draw()-Aufrufs (0 vor erstem Aufruf)
};

#endif // WORKSHOP_TOOL_SI_PREFIXES_H

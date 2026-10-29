#ifndef WORKSHOPTOOL_H
#define WORKSHOPTOOL_H

#include <stdint.h>

#include "Gfx.h"

class Workshop;

/**
 * @brief Reines Interface für interaktive Werkzeuge im Workshop-Modul.
 *
 * Jedes Werkzeug zeichnet ausschließlich in den übergebenen Work-Sprite (m_workSprite des Workshop-Moduls) ohne eigene Sprites zu besitzen. Der Lebenszyklus wird von Workshop verwaltet: Erzeugung über new in createTool(), Freigabe über delete beim Tool-Wechsel und im Destruktor von Workshop. Da Tools keine PODs sind (virtuelle Methoden), kommen hier reguläre new/delete-Operatoren zum Einsatz; größere interne Buffer der Tools sollten weiterhin in PSRAM allokiert werden.
 *
 * Koordinatensystem: Der Zeichenbereich beginnt bei (originX, originY) relativ zum Work-Sprite. handleTouch() erhält absolute Display-Koordinaten.
 *
 * Werkzeuge können optional Steuerelemente in der Fußzeile platzieren (links von Uhr und Exit-Button). Dazu drawFooter() und handleFooterTouch() überschreiben. Der verfügbare Bereich wird von Workshop als Parameter übergeben (siehe WorkshopCfg::TOOL_FOOTER_*).
 */
class WorkshopTool
{
public:
    virtual ~WorkshopTool() = default;

    /**
     * @brief Initialisierung beim Aktivieren des Werkzeugs.
     * @param tft Display-Objekt (für spätere Buttoninitialisierung wenn nötig)
     */
    virtual void onEnter(LGFX *tft) {}

    /**
     * @brief Aufräumen beim Verlassen des Werkzeugs.
     */
    virtual void onExit() {}

    /**
     * @brief Inhaltsbereich in den Work-Sprite zeichnen.
     * @param sprite  Work-Sprite von Workshop (m_workSprite)
     * @param originX Linke Kante des Inhaltsbereichs (sprite-relativ)
     * @param originY Obere Kante des Inhaltsbereichs (sprite-relativ)
     * @param w       Verfügbare Breite in Pixeln
     * @param h       Verfügbare Höhe in Pixeln
     */
    virtual void draw(Gfx_Sprite *sprite,
                      int originX, int originY,
                      int w, int h) = 0;

    /**
     * @brief Zeichnet nur den bewegten Anteil eines Werkzeugs in den vorhandenen Work-Sprite.
     * @param sprite  Work-Sprite von Workshop (m_workSprite)
     * @param originX Linke Kante des Inhaltsbereichs (sprite-relativ)
     * @param originY Obere Kante des Inhaltsbereichs (sprite-relativ)
     * @param w       Verfügbare Breite in Pixeln
     * @param h       Verfügbare Höhe in Pixeln
     *
     * Die Default-Implementierung zeichnet das vollständige Werkzeug. Animierte Werkzeuge können diese Methode überschreiben, wenn der statische Inhalt bereits im Work-Sprite steht und pro Frame nur eine kleine Region neu gezeichnet werden muss.
     */
    virtual void drawAnimationFrame(Gfx_Sprite *sprite,
                                    int originX, int originY,
                                    int w, int h)
    {
        draw(sprite, originX, originY, w, h);
    }

    /**
     * @brief Touch-Event verarbeiten.
     * @param tx      Absoluter Display-X (bei touched=false irrelevant / 0)
     * @param ty      Absoluter Display-Y (bei touched=false irrelevant / 0)
     * @param touched true wenn Display berührt wird
     * @return true wenn der Arbeitsbereich neu gezeichnet werden soll
     */
    virtual bool handleTouch(int32_t tx, int32_t ty, bool touched) = 0;

    // ========================================================================
    // Footer-Steuerbereich (optional)
    // ========================================================================

    /**
     * @brief Zeichnet werkzeugspezifische Steuerelemente in die Fußzeile.
     * @param tft Display-Objekt (direktes Zeichnen, kein Sprite)
     * @param x   Linke Kante des verfügbaren Bereichs (absolut)
     * @param y   Obere Kante des verfügbaren Bereichs (absolut)
     * @param w   Verfügbare Breite in Pixeln
     * @param h   Verfügbare Höhe in Pixeln
     *
     * Wird von Workshop nach dem Zeichnen der Fußzeile aufgerufen, sofern ein aktives Werkzeug vorhanden ist. Die Default-Implementierung tut nichts; Tools ohne Footer-Controls müssen nichts überschreiben.
     */
    virtual void drawFooter(LGFX *tft, int x, int y, int w, int h)
    {
        (void)tft; (void)x; (void)y; (void)w; (void)h;
    }

    /**
     * @brief Verarbeitet Touch-Events im Footer-Steuerbereich.
     * @param tx      Absoluter Display-X (bei touched=false irrelevant / 0)
     * @param ty      Absoluter Display-Y (bei touched=false irrelevant / 0)
     * @param touched true wenn Display berührt wird
     * @return true wenn sich der Werkzeugzustand geändert hat und ein vollständiger Redraw (Arbeitsbereich + Footer) erforderlich ist
     *
     * Das Werkzeug verwaltet seine Buttons selbst und zeichnet visuelle Rückmeldungen (Press-Animation) direkt auf das TFT. Rückgabe true signalisiert Workshop, dass invalidateWork() + m_footerDirty nötig ist.
     */
    virtual bool handleFooterTouch(int32_t tx, int32_t ty, bool touched)
    {
        (void)tx; (void)ty; (void)touched;
        return false;
    }

    /**
     * @brief Gibt einen dynamischen Untertitel zurück (optional).
     * @return Zeiger auf statischen/langlebigen String, oder nullptr
     *
     * Wenn nullptr (Default), verwendet Workshop den statischen Untertitel aus der PageDef-Struktur. Wenn non-null, wird dieser stattdessen in der Titelbox angezeigt. Nützlich für Werkzeuge deren Untertitel sich zur Laufzeit ändert (z.B. ASCII-Tabelle mit Bereichs-Toggle).
     */
    virtual const char *getSubtitle() const { return nullptr; }

    /**
     * @brief Signalisiert, ob das Werkzeug pro Frame einen Redraw braucht.
     * @return true wenn Workshop in jedem draw()-Aufruf invalidateWorkContent() auslösen soll
     *
     * Wird von Workshop pro Frame abgefragt. Werkzeuge mit eigener Animation (z.B. Pendel, Fortschrittsbalken) liefern hier true, solange die Animation läuft, und false, wenn nichts mehr animiert wird. Default false belässt das Verhalten unverändert; Workshop greift dann nur über den Cursor-Blink-Tick auf das Werkzeug zu.
     */
    virtual bool needsAnimationFrame() const { return false; }

    /**
     * @brief Liefert eine sprite-relative Bounding-Box für einen geclippten Push aufs Display.
     * @param x Linke Kante der Box im Work-Sprite (out)
     * @param y Obere Kante der Box im Work-Sprite (out)
     * @param w Breite der Box in Pixeln (out)
     * @param h Höhe der Box in Pixeln (out)
     * @return true wenn nur ein Teilbereich des Sprites gepusht werden soll; false für vollen Push
     *
     * Workshop wertet diesen Hinweis nach jedem draw() in drawWorkspaceContentOnly() aus. Bei true setzt Workshop vor dem Sprite-Push einen Display-Clip auf die Bounding-Box, sodass nur dieser Bereich tatsächlich aufs TFT übertragen wird. Damit bleibt das Tearing-Risiko bei Animationen auf eine kleine Region begrenzt. Werkzeuge ohne eigene Animation lassen die Default-Implementierung greifen.
     */
    virtual bool getPushBoundingBox(int &x, int &y, int &w, int &h) const
    {
        (void)x; (void)y; (void)w; (void)h;
        return false;
    }

    /**
     * @brief Überträgt den animierten Anteil selbst zum Display.
     * @param tft Display-Objekt
     * @param sprite Work-Sprite von Workshop (m_workSprite)
     * @param spriteX Displayposition der linken Sprite-Kante
     * @param spriteY Displayposition der oberen Sprite-Kante
     * @return true wenn das Werkzeug den Push erledigt hat; Workshop überspringt dann seinen eigenen Push
     *
     * Workshop ruft diese Methode in drawWorkspaceContentOnly() direkt nach drawAnimationFrame() auf. Werkzeuge mit mehreren kleinen Änderungsbereichen pro Frame können damit nur diese Bereiche pushen statt der gemeinsamen Bounding-Box aus getPushBoundingBox(). Die Default-Implementierung liefert false und belässt den Push bei Workshop.
     */
    virtual bool pushAnimationFrame(LGFX *tft, Gfx_Sprite *sprite, int spriteX, int spriteY)
    {
        (void)tft; (void)sprite; (void)spriteX; (void)spriteY;
        return false;
    }

    /**
     * @brief Hinterlegt einen Zeiger auf das Workshop-Modul, das dieses Werkzeug verwaltet. Wird von Workshop nach createTool() gesetzt.
     * @param owner Zeiger auf die verwaltende Workshop-Instanz
     *
     * Erlaubt Werkzeugen, übergeordnete Funktionen wie das Pausieren des Inhalts-Redraws während eines modalen Dialogs oder das Abspielen eines Sounds anzustoßen. Tools, die diese Funktionalität nicht benötigen, können den Owner ignorieren.
     */
    void setOwner(Workshop *owner) { m_owner = owner; }

    /**
     * @brief Liefert den hinterlegten Workshop-Zeiger oder nullptr.
     */
    Workshop *owner() const { return m_owner; }

private:
    Workshop *m_owner = nullptr;
};

#endif // WORKSHOPTOOL_H

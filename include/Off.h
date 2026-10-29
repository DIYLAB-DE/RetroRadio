#ifndef OFF_H
#define OFF_H

#include <stdint.h>

#include "BaseModule.h" // für AppModule
#include "Button.h"     // für Button
#include "Gfx.h"        // für LGFX

/**
 * @brief Callbacks des Off-Moduls
 */
struct OffCallbacks
{
    void (*onExitRequested)() = nullptr;     ///< Zurück zum Hauptbildschirm
    void (*onPowerOffRequested)() = nullptr; ///< Gerät ausschalten (Sperrbildschirm beim Aufwachen)
};

/**
 * @brief Ausschalt-Bildschirm mit QR-Code zum Projekt und zwei Aktionen.
 *
 * Zeigt links einen grossen QR-Code auf die Projektseite, rechts einen kurzen Hinweistext mit der Adresse. Über zwei Buttons kehrt der Anwender entweder zum Hauptbildschirm zurück oder schaltet das Gerät aus.
 */
class Off : public BaseModule
{
public:
    Off();
    ~Off();

    Off(const Off &) = delete;
    Off &operator=(const Off &) = delete;

    /**
     * @brief Initialisiert das Modul und zeichnet den Bildschirm
     * @param tft Referenz auf das TFT-Display
     * @param callbacks Callbacks für Zurück und Ausschalten
     * @return true bei Erfolg
     */
    bool init(LGFX &tft, const OffCallbacks &callbacks);

    /**
     * @brief Verarbeitet pro Frame die Touch-Eingaben der beiden Buttons
     */
    void draw() override;

    /**
     * @brief Gibt den Modultyp zurück
     */
    AppModule type() const override;

private:
    LGFX *m_tft = nullptr;      ///< Zeiger auf das Display
    OffCallbacks m_callbacks;   ///< Callbacks zum Schließen und Ausschalten
    bool m_initialized = false; ///< Modul wurde erfolgreich initialisiert

    Button m_backButton; ///< Button "zurück" zum Hauptbildschirm
    Button m_offButton;  ///< Button "ausschalten"

    void initButtons();
    void drawScreen();
    void drawQrCode();
    void drawInfoText();

    /**
     * @brief Wertet einen Button aus und löst beim Loslassen den Callback aus
     * @param btn Button-Referenz
     * @param touched Touch im aktuellen Frame aktiv
     * @param tx Touch-X
     * @param ty Touch-Y
     * @param callback Beim Loslassen aufzurufender Callback (darf nullptr sein)
     */
    void handleButton(Button &btn, bool touched, int32_t tx, int32_t ty, void (*callback)());

    /**
     * @brief Zeichnet einen Text mit Wortumbruch in eine feste Breite
     * @param text Auszugebender Text (UTF-8)
     * @param x Linke Kante der ersten Zeile
     * @param y Obere Kante der ersten Zeile
     * @param maxWidth Maximale Zeilenbreite in Pixeln
     * @param lineHeight Zeilenhöhe in Pixeln
     * @param color Textfarbe
     * @return Y-Position direkt unterhalb der letzten gezeichneten Zeile
     *
     * Erwartet eine bereits geladene Schrift. Bricht ausschließlich an Leerzeichen um, sodass mehrbyteige UTF-8-Zeichen nie zerschnitten werden.
     */
    int drawWrappedText(const char *text, int x, int y, int maxWidth, int lineHeight, uint16_t color);
};

#endif // OFF_H

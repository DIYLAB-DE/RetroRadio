#ifndef TOOLS_H
#define TOOLS_H

#include <stdint.h>

/**
 * @brief Gibt Chip-Modell, Core-Anzahl, Taktfrequenz, Heap-, PSRAM- und Flash-Größen auf dem Logger aus.
 */
void systemInfo();

/**
 * @brief Gibt Kartentyp, Kartengröße, Gesamtplatz und belegten Platz der SD-Karte auf dem Logger aus.
 * @param mount true wenn das Mounten der SD-Karte erfolgreich war, false bei Fehler
 */
void sdCardInfo(bool mount);

/**
 * @brief Gibt den Initialisierungsstatus der RTC auf dem Logger aus.
 * @param init true wenn die RTC-Initialisierung erfolgreich war, false bei Fehler
 */
void rtcInfo(bool init);

// Temporäre Nordlicht-Messsonde; alle Aufrufe erfolgen aus der Hauptschleife.
namespace auroraprobe
{
/**
 * @brief Gemessene Abschnitte der Hauptschleife
 */
enum class LoopSection : uint8_t
{
    Network, ///< WLAN-Verwaltung und ausstehende Initialisierung
    Audio, ///< Audio-Transportaufruf
    Status, ///< WLAN- und Zeitstatus
    Statistics, ///< Systemstatistik und SSE-Versand
    Ftp, ///< FTP-Verarbeitung
    Ui, ///< Oberflächenaktualisierung einschließlich Nordlicht
    UiTimeWindow, ///< Zeitfensterprüfung innerhalb der Oberflächenaktualisierung
    UiTouch, ///< Touch-Abfrage und Visualizer-Gesten im SD-Player
    UiData, ///< Übergabe der Audio- oder Nullwerte im SD-Player
    UiVisualizer, ///< Gesamter Visualizer-Aufruf im SD-Player
    UiControls, ///< Bedienelemente, Events und Kassette im SD-Player
    UiClock, ///< Uhr und verbleibende Timerzeit im SD-Player
    UiCassette, ///< Kassettenaktualisierung innerhalb der SD-Bedienelemente
    Count ///< Anzahl der gemessenen Abschnitte
};

/**
 * @brief Reserviert den festen PSRAM-Messspeicher einmal beim Start
 * Bei fehlendem Speicher bleiben Aufnahmebefehle wirkungslos; die Oberfläche läuft weiter.
 */
void init();

/**
 * @brief Meldet den aktuellen SD-Zustand an die Messsonde
 * @param path Geladener Dateipfad; dient nur zur Wechselerkennung und wird nicht ausgegeben
 * @param paused true bei ausdrücklich pausierter Wiedergabe
 * @param ready true bei geladenem Titel ohne Dialog oder Grafiksperre
 */
void setSdSource(const char *path, bool paused, bool ready);

/**
 * @brief Erfasst die Laufzeit eines Hauptschleifenabschnitts ohne Ausgabe
 * @param section Gemessener Abschnitt
 * @param elapsedUs Laufzeit in Mikrosekunden einschließlich Unterbrechungen
 */
void recordSection(LoopSection section, uint32_t elapsedUs);

/**
 * @brief Erfasst Laufzeit und Abstand vollständiger Hauptschleifendurchläufe
 * @param startUs Beginn des Durchlaufs in Mikrosekunden
 * @param endUs Ende vor der Messberichtsausgabe in Mikrosekunden
 */
void recordLoop(uint32_t startUs, uint32_t endUs);

/**
 * @brief Bricht eine laufende Einzelaufnahme bei Visualizer-Reinitialisierung ab
 */
void reset();

/**
 * @brief Beginnt die Zeitmessung eines neuen Nordlicht-Bildes
 * @param startUs Beginn der Bildberechnung in Mikrosekunden
 * @param animationMs Tatsächlich verwendeter Bewegungsschritt nach der bestehenden Begrenzung
 */
void beginFrame(uint32_t startUs, uint32_t animationMs);

/**
 * @brief Erfasst einen abgeschlossenen Rechenabschnitt ohne Logausgabe
 * @param elapsedUs Dauer des Abschnitts einschließlich Bildvorbereitung in Mikrosekunden
 * @param columns Anzahl der berechneten Spalten
 */
void recordSlice(uint32_t elapsedUs, uint16_t columns);

/**
 * @brief Erfasst ein vollständig ausgegebenes Bild ohne Logausgabe
 * @param pushStartUs Beginn der Bildübertragung in Mikrosekunden
 * @param endUs Ende der Bildübertragung in Mikrosekunden
 */
void endFrame(uint32_t pushStartUs, uint32_t endUs);

/**
 * @brief Erfasst den Abstand zum vorigen Audio-Transport-Aufruf ohne Logausgabe
 * @param startUs Beginn des Audio-Aufrufs in Mikrosekunden
 */
void recordAudio(uint32_t startUs);

/**
 * @brief Steuert begrenzte Einzelaufnahmen mit nachgelagerter serieller Ausgabe
 * @param active true bei sichtbarem Nordlicht ohne Zeichenpause
 * @param playing true bei laufender Audiowiedergabe
 * Die seriellen Zeichen P, M und F starten eine Aufnahme im passenden SD-Zustand Pause, MP3 oder FLAC. Nach zwei Sekunden Ruhe werden 128 vollständige Bildintervalle ohne Messausgaben erfasst. Zustandswechsel und ein Zeitlimit von 20 Sekunden brechen die Aufnahme ab. Erst danach erfolgt höchstens eine kurze Ausgabezeile je Hauptschleifendurchlauf. Die Zeitstempel bezeichnen Software-Bildkopien, keine Display-Abtastung.
 */
void report(bool active, bool playing);
}

#endif // TOOLS_H

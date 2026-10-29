#ifndef MAIN_H
#define MAIN_H

#include <stdint.h>
#include "SDPlayer.h"

// Vorwärtsdeklaration für FS-Typ
namespace fs
{
    class FS;
}

class Audio;

// ============================================================================
// Globale Objekte
// ============================================================================

/**
 * @brief Globale Audio-Instanz für alle Module.
 */
extern Audio audio;

// ============================================================================
// Radio-/System-Funktionen
// ============================================================================

/**
 * @brief Wechselt die aktuelle Radiostation.
 * @param id Eindeutige ID der gewünschten Radiostation (0-basierter Index).
 */
void setNewStation(uint8_t id);

/**
 * @brief Stoppt den aktuell wiedergegebenen Radiosender sofort.
 */
void stopCurrentStation();

/**
 * @brief Aktualisiert periodisch alle relevanten System-Statuswerte.
 * @param displayOn True, wenn das Display eingeschaltet ist, sonst False.
 */
void updateStatus(bool displayOn);

/**
 * @brief Gibt Systeminformationen aus.
 */
void systemInfo();

/**
 * @brief Gibt Informationen zur SD-Karte und ihrem Status aus.
 * @param mount True, wenn die SD-Karte erfolgreich gemountet wurde, sonst False.
 */
void sdCardInfo(bool mount);

/**
 * @brief Gibt Informationen zur RTC (Real Time Clock) aus.
 * @param init True, wenn die RTC erfolgreich initialisiert wurde, sonst False.
 */
void rtcInfo(bool init);

/**
 * @brief Schaltet die Display-Hintergrundbeleuchtung ein oder aus.
 * @param on True für Ein, False für Aus
 */
void setBacklight(bool on);

// ============================================================================
// Callbacks für UI/Bedienung
// ============================================================================

/**
 * @brief Handler für gedrückte Buttons des Retro-Moduls.
 * @param btnIdx Index des gedrückten Buttons (0-basiert).
 * @param buttonState Zustand des Buttons (true = gedrückt, false = losgelassen).
 */
void handleButtonPressed(uint16_t btnIdx, bool buttonState);

/**
 * @brief Handler für Stationsauswahl über die Skala.
 * @param staIdx Index der ausgewählten Station (0-basiert).
 */
void handleStationPressed(uint16_t staIdx);

/**
 * @brief Handler für Lautstärkeänderungen.
 * @param volume Neue Lautstärke (0-100).
 */
void handleVolumeChanged(uint8_t volume);

/**
 * @brief Handler zum Speichern der Lautstärke vor Stummschaltung.
 * @param volume Lautstärke vor Stummschaltung (0-100).
 */
void handleVolumeBeforeMute(uint8_t volume);

/**
 * @brief Handler für Stummschaltung.
 * @param mute True für Stummschaltung, False für normale Wiedergabe.
 */
void handleMuteChanged(bool mute);

/**
 * @brief Handler für Mono-/Stereo-Umschaltung.
 * @param mono True für erzwungenen Mono-Betrieb, False für Stereo.
 */
void handleMonoChanged(bool mono);

/**
 * @brief Handler für Balance-Einstellungen.
 * @param balance Neue Balance (-16 bis +16, 0 = mittig).
 */
void handleBalanceChanged(int8_t balance);

/**
 * @brief Handler für Tiefenregler (Low-Pass Filter).
 * @param gainLowPass Neue Verstärkung für Tiefen (-12 bis +12 dB).
 */
void handleGainLowPassChanged(int8_t gainLowPass);

/**
 * @brief Handler für Mittenregler (Band-Pass Filter).
 * @param gainBandPass Neue Verstärkung für Mitten (-12 bis +12 dB).
 */
void handleGainBandPassChanged(int8_t gainBandPass);

/**
 * @brief Handler für Höhenregler (High-Pass Filter).
 * @param gainHighPass Neue Verstärkung für Höhen (-12 bis +12 dB).
 */
void handleGainHighPassChanged(int8_t gainHighPass);

/**
 * @brief Wendet die drei Filter-Eckfrequenzen der Klangregelung an.
 * @param freqLow Eckfrequenz des Tiefen-Filters in Hz.
 * @param freqPeak Mittenfrequenz des Mitten-Filters in Hz.
 * @param freqHigh Eckfrequenz des Höhen-Filters in Hz.
 *
 * Setzt die Frequenzen und rechnet die Filter-Koeffizienten anhand der aktuellen Verstärkungen neu, sodass die Änderung sofort hörbar ist.
 */
void applyEqFrequencies(uint16_t freqLow, uint16_t freqPeak, uint16_t freqHigh);

/**
 * @brief Wendet Eckfrequenzen und Loudness-Korrektur auf die Klangregelung an.
 * @param freqLow Eckfrequenz des Tiefen-Filters in Hz.
 * @param freqPeak Mittenfrequenz des Mitten-Filters in Hz.
 * @param freqHigh Eckfrequenz des Höhen-Filters in Hz.
 * @param loudness Loudness-Stärke in Prozent.
 */
void applyAudioTone(uint16_t freqLow, uint16_t freqPeak, uint16_t freqHigh, uint8_t loudness);

/**
 * @brief Wendet die Basisbreite auf die Audio-Ausgabe an.
 * @param width Basisbreite in Prozent, 100 bedeutet unverändert.
 */
void applyStereoWidth(uint8_t width);

/**
 * @brief Schaltet die internen Lautsprecher über die FETs am RS485-Ausgang B.
 * @param enabled true gibt die Lautsprecher frei, false schaltet sie ab.
 *
 * I2S-Daten und DAC-Ausgabe laufen unverändert weiter. Der RS485-Abschlusswiderstand muss ausgeschaltet sein. Diese Funktion speichert den Zustand nicht.
 */
void applySpeakerOutput(bool enabled);

// ============================================================================
// SDPlayer-Callbacks
// ============================================================================

/**
 * @brief Startet die Wiedergabe einer Datei von der SD-Karte.
 * @param path Vollständiger Pfad zur abzuspielenden Datei.
 */
bool playFileFromSd(const char *path);
void handlePlayFileRequested(const char *path);

/**
 * @brief Stoppt die aktuelle SD-/Stream-Wiedergabe.
 */
void handleStopRequested();

/**
 * @brief Pausiert oder setzt die Wiedergabe fort (Toggle).
 */
void handlePauseResumeRequested();

/**
 * @brief Handler bei Track-Ende (Auto-Play zum nächsten).
 */
void handleTrackFinished();

/**
 * @brief Aktualisiert Metadaten im SDPlayer.
 * @param artist Interpret/Künstler.
 * @param title  Titel des Songs.
 * @param bitrate Bitrate als String.
 */
void handleMetadataUpdate(const char *artist, const char *title, const char *bitrate);

/**
 * @brief Zeichnet ein MP3-Cover aus der Datei (optional).
 * @param fs   Dateisystem-Referenz.
 * @param path Pfad zur Datei.
 * @return True bei Erfolg, sonst False.
 */
bool drawMp3Cover(fs::FS &fs, const char *path);

#endif // MAIN_H

#include "SDPlayer.h"

#include <JPEGDEC.h>
#include <SD.h>

#include "AppState.h"
#include "I18n.h"
#include "Strings_sdplayer.h"
#include "StringUtils.h"
#include "Theme.h"
#include "Clocks.h"

// Fonts
#include "fonts/ComfortaaBold10.h"
#include "fonts/ComfortaaBold14.h"
#include "fonts/ComfortaaBold16.h"
#include "fonts/ComfortaaBold18.h"
#include "fonts/LeagueGothicCond34.h"

// Icons, Bilder
#include "images/nocover.h"
#include "images/cassette_auralux.h"
#include "images/cassette_modutape.h"
#include "images/cassette_sunbeam.h"
#include "images/cassette_ember.h"
#include "images/cassette_glasstone.h"
#include "images/cassette_auralite.h"
#include "images/cassette_lumotape.h"
#include "images/cassette_aurion.h"
#include "IconShapes.h"

JPEGDEC SdPlayer::s_jpeg;

namespace
{
    // Farbe des gezeichneten Stummschaltsymbols, an der früheren Bitmap abgelesen
    constexpr uint16_t ICON_MUTE_COLOR = 0x9CF3;      ///< Strichfarbe des Stummschaltsymbols
    constexpr uint16_t ICON_MUTE_BACKGROUND = 0x0000; ///< Freie Fläche des Stummschaltsymbols
}

/**
 * @brief Kopiert den Namen des übergeordneten Verzeichnisses eines Dateipfads nach out; false, wenn es keins gibt oder der Name nicht in out passt
 */
static bool parentDirName(const char *path, char *out, size_t outSize)
{
    const char *lastSlash = strrchr(path, '/');
    if (!lastSlash || lastSlash == path)
        return false;

    const char *secondLastSlash = lastSlash - 1;
    while (secondLastSlash > path && *secondLastSlash != '/')
        secondLastSlash--;
    if (*secondLastSlash != '/' || secondLastSlash >= lastSlash - 1)
        return false;

    const size_t dirLen = (size_t)(lastSlash - secondLastSlash - 1);
    if (dirLen == 0 || dirLen >= outSize - 1)
        return false;

    stringutils::safeCopy(out, dirLen + 1, secondLastSlash + 1);
    return true;
}

/**
 * @brief Draw-Callback für JPEGDEC
 */
static int jpegDrawCB(JPEGDRAW *p)
{
    LGFX_Sprite *spr = static_cast<LGFX_Sprite *>(p->pUser);
    if (!spr)
        return 0;

    // Block direkt in das Sprite schreiben
    spr->pushImage(p->x, p->y, p->iWidth, p->iHeight, reinterpret_cast<const uint16_t *>(p->pPixels));
    return 1;
}

// ===================================================================
// Button-Definitionen
// ===================================================================

// Statische Member für SleepTimer
const char *SdPlayer::m_sleepLabels[] = {"5", "15", "25", "45", "60", "^POWER:32:D206^"};
int SdPlayer::m_sleepValues[] = {5, 15, 25, 45, 60, 0};

const SdPlayerButton SdPlayer::m_buttonDefs[4] = {
    {513, 435, 75, 45, 4, 0x39E8, TFT_BLACK, TFT_DARKGREY, "durchs."},        // Playlist durchsuchen
    {598, 435, 70, 45, 4, 0x39E8, TFT_BLACK, TFT_DARKGREY, "leeren"},         // Playlist leeren
    {678, 435, 67, 45, 4, 0x39E8, TFT_BLACK, TFT_DARKGREY, "reset"},          // Reset
    {755, 435, 45, 45, 4, 0x39E8, TFT_BLACK, TFT_DARKGREY, "^POWER:32:D206^"} // Power/SleepTimer
};

const SdPlayerButton SdPlayer::m_playerButtonDefs[5] = {
    {10, PLAYER_CONTROLS_Y, 45, 45, 4, 0x39E8, TFT_BLACK, 0x5370, "^SKIPB:32^"},                          // Titelanfang/voriger Titel, halten spult zurück
    {10 + 45 + PADDING, PLAYER_CONTROLS_Y, 45, 45, 4, 0x39E8, TFT_BLACK, 0x5370, "^PLAY:32^"},            // Wiedergabe
    {10 + 2 * (45 + PADDING), PLAYER_CONTROLS_Y, 45, 45, 4, 0x39E8, TFT_BLACK, 0x5370, "^PAUSE:32^"},     // Pause
    {10 + 3 * (45 + PADDING), PLAYER_CONTROLS_Y, 45, 45, 4, 0x39E8, TFT_BLACK, 0xD206, "^STOP:32:D206^"}, // Stopp
    {10 + 4 * (45 + PADDING), PLAYER_CONTROLS_Y, 45, 45, 4, 0x39E8, TFT_BLACK, 0x5370, "^SKIPF:32^"}};    // nächster Titel, halten spult vor

// Kassetten-Designs auf Basis fotorealistischer 285x184-Farbtabellen. Reihenfolge: bitmap, labelText, labelX, labelY, labelW, tape, tapeEdge, winX, winY, winW, winH, hubY, hubXLeft, hubXRight, hubCoverR, reelRMin, reelRMax
const CassetteDesign SdPlayer::m_cassetteDesigns[SdPlayer::CASSETTE_DESIGN_COUNT] = {
    {cassette_auralux, 0x2124, 46, 22, 215, 0x3122, 0x4163, 108, 58, 67, 46, 79, 84, 200, 18, 30, 67},   // dunkle Studiokassette mit Silberstreifen
    {cassette_modutape, 0x2124, 46, 22, 215, 0x3102, 0x3943, 110, 63, 65, 43, 84, 85, 200, 20, 30, 66},  // schwarz-weiß mit grünem Fenster
    {cassette_sunbeam, 0x2124, 0, 0, 0, 0x5183, 0x69C3, 41, 52, 204, 59, 81, 82, 202, 18, 25, 69},       // Retro-Orange mit Panoramafenster
    {cassette_ember, 0x2124, 0, 0, 0, 0x3922, 0x4963, 37, 50, 212, 62, 80, 82, 202, 23, 27, 69},         // Schwarz-Kupfer mit Klarsichtfenster
    {cassette_glasstone, 0x2965, 46, 29, 205, 0x3102, 0x4143, 55, 55, 179, 55, 88, 82, 202, 19, 25, 69}, // Volltransparent mit Silberstreifen
    {cassette_auralite, 0x39E7, 48, 25, 180, 0x1040, 0x28E2, 59, 58, 166, 48, 82, 82, 202, 17, 25, 69},  // Bernstein-Klarsicht mit Creme-Etikett (Vorlage Auralite_neu)
    {cassette_lumotape, 0x2124, 46, 32, 205, 0x3102, 0x3943, 56, 62, 173, 48, 86, 82, 202, 19, 25, 69},  // Klarsicht mit Neongelb-Etikett
    {cassette_aurion, 0x2124, 0, 0, 0, 0x3922, 0x4963, 16, 20, 252, 111, 75, 82, 202, 18, 25, 69}};      // Volltransparent schwarz, Wickel frei sichtbar

// ===================================================================
// Konstruktor & Destruktor
// ===================================================================

/**
 * @brief Konstruktor
 */
SdPlayer::SdPlayer(LGFX &tft, Audio &audio) : m_tft(&tft), m_audio(&audio)
{
}

/**
 * @brief Destruktor
 */
SdPlayer::~SdPlayer()
{
    // FileExplorer zuerst zerstören, bevor andere Member ungültig werden. FileExplorer-Callbacks halten 'this' als Raw-Pointer und könnten sonst auf bereits gelöschte Member zugreifen.
    m_explorer.reset();

    // Stop der Audio-Wiedergabe
    requestBackendStop();

    // Visualizer schließen
    zeroVisualizer();

    // Reset des Audio-Status
    m_audioStatus = AudioStatus();

    // JPEG Decoder schließen
    s_jpeg.close();

    // Cover-Buffer freigeben (Grow-only Buffer)
    if (m_coverBuffer)
    {
        heap_caps_free(m_coverBuffer);
        m_coverBuffer = nullptr;
        m_coverBufferCapacity = 0;
    }

    // Ordner-Symbol der leeren Playlist freigeben
    if (m_emptyFolderIcon)
    {
        free(m_emptyFolderIcon);
        m_emptyFolderIcon = nullptr;
    }

    // Sprites aufräumen
    m_tft->safeDeleteSprite(m_sprDiv);
    m_tft->safeDeleteSprite(m_sprKnob);
    m_tft->safeDeleteSprite(m_sprKnobText);
    m_tft->safeDeleteSprite(m_sprTracks);
    m_tft->safeDeleteSprite(m_sprPlaylist);
    m_tft->safeDeleteSprite(m_sprScrollbar);
    m_tft->safeDeleteSprite(m_sprProgress);
    m_tft->safeDeleteSprite(m_sprMeta);
    m_tft->safeDeleteSprite(m_sprCover);
    m_tft->safeDeleteSprite(m_sprDetails);
    m_tft->safeDeleteSprite(m_sprCassette);
    m_tft->safeDeleteSprite(m_sprTapeLabel);
    m_tft->safeDeleteSprite(m_sprWinGround);
    m_tft->safeDeleteSprite(m_sprWin);
    m_tft->safeDeleteSprite(m_sprDorn);
    m_tft->safeDeleteSprite(m_sprWrap);
    m_tft->safeDeleteSprite(m_sprRetroAnalogClock);
    m_tft->safeDeleteSprite(m_sprRetroDigitalClock);

    // Fonts entladen
    m_tft->unloadFont();
}

// ===================================================================
// Hilfsfunktionen
// ===================================================================

/**
 * @brief Gibt den Modul-Typ zurück
 */
AppModule SdPlayer::type() const
{
    return AppModule::SD_PLAYER;
}

/**
 * @brief Prüft ob eine Datei bereits in der Playlist vorhanden ist
 */
bool SdPlayer::isFileInPlaylist(const char *path) const
{
    for (const auto &entry : m_playlist)
    {
        if (strcmp(entry.path, path) == 0)
            return true;
    }

    return false;
}

/**
 * @brief Prüft, ob einer der Dialoge aktiv ist
 */
bool SdPlayer::isDialogOpen() const
{
    return (m_messageBox && (m_messageBox->isActive() || m_messageBox->isBusy())) || (m_sleepDlg && m_sleepDlg->isActive()) || (m_explorer && m_explorer->isActive());
}

/**
 * @brief Setzt den Visualizer auf Nullwerte
 */
void SdPlayer::zeroVisualizer()
{
    static int16_t s_zeros[16];
    m_visualizer.setAudioValues(0.0f, 0.0f, 0.0f, 0.0f);
    m_visualizer.setSampleData(s_zeros, s_zeros, 16, 0);
}

/**
 * @brief Fordert das Backend zum Stoppen der Wiedergabe auf
 */
void SdPlayer::requestBackendStop()
{
    if (m_callbacks.onStopRequested)
        m_callbacks.onStopRequested();

    // Der Stopp läuft synchron durch bis audio.stopSong(); alle danach noch gepufferten Ereignisse (Tags, Cover, EOF) gehören dem alten Titel und werden verworfen, bevor sie am nächsten Titel landen
    AudioManagerEventType staleType;
    while (g_audioManager.dequeueEvent(staleType, m_eventText, sizeof(m_eventText), &m_eventCoverData))
        continue;

    m_eventCoverData.clear();
}

/**
 * @brief Bricht wartende Start-/Cover-Zustände ab
 */
void SdPlayer::cancelPendingPlaybackWaits()
{
    m_waitingForPlaybackStart = false;
    m_playbackStartWaitMs = 0;
    m_waitingForCoverEvent = false;
    m_coverEventWaitStartMs = 0;
}

/**
 * @brief Setzt laufzeitbezogene Playback-Daten zurück
 */
void SdPlayer::resetRuntimePlaybackState()
{
    cancelPendingPlaybackWaits();

    m_cachedDuration = 0;
    m_audioStatus.fileSize = 0;
    m_audioStatus.filePos = 0;
    m_audioStatus.duration = 0;
    m_audioStatus.currentTime = 0;
    m_audioStatus.progress = 0;
    m_audioStatus.isPlaying = false;

    m_isVbrTrack = false;
    m_lastDurationUpdateMs = 0;
    m_needsProgressRedraw = true;
    m_needsDetailsRedraw = true;

    // Ein laufendes Scrubbing gehört zum alten Titel und endet mit ihm; sonst spränge beim Loslassen der NEUE Titel an die Fingerposition. handleProgressTouch startet ohne frisches Aufsetzen kein neues Scrubbing.
    m_progressScrubbing = false;
}

/**
 * @brief Leert die angezeigten Metadaten
 */
void SdPlayer::clearDisplayedMetadata(bool clearCurrentFile)
{
    m_currentArtist[0] = '\0';
    m_currentTitle[0] = '\0';
    m_currentAlbum[0] = '\0';
    m_currentBitrate[0] = '\0';
    m_displayBitrate[0] = '\0';

    if (clearCurrentFile)
        m_currentAudioFile[0] = '\0';

    m_metadataUpdated = true;
    m_needsCassetteLabelRedraw = true;
    m_needsDetailsRedraw = true;
}

/**
 * @brief Behandelt einen regulär beendeten oder abgebrochenen Track konsistent
 */
void SdPlayer::finalizeCurrentTrack(TrackFinishType finishType, bool advanceToNext)
{
    if (m_playlist.empty())
        return;

    if (m_playlistFinished && finishType == TrackFinishType::COMPLETED)
        return;

    if (finishType == TrackFinishType::COMPLETED && m_trackFinishedProcessing)
    {
        LOG_INFO("EOF ignoriert (bereits in Verarbeitung)");
        return;
    }

    if (finishType == TrackFinishType::COMPLETED)
        m_trackFinishedProcessing = true;

    const int finishedIndex = m_currentTrackIndex;

    if (finishType == TrackFinishType::COMPLETED)
        LOG_INFO_F("Track %d beendet", finishedIndex + 1);
    else
        LOG_WARNING_F("Track %d abgebrochen", finishedIndex + 1);

    resetRuntimePlaybackState();
    drawProgressBar();

    if (finishType == TrackFinishType::COMPLETED)
    {
        if (finishedIndex >= 0 && finishedIndex < (int)m_playedTracks.size())
            m_playedTracks[finishedIndex] = true;

        if (advanceToNext)
            playNextTrack();
    }
    else
    {
        m_playbackState = PlaybackState::STOPPED;
        updatePlayerButtonStates();
        APP_STATE.setStationSetDone(false);
        m_trackFinishedProcessing = false;
    }

    if (m_playlistFinished)
    {
        m_playbackState = PlaybackState::STOPPED;
        updatePlayerButtonStates();
        m_trackFinishedProcessing = false;
    }
}

/**
 * @brief Verfeinert die Duration bei VBR-Tracks während des Playbacks
 */
void SdPlayer::refineDurationForVbr()
{
    // Nur bei VBR-Tracks alle 10 Sekunden updaten
    if (!m_isVbrTrack || (millis() - m_lastDurationUpdateMs < 10000))
        return;

    m_lastDurationUpdateMs = millis();

    // Neue Duration von Library holen (wird bei VBR während Playback präziser)
    uint32_t newDuration = m_audio->getAudioFileDuration();

    // Nur updaten wenn signifikant unterschiedlich (>2 Sekunden)
    if (newDuration > 0 && abs((int32_t)newDuration - (int32_t)m_audioStatus.duration) > 2)
    {
        m_audioStatus.duration = newDuration;
        m_cachedDuration = newDuration;
        m_needsProgressRedraw = true;

        LOG_INFO_F("Duration verfeinert (VBR): %u Sekunden", newDuration);
    }
}

/**
 * @brief Kürzt einen Text so, dass er in maxWidth passt (mit "...")
 */
void SdPlayer::truncateWithEllipsis(Gfx_Sprite *spr, const char *in, char *out, int outSize, int maxWidth)
{
    if (!spr || !in || !in[0] || !out || outSize < 4)
    {
        if (out && outSize > 0)
            out[0] = '\0';

        return;
    }

    // Vollständig kopieren
    stringutils::safeCopy(out, outSize, in);

    // Wenn schon kurz genug -> fertig
    const int ellW = spr->textWidth("...");
    if (spr->textWidth(out) <= maxWidth)
        return;

    // Falls "..." allein schon zu breit ist -> nur "." oder gar nichts ausgeben
    if (ellW > maxWidth)
    {
        // Extremfall: selbst "..." passt nicht -> so viel wie möglich zeigen
        stringutils::safeCopy(out, outSize, ".");
        if (spr->textWidth(".") > maxWidth)
            out[0] = '\0'; // nichts passt -> leer

        return;
    }

    // Schnellabschätzung: wie viele Zeichen passen ungefähr?
    int len = (int)strlen(out);
    int w = spr->textWidth(out);

    if (w > 0)
    {
        int est = (len * (maxWidth - ellW)) / w;
        est = std::max(0, std::min(est, len - 1)); // mindestens ein Zeichen kürzen, notfalls bleibt keines stehen

        // Nie mitten in einer UTF-8-Sequenz terminieren, sonst bleibt ein halbes Zeichen vor dem "..." stehen
        while (est > 0 && ((unsigned char)out[est] & 0xC0) == 0x80)
            est--;

        out[est] = '\0';
        len = est;
    }

    // Feinjustierung: Zeichen für Zeichen kürzen, bis es passt; Fortsetzungsbytes zählen dabei zum Zeichen davor
    while (len > 0 && spr->textWidth(out) > maxWidth - ellW)
    {
        len--;
        while (len > 0 && ((unsigned char)out[len] & 0xC0) == 0x80)
            len--;

        out[len] = '\0';
    }

    // "..." anhängen - safeAppend sorgt selbst für Null-Terminierung
    stringutils::safeAppend(out, outSize, "...");
}

/**
 * @brief Entfernt Track-Nummerierungen vom Anfang des Dateinamens
 */
void SdPlayer::cleanupFilename(const char *filename, char *output, size_t maxLen)
{
    if (!filename || !output || maxLen == 0)
    {
        // Nur in einen vorhandenen Puffer terminieren, der Schutzzweig darf nicht selbst über den Null-Zeiger schreiben
        if (output && maxLen > 0)
            output[0] = '\0';

        return;
    }

    const char *ptr = filename;

    // Optionale Präfixe wie "Track", "CD", "Disc", "Disk" überspringen
    const char *prefixes[] = {"Track ", "track ", "CD ", "cd ", "Disc ", "disc ", "Disk ", "disk "};
    for (int i = 0; i < 8; i++)
    {
        size_t prefixLen = strlen(prefixes[i]);
        if (strncmp(ptr, prefixes[i], prefixLen) == 0)
        {
            ptr += prefixLen;
            break;
        }
    }

    // Zeiger zum Anfang der Nummerierung
    const char *start = ptr;

    // Erste Zahlengruppe finden (1-3 Ziffern)
    int digitCount = 0;
    while (*ptr >= '0' && *ptr <= '9' && digitCount < 3)
    {
        ptr++;
        digitCount++;
    }

    // Wenn keine Ziffern gefunden wurden, Original zurückgeben
    if (digitCount == 0)
    {
        stringutils::safeCopy(output, maxLen, filename);

        // Extension entfernen
        char *dot = strrchr(output, '.');
        if (dot)
            *dot = '\0';

        return;
    }

    // Trennzeichen zwischen Disc und Track (z.B. "-" in "2-01")
    if (*ptr == '-' || *ptr == '_' || *ptr == '.')
    {
        ptr++;

        // Zweite Zahlengruppe (Track-Nummer nach Disc-Nummer)
        digitCount = 0;
        while (*ptr >= '0' && *ptr <= '9' && digitCount < 3)
        {
            ptr++;
            digitCount++;
        }
    }

    // Jetzt könnten Trennzeichen folgen (Leerzeichen, Minus, Punkt, Unterstrich)
    bool foundSeparator = false;

    // Übliche Trennzeichen-Kombinationen:
    // " - ", " ", ".", "_", "-", ". ", "._", etc.
    while (*ptr != '\0')
    {
        if (*ptr == ' ' || *ptr == '-' || *ptr == '.' || *ptr == '_')
        {
            ptr++;
            foundSeparator = true;
        }
        else
        {
            break;
        }
    }

    // Wenn kein gültiges Trennzeichen gefunden wurde, Original zurückgeben
    if (!foundSeparator || *ptr == '\0')
    {
        stringutils::safeCopy(output, maxLen, filename);

        // Extension entfernen
        char *dot = strrchr(output, '.');
        if (dot)
            *dot = '\0';

        return;
    }

    // Rest als Titel kopieren
    stringutils::safeCopy(output, maxLen, ptr);

    // Extension entfernen
    char *dot = strrchr(output, '.');
    if (dot)
        *dot = '\0';

    // Führende Leerzeichen entfernen
    char *trimmed = output;
    while (*trimmed == ' ')
        trimmed++;

    if (trimmed != output)
        memmove(output, trimmed, strlen(trimmed) + 1);
}

// ===================================================================
// Cover-Image-Verwaltung
// ===================================================================

/**
 * @brief Setzt das Cover zurück
 */
void SdPlayer::resetCover()
{
    m_hasCover = false;
    m_coverOffsets.clear();
    m_eventCoverData.clear();
    m_needsCoverRedraw = true;

    // Cover-Tracking zurücksetzen
    m_lastDisplayedCoverFile[0] = '\0';
    m_lastDisplayedCoverOffsets.clear();

    // Timeout-Flag zurücksetzen
    m_waitingForCoverEvent = false;
}

/**
 * @brief Lädt und zeichnet das eingebettete Cover-Bild
 */
bool SdPlayer::loadAndDrawCover()
{
    // Prüfen ob das Cover bereits angezeigt wird
    if (m_hasCover && strcmp(m_currentAudioFile, m_lastDisplayedCoverFile) == 0 && m_coverOffsets == m_lastDisplayedCoverOffsets)
    {
        LOG_AUDIO_F("Das Cover-Bild von '%s' wird bereits angezeigt und daher übersprungen.", m_currentAudioFile);
        return true;
    }

    // Konstanten
    constexpr uint32_t MAX_COVER_FILE_SIZE = 1 * 1000 * 1000; // Max. 1 MB für Cover-Datei
    constexpr uint32_t MIN_IMAGE_SIZE = 100;                  // Minimale sinnvolle Bilddatengröße
    constexpr int DOWNSCALE_THRESHOLD = 500;                  // Ab dieser Größe JPEGDEC-Skalierung nutzen
    constexpr int MAX_IMAGE_DIMENSION = 500;                  // Maximale PNG-Auflösung; größere PNGs werden abgelehnt, JPEG wird stattdessen herunterskaliert

    // Eingangsprüfung: Offset/Länge aus dem Event
    if (m_coverOffsets.size() < 2)
    {
        LOG_ERROR("Cover-Offsets ungültig");
        return false;
    }

    const uint32_t coverOffset = m_coverOffsets[0];
    const uint32_t coverLength = m_coverOffsets[1];

    // Validierung der Cover-Dateigröße
    if (coverLength == 0 || coverLength > MAX_COVER_FILE_SIZE)
    {
        LOG_ERROR_F("Cover-Länge ungültig: %u (max: %u)", coverLength, MAX_COVER_FILE_SIZE);
        return false;
    }

    // ===================================================================
    // Grow-only PSRAM-Buffer
    // ===================================================================
    // Der Puffer wird nur vergrößert und nie verkleinert, damit nicht ständig unterschiedlich große Blöcke freigegeben und neu allokiert werden.
    if (coverLength > m_coverBufferCapacity)
    {
        // Alten Buffer freigeben
        if (m_coverBuffer)
        {
            heap_caps_free(m_coverBuffer);
            m_coverBuffer = nullptr;
            m_coverBufferCapacity = 0;
        }

        // Neuen, größeren Buffer allokieren (mit 10% Reserve für zukünftige Cover)
        const size_t newCapacity = coverLength + (coverLength / 10);
        m_coverBuffer = static_cast<uint8_t *>(heap_caps_malloc(newCapacity, MALLOC_CAP_SPIRAM));

        if (!m_coverBuffer)
        {
            LOG_ERROR_F("PSRAM alloc fehlgeschlagen (benötigt: %u Bytes)", newCapacity);
            return false;
        }

        m_coverBufferCapacity = newCapacity;
        LOG_INFO_F("Cover-Buffer vergrößert: %u Bytes", newCapacity);
    }

    // File-Handling mit automatischem Close
    File f = SD.open(m_currentAudioFile, FILE_READ);
    if (!f)
    {
        LOG_ERROR_F("Datei '%s' kann nicht geöffnet werden.", m_currentAudioFile);
        return false;
    }

    // Schließt automatisch die Datei beim Verlassen
    struct FileGuard
    {
        File &file;
        explicit FileGuard(File &f) : file(f) {}
        ~FileGuard()
        {
            if (file)
                file.close();
        }
    } fileGuard(f);

    if (!f.seek(coverOffset))
    {
        LOG_ERROR("Seek fehlgeschlagen.");
        return false;
    }

    // Cover-Daten in wiederverwendbaren Buffer lesen
    const size_t n = f.read(m_coverBuffer, coverLength);

    if (n != coverLength)
    {
        LOG_ERROR_F("Cover unvollständig: %u/%u", (unsigned)n, (unsigned)coverLength);
        return false;
    }

    // Container-Parse (APIC, FLAC) + Fallback
    // Hilfsfunktion: 32-Bit Big-Endian lesen
    auto rd32 = [](const uint8_t *p) -> uint32_t
    {
        return (uint32_t(p[0]) << 24) | (uint32_t(p[1]) << 16) | (uint32_t(p[2]) << 8) | uint32_t(p[3]);
    };

    // PNG-IHDR-Parser: prüft Validität und liefert Dimensionen
    auto parse_png_ihdr = [&](const uint8_t *ptr, uint32_t totalLen, int &w, int &h) -> bool
    {
        const uint8_t PNG_SIG[8] = {0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A};
        const uint8_t *p = ptr;
        uint32_t len = totalLen;

        // Falls PNG-Signatur am Anfang -> überspringen
        if (len >= 8 && memcmp(p, PNG_SIG, 8) == 0)
        {
            p += 8;
            len -= 8;
        }

        // Mindestens [length(4)][type(4)] nötig
        if (len < 8)
            return false;

        // Erster Chunk sollte IHDR sein
        uint32_t chunkDataLen = rd32(p);
        const uint8_t *chunkType = p + 4;
        p += 8;
        len -= 8;

        // Validierung
        if (len < chunkDataLen + 4)
            return false; // Daten + CRC müssen noch da sein
        if (memcmp(chunkType, "IHDR", 4) != 0)
            return false;
        if (chunkDataLen != 13)
            return false; // IHDR hat exakt 13 Bytes

        w = (int)rd32(p + 0);
        h = (int)rd32(p + 4);

        // Sanity checks für Dimensionen
        if (w <= 0 || h <= 0)
        {
            LOG_ERROR_F("PNG ungültige Dimensionen: %dx%d", w, h);
            return false;
        }
        if (w > MAX_IMAGE_DIMENSION || h > MAX_IMAGE_DIMENSION)
        {
            LOG_WARNING_F("PNG zu groß für PSRAM: %dx%d (max: %dx%d)", w, h, MAX_IMAGE_DIMENSION, MAX_IMAGE_DIMENSION);
            return false;
        }

        return true;
    };

    uint8_t *img = nullptr; // Zeiger auf den Anfang der eigentlichen Bilddaten
    uint32_t imgLen = 0;    // exakte Länge der Bilddaten

    // ID3v2 APIC (MP3): [encoding(1)][MIME bis NUL][picType(1)][desc (term.)][Bilddaten ...]
    if (!img && coverLength >= 10 && m_coverBuffer[0] <= 0x03)
    { // 0..3 sind gültige ID3-Encoding-Werte
        uint32_t p = 1;
        constexpr uint32_t MAX_MIME_LEN = 50; // MIME-Type sollte nicht länger sein

        // MIME suchen (bis NUL oder MAX_MIME_LEN)
        while (p < coverLength && p < MAX_MIME_LEN && m_coverBuffer[p] != 0x00)
            ++p;

        // Robustheits-Check: Prüfen ob tatsächlich NUL-Terminator gefunden wurde (nicht nur Abbruch wegen MAX_MIME_LEN erreicht)
        const bool mimeTerminated = (p < coverLength && m_coverBuffer[p] == 0x00);

        if (mimeTerminated && p >= 10 && memmem(m_coverBuffer + 1, p - 1, "image/", 6) != nullptr)
        {
            ++p; // NUL hinter MIME überspringen
            if (p < coverLength)
                ++p; // 1 Byte picture type

            const uint8_t enc = m_coverBuffer[0];
            if (enc == 0x00 || enc == 0x03)
            { // ISO-8859-1 / UTF-8: 0x00-terminiert
                while (p < coverLength && m_coverBuffer[p] != 0x00)
                    ++p;
                if (p < coverLength)
                    ++p; // NUL der Description überspringen
            }
            else
            { // UTF-16: 0x00 0x00-terminiert
                while (p + 1 < coverLength && !(m_coverBuffer[p] == 0x00 && m_coverBuffer[p + 1] == 0x00))
                    p += 2;
                if (p + 1 < coverLength)
                    p += 2;
            }

            if (p < coverLength)
            {
                img = m_coverBuffer + p;
                imgLen = coverLength - p;
            }
        }
    }

    // FLAC PICTURE: [type(4)][mimeLen(4)][mime][descLen(4)][desc][w(4)][h(4)][depth(4)][colors(4)][dataLen(4)][data]
    if (!img && coverLength >= 32)
    { // FLAC Picture braucht mindestens 32 Bytes Header
        const uint32_t mimeLen = rd32(m_coverBuffer + 4);

        if (mimeLen > 0 && mimeLen < 100 && (8 + mimeLen) < coverLength &&
            memmem(m_coverBuffer + 8, mimeLen, "image/", 6) != nullptr)
        {
            uint32_t p = 8 + mimeLen; // hinter MIME

            // descLen lesen - erst Bounds prüfen, dann lesen, dann addieren
            if (p + 4 <= coverLength)
            {
                const uint32_t descLen = rd32(m_coverBuffer + p);

                // Overflow-/Bounds-Check VOR der Addition: descLen darf nicht größer als verbleibender Platz sein
                if (descLen <= coverLength && p + 4 + descLen <= coverLength)
                {
                    p += 4 + descLen; // Description überspringen

                    // w(4) + h(4) + depth(4) + colors(4) + dataLen(4) = 20 Bytes
                    if (p + 20 <= coverLength)
                    {
                        p += 16; // w, h, depth, colors überspringen
                        const uint32_t dataLen = rd32(m_coverBuffer + p);
                        p += 4;

                        // Finale Bounds-Prüfung für die Bilddaten
                        if (dataLen <= coverLength && p + dataLen <= coverLength)
                        {
                            img = m_coverBuffer + p;
                            imgLen = dataLen; // exakte Bilddaten-Länge aus dem Container
                        }
                    }
                }
            }
        }
    }

    // Fallback: JPEG/PNG im Block suchen
    if (!img)
    {
        for (uint32_t i = 0; i + 8 <= coverLength; ++i)
        {
            // JPEG SOI + JFIF/Exif Marker
            if (m_coverBuffer[i] == 0xFF && m_coverBuffer[i + 1] == 0xD8 && m_coverBuffer[i + 2] == 0xFF)
            {
                img = m_coverBuffer + i;
                imgLen = coverLength - i;
                break;
            }

            // PNG Signatur (8 Bytes)
            if (m_coverBuffer[i] == 0x89 && m_coverBuffer[i + 1] == 0x50 && m_coverBuffer[i + 2] == 0x4E &&
                m_coverBuffer[i + 3] == 0x47 && m_coverBuffer[i + 4] == 0x0D && m_coverBuffer[i + 5] == 0x0A &&
                m_coverBuffer[i + 6] == 0x1A && m_coverBuffer[i + 7] == 0x0A)
            {
                img = m_coverBuffer + i;
                imgLen = coverLength - i;
                break;
            }
        }

        if (!img)
        {
            LOG_ERROR("Kein JPEG/PNG erkannt");
            return false;
        }
    }

    // Minimale Bildgröße prüfen
    if (imgLen < MIN_IMAGE_SIZE)
    {
        LOG_ERROR_F("Bilddaten zu klein: %u Bytes", imgLen);
        return false;
    }

    // Format bestimmen mit erweiterter Validierung
    const bool isJPG = (imgLen >= 3 && img[0] == 0xFF && img[1] == 0xD8 && img[2] == 0xFF); // SOI
    const bool isPNG = (imgLen >= 8 && img[0] == 0x89 && img[1] == 0x50 && img[2] == 0x4E && img[3] == 0x47 && img[4] == 0x0D && img[5] == 0x0A && img[6] == 0x1A && img[7] == 0x0A);

    // Frühzeitige Format-Validierung
    if (!isJPG && !isPNG)
    {
        LOG_ERROR("Unbekanntes oder beschädigtes Bildformat.");
        return false;
    }

    // Ziel-Sprite vorbereiten / leeren
    m_sprCover->clear(0x1082);

    bool ok = false; // Gesamterfolg

    // Dekodierung / Skalierung
    if (isJPG)
    {
        // RAII Guard für JPEG Decoder - schließt automatisch bei Fehler oder Erfolg
        struct JpegGuard
        {
            JPEGDEC &decoder;
            explicit JpegGuard(JPEGDEC &dec) : decoder(dec) {}
            ~JpegGuard() { decoder.close(); }
        } jpegGuard(s_jpeg);

        // JPEG öffnen
        if (s_jpeg.openRAM(img, (int)imgLen, jpegDrawCB) != 1)
        {
            LOG_ERROR("JPEG openRAM() fehlgeschlagen.");
            return false; // JpegGuard räumt automatisch auf
        }

        // JPEG-Quellgröße
        const int srcW = s_jpeg.getWidth();
        const int srcH = s_jpeg.getHeight();

        LOG_INFO_F("JPEG Originalgröße: %dx%d", srcW, srcH);

        // Stufenwahl mit Threshold
        auto chooseScaleBitsCover = [](int w, int h, int minW, int minH) -> int
        {
            // 0=1:1, 1=1/2, 2=1/4, 3=1/8: größte Vorskalierung, deren Ausgabe das Feld in beiden Richtungen noch abdeckt
            int bits = 0;
            while (bits < 3 && (w >> (bits + 1)) >= minW && (h >> (bits + 1)) >= minH)
                ++bits;
            return bits;
        };

        int scaleBits = 0; // Standard: keine JPEGDEC-Skalierung
        if (srcW > DOWNSCALE_THRESHOLD || srcH > DOWNSCALE_THRESHOLD)
            scaleBits = chooseScaleBitsCover(srcW, srcH, COVER_W, COVER_H);

        // Performance-Optimierung für große Bilder ohne JPEGDEC-Skalierung
        if (scaleBits == 0 && (srcW * srcH) > 250000)
            s_jpeg.setMaxOutputSize(32 * 1024); // 32KB Blöcke statt default
        else if (scaleBits > 0)
            s_jpeg.setMaxOutputSize(16 * 1024); // 16KB bei Skalierung

        int opts = 0;
        if (scaleBits == 1)
            opts = JPEG_SCALE_HALF;
        else if (scaleBits == 2)
            opts = JPEG_SCALE_QUARTER;
        else if (scaleBits == 3)
            opts = JPEG_SCALE_EIGHTH;

        // Effektive Decoder-Ausgabegröße
        const int outW = std::max(1, srcW >> scaleBits);
        const int outH = std::max(1, srcH >> scaleBits);

        LOG_INFO_F("JPEG decode: %dx%d -> %dx%d (scale 1/%d)", srcW, srcH, outW, outH, 1 << scaleBits);

        // ===================================================================
        // Temporäres Quell-Sprite mit exakter Decoder-Ausgabegröße
        // ===================================================================
        // Das Sprite muss die exakte Ausgabegröße haben, da pushRotateZoom den gesamten Sprite-Inhalt transformiert. Bei falscher Größe entstehen Artefakte durch mittransformierten ungenutzten Bereich. Der nur wachsende Cover-Puffer m_coverBuffer reduziert zusätzliche Speicherbewegungen.
        LGFX_Sprite sprSrc(m_tft);
        sprSrc.setPsram(true);

        if (!sprSrc.createSprite(outW, outH))
        {
            LOG_ERROR_F("sprSrc.createSprite(%dx%d) fehlgeschlagen - zu wenig PSRAM?", outW, outH);
            return false;
        }

        // JPEG in temporäres Sprite dekodieren
        s_jpeg.setUserPointer(&sprSrc);
        s_jpeg.setPixelType(RGB565_BIG_ENDIAN); // für LGFX-Sprite passend

        ok = (s_jpeg.decode(0, 0, opts) == 1);
        // JPEG wird automatisch durch JpegGuard geschlossen

        if (ok)
        {
            // Ausfüllen ohne Verzerrung: isotrop so skalieren, dass COVER_W x COVER_H bedeckt ist; der Überstand wird vom Sprite-Rand mittig beschnitten
            const float sx = (float)COVER_W / (float)outW;
            const float sy = (float)COVER_H / (float)outH;
            const float s = (sx > sy) ? sx : sy;

            m_sprCover->setPivot(COVER_W / 2, COVER_H / 2);
            sprSrc.pushRotateZoom(m_sprCover, COVER_W / 2, COVER_H / 2, 0.0f, s, s);
        }
        else
        {
            LOG_ERROR("JPEG decode() fehlgeschlagen.");
        }

        // Temporäres Sprite freigeben
        sprSrc.deleteSprite();
    }
    else if (isPNG)
    {
        // PNG: keine Verzerrung -> Quellgröße aus IHDR lesen, isotrop ausfüllend zoomen und zentrieren
        int srcW = 0, srcH = 0;
        if (!parse_png_ihdr(img, imgLen, srcW, srcH))
        {
            LOG_ERROR("PNG IHDR ungültig/fehlt");
            return false;
        }

        LOG_INFO_F("PNG Originalgröße: %dx%d", srcW, srcH);

        // isotrope Skalierung, das Feld wird in beiden Richtungen bedeckt
        const float sx = (float)COVER_W / (float)srcW;
        const float sy = (float)COVER_H / (float)srcH;
        const float s = (sx > sy) ? sx : sy;

        // Zielposition so wählen, dass das Bild im COVER_W x COVER_H Sprite mittig liegt; negative Werte bedeuten Beschnitt, der Dekoder rechnet sie in einen Bildversatz um
        const int dstW = std::max(1, (int)std::lround(srcW * s));
        const int dstH = std::max(1, (int)std::lround(srcH * s));
        const int dstX = (COVER_W - dstW) / 2;
        const int dstY = (COVER_H - dstH) / 2;

        // PNG mit Zoom-Parametern Signatur: drawPng(data, len, x, y, maxW, maxH, offX, offY, zoom_x, zoom_y)
        ok = m_sprCover->drawPng(img, imgLen, dstX, dstY, 0, 0, 0, 0, s, s); // isotrope Skalierung

        if (!ok)
            LOG_ERROR("PNG drawPng() fehlgeschlagen");
    }

    // Anzeige / Rückgabe
    if (!ok)
    {
        drawCoverFallback(nullptr);
        LOG_ERROR("Cover-Dekodierung fehlgeschlagen");
        return false;
    }

    // Erfolgsfall: Sprite aufs Panel pushen
    m_sprCover->pushSprite(COVER_X, COVER_Y);
    m_hasCover = true;

    // Cover-Tracking aktualisieren
    stringutils::safeCopy(m_lastDisplayedCoverFile, m_currentAudioFile);
    m_lastDisplayedCoverOffsets = m_coverOffsets;

    LOG_SUCCESS("Cover angezeigt");
    return true;
}

/**
 * @brief Zeichnet ein generisches Fallback-Cover
 */
void SdPlayer::drawCoverFallback(const char *preferredText)
{
    if (!m_sprCover)
        return;

    // Während Dialog/Grafik-Sperre nur vormerken: der Flag-Pfad in draw() zeichnet das Fallback nach dem Schließen
    if (m_uiSuspended)
    {
        m_needsCoverRedraw = true;
        return;
    }

    // Intelligentes Caching: Nur skippen wenn bereits ein Fallback für diese Datei angezeigt wird. Nicht skippen bei Init (wenn m_lastDisplayedCoverFile leer ist).
    if (!m_hasCover && m_lastDisplayedCoverFile[0] != '\0' && strcmp(m_currentAudioFile, m_lastDisplayedCoverFile) == 0 && m_lastDisplayedCoverOffsets.empty())
        return;

    // Basistext bestimmen (Parameter -> bereinigter Dateiname)
    char baseName[128] = {0};
    if (preferredText && preferredText[0] != '\0')
    {
        stringutils::safeCopy(baseName, preferredText);
    }
    else if (m_currentAudioFile[0])
    {
        const char *p = strrchr(m_currentAudioFile, '/');
        p = p ? p + 1 : m_currentAudioFile;
        cleanupFilename(p, baseName, sizeof(baseName));
    }
    else
    {
        stringutils::safeCopy(baseName, reinterpret_cast<PGM_P>(STR(SDPlayerKey::SDPLAYER_COVER_NOCOVER)));
    }

    // Cover-Sprite leeren und Bild einfügen
    m_sprCover->clear(0x1082);
    m_sprCover->pushImage(0, 0, COVER_W, COVER_H, nocover);

    // links und rechts Rahmenlinien zeichnen
    m_sprCover->drawFastVLine(0, 0, COVER_H, Theme::ColorButtonBorder);
    m_sprCover->drawFastVLine(COVER_W - 1, 0, COVER_H, Theme::ColorButtonBorder);

    // Initialen nur zeichnen, wenn ein Track vorhanden ist
    const bool hasTrackText = (m_currentAudioFile[0] != '\0' || (preferredText && preferredText[0] != '\0'));
    if (hasTrackText)
    {
        // Initialen aus Wortanfängen (max. 3 Zeichen)
        char initials[4] = {0};
        {
            int n = 0;
            bool inWord = false;
            for (const char *p = baseName; *p && n < 3; ++p)
            {
                if (isalnum((unsigned char)*p) && !inWord)
                {
                    initials[n++] = (char)toupper((unsigned char)*p);
                    inWord = true;
                }
                else if (!isalnum((unsigned char)*p))
                {
                    inWord = false;
                }
            }
            if (n == 0)
            {
                initials[0] = '?';
                initials[1] = '\0';
            }
            else
            {
                initials[n] = '\0';
            }
        }

        m_sprCover->setFont(&fonts::Orbitron_Light_32);
        m_sprCover->setTextDatum(TL_DATUM);
        m_sprCover->setTextColor(Theme::ColorRetroRed);
        m_sprCover->drawString(initials, 15, 15);
        m_sprCover->unloadFont();
    }

    // Textzeile unten: mit Titel der Name rechtsbündig, ohne Titel der Platzhalter zentriert mit hellem erstem Wort und Rest in Orange
    m_sprCover->loadFont(ComfortaaBold16);
    if (hasTrackText)
    {
        m_sprCover->setTextColor(Theme::ColorAmber);
        m_sprCover->setTextDatum(BR_DATUM);
        char ell[100];
        ell[0] = '\0';
        truncateWithEllipsis(m_sprCover, baseName, ell, sizeof(ell), COVER_W - 15);
        m_sprCover->drawString(ell, COVER_W - 15, COVER_H - 15);
    }
    else
    {
        constexpr int TEXT_MARGIN = 15;
        char firstWord[sizeof(baseName)];
        stringutils::safeCopy(firstWord, baseName);
        const char *rest = "";
        char *space = strchr(firstWord, ' ');
        if (space)
        {
            *space = '\0';
            rest = baseName + (space - firstWord);
        }
        const int firstW = m_sprCover->textWidth(firstWord);
        const int totalW = firstW + m_sprCover->textWidth(rest);
        const int textX = (COVER_W - totalW) / 2;
        m_sprCover->setTextDatum(BL_DATUM);
        m_sprCover->setTextColor(Theme::ColorMainText);
        m_sprCover->drawString(firstWord, textX, COVER_H - TEXT_MARGIN);
        m_sprCover->setTextColor(Theme::ColorTitle);
        m_sprCover->drawString(rest, textX + firstW, COVER_H - TEXT_MARGIN);
    }
    m_sprCover->setTextDatum(TL_DATUM);
    m_sprCover->unloadFont();

    // Ausgabe
    m_sprCover->pushSprite(COVER_X, COVER_Y);
    m_hasCover = false;

    // Cover-Tracking aktualisieren (Fallback = kein echtes Cover)
    stringutils::safeCopy(m_lastDisplayedCoverFile, m_currentAudioFile);
    m_lastDisplayedCoverOffsets.clear();
}

/**
 * @brief Details unter dem Cover zeichnen (Album, Format, Bitrate, Track xx/xx etc.)
 */
void SdPlayer::drawDetails(bool init)
{
    // Sprite-Initialisierung
    if (init)
    {
        if (!m_tft->safeCreateSprite(m_sprDetails, COVER_W, 45))
        {
            LOG_ERROR("Konnte m_sprDetails Sprite nicht erstellen!");
            return;
        }

        m_sprDetails->clear(0x2104);
        m_sprDetails->drawFastVLine(0, 0, m_sprDetails->height(), Theme::ColorButtonBorder);
        m_sprDetails->drawFastVLine(m_sprDetails->width() - 1, 0, m_sprDetails->height(), Theme::ColorButtonBorder);
        m_sprDetails->pushSprite(COVER_X, PLAYER_CONTROLS_Y);

        // Kapazität der SD-Karte einmal lesen, sie ändert sich im Betrieb nicht
        m_sdTotalBytes = SD.totalBytes();
        m_sdUsedBytes = SD.usedBytes();

        // Cache zurücksetzen
        m_lastAlbumDrawn[0] = '\0';
        m_lastFileTypeDrawn[0] = '\0';
        m_lastBitrateDrawn[0] = '\0';
        m_lastTrackIndexDrawn = -1;
        m_lastPlaylistSizeDrawn = -1;

        return;
    }

    // Nur neu zeichnen, wenn sich etwas geändert hat
    if (!m_needsDetailsRedraw)
        return;

    // Aktuellen Dateityp ermitteln
    char currentFileType[8] = {0};
    if (m_currentAudioFile[0] != '\0')
    {
        const char *ext = strrchr(m_currentAudioFile, '.');
        if (ext && ext[1] != '\0')
        {
            // Extension ohne Punkt, in Großbuchstaben
            const char *extName = ext + 1;
            for (int i = 0; i < 7 && extName[i] != '\0'; i++)
                currentFileType[i] = toupper(extName[i]);
        }
    }

    // Track-Info erstellen
    char trackInfo[64] = {0};
    if (!m_playlist.empty() && m_currentTrackIndex >= 0 && m_currentTrackIndex < m_playlist.size())
        STRF(trackInfo, SDPlayerKey::SDPLAYER_TRACK_INFO_FORMAT, m_currentTrackIndex + 1, (int)m_playlist.size());

    // Prüfen ob sich etwas geändert hat
    bool albumChanged = (strcmp(m_lastAlbumDrawn, m_currentAlbum) != 0);
    bool fileTypeChanged = (strcmp(m_lastFileTypeDrawn, currentFileType) != 0);
    bool bitrateChanged = (strcmp(m_lastBitrateDrawn, m_displayBitrate) != 0);
    bool trackChanged = (m_lastTrackIndexDrawn != m_currentTrackIndex || m_lastPlaylistSizeDrawn != (int)m_playlist.size());

    if (!albumChanged && !fileTypeChanged && !bitrateChanged && !trackChanged)
    {
        m_needsDetailsRedraw = false;
        return;
    }

    // Sprite neu zeichnen
    m_sprDetails->clear(0x2104);

    // Rahmenlinien
    m_sprDetails->drawFastVLine(0, 0, m_sprDetails->height(), Theme::ColorButtonBorder);
    m_sprDetails->drawFastVLine(m_sprDetails->width() - 1, 0, m_sprDetails->height(), Theme::ColorButtonBorder);

    // Badges rechts nebeneinander - jedes Badge hat seine eigene Breite
    const int badgeH = 18;
    const int badgeSpacing = 2;
    const int badgeRightMargin = 5;

    bool hasBitrate = (m_displayBitrate[0] != '\0');
    bool hasFileType = (currentFileType[0] != '\0');

    int bitrateW = 0;
    int fileTypeW = 0;

    // Badge-Breiten berechnen
    m_sprDetails->loadFont(ComfortaaBold10);

    if (hasBitrate)
        bitrateW = m_sprDetails->textWidth(m_displayBitrate) + 8; // Padding

    if (hasFileType)
        fileTypeW = m_sprDetails->textWidth(currentFileType) + 8; // Padding

    // Positionen berechnen (von rechts nach links)
    const int fileTypeBadgeX = m_sprDetails->width() - fileTypeW - badgeRightMargin;
    const int bitrateBadgeX = fileTypeBadgeX - bitrateW - badgeSpacing;
    const int badgeY = 23;

    // Bitrate-Badge zeichnen (nur wenn eine Bitrate vorhanden ist)
    if (hasBitrate)
    {
        // Badge zeichnen
        m_sprDetails->fillRect(bitrateBadgeX, badgeY, bitrateW, badgeH, 0x6C64);

        // Text zentrieren
        m_sprDetails->setTextDatum(MC_DATUM);
        m_sprDetails->setTextColor(TFT_BLACK);
        m_sprDetails->drawString(m_displayBitrate, bitrateBadgeX + bitrateW / 2, badgeY + badgeH / 2);
    }

    // FileType-Badge zeichnen
    if (hasFileType)
    {
        // Farbe je nach Dateityp
        uint16_t badgeColor = TFT_DARKGREY;
        if (strcmp(currentFileType, "MP3") == 0)
            badgeColor = 0x5370; // Blau
        else if (strcmp(currentFileType, "FLAC") == 0)
            badgeColor = 0x07E0; // Grün
        else if (strcmp(currentFileType, "WAV") == 0)
            badgeColor = 0xFD20; // Orange
        else if (strcmp(currentFileType, "AAC") == 0 || strcmp(currentFileType, "M4A") == 0)
            badgeColor = 0x7BEF; // Hellblau
        else if (strcmp(currentFileType, "OGG") == 0)
            badgeColor = 0xF81F; // Magenta

        // Badge zeichnen
        m_sprDetails->fillRect(fileTypeBadgeX, badgeY, fileTypeW, badgeH, badgeColor);

        // Text zentrieren
        m_sprDetails->setTextDatum(MC_DATUM);
        m_sprDetails->setTextColor(TFT_BLACK);
        m_sprDetails->drawString(currentFileType, fileTypeBadgeX + fileTypeW / 2, badgeY + badgeH / 2);
    }

    m_sprDetails->unloadFont();

    // Ohne geladenen Titel zeigt die Box SD-Kapazität und Playlist-Kurzfassung oder Hinweis, sonst Album-Text und Track-Info
    if (m_playlist.empty() || m_currentAudioFile[0] == '\0')
    {
        drawIdleDetails();
    }
    else
    {
        // Maximale Textbreite berechnen
        int badgesWidth = 0;
        if (hasBitrate && hasFileType)
            badgesWidth = bitrateW + fileTypeW + badgeSpacing + badgeRightMargin;
        else if (hasFileType)
            badgesWidth = fileTypeW + badgeRightMargin;
        else if (hasBitrate)
            badgesWidth = bitrateW + badgeRightMargin;

        const int textX = 8;
        const int albumY = 6;
        const int trackY = 26;
        const int maxAlbumWidth = m_sprDetails->width() - textX - 10;              // Album nutzt volle Breite
        const int maxTrackWidth = m_sprDetails->width() - textX - badgesWidth - 5; // Tracknummer lässt Platz für Badges

        m_sprDetails->loadFont(ComfortaaBold14);
        m_sprDetails->setTextDatum(TL_DATUM);
        m_sprDetails->setTextColor(Theme::ColorMainText);

        // Album-Text mit intelligentem Fallback
        char albumText[128];
        if (m_currentAlbum[0] != '\0')
        {
            // 1. Priorität: ID3-Album-Tag
            stringutils::safeCopy(albumText, m_currentAlbum);
        }
        else if (m_currentAudioFile[0] != '\0')
        {
            // 2. Priorität: Verzeichnisname als "Pseudo-Album"
            // Pfad z.B.: "/sd/Musik/Pink Floyd/The Dark Side of the Moon/01 - Track.mp3"
            // Extrakt: "The Dark Side of the Moon"
            if (!parentDirName(m_currentAudioFile, albumText, sizeof(albumText)))
                stringutils::safeCopy(albumText, reinterpret_cast<PGM_P>(STR(SDPlayerKey::SDPLAYER_ALBUM_UNKNOWN)));
        }
        else
        {
            // Fallback: Kein Album und kein Pfad
            stringutils::safeCopy(albumText, reinterpret_cast<PGM_P>(STR(SDPlayerKey::SDPLAYER_ALBUM_UNKNOWN)));
        }
        albumText[sizeof(albumText) - 1] = '\0';

        // Bei Überlänge kürzen und "..." anhängen
        char albumDrawn[sizeof(albumText)];
        truncateWithEllipsis(m_sprDetails, albumText, albumDrawn, sizeof(albumDrawn), maxAlbumWidth);

        m_sprDetails->drawString(albumDrawn, textX, albumY);

        // Track-Info
        if (trackInfo[0] != '\0')
        {
            // Prüfen ob Track-Info in den verfügbaren Platz passt
            int trackWidth = m_sprDetails->textWidth(trackInfo);
            if (trackWidth <= maxTrackWidth)
                m_sprDetails->drawString(trackInfo, textX, trackY);
        }

        m_sprDetails->unloadFont();
    }

    // Sprite auf Display pushen
    m_sprDetails->pushSprite(COVER_X, PLAYER_CONTROLS_Y);

    // Cache aktualisieren
    stringutils::safeCopy(m_lastAlbumDrawn, m_currentAlbum);
    stringutils::safeCopy(m_lastFileTypeDrawn, currentFileType);
    stringutils::safeCopy(m_lastBitrateDrawn, m_displayBitrate);
    m_lastTrackIndexDrawn = m_currentTrackIndex;
    m_lastPlaylistSizeDrawn = (int)m_playlist.size();

    m_needsDetailsRedraw = false;
}

/**
 * @brief Infobox ohne geladenen Titel: SD-Kapazität oben, darunter Playlist-Kurzfassung oder Hinweis auf die Durchsuchen-Taste
 */
void SdPlayer::drawIdleDetails()
{
    constexpr int TEXT_X = 8;
    constexpr int LINE1_Y = 6;
    constexpr int LINE2_Y = 26;
    constexpr uint64_t GIB = 1024ULL * 1024ULL * 1024ULL;
    const int maxWidth = m_sprDetails->width() - 2 * TEXT_X;

    // Zeile 1: freier und gesamter Platz in Zehntel-GB, das Dezimaltrennzeichen liefert der Sprachtext
    char line[160];
    if (m_sdTotalBytes > 0)
    {
        const uint64_t freeBytes = (m_sdTotalBytes > m_sdUsedBytes) ? (m_sdTotalBytes - m_sdUsedBytes) : 0;
        const unsigned freeTenths = (unsigned)((freeBytes * 10ULL + GIB / 2) / GIB);
        const unsigned totalTenths = (unsigned)((m_sdTotalBytes * 10ULL + GIB / 2) / GIB);
        STRF(line, SDPlayerKey::SDPLAYER_SD_CAPACITY_FORMAT, freeTenths / 10, freeTenths % 10, totalTenths / 10, totalTenths % 10);
    }
    else
    {
        stringutils::safeCopy(line, reinterpret_cast<PGM_P>(STR(SDPlayerKey::SDPLAYER_SD_NONE)));
    }

    m_sprDetails->loadFont(ComfortaaBold14);
    m_sprDetails->setTextDatum(TL_DATUM);
    m_sprDetails->setTextColor(Theme::ColorMainText);
    char drawn[sizeof(line)];
    truncateWithEllipsis(m_sprDetails, line, drawn, sizeof(drawn), maxWidth);
    m_sprDetails->drawString(drawn, TEXT_X, LINE1_Y);

    // Zeile 2: Anzahl und Ordner der geladenen Titel, ohne Playlist der Hinweis auf die Durchsuchen-Taste
    if (m_playlist.empty())
    {
        stringutils::safeCopy(line, reinterpret_cast<PGM_P>(STR(SDPlayerKey::SDPLAYER_HINT_ADD_TRACKS)));
        m_sprDetails->setTextColor(Theme::ColorButtonText);
    }
    else
    {
        const size_t idx = (m_currentTrackIndex >= 0 && (size_t)m_currentTrackIndex < m_playlist.size()) ? (size_t)m_currentTrackIndex : 0;
        char folder[128];
        if (!parentDirName(m_playlist[idx].path, folder, sizeof(folder)))
            stringutils::safeCopy(folder, reinterpret_cast<PGM_P>(STR(SDPlayerKey::SDPLAYER_ALBUM_UNKNOWN)));
        if (m_playlist.size() == 1)
            STRF(line, SDPlayerKey::SDPLAYER_PLAYLIST_SUMMARY_ONE_FORMAT, folder);
        else
            STRF(line, SDPlayerKey::SDPLAYER_PLAYLIST_SUMMARY_FORMAT, (int)m_playlist.size(), folder);
    }
    truncateWithEllipsis(m_sprDetails, line, drawn, sizeof(drawn), maxWidth);
    m_sprDetails->drawString(drawn, TEXT_X, LINE2_Y);
    m_sprDetails->unloadFont();
}

// ===================================================================
// Initialisierung
// ===================================================================

/**
 * @brief Initialisiert das SD-Player Modul
 */
bool SdPlayer::init(DS1307_RTC &rtc, const SdPlayerCallbacks &callbacks)
{
    m_rtc = &rtc;
    m_callbacks = callbacks;

    // ===================================================================
    // Initiale Kapazität für Vektoren reservieren
    // ===================================================================
    // shrink_to_fit() wird vermieden, da es free und realloc auslöst und den Heap fragmentiert. Die Kapazität wird stattdessen beim Start einmalig mit reserve() festgelegt.
    m_playlist.reserve(PLAYLIST_RESERVE_CAPACITY);
    m_playedTracks.reserve(PLAYED_TRACKS_RESERVE_CAPACITY);
    m_eventCoverData.reserve(COVER_EVENT_RESERVE_CAPACITY);

    // Dialoge erzeugen
    m_explorer = std::make_unique<FileExplorer>(m_tft);
    m_messageBox = std::make_unique<MessageBox>(m_tft);
    m_sleepDlg = std::make_unique<SleepTimer>(m_tft);

    // Komplettes Display mit schwarz leeren
    m_tft->clear(TFT_BLACK);

    // Div-Sprites erstellen
    if (!m_tft->safeCreateSprite(m_sprDiv, 800, 3))
    {
        LOG_ERROR("Konnte Div-Sprites nicht erstellen");
        return false;
    }

    // Hintergrund zeichnen
    m_tft->fillRect(0, 5, 800 - PLAYLIST_W - SCROLLBAR_W - 10, 300, 0x1082);

    // ===================================================================
    // Kassette
    // ===================================================================
    if (!m_tft->safeCreateSprite(m_sprCassette, CASSETTE_W, CASSETTE_H))
    {
        LOG_ERROR("Konnte Kassette Sprite nicht erstellen!");
        return false;
    }

    if (!m_tft->safeCreateSprite(m_sprWrap, WRAP_SPR_SIZE, WRAP_SPR_SIZE) ||
        !m_tft->safeCreateSprite(m_sprDorn, WRAP_SPR_SIZE, WRAP_SPR_SIZE))
    {
        LOG_ERROR("Konnte Wickeldorn Sprite nicht erstellen!");
        return false;
    }

    if (!m_tft->safeCreateSprite(m_sprTapeLabel, TAPE_LABEL_W, TAPE_LABEL_H))
    {
        LOG_ERROR("Konnte Tape-Label Sprite nicht erstellen!");
        return false;
    }

    m_sprTapeLabel->setTextWrap(false);
    m_sprTapeLabel->setTextDatum(MC_DATUM); // Middle-Center

    // Startdesign würfeln und die komplette Kassette aufbauen
    m_cassetteDesignIdx = random(CASSETTE_DESIGN_COUNT);
    applyCassetteDesign();

    // ===================================================================
    // Cover-Sprite erstellen und Platzhalter anzeigen
    // ===================================================================
    if (!m_tft->safeCreateSprite(m_sprCover, COVER_W, COVER_H))
    {
        LOG_ERROR("Konnte Cover Sprite nicht erstellen!");
        return false;
    }

    drawCoverFallback(nullptr);

    // Separatoren zeichnen
    m_sprDiv->clear(0x79E2);
    m_sprDiv->fillRect(800 - PLAYLIST_W - SCROLLBAR_W - 10, 0, 10, 3, TFT_BLACK);
    m_sprDiv->pushSprite(0, 0);
    m_sprDiv->pushSprite(0, 307);

    // Control-Buttons initialisieren
    for (int i = 0; i < 4; i++)
    {
        m_controlButtons[i].initButton(
            m_tft,                                     // TFT-Zeichenobjekt
            m_buttonDefs[i].x + m_buttonDefs[i].w / 2, // X-Position
            m_buttonDefs[i].y + m_buttonDefs[i].h / 2, // Y-Position
            m_buttonDefs[i].w,                         // Breite
            m_buttonDefs[i].h,                         // Höhe
            m_buttonDefs[i].r,                         // Eckradius
            m_buttonDefs[i].outlinecolor,              // Randfarbe
            m_buttonDefs[i].fillcolor,                 // Hintergrundfarbe
            m_buttonDefs[i].textcolor,                 // Textfarbe
            m_buttonDefs[i].label,                     // Beschriftung
            nullptr                                    // Icon
        );

        m_controlButtons[i].setFont(LeagueGothicCond34);
    }

    // Browse- und Clear-Button mit Sprach-Labels überschreiben (reset und Power-Symbol bleiben)
    m_controlButtons[0].setLabelText(reinterpret_cast<PGM_P>(STR(SDPlayerKey::SDPLAYER_BTN_BROWSE)));
    m_controlButtons[1].setLabelText(reinterpret_cast<PGM_P>(STR(SDPlayerKey::SDPLAYER_BTN_CLEAR)));

    for (int i = 0; i < 4; i++)
        m_controlButtons[i].drawButton();

    // Player-Buttons initialisieren
    for (int i = 0; i < 5; i++)
    {
        m_playerButtons[i].initButton(
            m_tft,                                                 // TFT-Zeichenobjekt
            m_playerButtonDefs[i].x + m_playerButtonDefs[i].w / 2, // X-Position
            m_playerButtonDefs[i].y + m_playerButtonDefs[i].h / 2, // Y-Position
            m_playerButtonDefs[i].w,                               // Breite
            m_playerButtonDefs[i].h,                               // Höhe
            m_playerButtonDefs[i].r,                               // Eckradius
            m_playerButtonDefs[i].outlinecolor,                    // Randfarbe
            m_playerButtonDefs[i].fillcolor,                       // Hintergrundfarbe
            m_playerButtonDefs[i].textcolor,                       // Textfarbe
            m_playerButtonDefs[i].label,                           // Beschriftung
            nullptr                                                // Icon
        );

        m_playerButtons[i].drawButton();
    }

    // Sprites für Playlist erstellen
    if (!m_tft->safeCreateSprite(m_sprPlaylist, PLAYLIST_W, PLAYLIST_H) ||
        !m_tft->safeCreateSprite(m_sprScrollbar, SCROLLBAR_W, PLAYLIST_H))
    {
        LOG_ERROR("Konnte Playlist-Sprites nicht erstellen!");
        return false;
    }

    // Progress-Bar Sprite erstellen
    if (!m_tft->safeCreateSprite(m_sprProgress, PROGRESS_W, PROGRESS_H))
    {
        LOG_ERROR("Konnte Progress Sprite nicht erstellen!");
        return false;
    }

    // Initial zeichnen
    drawProgressBar();

    // Scroll-State initialisieren
    m_scrollState.viewportHeight = PLAYLIST_H;
    m_scrollState.scrollOffset = 0;
    m_scrollState.scrollVelocity = 0;

    // Metadaten-Anzeige initialisieren
    drawMetaText(true);
    drawMetaText(false, " ", reinterpret_cast<PGM_P>(STR(SDPlayerKey::SDPLAYER_STATUS_NO_PLAYBACK)),
                 Theme::ColorMainText, Theme::ColorMainText, Theme::ColorMainText, " - ");

    // Lautstärkeregler und Equalizer initialisieren
    drawKnob(true, APP_STATE.getCurrentVolume());
    drawTracks(true, APP_STATE.getCurrentBalance(), APP_STATE.getCurrentGainLowPass(), APP_STATE.getCurrentGainBandPass(), APP_STATE.getCurrentGainHighPass());

    // Visualizer initialisieren, bevor ihn etwas zeichnen lässt: clearPlaylist() ruft draw(), und ohne Initialisierung hätte der Visualizer weder Display noch Sprites
    if (!m_visualizer.init(*m_tft, rtc))
    {
        LOG_ERROR("Visualizer-Initialisierung im SD-Player fehlgeschlagen!");
        return false;
    }

    // Dem Visualizer das aktive Modul mitteilen
    m_visualizer.setActiveModule(AppModule::SD_PLAYER);

    m_visualizer.setMode(static_cast<VisMode>(APP_STATE.getVisMode()));
    m_visualizer.draw(true);

    // UI-Elemente initial zeichnen
    clearPlaylist();
    drawPlaylist();

    drawScrollbar();

    // Details unter dem Cover initialisieren
    drawDetails(true);

    // Uhr initialisieren
    m_lastDrawnSecond = -1;

    // analoge Uhr
    drawAnalogClock(true, 0, 0, 0, 0);

    // digitale Uhr
    drawDigitalClock(true, 0, 0, 0, 0);

    m_initialized = true;
    LOG_SUCCESS("SD-Player-Modul erfolgreich initialisiert.");
    return true;
}

/**
 * @brief Zeichnet die komplette UI nach einem Sprachwechsel neu
 */
void SdPlayer::redrawAll()
{
    if (!m_initialized || !m_tft)
        return;

    // Display vollständig löschen
    m_tft->clear(TFT_BLACK);

    // Hintergrund-Rect im Cover-/Kassettenbereich
    m_tft->fillRect(0, 5, 800 - PLAYLIST_W - SCROLLBAR_W - 10, 300, 0x1082);

    // Separatoren neu pushen
    if (m_sprDiv)
    {
        m_sprDiv->pushSprite(0, 0);
        m_sprDiv->pushSprite(0, 307);
    }

    // Kassette samt Wickeln und Zahnkränzen neu aufbauen
    if (m_sprCassette)
    {
        m_sprCassette->pushSprite(CASSETTE_X, CASSETTE_Y);
        updateCassette(true);
    }

    // Kassetten-Label-Cache invalidieren und neu zeichnen
    m_lastArtistDrawn[0] = '\0';
    drawCassetteLabel(true);

    // Cover neu zeichnen (echtes Cover, falls vorhanden, sonst Fallback)
    if (m_hasCover && m_currentAudioFile[0] != '\0' && !m_coverOffsets.empty())
    {
        loadAndDrawCover();
    }
    else
    {
        // Fallback-Cache invalidieren, damit das Fallback wirklich neu gezeichnet wird
        m_lastDisplayedCoverFile[0] = '\0';
        drawCoverFallback(nullptr);
    }

    // Control-Buttons mit aktuellen Sprach-Labels neu zeichnen
    m_controlButtons[0].setLabelText(reinterpret_cast<PGM_P>(STR(SDPlayerKey::SDPLAYER_BTN_BROWSE)));
    m_controlButtons[1].setLabelText(reinterpret_cast<PGM_P>(STR(SDPlayerKey::SDPLAYER_BTN_CLEAR)));
    for (int i = 0; i < 4; i++)
        m_controlButtons[i].drawButton();

    // Player-Buttons neu zeichnen (Symbole, keine Sprach-Strings)
    for (int i = 0; i < 5; i++)
        m_playerButtons[i].drawButton();
    updatePlayerButtonStates();

    // Progress-Bar neu zeichnen
    drawProgressBar();

    // Metadaten-Sprite neu erstellen und Statuszeile passend zum Playback-Zustand setzen
    drawMetaText(true);
    if (m_playlistFinished)
    {
        drawMetaText(false, " ", reinterpret_cast<PGM_P>(STR(SDPlayerKey::SDPLAYER_STATUS_PLAYLIST_END)),
                     Theme::ColorMainText, Theme::ColorMainText, Theme::ColorMainText, " - ");
    }
    else if (!m_playlist.empty() && m_currentTitle[0] != '\0')
    {
        drawMetaText(false, m_currentArtist, m_currentTitle, 0x5370, TFT_SILVER, TFT_DARKGRAY, " | ");
    }
    else
    {
        drawMetaText(false, " ", reinterpret_cast<PGM_P>(STR(SDPlayerKey::SDPLAYER_STATUS_NO_PLAYBACK)),
                     Theme::ColorMainText, Theme::ColorMainText, Theme::ColorMainText, " - ");
    }

    // Lautstärke und Equalizer neu zeichnen ohne Audio-Callbacks zu feuern
    drawKnob(true, APP_STATE.getCurrentVolume(), /*skipCallbacks=*/true);
    drawTracks(true,
               APP_STATE.getCurrentBalance(),
               APP_STATE.getCurrentGainLowPass(),
               APP_STATE.getCurrentGainBandPass(),
               APP_STATE.getCurrentGainHighPass(),
               /*skipCallbacks=*/true);

    // Playlist und Scrollbar neu zeichnen
    m_lastScrollOffset = -1;
    drawPlaylist();
    drawScrollbar();

    // Details-Sprite neu initialisieren und Befüllung im nächsten Frame triggern
    drawDetails(true);
    m_needsDetailsRedraw = true;

    // Visualizer als Sub-Komponente komplett neu zeichnen (kein eigener Sprachwechsel-Hook)
    m_visualizer.draw(true);

    // Uhren neu initialisieren, damit Wochentag/Datum in der neuen Sprache erscheinen
    m_lastDrawnSecond = -1;
    drawAnalogClock(true, 0, 0, 0, 0);
    drawDigitalClock(true, 0, 0, 0, 0);
}

/**
 * @brief Aktualisiert die Kassetten-Anzeige
 */
void SdPlayer::updateCassette(bool force)
{
    if (!m_sprWrap || !m_sprCassette || !m_sprWinGround)
        return;

    // Auch den UI-Zustand abfragen: m_playbackState kippt beim Tastendruck sofort, m_audioStatus zieht erst mit dem nächsten Status-Poll nach - ohne diese Kopplung dreht der Dorn nach dem Pausieren noch kurz weiter
    const bool playing = (m_playbackState == PlaybackState::PLAYING) &&
                         m_audioStatus.isPlaying && (m_audioStatus.duration > 0);
    const uint32_t nowMs = millis();

    // ===================================================================
    // Performance-Optimierung: State-Change-Detection
    // ===================================================================
    // Wickeldorne nur rendern wenn: 1. Wiedergabe aktiv ist (Animation läuft) oder 2. sich der Wiedergabe-Status geändert hat (Start/Stop/Pause).
    // Das spart ca. 10-20ms pro Frame wenn keine Wiedergabe läuft.
    const bool stateChanged = (playing != m_lastCassettePlaying);
    m_lastCassettePlaying = playing;

    // Wenn nicht playing und kein State-Change, nur minimales Update
    if (!force && !playing && !stateChanged)
    {
        // Delta-Time trotzdem aktualisieren, um Sprünge beim Resume zu vermeiden
        m_lastWrapTime = nowMs;
        return;
    }

    // Delta-Time-basierte Zeitbasis
    if (m_lastWrapTime == 0)
        m_lastWrapTime = nowMs;

    float dt = (nowMs - m_lastWrapTime) / 1000.0f; // Sekunden seit letztem Frame
    m_lastWrapTime = nowMs;

    // Sicherheits-Clamping bei extremen Delays
    if (dt < 0.f)
        dt = 0.f; // Zeit läuft nie rückwärts

    if (dt > 0.05f)
        dt = 0.05f; // Max 50ms (verhindert Sprünge bei Task-Blockierung)

    // Fortschritt & aktuelle Radien: auch in der Pause aus dem Status rechnen,
    // sonst springen die Wickel beim Pausieren auf Anfangsstellung (p = 0)
    float p = 0.0f;
    if (m_audioStatus.duration > 0)
    {
        p = (float)m_audioStatus.currentTime / (float)m_audioStatus.duration;
        if (p < 0.f)
            p = 0.f;

        if (p > 1.f)
            p = 1.f;
    }

    const CassetteDesign &design = m_cassetteDesigns[m_cassetteDesignIdx];
    const float rmin2 = (float)design.reelRMin * design.reelRMin;
    const float rmax2 = (float)design.reelRMax * design.reelRMax;
    const float d2 = rmax2 - rmin2;

    float rL = sqrtf(rmin2 + (1.0f - p) * d2); // linker Wickel
    float rR = sqrtf(rmin2 + (p)*d2);          // rechter Wickel

    // Drehgeschwindigkeit ω = ω_ref * (r_ref / r) -> kleinerer Radius = schneller
    const float rRef = 0.5f * (design.reelRMin + design.reelRMax);
    float omegaL = WRAP_DPS_AT_REF * (rRef / (rL > 0.f ? rL : rRef));
    float omegaR = WRAP_DPS_AT_REF * (rRef / (rR > 0.f ? rR : rRef));

    if (playing)
    {
        // Delta-Time-basierte Winkel-Aktualisierung
        m_wrapAngleL -= omegaL * dt; // deg/s * s = deg
        m_wrapAngleR -= omegaR * dt;

        // Winkel normalisieren auf -360..360 Grad
        if (m_wrapAngleL < -360.0f)
            m_wrapAngleL += 360.0f;

        if (m_wrapAngleR < -360.0f)
            m_wrapAngleR += 360.0f;
    }

    // ===================================================================
    // Bandwickel nur rendern, wenn sich ein Radius sichtbar geändert hat
    // ===================================================================
    // Die Radien wandern über die Titeldauer nur wenige Bildpunkte, die Neuzeichnung ist deshalb selten
    const int rLpx = (int)lroundf(rL);
    const int rRpx = (int)lroundf(rR);

    if (force || rLpx != m_lastReelRadiusL || rRpx != m_lastReelRadiusR)
    {
        drawReels(rLpx, rRpx);
        m_lastReelRadiusL = rLpx;
        m_lastReelRadiusR = rRpx;
    }

    // ===================================================================
    // Naben rotieren: der gezeichnete Einheitsdorn wird gedreht in den Hintergrund komponiert, Zeit-Drossel deckelt die Last
    // ===================================================================
    const float angleDeltaL = fabsf(m_wrapAngleL - m_lastWrapAngleL);
    const float angleDeltaR = fabsf(m_wrapAngleR - m_lastWrapAngleR);
    const bool wrapDue = (nowMs - m_lastWrapPushMs) >= WRAP_UPDATE_MS;
    const bool needsWrapRedraw = force || stateChanged || (wrapDue && (angleDeltaL >= 2.0f || angleDeltaR >= 2.0f));

    if (needsWrapRedraw)
    {
        m_lastWrapPushMs = nowMs;

        pushDorn(design.hubXLeft, design.hubY, m_wrapAngleL);
        pushDorn(design.hubXRight, design.hubY, m_wrapAngleR);

        // Gerenderte Winkel speichern
        m_lastWrapAngleL = m_wrapAngleL;
        m_lastWrapAngleR = m_wrapAngleR;
    }
}

/**
 * @brief Kassetten-Label, zentriert & mit Ellipsis.
 */
void SdPlayer::drawCassetteLabel(bool force)
{
    if (!m_sprTapeLabel)
        return;

    const char *artist = m_currentArtist;
    if (!artist)
        artist = "";

    const CassetteDesign &design = m_cassetteDesigns[m_cassetteDesignIdx];

    // Vorlagen ohne freien Beschriftungsstreifen tragen keinen Interpreten auf der Kassette
    if (design.labelW <= 0)
        return;

    // Label-Hintergrund: der Ausschnitt des Beschriftungsstreifens aus der Farbtabelle, zeilenweise wegen des Zeilenversatzes der Quelle
    for (int y = 0; y < TAPE_LABEL_H; y++)
        m_sprTapeLabel->pushImage(0, y, design.labelW, 1, design.bitmap + (design.labelY + y) * CASSETTE_W + design.labelX);

    char finalText[96];
    finalText[0] = '\0';

    m_sprTapeLabel->unloadFont();
    m_sprTapeLabel->loadFont(ComfortaaBold14);
    truncateWithEllipsis(m_sprTapeLabel, artist, finalText, sizeof(finalText), design.labelW - 6);

    // Nichts weiter tun, wenn der sichtbare Text unverändert ist und nicht erzwungen wurde
    if (!force && strcmp(m_lastArtistDrawn, finalText) == 0)
        return;

    // zentriert zeichnen, die Schrift glättet sich gegen den Streifengrund
    m_sprTapeLabel->setTextColor(design.labelText);
    m_sprTapeLabel->setTextDatum(MC_DATUM);
    m_sprTapeLabel->drawString(finalText, design.labelW / 2, TAPE_LABEL_H / 2);

    // auf das Display übertragen; das Sprite ist exakt streifenbreit, denn setClipRect ist beim Sprite-Push auf dem RGB-Panel wirkungslos
    m_sprTapeLabel->pushSprite(CASSETTE_X + design.labelX, CASSETTE_Y + design.labelY);

    // Cache aktualisieren
    stringutils::safeCopy(m_lastArtistDrawn, finalText);
}

/**
 * @brief Bringt die Farbtabelle des aktiven Designs in das Kassetten-Sprite
 */
void SdPlayer::drawCassetteBody()
{
    const CassetteDesign &d = m_cassetteDesigns[m_cassetteDesignIdx];
    m_sprCassette->pushImage(0, 0, CASSETTE_W, CASSETTE_H, d.bitmap);
}

/**
 * @brief Zeichnet den Einheitsdorn für das aktive Design in das Naben-Sprite
 *
 * Ein zurückhaltender Dorn: dunkler Grund, darauf nur ein schmaler heller Zahnkranz-Ring mit sechs nach innen ragenden Zähnen und ein grauer Dornstift. Der Deckradius des dunklen Grunds kommt aus dem Design: groß genug für dessen Vorlagen-Teller, klein genug für dessen Nabenöffnung. Außerhalb bleibt die Transparenzfarbe, damit der rotierte Push die Umgebung stehen lässt.
 */
void SdPlayer::buildWrapSprite()
{
    constexpr int MID = WRAP_SPR_SIZE / 2;
    constexpr uint16_t RING_BG = 0x18C3;  ///< Öffnungsgrund außen
    constexpr uint16_t HUB_DARK = 0x1082; ///< dunkle Nabenfläche
    constexpr uint16_t RIM_COLOR = 0xCE79; ///< gedämpft heller Zahnkranz
    constexpr uint16_t AXIS_COLOR = 0x632C; ///< grauer Dornstift

    const CassetteDesign &d = m_cassetteDesigns[m_cassetteDesignIdx];

    m_sprWrap->clear(WRAP_TRANSP);
    m_sprWrap->fillSmoothCircle(MID, MID, d.hubCoverR, RING_BG);
    m_sprWrap->fillSmoothCircle(MID, MID, 16, HUB_DARK);
    m_sprWrap->drawSmoothCircle(MID, MID, 14.5f, 2.6f, RIM_COLOR, HUB_DARK);

    // 6 Zähne ragen vom Ring nach innen (bei 0°, 60°, 120°, 180°, 240°, 300°)
    for (int i = 0; i < 6; ++i)
    {
        const float a = i * 60.0f * 3.1415927f / 180.0f;
        const float sx = sinf(a);
        const float cy = cosf(a);
        m_sprWrap->drawWideLine((int)roundf(MID + 13.0f * sx), (int)roundf(MID - 13.0f * cy),
                                (int)roundf(MID + 9.0f * sx), (int)roundf(MID - 9.0f * cy), 1.8f, RIM_COLOR);
    }

    m_sprWrap->fillSmoothCircle(MID, MID, 2.5f, AXIS_COLOR);
    m_sprWrap->setPivot(MID, MID);
}

/**
 * @brief Setzt den rotierten Dorn offscreen zusammen und pusht ihn über den sicheren Weg
 *
 * pushRotateZoomWithAA direkt auf den Bildspeicher liest und schreibt dort fürs Kantenglätten - genau diese Zugriffsart kam auf dem RGB-Panel nur häppchenweise an (gleiche Fehlerfamilie wie setClipRect beim Sprite-Push): die Dorne bauten sich sichtbar langsam auf und verschmierten ihre Umgebung. Deshalb wird der Hintergrund aus Fenster- bzw. Körperpuffer in den Kompositionspuffer kopiert, der Dorn dort hineinrotiert und das Ergebnis normal gepusht.
 */
void SdPlayer::pushDorn(int hubX, int hubY, float angle)
{
    if (!m_sprDorn || !m_sprWrap || !m_sprCassette)
        return;

    constexpr int S = WRAP_SPR_SIZE;
    constexpr int HALF = S / 2;
    const CassetteDesign &d = m_cassetteDesigns[m_cassetteDesignIdx];
    const int x0 = hubX - HALF;
    const int y0 = hubY - HALF;

    uint16_t *dst = (uint16_t *)m_sprDorn->getBuffer();
    const uint16_t *body = (const uint16_t *)m_sprCassette->getBuffer();
    const uint16_t *win = m_sprWin ? (const uint16_t *)m_sprWin->getBuffer() : nullptr;

    for (int y = 0; y < S; y++)
    {
        const int ty = y0 + y;

        // Grundlage ist der Kassettenkörper, darüber der Fensteranteil (Grund samt Wickeln)
        memcpy(dst + y * S, body + ty * CASSETTE_W + x0, S * sizeof(uint16_t));

        if (win && ty >= d.winY && ty < d.winY + d.winH)
        {
            const int sx = (x0 > d.winX) ? x0 : d.winX;
            const int ex = (x0 + S < d.winX + d.winW) ? (x0 + S) : (d.winX + d.winW);
            if (ex > sx)
                memcpy(dst + y * S + (sx - x0),
                       win + (ty - d.winY) * d.winW + (sx - d.winX),
                       (ex - sx) * sizeof(uint16_t));
        }
    }

    m_sprWrap->pushRotateZoomWithAA(m_sprDorn, (float)HALF, (float)HALF, angle, 1.0f, 1.0f, WRAP_TRANSP);
    m_sprDorn->pushSprite(CASSETTE_X + x0, CASSETTE_Y + y0);
}

/**
 * @brief Zeichnet beide Bandwickel in das Bandfenster-Sprite und bringt es aufs Panel
 *
 * Der Fenstergrund kommt per memcpy aus dem PSRAM-Puffer (einmal je Design gefüllt), die Wickel werden direkt in Fensterkoordinaten gezeichnet - alles außerhalb beschneidet das Sprite von selbst. Das ersetzt den früheren Umweg über eine flashgeladene Wickelzonen-Spanne, deren 20-ms-Lastspitze das Panel bei jedem Radiusschritt sichtbar aus dem Tritt brachte. Der Push bleibt ungeclippt: setClipRect blieb beim Sprite-Push auf dem RGB-Panel nachweislich wirkungslos.
 */
void SdPlayer::drawReels(int radiusLeftPx, int radiusRightPx)
{
    if (!m_sprWin || !m_sprWinGround)
        return;

    const CassetteDesign &d = m_cassetteDesigns[m_cassetteDesignIdx];

    // Grund übernehmen: gleiche Größe und Farbtiefe, ein zusammenhängendes memcpy genügt
    memcpy(m_sprWin->getBuffer(), m_sprWinGround->getBuffer(), (size_t)d.winW * d.winH * sizeof(uint16_t));

    // Beide Bandwickel in einheitlicher Grundfarbe mit dunkleren Lagenringen, alle Flächen geglättet
    const int cy = d.hubY - d.winY;
    auto reel = [&](int cx, int radiusPx)
    {
        m_sprWin->fillSmoothCircle(cx, cy, radiusPx, d.tapeEdge);
        m_sprWin->drawSmoothCircle(cx, cy, radiusPx * 0.82f, 1.0f, d.tape, d.tapeEdge);
        m_sprWin->drawSmoothCircle(cx, cy, radiusPx * 0.64f, 1.0f, d.tape, d.tapeEdge);
    };
    // Naben außerhalb des Fensters (Bullaugen-Designs): die Wickel sitzen einen Bildpunkt weiter außen, sonst bleibt am Glasrand eine Lückenlinie. Sichtbare Naben: exakt zentriert, sonst steht der kleine Wickel sichtbar schief um den Dorn.
    const int out = (d.hubXLeft < d.winX) ? 1 : 0;
    reel(d.hubXLeft - d.winX - out, radiusLeftPx);
    reel(d.hubXRight - d.winX + out, radiusRightPx);

    // Dorne vor dem Push mit hineinrotieren: sonst zeigt das Panel für einen Wimpernschlag ein dornloses Fenster - das sichtbare Zucken bei jedem Radiusschritt
    if (m_sprWrap)
    {
        m_sprWrap->pushRotateZoomWithAA(m_sprWin, (float)(d.hubXLeft - d.winX), (float)(d.hubY - d.winY), m_wrapAngleL, 1.0f, 1.0f, WRAP_TRANSP);
        m_sprWrap->pushRotateZoomWithAA(m_sprWin, (float)(d.hubXRight - d.winX), (float)(d.hubY - d.winY), m_wrapAngleR, 1.0f, 1.0f, WRAP_TRANSP);
    }

    m_sprWin->pushSprite(CASSETTE_X + d.winX, CASSETTE_Y + d.winY);
}

/**
 * @brief Baut die Kassette nach einem Designwechsel komplett neu auf
 */
void SdPlayer::applyCassetteDesign()
{
    const CassetteDesign &d = m_cassetteDesigns[m_cassetteDesignIdx];

    // Fenster-Sprite und Fenstergrund-Puffer auf die Bandfenstergröße des Designs bringen
    if (!m_tft->safeCreateSprite(m_sprWin, d.winW, d.winH) ||
        !m_tft->safeCreateSprite(m_sprWinGround, d.winW, d.winH))
    {
        LOG_ERROR("Konnte Bandfenster Sprite nicht erstellen!");
        return;
    }

    // Label-Sprite exakt auf die Streifenbreite des Designs bringen: der Push läuft ungeclippt, ein breiteres Sprite trüge Reste des vorigen Designs aufs Gehäuse
    if (d.labelW > 0)
    {
        if (!m_tft->safeCreateSprite(m_sprTapeLabel, d.labelW, TAPE_LABEL_H))
        {
            LOG_ERROR("Konnte Kassetten-Label Sprite nicht erstellen!");
            return;
        }

        m_sprTapeLabel->setTextWrap(false);
    }

    // Fenstergrund einmalig aus der Farbtabelle laden, zeilenweise wegen des Zeilenversatzes der Quelle - danach kommt jeder Wickel-Neuaufbau ohne Flash-Zugriff aus
    for (int y = 0; y < d.winH; y++)
        m_sprWinGround->pushImage(0, y, d.winW, 1, d.bitmap + (d.winY + y) * CASSETTE_W + d.winX);

    drawCassetteBody();
    buildWrapSprite();
    m_sprCassette->pushSprite(CASSETTE_X, CASSETTE_Y);
    drawCassetteLabel(true);
    updateCassette(true);
}

/**
 * @brief Wählt zufällig ein anderes Design als das aktive und wendet es an
 */
void SdPlayer::selectRandomCassetteDesign()
{
    // Mit weniger als zwei Designs gibt es nichts zu wählen, die Ausweichlogik liefe sonst aus dem Array
    if (CASSETTE_DESIGN_COUNT < 2)
        return;

    uint8_t idx = random(CASSETTE_DESIGN_COUNT - 1);
    if (idx >= m_cassetteDesignIdx)
        idx++;
    m_cassetteDesignIdx = idx;

    // Nicht sofort zeichnen: das Leeren läuft aus der Bestätigungs-Meldungsbox heraus, der Aufbau folgt im nächsten dialogfreien Frame
    m_pendingCassetteApply = true;
}

/**
 * @brief Blättert bei einem Tipp auf die Kassette zum nächsten Design
 */
void SdPlayer::handleCassetteTouch()
{
    const bool freshTouch = m_touched && !m_cassetteTouchSeen;
    m_cassetteTouchSeen = m_touched;

    if (!freshTouch)
        return;
    if (m_touchX < CASSETTE_X || m_touchX >= CASSETTE_X + CASSETTE_W)
        return;
    if (m_touchY < CASSETTE_Y || m_touchY >= CASSETTE_Y + CASSETTE_H)
        return;

    // Touch-Prellen liefert pro Tipp mehrere Flanken; jede schaltete ein Design weiter und die Dorn-Pushes verschiedener Deckradien verschmierten sichtbar
    const uint32_t nowMs = millis();
    if (nowMs - m_lastCassetteSwitchMs < 400)
        return;
    m_lastCassetteSwitchMs = nowMs;

    m_cassetteDesignIdx = (m_cassetteDesignIdx + 1) % CASSETTE_DESIGN_COUNT;
    applyCassetteDesign();
}

// ===================================================================
// Hauptzeichnungsschleife & Event-Verarbeitung
// ===================================================================

/**
 * @brief Hauptzeichnungsschleife - muss zyklisch aufgerufen werden
 */
void SdPlayer::draw()
{
    if (!m_initialized)
        return;

    // Sprachwechsel-Hook: Vollredraw bei erstem Aufruf nach setUiLanguage()
    if (I18n::consumeLanguageChanged())
        redrawAll();

    // GFX-Status abfragen
    const bool gfxBlocked = APP_STATE.getBlockGFX();
    const bool dialogOpen = isDialogOpen();

    // Event-Queue und Audio-Status laufen auch bei Dialog oder Grafik-Sperre weiter, sonst bleibt ein Titelende liegen und der Player verstummt bis zum Dialogschluss. m_uiSuspended lenkt die Zeichenaufrufe der EOF-Kette auf die Redraw-Flags um; nach dem Schließen stellt der Dialog seinen Schnappschuss wieder her und die Flags holen das Zeichnen nach.
    if (gfxBlocked || dialogOpen)
    {
        m_uiSuspended = true;
        processEventQueue();

        static unsigned long lastAudioStatusUpdateSuspended = 0;
        if (millis() - lastAudioStatusUpdateSuspended >= 1000)
        {
            lastAudioStatusUpdateSuspended = millis();
            updateAudioStatus();
        }

        m_uiSuspended = false;
    }

    // ===================================================================
    // Dialog-Handling (höchste Priorität - Early Return)
    // ===================================================================

    // FileExplorer
    if (m_explorer && m_explorer->isActive())
    {
        m_explorer->update();
        return;
    }
    // MessageBox
    if (m_messageBox && (m_messageBox->isActive() || m_messageBox->isBusy()))
    {
        m_messageBox->update();

        // Completion-Check für einfache Dialoge
        if (m_messageBox->isBusy())
        {
            m_messageBox->completeConfirmation(true);
            return;
        }

        return;
    }
    // SleepTimer updaten
    if (m_sleepDlg)
    {
        m_sleepDlg->update();

        // Wenn der Dialog aktiv ist, den Rest der UI nicht zeichnen
        if (m_sleepDlg->isActive())
            return;
    }

    // ===================================================================
    // Bei gfxBlocked: nichts zeichnen (Events liefen oben schon)
    // ===================================================================
    if (gfxBlocked)
        return;

    // ===================================================================
    // Normaler Frame-Ablauf (gfxBlocked == false)
    // ===================================================================

    // Touch-Handling
    m_touched = m_tft->getTouch(&m_touchX, &m_touchY);
    m_visualizer.handleTouch(m_touchX, m_touchY, m_touched);

    // Audio-Daten für Visualisierung abrufen
    if (g_audioManager.isAudioDataReady())
    {
        // Nur wenn nicht pausiert: Audio-Daten an Visualizer durchreichen
        if (m_playbackState != PlaybackState::PAUSED)
        {
            // Audio-Level durchreichen
            m_visualizer.setAudioValues(
                g_audioManager.getRMSLeft(),
                g_audioManager.getRMSRight(),
                g_audioManager.getPeakLeft(),
                g_audioManager.getPeakRight());

            // Audio-Samples durchreichen
            m_visualizer.setSampleData(
                g_audioManager.getSampleBufferLeft(),
                g_audioManager.getSampleBufferRight(),
                g_audioManager.getValidSamplesCount(),
                g_audioManager.getSampleRate());
        }

        // Audio-Daten als verarbeitet markieren
        { extern volatile uint32_t g_visDataCount; g_visDataCount = g_visDataCount + 1; } // Messzähler Visualisierungs-Datenrate
        g_audioManager.consumeAudioData();
    }

    // Ohne Daten keine Visualisierung: Bei Pause oder Stop auf Null setzen
    if (m_playbackState == PlaybackState::PAUSED || m_playbackState == PlaybackState::STOPPED)
        zeroVisualizer();

    // Visualizer zeichnen
    if (!gfxBlocked)
        m_visualizer.draw(false);

    // Reset-Request behandeln
    if (m_resetRequested)
    {
        m_resetRequested = false;

        constexpr int8_t DEFAULT_GAIN_LOW = AppState::DEFAULT_GAIN_LOW_PASS_DB;
        constexpr int8_t DEFAULT_GAIN_BAND = AppState::DEFAULT_GAIN_BAND_PASS_DB;
        constexpr int8_t DEFAULT_GAIN_HIGH = AppState::DEFAULT_GAIN_HIGH_PASS_DB;
        constexpr uint8_t DEFAULT_VOLUME = AppState::DEFAULT_VOLUME;

        // Alle Einstellungen zurücksetzen
        APP_STATE.setCurrentVolume(DEFAULT_VOLUME);
        APP_STATE.setCurrentBalance(0);
        APP_STATE.setCurrentGainLowPass(DEFAULT_GAIN_LOW);
        APP_STATE.setCurrentGainBandPass(DEFAULT_GAIN_BAND);
        APP_STATE.setCurrentGainHighPass(DEFAULT_GAIN_HIGH);

        // Visualisierungen zurücksetzen
        m_visualizer.setMode(VisMode::VIS_ANALOG);

        // UI neu zeichnen
        drawKnob(true, DEFAULT_VOLUME);
        drawTracks(true, 0, DEFAULT_GAIN_LOW, DEFAULT_GAIN_BAND, DEFAULT_GAIN_HIGH);

        // Callbacks
        if (m_callbacks.onVolumeChanged)
            m_callbacks.onVolumeChanged(DEFAULT_VOLUME);
        if (m_callbacks.onBalanceChanged)
            m_callbacks.onBalanceChanged(0);
        if (m_callbacks.onGainLowPassChanged)
            m_callbacks.onGainLowPassChanged(DEFAULT_GAIN_LOW);
        if (m_callbacks.onGainBandPassChanged)
            m_callbacks.onGainBandPassChanged(DEFAULT_GAIN_BAND);
        if (m_callbacks.onGainHighPassChanged)
            m_callbacks.onGainHighPassChanged(DEFAULT_GAIN_HIGH);
        if (m_callbacks.onVUModeChanged)
            m_callbacks.onVUModeChanged(VisMode::VIS_ANALOG);
    }

    // Event-Queue abarbeiten
    processEventQueue();

    // Cover-Event-Buffer begrenzen (Schutz vor Memory-Leak bei langen Tracks) Hinweis: Kein shrink_to_fit() verwenden - verursacht Heap-Fragmentierung auf ESP32
    if (m_eventCoverData.size() > 100)
    {
        m_eventCoverData.clear();
        LOG_WARNING("Cover-Event-Buffer überlauf - zurückgesetzt");
    }

    // Sekündlich Audio-Status überprüfen
    static unsigned long lastAudioStatusUpdate = 0;
    if (millis() - lastAudioStatusUpdate >= 1000)
    {
        lastAudioStatusUpdate = millis();
        updateAudioStatus();
    }

    // UI-Elemente aktualisieren
    {
        handleProgressTouch();

        // Während des Ziehens am Fortschrittsbalken gehört die Berührung dem Balken allein
        if (!m_progressScrubbing)
        {
            handleCassetteTouch();
            handleButtons();
            handlePlayerButtons();
            drawKnob(false);
            drawTracks(false);
            handlePlaylistTouch(m_touchX, m_touchY, m_touched);
        }
        updateScrollPhysics();

        // Button-States aktualisieren
        static PlaybackState lastPlaybackState = PlaybackState::STOPPED;
        static uint32_t lastPulseUpdate = 0;

        // Bei State-Änderung: Sofort aktualisieren
        if (m_playbackState != lastPlaybackState)
        {
            updatePlayerButtonStates();
            lastPlaybackState = m_playbackState;
            lastPulseUpdate = millis();
        }
        // Bei "PAUSE" -> kontinuierlich pulsieren
        else if (m_playbackState == PlaybackState::PAUSED && (millis() - lastPulseUpdate >= 50))
        {
            updatePlayerButtonStates();
            lastPulseUpdate = millis();
        }

        // Auto-Scroll zum aktuellen Track
        if (m_autoScrollRequested)
        {
            scrollToCurrentTrack();
            m_autoScrollRequested = false;
        }

        // Playlist bei Bedarf neu zeichnen
        if (m_needsPlaylistRedraw || m_scrollState.scrollOffset != m_lastScrollOffset)
        {
            drawPlaylist();
            drawScrollbar();
            m_lastScrollOffset = m_scrollState.scrollOffset;
            m_needsPlaylistRedraw = false;
        }

        // Details unter dem Cover bei Bedarf neu zeichnen
        if (m_needsDetailsRedraw)
        {
            drawDetails(false);
            m_needsDetailsRedraw = false;
        }

        // Metadaten bei Bedarf neu zeichnen
        if (m_metadataUpdated)
        {
            drawMetaText(false, m_currentArtist, m_currentTitle, 0x5370, TFT_SILVER, TFT_DARKGRAY, " | ");
            m_metadataUpdated = false;
        }

        // Fortschrittsanzeige zeichnen
        if (m_needsProgressRedraw)
        {
            drawProgressBar();
            m_needsProgressRedraw = false;
        }

        // ===================================================================
        // Cover-Loading mit Frame-Budget-Konzept
        // ===================================================================
        // Das Laden und Dekodieren eines Covers kann 50 bis 150 ms dauern und wird deshalb auf mehrere Frames verteilt:
        // 1. Cover-Laden nur wenn keine anderen UI-Updates ausstehen
        // 2. Throttling auf 500ms Intervall
        // 3. Bei aktiver Wiedergabe: Priorität auf Visualizer
        static unsigned long lastCoverCheck = 0;
        constexpr unsigned long COVER_CHECK_INTERVAL = 500; // Zeit zwischen Cover-Checks

        // Cover-Timeout-Handling: Warten auf Cover-Event
        if (m_waitingForCoverEvent)
        {
            uint32_t elapsed = millis() - m_coverEventWaitStartMs;
            if (elapsed >= COVER_EVENT_TIMEOUT_MS)
            {
                // Timeout abgelaufen -> Cover-Event ist nicht gekommen
                m_waitingForCoverEvent = false;
                m_coverOffsets.clear();
                m_needsCoverRedraw = true;
            }
            else
            {
                m_needsCoverRedraw = false;
            }
            // Sonst weiter warten, altes Cover bleibt sichtbar
        }

        // Cover bei Bedarf neu zeichnen - aber nur wenn "Frame-Budget" erlaubt
        if (m_needsCoverRedraw && !m_waitingForCoverEvent)
        {
            unsigned long now = millis();

            // Prüfung 1: Throttling-Intervall einhalten
            bool intervalOk = (now - lastCoverCheck >= COVER_CHECK_INTERVAL);

            // Prüfung 2: Keine anderen pending UI-Updates (diese haben Priorität)
            bool noPendingUpdates = !m_needsPlaylistRedraw && !m_needsDetailsRedraw && !m_metadataUpdated && !m_needsProgressRedraw;

            // Prüfung 3: Bei aktiver Wiedergabe das Cover-Laden zusätzlich verzögern (Visualizer hat Priorität für flüssige Animation)
            bool visualizerBusy = (m_playbackState == PlaybackState::PLAYING);

            // Bei beschäftigtem Visualizer: Längeres Intervall (1000ms statt 500ms)
            unsigned long effectiveInterval = visualizerBusy ? 1000 : COVER_CHECK_INTERVAL;
            intervalOk = (now - lastCoverCheck >= effectiveInterval);

            if (intervalOk && noPendingUpdates)
            {
                lastCoverCheck = now;

                if (m_coverOffsets.empty())
                {
                    // Fallback zeichnen (schnell)
                    if (m_lastDisplayedCoverFile[0] == '\0' || strcmp(m_currentAudioFile, m_lastDisplayedCoverFile) != 0)
                        drawCoverFallback(nullptr);
                }
                else
                {
                    // Cover laden (langsam - SD-Read + JPEG-Decode)
                    loadAndDrawCover();
                }
                m_needsCoverRedraw = false;
            }
            // Sonst: Cover-Loading auf nächsten Frame verschieben
        }
    }

    // Kassette, Wickeldorne, Band aktualisieren
    if (!gfxBlocked && !isDialogOpen())
    {
        // Beim Leeren gewählter Designwechsel: erst jetzt, dialogfrei, aufbauen
        if (m_pendingCassetteApply)
        {
            m_pendingCassetteApply = false;
            applyCassetteDesign();
        }
        updateCassette();
    }

    // Beschriftung der Kassette bei Bedarf neu zeichnen
    if (m_needsCassetteLabelRedraw)
    {
        drawCassetteLabel(false);
        m_needsCassetteLabelRedraw = false;
    }

    // Uhren aktualisieren
    if (m_sleepDlg && m_sleepDlg->isTimerActive())
    {
        int remaining = 0, seconds = 0;
        m_sleepDlg->getRemainingTime(remaining, seconds);
        (m_visualizer.getMode() == VisMode::VIS_GONIO) ? drawAnalogClock(false, 721, 354, remaining, seconds) : drawDigitalClock(false, 355, 435, remaining, seconds);
    }
    else
    {
        (m_visualizer.getMode() == VisMode::VIS_GONIO) ? drawAnalogClock(false, 721, 354, -1, 0) : drawDigitalClock(false, 355, 435, -1, 0);
    }
}

/**
 * @brief Verarbeitet die AudioManager-Event-Queue
 *
 * Ausgelagert aus draw() für bessere Performance-Kontrolle. Läuft auch bei offenem Dialog und bei gesetztem BlockGFX; m_uiSuspended lenkt die Zeichenaufrufe der EOF-Kette dann auf die Redraw-Flags um, gezeichnet wird erst wieder im nächsten freien Frame.
 */
void SdPlayer::processEventQueue()
{
    AudioManagerEventType qType;
    bool metaChanged = false;
    int eventCount = 0;
    constexpr int MAX_EVENTS_PER_FRAME = 3; // Maximal 3 Events pro Frame

    while (eventCount < MAX_EVENTS_PER_FRAME && g_audioManager.dequeueEvent(qType, m_eventText, sizeof(m_eventText), &m_eventCoverData))
    {
        eventCount++;

        switch (qType)
        {
        case AudioManagerEventType::EOF_AUDIO:
        {
            // Verspätet konsumierte EOFs des vorigen Titels abfangen: nach Stop oder im Startfenster eines frisch gewechselten Titels (Tipp/Sprung) gehört ein EOF nicht zum aktuellen Titel und würde ihn fälschlich als gespielt markieren und überspringen
            if (!m_playlistFinished && m_playbackState != PlaybackState::STOPPED && !m_waitingForPlaybackStart)
            {
                LOG_INFO_F("Track %d beendet (EOF, Queue)", m_currentTrackIndex + 1);
                onAudioTrackFinished();
            }
        }
        break;

        case AudioManagerEventType::ID3_ARTIST:
        {
            if (m_eventText[0] != '\0' && strcmp(m_currentArtist, m_eventText) != 0)
            {
                stringutils::safeCopy(m_currentArtist, m_eventText);
                metaChanged = true;
                m_needsCassetteLabelRedraw = true;
                LOG_INFO_F("ID3 Artist (Queue): %s", m_currentArtist);
            }
        }
        break;

        case AudioManagerEventType::ID3_TITLE:
        {
            if (m_eventText[0] != '\0' && strcmp(m_currentTitle, m_eventText) != 0)
            {
                stringutils::safeCopy(m_currentTitle, m_eventText);
                metaChanged = true;
                LOG_INFO_F("ID3 Title (Queue): %s", m_currentTitle);
            }
        }
        break;

        case AudioManagerEventType::ID3_ALBUM:
        {
            if (m_eventText[0] != '\0' && strcmp(m_currentAlbum, m_eventText) != 0)
            {
                stringutils::safeCopy(m_currentAlbum, m_eventText);
                m_needsDetailsRedraw = true;
                LOG_INFO_F("ID3 Album (Queue): %s", m_currentAlbum);
            }
        }
        break;

        case AudioManagerEventType::ID3DATA:
        {
            // Fallback für generische ID3-Daten, die nicht Artist/Title/Album sind
            LOG_INFO_F("ID3 Generic Data (Queue): %s", m_eventText);
        }
        break;

        case AudioManagerEventType::BITRATE:
        {
            if (m_eventText[0] != '\0')
            {
                updateMetadata("", "", m_eventText);
                LOG_INFO_F("Evt-Bitrate (Queue): %s kbps", m_eventText);
                metaChanged = true;
            }
        }
        break;

        case AudioManagerEventType::COVER_IMAGE:
        {
            if (!m_eventCoverData.empty() && m_eventCoverData.size() >= 2)
            {
                // Cover gefunden -> Timeout stoppen
                m_waitingForCoverEvent = false;
                m_coverOffsets = m_eventCoverData;
                m_needsCoverRedraw = true;
            }
        }
        break;
        }
    }

    if (metaChanged)
        m_metadataUpdated = true;
}

/**
 * @brief Callback für Track-Ende-Event
 */
void SdPlayer::onAudioTrackFinished()
{
    finalizeCurrentTrack(TrackFinishType::COMPLETED, true);
}

// ===================================================================
// Progress & Metadaten
// ===================================================================

/**
 * @brief Zeichnet die Fortschrittsanzeige mit Zeitangabe
 */
void SdPlayer::drawProgressBar()
{
    if (!m_sprProgress)
        return;

    // Während Dialog/Grafik-Sperre nur vormerken: der Flag-Pfad in draw() zeichnet den Balken nach dem Schließen
    if (APP_STATE.getBlockGFX() || m_uiSuspended)
    {
        m_needsProgressRedraw = true;
        return;
    }

    constexpr uint16_t barBgColor = 0x2104;
    constexpr uint16_t barFillColor = 0x4208;
    constexpr uint16_t textColor = TFT_WHITE;

    char currentTimeStr[8];
    char durationTimeStr[8];
    int curMin = 0;
    int curSec = 0;
    int durMin = 0;
    int durSec = 0;
    int fillW = 0;

    bool isPlaying = (m_audioStatus.duration > 0);

    // Beim Ziehen am Balken zeigen Zeit und Füllstand die Zielposition als Vorschau
    uint32_t showTime = m_audioStatus.currentTime;
    uint32_t showProgress = m_audioStatus.progress;
    if (m_progressScrubbing && m_audioStatus.duration > 0)
    {
        showTime = m_scrubTargetSec;
        showProgress = (m_scrubTargetSec * 100) / m_audioStatus.duration;
    }

    if (isPlaying)
    {
        curMin = showTime / 60;
        curSec = showTime % 60;
        durMin = m_audioStatus.duration / 60;
        durSec = m_audioStatus.duration % 60;
        fillW = (PROGRESS_W * showProgress) / 100;
    }

    stringutils::safeFormat(currentTimeStr, "%02d:%02d", curMin, curSec);
    stringutils::safeFormat(durationTimeStr, "%02d:%02d", durMin, durSec);

    m_sprProgress->fillRect(0, 0, PROGRESS_W, PROGRESS_H, barBgColor);

    if (isPlaying && fillW > 0)
    {
        m_sprProgress->fillRect(0, 0, fillW, PROGRESS_H, barFillColor);
        m_sprProgress->fillRect(0, PROGRESS_H - 2, fillW, 2, Theme::ColorRetroRed);
    }

    m_sprProgress->loadFont(ComfortaaBold16);

    constexpr int centerX = PROGRESS_W / 2;
    constexpr int centerY = PROGRESS_H / 2;
    constexpr int timeOffset = 10;

    m_sprProgress->setTextColor(textColor);
    m_sprProgress->setTextDatum(MC_DATUM);
    m_sprProgress->drawString("/", centerX, centerY);

    m_sprProgress->setTextDatum(ML_DATUM);
    m_sprProgress->drawString(currentTimeStr, centerX - timeOffset - 40, centerY);
    m_sprProgress->drawString(durationTimeStr, centerX + timeOffset, centerY);

    m_sprProgress->unloadFont();
    m_sprProgress->pushSprite(PROGRESS_X, PROGRESS_Y);

    // Cache aktualisieren für Delta-Check
    m_lastDrawnProgress = m_audioStatus.progress;
    m_lastDrawnTimeSec = m_audioStatus.currentTime;
}

/**
 * @brief Aktualisiert Metadaten (Artist, Title, Bitrate)
 */
void SdPlayer::updateMetadata(const char *artist, const char *title, const char *bitrate)
{
    bool artistOrTitleChanged = false;
    bool artistChanged = false;

    // Artist aktualisieren
    if (artist && artist[0] != '\0' && strcmp(m_currentArtist, artist) != 0)
    {
        stringutils::safeCopy(m_currentArtist, artist);
        artistOrTitleChanged = true;
        artistChanged = true;
    }

    // Title aktualisieren
    if (title && title[0] != '\0' && strcmp(m_currentTitle, title) != 0)
    {
        stringutils::safeCopy(m_currentTitle, title);
        artistOrTitleChanged = true;
    }

    // Bitrate aktualisieren
    if (bitrate && bitrate[0] != '\0')
    {
        int bitrateValue = atoi(bitrate);
        if (bitrateValue == 0)
            return;

        // Bitrate von bps nach kbps umrechnen
        int bitrateKbps = bitrateValue / 1000;

        // Dateityp prüfen: verlustfreie Formate (FLAC/WAV/ALAC/APE) zeigen die rohe Bitrate, alle anderen werden auf Standard-Bitraten gerundet
        bool isLossless = false;
        if (m_currentAudioFile[0] != '\0')
        {
            const char *ext = strrchr(m_currentAudioFile, '.');
            if (ext)
            {
                ext++; // Skip '.'
                if (strcasecmp(ext, "flac") == 0 ||
                    strcasecmp(ext, "wav") == 0 ||
                    strcasecmp(ext, "alac") == 0 ||
                    strcasecmp(ext, "ape") == 0)
                {
                    isLossless = true;
                }
            }
        }

        int displayKbps = bitrateKbps;

        // Nur bei verlustbehafteten Formaten (MP3) auf Standard-Bitrate runden
        if (!isLossless)
        {
            uint32_t snappedBitrate = snapToStandardBitrate(bitrateValue);
            displayKbps = snappedBitrate / 1000;
        }

        char bitrateStr[32];
        stringutils::safeFormat(bitrateStr, "%d", displayKbps);

        // Bitrate aktualisieren wenn geändert
        if (strcmp(m_currentBitrate, bitrateStr) != 0)
        {
            stringutils::safeCopy(m_currentBitrate, bitrateStr);

            // Display-Bitrate mit "kbps" Suffix formatieren
            stringutils::safeFormat(m_displayBitrate, "%d kbps", displayKbps);

            m_needsDetailsRedraw = true; // Details neu zeichnen bei Bitrate-Änderung

            if (m_currentTitle[0] != '\0')
                artistOrTitleChanged = true;
        }
    }

    if (artistOrTitleChanged)
        m_metadataUpdated = true;

    if (artistChanged)
        m_needsCassetteLabelRedraw = true;
}

/**
 * @brief Zeichnet die Statuszeile: zwei Textteile mit Trennzeichen, bei Platzmangel kleinere Schrift und Kürzung mit "..."
 */
void SdPlayer::drawMetaText(bool init, const char *first, const char *second, uint16_t firstColor, uint16_t secondColor, uint16_t separatorColor, const char *separator, int reservedPixels, uint16_t backgroundColor)
{
    // Während Dialog/Grafik-Sperre nur vormerken: der Flag-Pfad in draw() baut die Statuszeile nach dem Schließen aus den Metadaten neu auf
    if (m_uiSuspended)
    {
        m_metadataUpdated = true;
        return;
    }

    static const uint8_t *fonts[] = {ComfortaaBold18, ComfortaaBold16, ComfortaaBold14};
    static const int fontCount = sizeof(fonts) / sizeof(fonts[0]);
    static const char *ellipsis = "...";
    static const uint16_t lineColor = 0x39E8;
    static const int spriteWidth = 800;
    static const int spriteHeight = 32;
    static const int spriteY = 316;

    if (init)
    {
        if (!m_tft->safeCreateSprite(m_sprMeta, spriteWidth, spriteHeight))
        {
            LOG_ERROR("Konnte m_sprMeta Sprite nicht erstellen!");
            return;
        }

        m_sprMeta->setTextWrap(false);
        m_sprMeta->setTextDatum(ML_DATUM);
        m_sprMeta->clear(backgroundColor);
        m_sprMeta->drawFastHLine(0, 0, spriteWidth, lineColor);
        m_sprMeta->drawFastHLine(0, spriteHeight - 1, spriteWidth, lineColor);
        m_sprMeta->pushSprite(0, spriteY);

        return;
    }

    if (!first || !second || !separator)
        return;

    const bool hasSender = first[0] != '\0';
    const bool hasTitle = second[0] != '\0';

    if (!hasSender && !hasTitle)
        return;

    m_sprMeta->fillRect(0, 1, spriteWidth, spriteHeight - 2, backgroundColor);

    const int availableWidth = spriteWidth - reservedPixels;
    const int centerY = spriteHeight / 2;

    auto calcFooterWidth = [&](const char *snd, const char *ttl) -> int
    {
        int width = 0;
        bool hasPrevious = false;
        const int separatorWidth = m_sprMeta->textWidth(separator);

        if (snd[0] != '\0')
        {
            width += m_sprMeta->textWidth(snd);
            hasPrevious = true;
        }

        if (ttl[0] != '\0')
        {
            if (hasPrevious)
                width += separatorWidth;

            width += m_sprMeta->textWidth(ttl);
        }

        return width;
    };

    auto drawFooter = [&](const char *snd, const char *ttl, int startX)
    {
        int cursorX = startX;
        bool hasPrevious = false;

        if (snd[0] != '\0')
        {
            m_sprMeta->setTextColor(firstColor, backgroundColor);
            m_sprMeta->drawString(snd, cursorX, centerY);
            cursorX += m_sprMeta->textWidth(snd);
            hasPrevious = true;
        }

        if (ttl[0] != '\0')
        {
            if (hasPrevious)
            {
                m_sprMeta->setTextColor(separatorColor, backgroundColor);
                m_sprMeta->drawString(separator, cursorX, centerY);
                cursorX += m_sprMeta->textWidth(separator);
            }

            m_sprMeta->setTextColor(secondColor, backgroundColor);
            m_sprMeta->drawString(ttl, cursorX, centerY);
        }
    };

    for (int fontIndex = 0; fontIndex < fontCount; fontIndex++)
    {
        m_sprMeta->unloadFont();
        m_sprMeta->loadFont(fonts[fontIndex]);

        int totalWidth = calcFooterWidth(first, second);

        if (totalWidth <= availableWidth)
        {
            // Font passt - zeichne den Text
            int startX = (spriteWidth - totalWidth) / 2;
            drawFooter(first, second, startX);
            m_sprMeta->unloadFont();
            m_sprMeta->pushSprite(0, spriteY);

            return;
        }
    }

    // Intelligente Kürzung, wenn der Text zu lang ist
    m_sprMeta->unloadFont();
    m_sprMeta->loadFont(fonts[fontCount - 1]);

    const int separatorWidth = m_sprMeta->textWidth(separator);
    const int ellipsisWidth = m_sprMeta->textWidth(ellipsis);

    // Berechne verfügbare Breite für Text (ohne Separator)
    int availableForText = availableWidth;
    if (hasSender && hasTitle)
        availableForText -= separatorWidth;

    char truncatedArtist[256];
    char truncatedTitle[256];
    truncatedArtist[0] = '\0';
    truncatedTitle[0] = '\0';

    if (hasSender && hasTitle)
    {
        int artistWidth = m_sprMeta->textWidth(first);
        int titleWidth = m_sprMeta->textWidth(second);
        int totalTextWidth = artistWidth + titleWidth;

        if (totalTextWidth > availableForText)
        {
            // Intelligente Platzverteilung basierend auf tatsächlichen Längen
            int minWidth = ellipsisWidth + m_sprMeta->textWidth("AAA"); // Minimum pro Feld

            // Prüfe ob einer der Texte ungekürzt passt
            if (artistWidth + minWidth <= availableForText)
            {
                // Artist passt ungekürzt, kürze nur Title
                stringutils::safeCopy(truncatedArtist, 256, first);
                int remainingWidth = availableForText - artistWidth;
                truncateWithEllipsis(m_sprMeta, second, truncatedTitle, 256, remainingWidth);
            }
            else if (titleWidth + minWidth <= availableForText)
            {
                // Title passt ungekürzt, kürze nur Artist
                stringutils::safeCopy(truncatedTitle, 256, second);
                int remainingWidth = availableForText - titleWidth;
                truncateWithEllipsis(m_sprMeta, first, truncatedArtist, 256, remainingWidth);
            }
            else
            {
                // Beide müssen gekürzt werden
                // Feste Verteilung: 40% Artist, 60% Title
                int maxArtistWidth = (availableForText * 40) / 100;
                int maxTitleWidth = availableForText - maxArtistWidth;

                truncateWithEllipsis(m_sprMeta, first, truncatedArtist, 256, maxArtistWidth);
                truncateWithEllipsis(m_sprMeta, second, truncatedTitle, 256, maxTitleWidth);
            }
        }
        else
        {
            // Beide passen ungekürzt
            stringutils::safeCopy(truncatedArtist, 256, first);
            stringutils::safeCopy(truncatedTitle, 256, second);
        }
    }
    else if (hasSender)
    {
        truncateWithEllipsis(m_sprMeta, first, truncatedArtist, 256, availableForText);
    }
    else if (hasTitle)
    {
        truncateWithEllipsis(m_sprMeta, second, truncatedTitle, 256, availableForText);
    }

    const char *finalArtist = (hasSender && truncatedArtist[0] != '\0') ? truncatedArtist : "";
    const char *finalTitle = (hasTitle && truncatedTitle[0] != '\0') ? truncatedTitle : "";

    int finalWidth = calcFooterWidth(finalArtist, finalTitle);
    int startX = (spriteWidth - finalWidth) / 2;

    drawFooter(finalArtist, finalTitle, startX);
    m_sprMeta->unloadFont();
    m_sprMeta->pushSprite(0, spriteY);
}

// ===================================================================
// Lautstärkeregler (Knob)
// ===================================================================

/**
 * @brief Berechnet Marker-Position auf dem Kreis
 */
void SdPlayer::calcMarkerPos(float angleDeg, int center, int radiusOffset, int &outX, int &outY)
{
    float rad = (angleDeg - 90.0f) * 3.1415927f / 180.0f;
    outX = center + cosf(rad) * (center - radiusOffset);
    outY = center + sinf(rad) * (center - radiusOffset);
}

/**
 * @brief Zeichnet den Lautstärkeregler
 */
void SdPlayer::drawKnob(bool init, uint8_t vol, bool skipCallbacks)
{
    int markerX = 0, markerY = 0;
    vol = constrain(vol, 0, 100);

    constexpr int x = 0;
    constexpr int y = 373;
    constexpr int wh = 107;
    constexpr int mid = wh / 2;
    constexpr int minTouchDistance = 20;
    constexpr int padding = 20;
    constexpr int markerRadiusOffset = 20;
    constexpr int markerCircleRadius = 13;
    constexpr int markerCircleThickness = 3;
    constexpr uint16_t colorActive = 0x867d;
    constexpr uint16_t colorTrack = 0x39E8;
    constexpr uint16_t colorMarker = 0x5370;
    constexpr uint16_t colorText = TFT_LIGHTGREY;
    constexpr uint16_t colorTextBg = TFT_BLACK;
    constexpr uint16_t colorTextVal = TFT_DARKGREY;
    constexpr int muteAreaRadius = 16;

    if (init)
    {
        m_knobTouchState = KnobTouchState::IDLE;
        m_muteTriggered = false;
        m_totalDegrees = vol * 36.0f;

        if (!m_tft->safeCreateSprite(m_sprKnobText, wh, 13))
        {
            LOG_ERROR("Konnte Knob-Text Sprite nicht erstellen!");
            return;
        }

        m_sprKnobText->clear(TFT_BLACK);
        m_sprKnobText->loadFont(ComfortaaBold10);
        m_sprKnobText->setTextColor(colorText, colorTextBg);
        m_sprKnobText->setTextDatum(top_center);
        m_sprKnobText->drawString(STR(SDPlayerKey::SDPLAYER_KNOB_LABEL), wh / 2, 0);
        m_sprKnobText->pushSprite(x, y - 13);
        m_sprKnobText->unloadFont();
        m_tft->safeDeleteSprite(m_sprKnobText);

        if (!m_tft->safeCreateSprite(m_sprKnob, wh, wh))
        {
            LOG_ERROR("Konnte Knob Sprite nicht erstellen!");
            return;
        }

        // Ohne zweite Farbe bleibt der Zeichenhintergrund durchsichtig. Mit ihr malt drawString die ganze Zeichenzelle aus, und die ist höher als die Ziffern selbst; sie löschte dadurch ein Stück des Markierungskreises.
        m_sprKnob->loadFont(LeagueGothicCond34);
        m_sprKnob->setTextColor(colorTextVal);
        m_sprKnob->clear(TFT_BLACK);
        m_sprKnob->drawSmoothGauge(mid, mid, mid - 1, 3.0f, 0, 360, 0, 100, vol, colorActive, colorTrack, true, TFT_BLACK);

        calcMarkerPos(m_totalDegrees, mid, markerRadiusOffset, markerX, markerY);
        m_sprKnob->drawSmoothCircle(markerX, markerY, markerCircleRadius, markerCircleThickness, colorMarker, TFT_BLACK);

        if (vol > 0)
        {
            char volStr[5];
            stringutils::safeFormat(volStr, "%d", vol);

            // Der Versatz hängt nur an der Stellenzahl und der Breite der ersten Ziffer, nicht an der Breite der ganzen Zahl. Dadurch steht sie still, statt bei jedem Wechsel um einen halben Bildpunkt zu springen, wie es eine echte Zentrierung täte.
            char firstDigit[2] = {volStr[0], '\0'};
            int32_t volTextW = m_sprKnob->textWidth(firstDigit);
            int xOffset = (vol < 10) ? (volTextW / 2) : (vol < 100) ? volTextW
                                                                    : (2 * volTextW);
            m_sprKnob->drawString(volStr, mid - xOffset, mid - 15);
        }
        else
        {
            m_sprKnob->pushImage(mid - 14, mid - 14, 28, 28,
                                    IconShapes::cached(IconId::Mute, ICON_MUTE_COLOR, ICON_MUTE_BACKGROUND));
        }

        m_sprKnob->pushSprite(x, y);

        if (!skipCallbacks && m_callbacks.onVolumeChanged != nullptr)
            m_callbacks.onVolumeChanged(vol);

        return;
    }

    // Touch-Verarbeitung
    if (m_touched && m_touchX > (x - padding) && m_touchX < (x + wh + padding) && m_touchY > (y - padding) && m_touchY < (y + wh + padding))
    {
        int deltaX = m_touchX - (x + mid);
        int deltaY = m_touchY - (y + mid);
        int touchDistance = deltaX * deltaX + deltaY * deltaY;

        // Touch-Mode festlegen (nur beim ersten Touch)
        if (m_knobTouchState == KnobTouchState::IDLE)
        {
            if (touchDistance <= muteAreaRadius * muteAreaRadius)
            {
                m_knobTouchState = KnobTouchState::MUTE;
            }
            else if (touchDistance >= minTouchDistance * minTouchDistance)
            {
                m_knobTouchState = KnobTouchState::ROTATING;
                // Startwinkel für Rotation initialisieren
                float rad = atan2f(deltaY, deltaX);
                m_lastAngle = rad * 57.29578f;
            }
            // else: Zu nah am Zentrum aber außerhalb Mute -> nichts tun
        }

        // Mute-Logik (nur einmal beim ersten Touch im Mute-Bereich)
        if (m_knobTouchState == KnobTouchState::MUTE)
        {
            if (!m_muteTriggered)
            {
                uint8_t currentVol = APP_STATE.getCurrentVolume();
                if (currentVol > 0)
                {
                    APP_STATE.setVolumeBeforeMute(currentVol);
                    APP_STATE.setCurrentVolume(0);

                    // Gleiche Stumm-Semantik wie das Radio: das Flag lässt den AudioManager auch die Pegelanzeigen nullen
                    APP_STATE.setIsMuted(true);
                    drawKnob(true, 0);

                    if (m_callbacks.onVolumeChanged)
                        m_callbacks.onVolumeChanged(0);
                }
                else
                {
                    uint8_t savedVol = APP_STATE.getVolumeBeforeMute();
                    APP_STATE.setCurrentVolume(savedVol);
                    drawKnob(true, savedVol);

                    // handleVolumeChanged hebt die Stummschaltung bei hörbarem Wert auf
                    if (m_callbacks.onVolumeChanged)
                        m_callbacks.onVolumeChanged(savedVol);
                }

                m_muteTriggered = true;
            }
            // Kein weiterer Code - warten bis User loslässt
            return;
        }

        // Rotations-Logik (nur wenn im Rotations-Modus)
        if (m_knobTouchState == KnobTouchState::ROTATING)
        {
            calcMarkerPos(m_totalDegrees, mid, markerRadiusOffset, markerX, markerY);

            float rad = atan2f(deltaY, deltaX);
            float currentAngle = rad * 57.29578f;

            float deltaAngle = fmod(currentAngle - m_lastAngle + 540.0f, 360.0f) - 180.0f;
            if (fabs(deltaAngle) < 2.0f)
                return;

            m_totalDegrees = constrain(m_totalDegrees + deltaAngle, 0.0f, 3600.0f);
            m_lastAngle = currentAngle;

            uint8_t newVol = roundf(m_totalDegrees / 36.0f);

            m_sprKnob->clear(TFT_BLACK);
            m_sprKnob->drawSmoothGauge(mid, mid, mid - 1, 3.0f, 0, 360, 0, 100, newVol, colorActive, colorTrack, true, TFT_BLACK);

            calcMarkerPos(m_totalDegrees, mid, markerRadiusOffset, markerX, markerY);
            m_sprKnob->drawSmoothCircle(markerX, markerY, markerCircleRadius, markerCircleThickness, colorMarker, TFT_BLACK);

            if (newVol > 0)
            {
                char newVolStr[5];
                stringutils::safeFormat(newVolStr, "%d", newVol);
                char firstDigit[2] = {newVolStr[0], '\0'};
                int32_t newVolTextW = m_sprKnob->textWidth(firstDigit);
                int xOffset = (newVol < 10) ? (newVolTextW / 2) : (newVol < 100) ? newVolTextW
                                                                                 : (2 * newVolTextW);
                m_sprKnob->drawString(newVolStr, mid - xOffset, mid - 15);
            }

            m_sprKnob->pushSprite(x, y);

            if (newVol != m_prevVol)
            {
                APP_STATE.setCurrentVolume(newVol);
                if (m_callbacks.onVolumeChanged != nullptr)
                {
                    m_callbacks.onVolumeChanged(newVol);
                }

                m_prevVol = newVol;
            }
        }
    }
    else
    {
        // Kein Touch mehr -> zurück zu IDLE und Flag zurücksetzen
        m_knobTouchState = KnobTouchState::IDLE;
        m_muteTriggered = false;
    }
}

// ===================================================================
// Equalizer
// ===================================================================

/**
 * @brief Zeichnet die Equalizer-Regler (Balance, Tiefen, Mitten, Höhen)
 */
void SdPlayer::drawTracks(bool init, int8_t balance, int8_t gainLowPass, int8_t gainBandPass, int8_t gainHighPass, bool skipCallbacks)
{
    // Position und Größe
    constexpr int sliderPosX = 130;
    constexpr int sliderPosY = 360;
    constexpr int sliderWeight = 190;
    constexpr int sliderHeight = 120;

    // Konstanten
    constexpr int SLIDER_X[] = {5, 55, 105, 155};
    constexpr int TOUCH_WIDTH = 31;
    constexpr int PIXEL_MIN = 104;
    constexpr int PIXEL_ZERO = 64;
    constexpr int PIXEL_MAX = 24;

    auto mapDbToPixel = [](int8_t db) -> int8_t
    {
        return (db <= 0) ? map(db, -12, 0, PIXEL_MIN, PIXEL_ZERO)
                         : map(db, 0, 12, PIXEL_ZERO, PIXEL_MAX);
    };

    auto mapPixelToDb = [](int y) -> int8_t
    {
        return (y > PIXEL_ZERO) ? map(y, PIXEL_MIN, PIXEL_ZERO, -12, 0)
                                : map(y, PIXEL_ZERO, PIXEL_MAX, 0, 12);
    };

    auto drawSlider = [this](int xPos, const __FlashStringHelper *label, int8_t pixelValue, bool withLabel = true)
    {
        m_sprTracks->fillGradientRect(xPos + 14, 2, 3, 116, 0x1082, 0x39E8, lgfx::v1::gradient_fill_styles::VLINEAR);

        if (withLabel && label != nullptr)
            m_sprTracks->drawString(label, (xPos == 5) ? xPos - 5 : xPos, 0);

        m_sprTracks->drawFastHLine(xPos, PIXEL_ZERO, TOUCH_WIDTH, TFT_DARKGREY);
        m_sprTracks->drawSmoothCircle(xPos + 15, pixelValue, 13, 3, 0x5370, TFT_BLACK);
        m_sprTracks->fillSmoothCircle(xPos + 15, pixelValue, 10, TFT_BLACK);
    };

    balance = constrain(balance, -16, 16);
    gainLowPass = constrain(gainLowPass, -12, 12);
    gainBandPass = constrain(gainBandPass, -12, 12);
    gainHighPass = constrain(gainHighPass, -12, 12);

    int8_t balancePixel = map(balance, -16, 16, PIXEL_MIN, PIXEL_MAX);
    int8_t lowPixel = mapDbToPixel(gainLowPass);
    int8_t midPixel = mapDbToPixel(gainBandPass);
    int8_t highPixel = mapDbToPixel(gainHighPass);

    if (init)
    {
        if (!m_tft->safeCreateSprite(m_sprTracks, sliderWeight, sliderHeight))
        {
            LOG_ERROR("Konnte Tracks Sprite nicht erstellen!");
            return;
        }

        m_sprTracks->clear(TFT_BLACK);
        m_sprTracks->loadFont(ComfortaaBold10);
        m_sprTracks->setTextColor(TFT_LIGHTGREY, TFT_BLACK);

        drawSlider(SLIDER_X[0], STR(SDPlayerKey::SDPLAYER_SLIDER_BALANCE), balancePixel);
        drawSlider(SLIDER_X[1], STR(SDPlayerKey::SDPLAYER_SLIDER_LOW), lowPixel);
        drawSlider(SLIDER_X[2], STR(SDPlayerKey::SDPLAYER_SLIDER_MID), midPixel);
        drawSlider(SLIDER_X[3], STR(SDPlayerKey::SDPLAYER_SLIDER_HIGH), highPixel);

        m_sprTracks->pushSprite(sliderPosX, sliderPosY);
        m_sprTracks->unloadFont();

        if (!skipCallbacks)
        {
            if (m_callbacks.onBalanceChanged)
                m_callbacks.onBalanceChanged(balance);

            if (m_callbacks.onGainLowPassChanged)
                m_callbacks.onGainLowPassChanged(gainLowPass);

            if (m_callbacks.onGainBandPassChanged)
                m_callbacks.onGainBandPassChanged(gainBandPass);

            if (m_callbacks.onGainHighPassChanged)
                m_callbacks.onGainHighPassChanged(gainHighPass);
        }

        m_balance_prev = balancePixel;
        m_gainLowPass_prev = lowPixel;
        m_gainBandPass_prev = midPixel;
        m_gainHighPass_prev = highPixel;

        return;
    }

    if (!m_touched)
        return;

    int deltaX = m_touchX - sliderPosX;
    int deltaY = m_touchY - sliderPosY;

    if (deltaY <= 23 || deltaY >= 105)
        return;

    typedef void (*CallbackFunc)(int8_t);
    auto updateSlider = [&](int sliderIndex, int8_t &prevValue, int8_t currentPixel, CallbackFunc callback, bool isDbSlider = false)
    {
        if (deltaX >= SLIDER_X[sliderIndex] && deltaX <= SLIDER_X[sliderIndex] + TOUCH_WIDTH)
        {
            deltaY = constrain(deltaY, PIXEL_MAX, PIXEL_MIN);

            if (prevValue != deltaY)
            {
                m_sprTracks->setClipRect(SLIDER_X[sliderIndex], 11, TOUCH_WIDTH, 107);
                m_sprTracks->clear(TFT_BLACK);
                drawSlider(SLIDER_X[sliderIndex], nullptr, deltaY, false);
                m_sprTracks->pushSprite(sliderPosX, sliderPosY);
                m_sprTracks->clearClipRect();

                int8_t value = isDbSlider ? mapPixelToDb(deltaY)
                                          : map(deltaY, PIXEL_MIN, PIXEL_MAX, -16, 16);

                if (sliderIndex == 0)
                    APP_STATE.setCurrentBalance(value);
                else if (sliderIndex == 1)
                    APP_STATE.setCurrentGainLowPass(value);
                else if (sliderIndex == 2)
                    APP_STATE.setCurrentGainBandPass(value);
                else if (sliderIndex == 3)
                    APP_STATE.setCurrentGainHighPass(value);

                if (callback != nullptr)
                    callback(value);

                prevValue = deltaY;
            }

            return true;
        }

        return false;
    };

    if (updateSlider(0, m_balance_prev, balancePixel, m_callbacks.onBalanceChanged))
        return;

    if (updateSlider(1, m_gainLowPass_prev, lowPixel, m_callbacks.onGainLowPassChanged, true))
        return;

    if (updateSlider(2, m_gainBandPass_prev, midPixel, m_callbacks.onGainBandPassChanged, true))
        return;

    if (updateSlider(3, m_gainHighPass_prev, highPixel, m_callbacks.onGainHighPassChanged, true))
        return;
}

// ===================================================================
// Playlist-Verwaltung
// ===================================================================

/**
 * @brief Leere Playlist: vergrößertes Ordner-Symbol, darunter "Keine Titel" und zwei Hinweiszeilen zur Durchsuchen-Taste
 */
void SdPlayer::drawEmptyPlaylist()
{
    constexpr uint16_t BACKGROUND = 0x1082;
    constexpr uint16_t HINT_COLOR = 0x5ACB;
    constexpr int ICON_SCALE = 4;
    constexpr int ICON_Y = 78;
    constexpr int TITLE_Y = 168;
    constexpr int HINT1_Y = 196;
    constexpr int HINT2_Y = 216;
    constexpr int HINT_MARGIN = 10;

    // Symbol einmal vergrößert zeichnen und für die Laufzeit des Moduls behalten
    if (!m_emptyFolderIcon)
        m_emptyFolderIcon = IconShapes::draw(IconId::Folder, TFT_DARKGREY, BACKGROUND, ICON_SCALE);

    if (m_emptyFolderIcon)
    {
        const int iconW = IconShapes::width(IconId::Folder) * ICON_SCALE;
        const int iconH = IconShapes::height(IconId::Folder) * ICON_SCALE;
        m_sprPlaylist->pushImage((PLAYLIST_W - iconW) / 2, ICON_Y, iconW, iconH, m_emptyFolderIcon);
    }

    m_sprPlaylist->setTextDatum(MC_DATUM);
    m_sprPlaylist->setTextColor(Theme::ColorMainText, BACKGROUND);
    m_sprPlaylist->drawString(STR(SDPlayerKey::SDPLAYER_PLAYLIST_EMPTY), PLAYLIST_W / 2, TITLE_Y);

    m_sprPlaylist->loadFont(ComfortaaBold14);
    m_sprPlaylist->setTextColor(HINT_COLOR, BACKGROUND);
    char hint[96];
    truncateWithEllipsis(m_sprPlaylist, reinterpret_cast<PGM_P>(STR(SDPlayerKey::SDPLAYER_PLAYLIST_HINT_1)), hint, sizeof(hint), PLAYLIST_W - 2 * HINT_MARGIN);
    m_sprPlaylist->drawString(hint, PLAYLIST_W / 2, HINT1_Y);
    truncateWithEllipsis(m_sprPlaylist, reinterpret_cast<PGM_P>(STR(SDPlayerKey::SDPLAYER_PLAYLIST_HINT_2)), hint, sizeof(hint), PLAYLIST_W - 2 * HINT_MARGIN);
    m_sprPlaylist->drawString(hint, PLAYLIST_W / 2, HINT2_Y);

    // Schrift und Ausrichtung der Listenzeilen wiederherstellen
    m_sprPlaylist->loadFont(ComfortaaBold16);
    m_sprPlaylist->setTextDatum(ML_DATUM);
}

/**
 * @brief Zeichnet die gesamte Playlist
 */
void SdPlayer::drawPlaylist()
{
    m_sprPlaylist->clear(0x1082);
    m_sprPlaylist->loadFont(ComfortaaBold16);
    m_sprPlaylist->setTextDatum(ML_DATUM);

    if (m_playlist.empty())
    {
        drawEmptyPlaylist();
    }
    else
    {
        int yPos = -m_scrollState.scrollOffset;
        for (size_t i = 0; i < m_playlist.size(); ++i)
        {
            if (yPos + PLAYLIST_ITEM_H >= 0 && yPos < m_scrollState.viewportHeight)
                drawPlaylistEntry(i, yPos);

            yPos += PLAYLIST_ITEM_H;
        }
    }

    m_sprPlaylist->unloadFont();
    m_sprPlaylist->pushSprite(PLAYLIST_X, PLAYLIST_Y);

    m_scrollState.contentHeight = m_playlist.size() * PLAYLIST_ITEM_H;
    m_scrollState.maxScroll = std::max(0, m_scrollState.contentHeight - m_scrollState.viewportHeight);
}

/**
 * @brief Zeichnet einen Eintrag der Playliste
 */
void SdPlayer::drawPlaylistEntry(int index, int yPos)
{
    const auto &entry = m_playlist[index];

    // Farben & Hintergrund
    uint16_t bgColor = 0x1082;                 // Standard-Hintergrund
    uint16_t textColor = Theme::ColorMainText; // Standard-Text
    uint16_t numberColor = 0x5ACB;             // Standard-Nummer

    if (index == m_currentTrackIndex && !m_playlistFinished)
    {
        // Aktiver Track
        bgColor = 0x328a;
        textColor = Theme::ColorMainText;
        numberColor = 0xFC60;
        m_sprPlaylist->fillRect(0, yPos, PLAYLIST_W, PLAYLIST_ITEM_H, bgColor);
    }
    else if (m_playedTracks.size() > index && m_playedTracks[index])
    {
        // Nur ein wirklich zu Ende gespielter Track wird ausgegraut, übersprungene bleiben weiß
        textColor = 0x39E8;
        numberColor = 0x39E8;
    }

    // Track-Nummer
    char numberStr[8];
    stringutils::safeFormat(numberStr, "%2d.", index + 1);

    // Breite der Nummernspalte
    const int numberAreaWidth = m_sprPlaylist->textWidth("999.");

    m_sprPlaylist->setTextDatum(ML_DATUM);
    m_sprPlaylist->setTextColor(numberColor, bgColor);
    m_sprPlaylist->drawString(numberStr, 10, yPos + PLAYLIST_ITEM_H / 2);

    // Dateiname extrahieren
    const char *filename = strrchr(entry.path, '/');
    filename = filename ? filename + 1 : entry.path;

    // Für die Anzeige bereinigen
    char cleanName[128];
    cleanupFilename(filename, cleanName, sizeof(cleanName));

    // Platzberechnung
    const int leftX = numberAreaWidth + 15; // linker Start direkt nach Nummer + Innenabstand
    const int rightPad = 10;                // rechter Innenabstand
    int maxWidth = PLAYLIST_W - leftX - rightPad;

    // Bei Bedarf kürzen
    char displayName[128];
    int w = m_sprPlaylist->textWidth(cleanName);

    if (w > maxWidth)
    {
        const char *ellipsis = "...";
        const int ellipsisWidth = m_sprPlaylist->textWidth(ellipsis);
        const int targetWidth = maxWidth - ellipsisWidth;

        displayName[0] = '\0';
        const size_t len = strlen(cleanName);

        // Rechtsseitige Kürzung mit Breitenprüfung
        for (size_t i = len; i > 0; --i)
        {
            // Nie mitten in einer UTF-8-Sequenz terminieren, sonst bleibt ein halbes Zeichen vor dem "..." stehen
            if (((unsigned char)cleanName[i] & 0xC0) == 0x80)
                continue;

            char tmp[128];
            stringutils::safeCopy(tmp, sizeof(tmp), cleanName);
            tmp[i] = '\0';

            if (m_sprPlaylist->textWidth(tmp) <= targetWidth)
            {
                stringutils::safeFormat(displayName, "%s%s", tmp, ellipsis);
                break;
            }
        }

        if (displayName[0] == '\0')
            stringutils::safeCopy(displayName, sizeof(displayName), ellipsis);
    }
    else
    {
        stringutils::safeCopy(displayName, sizeof(displayName), cleanName);
    }

    // Zeichnen des bereinigten Namens
    m_sprPlaylist->setTextColor(textColor, bgColor);
    m_sprPlaylist->drawString(displayName, leftX, yPos + PLAYLIST_ITEM_H / 2);
}

/**
 * @brief Scrollbar zeichnen
 */
void SdPlayer::drawScrollbar()
{
    m_sprScrollbar->clear(TFT_BLACK);

    // Passt der Inhalt in den Viewport, gibt es nichts zu scrollen: nur den Grund zeichnen, die Verhältnisrechnungen teilten sonst durch 0
    if (m_scrollState.maxScroll <= 0)
    {
        m_sprScrollbar->fillRect(0, 0, SCROLLBAR_W, m_scrollState.viewportHeight, 0x1082);
        m_sprScrollbar->pushSprite(PLAYLIST_X + PLAYLIST_W, PLAYLIST_Y);
        return;
    }

    float visibleRatio = (float)m_scrollState.viewportHeight / (float)m_scrollState.contentHeight;
    int barHeight = std::max(20, (int)lroundf(m_scrollState.viewportHeight * visibleRatio));
    int clampedOffset = std::max(0, std::min(m_scrollState.scrollOffset, m_scrollState.maxScroll));
    float scrollRatio = (float)clampedOffset / (float)m_scrollState.maxScroll;
    int barY = (int)lroundf(scrollRatio * (float)(m_scrollState.viewportHeight - barHeight));

    m_sprScrollbar->fillRect(0, 0, SCROLLBAR_W, m_scrollState.viewportHeight, 0x2104);

    uint16_t barColor = m_scrollState.isDragging ? 0x8410 : 0x5ACB;
    m_sprScrollbar->fillRect(0, barY, SCROLLBAR_W, barHeight, barColor);

    m_sprScrollbar->pushSprite(PLAYLIST_X + PLAYLIST_W, PLAYLIST_Y);
}

/**
 * @brief Scroll-Physik aktualisieren
 */
void SdPlayer::updateScrollPhysics()
{
    // Wenn Dragging aktiv ist
    if (m_scrollState.isDragging)
        return;

    // Auch ein in der Liste ruhender Finger hält Snap-Back und Trägheit an: er fängt die Liste, die Drag-Anker bleiben gültig und beim Überschreiten der Wegschwelle springt nichts (m_wasTouching ist nach handlePlaylistTouch der aktuelle Berührungszustand)
    if (m_wasTouching && m_scrollState.startedInPlaylist)
        return;

    // Keine Aktion nötig wenn scrollOffset im gültigen Bereich ist (0 bis maxScroll) und Scroll-Velocity vernachlässigbar ist
    if (m_scrollState.scrollOffset >= 0 && m_scrollState.scrollOffset <= m_scrollState.maxScroll && abs(m_scrollState.scrollVelocity) <= 0.5f)
    {
        // Velocity auf 0 setzen wenn sie sehr klein ist
        if (m_scrollState.scrollVelocity != 0)
            m_scrollState.scrollVelocity = 0;

        return;
    }

    // Scroll-Physik verarbeiten
    if (m_scrollState.scrollOffset < 0)
    {
        // Bounce-back nach oben
        float targetOffset = 0;
        float diff = targetOffset - m_scrollState.scrollOffset;
        m_scrollState.scrollOffset += diff * 0.35f;

        if (abs(m_scrollState.scrollOffset) < 1.0f)
        {
            m_scrollState.scrollOffset = 0;
            m_scrollState.scrollVelocity = 0;
        }

        m_needsPlaylistRedraw = true;
    }
    else if (m_scrollState.scrollOffset > m_scrollState.maxScroll)
    {
        // Bounce-back nach unten
        float targetOffset = m_scrollState.maxScroll;
        float diff = targetOffset - m_scrollState.scrollOffset;
        m_scrollState.scrollOffset += diff * 0.35f;

        if (abs(m_scrollState.scrollOffset - m_scrollState.maxScroll) < 1.0f)
        {
            m_scrollState.scrollOffset = m_scrollState.maxScroll;
            m_scrollState.scrollVelocity = 0;
        }

        m_needsPlaylistRedraw = true;
    }
    else if (abs(m_scrollState.scrollVelocity) > 0.5f)
    {
        // Scrolling: Bruchteil-Bildpunkte sammeln, die Ganzzahl-Position schnitte sonst Schritte unter einem Bildpunkt ab und die Liste bliebe je nach Richtung stehen oder kröche ruckelnd
        const float delta = m_scrollState.scrollVelocity / 60.0f + m_scrollState.inertiaRemainder;
        const int wholePixels = (int)lroundf(delta);
        m_scrollState.inertiaRemainder = delta - wholePixels;
        m_scrollState.scrollOffset += wholePixels;
        m_scrollState.scrollVelocity *= 0.92f;

        if (abs(m_scrollState.scrollVelocity) < 1)
            m_scrollState.scrollVelocity = 0;

        if (m_scrollState.scrollOffset < 0)
        {
            m_scrollState.scrollOffset = 0;
            m_scrollState.scrollVelocity = 0;
        }
        else if (m_scrollState.scrollOffset > m_scrollState.maxScroll)
        {
            m_scrollState.scrollOffset = m_scrollState.maxScroll;
            m_scrollState.scrollVelocity = 0;
        }

        if (wholePixels != 0)
            m_needsPlaylistRedraw = true;
    }
}

/**
 * @brief Verarbeitet Touch-Eingaben für Playlist-Scrolling und Titel-Tipp
 */
void SdPlayer::handlePlaylistTouch(int32_t x, int32_t y, bool is_touched)
{
    constexpr unsigned long TAP_MAX_HOLD_MS = 300; ///< Längster Druck, der noch als Tipp gilt
    constexpr float TAP_MAX_VELOCITY = 0.5f;       ///< Bis zu dieser Restgeschwindigkeit gilt die Liste als ruhend

    // Kein Touch und vorher auch keiner: nichts zu tun, m_wasTouching ist bereits false
    if (!is_touched && !m_wasTouching)
        return;

    // Nur bei Touch: Prüfen ob Touch in Playlist-Bereich ist
    bool inPlaylist = is_touched &&
                      x >= PLAYLIST_X &&
                      x <= PLAYLIST_X + PLAYLIST_W &&
                      y >= PLAYLIST_Y &&
                      y <= PLAYLIST_Y + PLAYLIST_H;

    unsigned long currentTime = millis();

    if (is_touched && !m_wasTouching)
    {
        if (inPlaylist)
        {
            // Nur eine ruhende Liste nimmt einen Tipp an, ein Tipp in die rollende Liste stoppt sie nur
            const bool listIdle = abs(m_scrollState.scrollVelocity) <= TAP_MAX_VELOCITY &&
                                  m_scrollState.scrollOffset >= 0 &&
                                  m_scrollState.scrollOffset <= m_scrollState.maxScroll;
            m_scrollState.tapCandidate = listIdle;
            m_scrollState.startedInPlaylist = true;
            m_scrollState.touchStartMs = currentTime;
            m_scrollState.isDragging = false;
            m_scrollState.scrollVelocity = 0;
            m_scrollState.inertiaRemainder = 0;
            m_scrollState.dragStartX = x;
            m_scrollState.dragStartY = y;
            m_scrollState.dragStartOffset = m_scrollState.scrollOffset;
        }
        else
        {
            m_scrollState.tapCandidate = false;
            m_scrollState.startedInPlaylist = false;
        }
    }
    else if (is_touched && m_wasTouching)
    {
        // Nur eine in der Liste begonnene Geste darf scrollen: eine hereinwandernde Berührung rechnete sonst mit den Drag-Ankern der vorigen Geste
        if (inPlaylist && m_scrollState.startedInPlaylist)
        {
            int deltaY = y - m_scrollState.dragStartY;

            // Seitliches Wegwischen ist kein Tipp, auch wenn der Y-Weg klein bleibt
            if (abs((int)(x - m_scrollState.dragStartX)) > 10)
                m_scrollState.tapCandidate = false;

            if (abs(deltaY) > 10)
            {
                m_scrollState.tapCandidate = false;
                m_scrollState.isDragging = true;
                m_scrollState.scrollOffset = m_scrollState.dragStartOffset - (deltaY * 1.8f);
                m_needsPlaylistRedraw = true;

                unsigned long dt = currentTime - m_scrollState.lastTouchTime;
                if (dt > 0 && dt < 100)
                {
                    float newVelocity = (m_scrollState.lastTouchY - y) * 1000.0f / dt;
                    m_scrollState.scrollVelocity = (newVelocity * 0.7f) + (m_scrollState.scrollVelocity * 0.3f);
                    m_scrollState.scrollVelocity *= 1.5f;
                }

                if (m_scrollState.scrollOffset < -50)
                {
                    m_scrollState.scrollOffset = -50 + (m_scrollState.scrollOffset + 50) * 0.3f;
                }
                else if (m_scrollState.scrollOffset > m_scrollState.maxScroll + 50)
                {
                    float excess = m_scrollState.scrollOffset - m_scrollState.maxScroll - 50;
                    m_scrollState.scrollOffset = m_scrollState.maxScroll + 50 + excess * 0.3f;
                }
            }

            m_scrollState.lastTouchY = y;
            m_scrollState.lastTouchTime = currentTime;
        }
        else
        {
            // Die Geste hat die Liste verlassen oder gehört ihr nicht: ein späteres Loslassen darf keinen Titel starten
            m_scrollState.tapCandidate = false;
        }
    }
    else if (!is_touched && m_wasTouching)
    {
        if (m_scrollState.scrollVelocity > 3000)
            m_scrollState.scrollVelocity = 3000;
        else if (m_scrollState.scrollVelocity < -3000)
            m_scrollState.scrollVelocity = -3000;

        if (m_scrollState.tapCandidate && currentTime - m_scrollState.touchStartMs <= TAP_MAX_HOLD_MS)
            handlePlaylistTap();

        // Der Daumen steht noch in der Zieh-Farbe; endet der Zug ohne Restgeschwindigkeit, zeichnete ihn sonst nichts um
        if (m_scrollState.isDragging)
            m_needsPlaylistRedraw = true;

        m_scrollState.tapCandidate = false;
        m_scrollState.isDragging = false;
    }

    m_wasTouching = is_touched;
}

/**
 * @brief Spielt den in der Playlist angetippten Titel ab
 */
void SdPlayer::handlePlaylistTap()
{
    if (m_playlist.empty())
        return;

    // Dieselbe Rechnung wie beim Zeichnen, nur rückwärts: Druckpunkt plus Scrollstand ergibt die Zeile
    const int tappedIndex = (m_scrollState.dragStartY - PLAYLIST_Y + m_scrollState.dragStartOffset) / PLAYLIST_ITEM_H;
    if (tappedIndex < 0 || tappedIndex >= (int)m_playlist.size())
        return;

    // Ein Tipp auf den laufenden, pausierten oder gerade startenden Titel startet ihn nicht neu (der Zustand bestätigt einen frischen Start erst mit dem nächsten Poll); im Stopp spielt auch der vorgemerkte Titel
    if (tappedIndex == m_currentTrackIndex && (m_playbackState != PlaybackState::STOPPED || m_waitingForPlaybackStart))
        return;

    // Nach dem Playlist-Ende beginnt ein Tipp wie die Play-Taste einen frischen Durchlauf
    if (m_playlistFinished)
        resetPlayedStatus();

    m_currentTrackIndex = tappedIndex;
    playCurrentTrack();
}

/**
 * @brief Scrollt automatisch zum aktuellen Track
 */
void SdPlayer::scrollToCurrentTrack()
{
    if (m_playlist.empty() || m_currentTrackIndex < 0 || m_currentTrackIndex >= m_playlist.size())
        return;

    int trackYPos = m_currentTrackIndex * PLAYLIST_ITEM_H;
    int targetOffset = trackYPos - (m_scrollState.viewportHeight / 2) + (PLAYLIST_ITEM_H / 2);
    targetOffset = constrain(targetOffset, 0, m_scrollState.maxScroll);

    m_scrollState.scrollOffset = targetOffset;
    m_scrollState.scrollVelocity = 0;
    m_needsPlaylistRedraw = true;
}

/**
 * @brief Setzt den Abspiel-Status aller Tracks zurück
 */
void SdPlayer::resetPlayedStatus()
{
    m_playedTracks.clear();
    m_playedTracks.resize(m_playlist.size(), false);
    m_playlistFinished = false;
    m_needsPlaylistRedraw = true;
}

// ===================================================================
// Playback-Steuerung
// ===================================================================

/**
 * @brief Spielt den aktuellen Track ab
 */
void SdPlayer::playCurrentTrack()
{
    if (m_playlist.empty())
        return;

    if (m_currentTrackIndex >= 0 && m_currentTrackIndex < m_playlist.size())
    {
        // Alten Track stoppen
        requestBackendStop();

        // SD-Playback beendet -> Visualizer sperren
        APP_STATE.setStationSetDone(false);

        // Metadaten im AudioManager zurücksetzen
        g_audioManager.resetMetadata();

        // Lokale Laufzeit-/Metadaten zurücksetzen
        resetRuntimePlaybackState();
        clearDisplayedMetadata(false);

        // Cover-Daten für neuen Track leeren
        m_coverOffsets.clear();
        m_eventCoverData.clear();

        // Cover-Timeout zuerst starten, bevor m_currentAudioFile gesetzt wird! Sonst kann draw() dazwischen kommen und das Fallback-Cover zeichnen.
        m_waitingForCoverEvent = true;
        m_coverEventWaitStartMs = millis();

        // Audio-Dateipfad für Cover-Zugriff speichern
        stringutils::safeCopy(m_currentAudioFile, sizeof(m_currentAudioFile), m_playlist[m_currentTrackIndex].path);

        // EOF-Verarbeitung für neuen Track zurücksetzen
        m_trackFinishedProcessing = false;

        // Dateinamen als Fallback-Title
        const char *fullPath = m_playlist[m_currentTrackIndex].path;
        const char *filename = strrchr(fullPath, '/');
        filename = filename ? filename + 1 : fullPath;

        char cleanedFilename[128];
        cleanupFilename(filename, cleanedFilename, sizeof(cleanedFilename));
        stringutils::safeCopy(m_currentTitle, cleanedFilename);

        drawMetaText(false, "", m_currentTitle, 0x5370, TFT_SILVER, TFT_DARKGRAY, " | ");

        m_autoScrollRequested = true;

        // Playback-Start-Timeout initialisieren
        m_waitingForPlaybackStart = true;
        m_playbackStartWaitMs = millis();

        if (m_callbacks.onPlayFileRequested)
            m_callbacks.onPlayFileRequested(m_playlist[m_currentTrackIndex].path);

        // SD-Playback aktiv -> Visualizer entsperren
        APP_STATE.setStationSetDone(true);

        // Zustand sofort führen, die Anzeige hängt daran (Pause-Puls, Visualizer-Daten, Wickeldorne); der 1-Hz-Poll bestätigt nur noch, und bei einem Fehlstart räumt das Start-Timeout auf
        m_playbackState = PlaybackState::PLAYING;
        updatePlayerButtonStates();

        m_needsPlaylistRedraw = true;
        m_needsDetailsRedraw = true;
    }
}

/**
 * @brief Spielt den nächsten Track ab
 */
void SdPlayer::playNextTrack()
{
    if (m_playlist.empty())
        return;

    if (m_currentTrackIndex >= m_playlist.size() - 1)
    {
        LOG_INFO("Playlist beendet");
        m_playlistFinished = true;
        m_needsPlaylistRedraw = true;

        // Player stoppen, um weitere EOF-Events zu verhindern
        requestBackendStop();

        // SD-Playback beendet -> Visualizer sperren
        APP_STATE.setStationSetDone(false);

        clearDisplayedMetadata(true);
        resetRuntimePlaybackState();
        drawProgressBar();

        // Cover zurücksetzen und Fallback anzeigen
        resetCover();

        m_currentAudioFile[0] = '\0'; // Leerer Pfad für "Kein Cover"
        drawCoverFallback(nullptr);
        drawMetaText(false, " ", reinterpret_cast<PGM_P>(STR(SDPlayerKey::SDPLAYER_STATUS_PLAYLIST_END)),
                     Theme::ColorMainText, Theme::ColorMainText, Theme::ColorMainText, " - ");

        return;
    }

    m_currentTrackIndex++;
    playCurrentTrack();
}

/**
 * @brief Spielt den vorherigen Track ab
 */
void SdPlayer::playPreviousTrack()
{
    if (m_playlist.empty())
        return;

    if (m_currentTrackIndex > 0)
    {
        m_currentTrackIndex--;
        playCurrentTrack();
        LOG_INFO_F("Vorheriger Track: %d", m_currentTrackIndex + 1);
    }
}

/**
 * @brief Leert die Playlist komplett
 */
void SdPlayer::clearPlaylist()
{
    // Nur ein echter Auswurf wechselt das Tape: der Initialaufruf beim Screen-Aufbau und das Leeren einer bereits leeren Playlist lassen es liegen
    const bool hadTracks = !m_playlist.empty();

    m_playlist.clear();
    m_playedTracks.clear();

    if (hadTracks)
        selectRandomCassetteDesign();

    // Hinweis: KEIN shrink_to_fit() verwenden! Auf ESP32/PSRAM triggert shrink_to_fit() free+realloc und verursacht Heap-Fragmentierung. Die ursprüngliche Kapazität bleibt erhalten und wird bei der nächsten Playlist-Befüllung wiederverwendet.

    m_currentTrackIndex = 0;
    m_playlistFinished = false;
    m_scrollState.scrollOffset = 0;
    m_scrollState.scrollVelocity = 0;
    m_needsPlaylistRedraw = true;
    m_needsDetailsRedraw = true;

    resetRuntimePlaybackState();
    clearDisplayedMetadata(false);

    // Playback-State auf STOPPED setzen
    m_playbackState = PlaybackState::STOPPED;

    // Visualisierungen zurücksetzen
    zeroVisualizer();

    // Visualisierung sofort neu zeichnen, damit Nullwerte gerendert werden
    m_visualizer.draw(false);

    // Button-Farben sofort aktualisieren
    updatePlayerButtonStates();

    // Cover zurücksetzen
    resetCover();

    // Dateipfad leeren
    m_currentAudioFile[0] = '\0';
    drawCoverFallback(nullptr);

    drawMetaText(false, " ", reinterpret_cast<PGM_P>(STR(SDPlayerKey::SDPLAYER_STATUS_NO_PLAYBACK)),
                 Theme::ColorMainText, Theme::ColorMainText, Theme::ColorMainText, " - ");
    LOG_INFO("Playlist geleert.");
}

// ===================================================================
// Button-Handling
// ===================================================================

/**
 * @brief Aktualisiert die visuellen Zustände der Player-Buttons
 */
void SdPlayer::updatePlayerButtonStates()
{
    // Während Dialog/Grafik-Sperre nicht zeichnen: die Zustandsüberwachung in draw() holt den Button-Zustand nach dem Schließen nach
    if (m_uiSuspended)
        return;

    // PLAY
    if (m_playbackState == PlaybackState::PLAYING)
        m_playerButtons[1].setLabelText("^PLAY:32:FD20^"); // orange
    else
        m_playerButtons[1].setLabelText("^PLAY:32:5370^"); // normal

    m_playerButtons[1].drawButton();

    // PAUSE
    if (m_playbackState == PlaybackState::PAUSED)
    {
        // pulsierend
        uint16_t pulsatingColor = getPulsatingColor(0x1082, TFT_WHITE, 3000);

        // Farbe als Hex-String formatieren
        char labelText[32];
        stringutils::safeFormat(labelText, "^PAUSE:32:%04X^", pulsatingColor);
        m_playerButtons[2].setLabelText(labelText);
    }
    else
    {
        m_playerButtons[2].setLabelText("^PAUSE:32:5370^"); // normal
    }

    m_playerButtons[2].drawButton();
}

/**
 * @brief Verarbeitet Tippen und Ziehen auf dem Fortschrittsbalken
 *
 * Eine frische Berührung im Balkenbereich beginnt das Ziehen: Der Balken zeigt die Zielzeit als Vorschau, das Loslassen springt dorthin. Bis zum Loslassen gehört die Berührung dem Balken, auch wenn der Finger den Bereich verlässt.
 */
void SdPlayer::handleProgressTouch()
{
    constexpr int TOUCH_MARGIN_Y = 8; ///< vertikale Toleranz um den Balken in Bildpunkten

    const bool freshTouch = m_touched && !m_progressTouchSeen;
    m_progressTouchSeen = m_touched;

    if (!m_progressScrubbing)
    {
        if (!freshTouch)
            return;
        if (m_playbackState != PlaybackState::PLAYING || m_audioStatus.duration == 0)
            return;
        if (m_touchX < PROGRESS_X || m_touchX >= PROGRESS_X + PROGRESS_W)
            return;
        if (m_touchY < PROGRESS_Y - TOUCH_MARGIN_Y || m_touchY >= PROGRESS_Y + PROGRESS_H + TOUCH_MARGIN_Y)
            return;

        m_progressScrubbing = true;
    }

    if (m_touched)
    {
        // Zielzeit aus der Fingerposition, auf den Balken begrenzt
        int relX = m_touchX - PROGRESS_X;
        if (relX < 0)
            relX = 0;
        if (relX > PROGRESS_W)
            relX = PROGRESS_W;

        const uint32_t target = ((uint64_t)relX * m_audioStatus.duration) / PROGRESS_W;
        if (target != m_scrubTargetSec)
        {
            m_scrubTargetSec = target;
            m_needsProgressRedraw = true;
        }
        return;
    }

    // Loslassen: an die Zielzeit springen
    m_progressScrubbing = false;
    m_needsProgressRedraw = true;

    if (m_callbacks.onSeekOffsetRequested)
    {
        const int offset = (int)m_scrubTargetSec - (int)m_audio->getAudioCurrentTime();
        if (offset != 0)
            m_callbacks.onSeekOffsetRequested(offset);

        // Anzeige sofort auf die Zielzeit setzen, sonst springt der Balken bis zum nächsten Status-Poll auf die alte Position zurück; der Poll korrigiert dann höchstens auf die exakt gelandete Stelle
        m_audioStatus.currentTime = m_scrubTargetSec;
        if (m_audioStatus.duration > 0)
            m_audioStatus.progress = (m_scrubTargetSec * 100) / m_audioStatus.duration;
    }
}

/**
 * @brief Verarbeitet Touch-Eingaben auf den Control-Buttons
 */
void SdPlayer::handleButtons()
{
    // Wenn nichts berührt und keine Buttons gedrückt waren
    if (!m_touched && !m_wasPressed[0] && !m_wasPressed[1] && !m_wasPressed[2] && !m_wasPressed[3])
        return;

    for (int i = 0; i < 4; i++)
    {
        bool isPressed = m_touched && m_controlButtons[i].contains(m_touchX, m_touchY);
        m_controlButtons[i].press(isPressed);

        // Nur ein frisches Aufsetzen macht die Taste scharf, eine hereinwandernde laufende Berührung nicht
        if (isPressed && !m_wasPressed[i] && !m_wasTouching)
        {
            m_controlButtons[i].drawButton(true);
            m_wasPressed[i] = true;
            m_pressTime[i] = millis();
        }

        if (!isPressed && m_wasPressed[i])
        {
            m_controlButtons[i].drawButton(false);
            m_wasPressed[i] = false;

            // Rutscht der Finger von der Taste, ist das ein Abbruch: nur echtes Abheben löst die Aktion aus
            if (m_touched)
                continue;

            // FileExplorer
            if (i == 0)
            {
                FileExplorerConfig config;
                config.title = reinterpret_cast<PGM_P>(STR(SDPlayerKey::FILEEXPLORER_TITLE));
                config.setExtensions({".mp3", ".aac", ".flac", ".m4a", ".ogg"});

                m_explorer->setBounds((m_tft->width() - 550) / 2, (m_tft->height() - 350) / 2, 550, 350);

                m_explorer->start(config, [this](std::vector<FileResult> selectedFiles)
                                  {
                    if (!selectedFiles.empty()) {
                        const bool startsNewPlaylist = m_playlist.empty();
                        int addedCount = 0;
                        int duplicateCount = 0;

                        for (const auto& file : selectedFiles) {
                            if (!isFileInPlaylist(file.path)) {
                                m_playlist.push_back(file);
                                addedCount++;
                                LOG_DIRECT_F("  -> %s", file.path);
                            } else {
                                duplicateCount++;
                            }
                        }

                        if (addedCount > 0) {
                            m_playedTracks.resize(m_playlist.size(), false);
                            LOG_INFO_F("%d Datei(en) zur Playlist hinzugefügt.", addedCount);
                            m_needsPlaylistRedraw = true;
                            m_needsDetailsRedraw = true;

                            // Das sichtbare Tape bleibt beim Befüllen liegen; gewechselt wird beim Leeren (Auswurf) oder von Hand per Tipp auf die Kassette
                        }

                        if (duplicateCount > 0) {
                            LOG_INFO_F("%d Datei(en) bereits in der Playlist (übersprungen).", duplicateCount);
                        }
                    } else {
                        LOG_INFO("FileExplorer ohne Auswahl geschlossen.");
                    } });
            }

            // MessageBox
            if (i == 1)
            {
                m_messageBox->setBounds((m_tft->width() - 300) / 2, (m_tft->height() - 200) / 2, 300, 200);
                m_messageBox->setTitle(reinterpret_cast<PGM_P>(STR(SDPlayerKey::SDPLAYER_MSG_CLEAR_TITLE)));
                m_messageBox->setMessage(reinterpret_cast<PGM_P>(STR(SDPlayerKey::SDPLAYER_MSG_CLEAR_MESSAGE)));
                m_messageBox->setButtonLabels(reinterpret_cast<PGM_P>(STR(CommonKey::COMMON_BTN_CONTINUE)),
                                              reinterpret_cast<PGM_P>(STR(CommonKey::COMMON_BTN_CANCEL)));
                m_messageBox->setTimeout(10000);
                m_messageBox->setCallback([this](bool confirmed)
                                          { onMessageBoxResult(confirmed); });
                m_messageBox->start();
            }

            // Reset
            if (i == 2)
            {
                APP_STATE.setBlockGFX(true);

                m_messageBox->setBounds((m_tft->width() - 300) / 2, (m_tft->height() - 200) / 2, 300, 200);
                m_messageBox->setTitle(reinterpret_cast<PGM_P>(STR(SDPlayerKey::SDPLAYER_MSG_RESET_TITLE)));
                m_messageBox->setMessage(reinterpret_cast<PGM_P>(STR(SDPlayerKey::SDPLAYER_MSG_RESET_MESSAGE)));
                m_messageBox->setButtonLabels(reinterpret_cast<PGM_P>(STR(CommonKey::COMMON_BTN_CONTINUE)),
                                              reinterpret_cast<PGM_P>(STR(CommonKey::COMMON_BTN_CANCEL)));
                m_messageBox->setTimeout(10000);

                // Eigene Lambda für Reset, da onMessageBoxResult() für "leeren" genutzt wird
                m_messageBox->setCallback([this](bool confirmed)
                                          {
                        if (confirmed) {
                            m_resetRequested = true;
                        }
                        APP_STATE.setBlockGFX(false); });
                m_messageBox->start();
            }

            // Sleep Timer / Ausschalten
            if (i == 3)
            {
                APP_STATE.setBlockGFX(true);

                m_sleepDlg->setBounds((m_tft->width() - 300) / 2, (m_tft->height() - 300) / 2, 300, 300);
                m_sleepDlg->setTitle(reinterpret_cast<PGM_P>(STR(SDPlayerKey::SDPLAYER_SLEEP_TITLE)));
                m_sleepDlg->setMessage(reinterpret_cast<PGM_P>(STR(SDPlayerKey::SDPLAYER_SLEEP_MESSAGE)));
                m_sleepDlg->setCancelLabel(reinterpret_cast<PGM_P>(STR(CommonKey::COMMON_BTN_CANCEL)));
                m_sleepDlg->setStopLabel(reinterpret_cast<PGM_P>(STR(CommonKey::COMMON_BTN_STOP)));
                m_sleepDlg->setOptions(m_sleepLabels, m_sleepValues, 6);
                m_sleepDlg->setTimeout(10000);

                m_sleepDlg->setCallback([this](int minutes)
                                        {
                        if (minutes == 0) {
                            // Sofort ausschalten
                            stopPlayerImmediately();
                        } else if (minutes > 0) {
                            // Timer starten
                            m_sleepDlg->startSleepCountdown(minutes, [this]() 
                            { 
                            // Timer ist abgelaufen
                            APP_STATE.setSleepModeTriggered(true);
                            stopPlayerImmediately(); 
                        });
                        }

                        // Bei "Abbruch" oder "Stop"nichts tun, Dialog schließt sich nur
                        APP_STATE.setBlockGFX(false); });

                m_sleepDlg->start();
            }
        }
    }
}

/**
 * @brief Verarbeitet Touch-Eingaben auf den Player-Buttons
 */
void SdPlayer::handlePlayerButtons()
{
    // Wenn nichts berührt und keine Buttons gedrückt waren
    if (!m_touched && !m_wasPlayerPressed[0] && !m_wasPlayerPressed[1] && !m_wasPlayerPressed[2] && !m_wasPlayerPressed[3] && !m_wasPlayerPressed[4])
        return;

    for (int i = 0; i < 5; i++)
    {
        bool isPressed = m_touched && m_playerButtons[i].contains(m_touchX, m_touchY);
        m_playerButtons[i].press(isPressed);

        // Nur ein frisches Aufsetzen macht die Taste scharf, eine hereinwandernde laufende Berührung nicht
        if (isPressed && !m_wasPlayerPressed[i] && !m_wasTouching)
        {
            m_playerButtons[i].drawButton(true);
            m_wasPlayerPressed[i] = true;
            m_playerPressTime[i] = millis();

            if (i == 0 || i == 4)
                m_seekHoldTriggered = false;
        }

        // Halten einer Sprungtaste spult im Titel
        if (isPressed && m_wasPlayerPressed[i] && (i == 0 || i == 4))
        {
            constexpr uint32_t SEEK_HOLD_MS = 600;   ///< Haltezeit bis zum Spulbeginn
            constexpr uint32_t SEEK_REPEAT_MS = 400; ///< Abstand der Spulschritte
            constexpr int SEEK_STEP_S = 10;          ///< Sprungweite je Schritt in Sekunden
            constexpr uint32_t SEEK_END_GUARD_S = 3; ///< Restzeit zum Titelende, ab der nicht weiter vorgespult wird

            const uint32_t now = millis();
            if (now - m_playerPressTime[i] >= SEEK_HOLD_MS)
            {
                if (!m_seekHoldTriggered)
                {
                    m_seekHoldTriggered = true;
                    m_lastSeekStepMs = 0;
                }

                if (m_playbackState == PlaybackState::PLAYING && now - m_lastSeekStepMs >= SEEK_REPEAT_MS)
                {
                    m_lastSeekStepMs = now;
                    const bool forward = (i == 4);
                    // Vorspulen endet kurz vor dem Titelende, der Titelwechsel bleibt der EOF-Verarbeitung überlassen
                    const bool nearEnd = forward && m_audioStatus.duration > 0 && m_audioStatus.currentTime + SEEK_STEP_S + SEEK_END_GUARD_S >= m_audioStatus.duration;
                    if (!nearEnd && m_callbacks.onSeekOffsetRequested)
                        m_callbacks.onSeekOffsetRequested(forward ? SEEK_STEP_S : -SEEK_STEP_S);
                }
            }
        }

        if (!isPressed && m_wasPlayerPressed[i])
        {
            m_playerButtons[i].drawButton(false);
            m_wasPlayerPressed[i] = false;

            // Rutscht der Finger von der Taste, ist das ein Abbruch: nur echtes Abheben löst die Aktion aus
            if (m_touched)
            {
                if (i == 0 || i == 4)
                    m_seekHoldTriggered = false;

                continue;
            }

            // Nach ausgelöstem Spulen ist das Loslassen der Taste verbraucht
            bool seekHoldConsumed = false;
            if (i == 0 || i == 4)
            {
                seekHoldConsumed = m_seekHoldTriggered;
                m_seekHoldTriggered = false;
            }

            // Läuft Wiedergabe oder Pause, spielt ein Titelsprung sofort weiter; im Stopp merkt er den Titel nur vor
            const bool activePlayback = (m_playbackState == PlaybackState::PLAYING || m_playbackState == PlaybackState::PAUSED);

            if (i == 0 && !seekHoldConsumed)
            { // Titelanfang oder voriger Titel
                constexpr uint32_t RESTART_THRESHOLD_S = 3;
                if (activePlayback && m_audioStatus.currentTime > RESTART_THRESHOLD_S)
                {
                    // Erster Druck springt an den Titelanfang, ein schneller zweiter Druck landet damit im Zweig darunter
                    playCurrentTrack();
                    LOG_INFO_F("Track %d neu gestartet", m_currentTrackIndex + 1);
                }
                else if (m_currentTrackIndex > 0)
                {
                    // Nach dem Playlist-Ende beginnt auch die Vormerkung einen frischen Durchlauf, sonst bliebe sie unsichtbar und Play spränge stattdessen auf Titel 1
                    if (m_playlistFinished)
                        resetPlayedStatus();

                    m_currentTrackIndex--;
                    m_needsPlaylistRedraw = true;
                    m_autoScrollRequested = true;

                    if (activePlayback)
                    {
                        playCurrentTrack();
                        LOG_INFO_F("Spiele Track: %d", m_currentTrackIndex + 1);
                    }
                    else
                    {
                        requestBackendStop();
                        resetRuntimePlaybackState();
                        m_playbackState = PlaybackState::STOPPED;
                        updatePlayerButtonStates();
                        LOG_INFO_F("Track gewechselt zu: %d (bereit)", m_currentTrackIndex + 1);
                    }
                }
            }

            if (i == 1)
            { // Abspielen
                if (!m_playlist.empty())
                {
                    // Wenn pausiert: nur Resume
                    if (m_playbackState == PlaybackState::PAUSED)
                    {
                        m_playbackState = PlaybackState::PLAYING;

                        if (m_callbacks.onPauseResumeRequested)
                        {
                            m_callbacks.onPauseResumeRequested();
                            LOG_INFO("Resume von Pause");
                        }

                        updatePlayerButtonStates();
                    }
                    // Wenn bereits spielend oder gerade startend: nichts tun (der gecachte Audio-Status hinkt bis zu 1 s hinterher, ein Doppeldruck startete den Titel sonst von vorn)
                    else if (m_playbackState == PlaybackState::PLAYING || m_waitingForPlaybackStart)
                    {
                        LOG_INFO("Track läuft bereits");
                    }
                    // Wenn gestoppt: neuen Track starten
                    else
                    {
                        if (m_playlistFinished)
                        {
                            m_currentTrackIndex = 0;
                            resetPlayedStatus();
                            LOG_INFO("Playlist wird neu gestartet");
                        }

                        playCurrentTrack();
                        LOG_INFO_F("Spiele Track: %d", m_currentTrackIndex + 1);
                    }
                }
            }

            if (i == 2)
            { // Pause
                // Im Stopp gibt es nichts zu pausieren: der Callback wuerde in der Library das Laufkennzeichen ohne offene Datei kippen, der naechste Status-Poll saehe ein laufendes Playback und die Play-Taste waere tot
                if (m_playbackState == PlaybackState::PLAYING || m_playbackState == PlaybackState::PAUSED)
                {
                    // Toggle zwischen PLAYING und PAUSED
                    if (m_playbackState == PlaybackState::PLAYING)
                        m_playbackState = PlaybackState::PAUSED;
                    else
                        m_playbackState = PlaybackState::PLAYING;

                    // Callback an Audio-Task
                    if (m_callbacks.onPauseResumeRequested)
                    {
                        m_callbacks.onPauseResumeRequested();
                        LOG_INFO_F("Pause/Resume - neuer State: %d", (int)m_playbackState);
                    }

                    // Button-States sofort aktualisieren
                    updatePlayerButtonStates();
                }
            }

            if (i == 3)
            { // Stop
                requestBackendStop();
                resetRuntimePlaybackState();
                clearDisplayedMetadata(false);

                m_playbackState = PlaybackState::STOPPED;
                updatePlayerButtonStates();

                LOG_INFO("Stop");
            }

            if (i == 4 && !seekHoldConsumed)
            { // nächster Titel
                // Der Leer-Check verhindert den size_t-Unterlauf von size() - 1, der den Index bei leerer Playlist grenzenlos wachsen ließe
                if (!m_playlist.empty() && m_currentTrackIndex < (int)m_playlist.size() - 1)
                {
                    m_currentTrackIndex++;
                    m_needsPlaylistRedraw = true;
                    m_autoScrollRequested = true;

                    if (activePlayback)
                    {
                        playCurrentTrack();
                        LOG_INFO_F("Spiele Track: %d", m_currentTrackIndex + 1);
                    }
                    else
                    {
                        requestBackendStop();
                        resetRuntimePlaybackState();
                        m_playbackState = PlaybackState::STOPPED;
                        updatePlayerButtonStates();
                        LOG_INFO_F("Track gewechselt zu: %d (bereit)", m_currentTrackIndex + 1);
                    }
                }
            }
        }
    }
}

/**
 * @brief Stoppt den Player sofort und verlässt das Modul
 */
void SdPlayer::stopPlayerImmediately()
{
    // Player stoppen
    requestBackendStop();

    // SD-Playback beendet -> Visualizer sperren
    APP_STATE.setStationSetDone(false);

    // Modul beenden
    if (m_callbacks.onExitRequested != nullptr)
        m_callbacks.onExitRequested();
}

/**
 * @brief Callback für MessageBox-Ergebnis
 */
void SdPlayer::onMessageBoxResult(bool confirmed)
{
    if (confirmed)
    {
        requestBackendStop();

        // SD-Playback beendet -> Visualizer sperren
        APP_STATE.setStationSetDone(false);
        clearPlaylist();
    }
}

// ===================================================================
// Audio-Status Update
// ===================================================================

/**
 * @brief Rundet eine Bitrate auf die nächste Standard-MP3-Bitrate
 * @param bitrate Gemessene Bitrate in bps
 * @return Nächste Standard-Bitrate in bps
 */
uint32_t SdPlayer::snapToStandardBitrate(uint32_t bitrate)
{
    static const uint32_t standardBitrates[] = {
        32000,  // 32 kbps
        40000,  // 40 kbps
        48000,  // 48 kbps
        56000,  // 56 kbps
        64000,  // 64 kbps
        80000,  // 80 kbps
        96000,  // 96 kbps
        112000, // 112 kbps
        128000, // 128 kbps
        160000, // 160 kbps
        192000, // 192 kbps
        224000, // 224 kbps
        256000, // 256 kbps
        320000  // 320 kbps
    };

    constexpr size_t count = sizeof(standardBitrates) / sizeof(standardBitrates[0]);

    // Nächstgelegene Standard-Bitrate finden
    uint32_t closest = standardBitrates[0];
    uint32_t minDiff = UINT32_MAX;

    for (size_t i = 0; i < count; ++i)
    {
        uint32_t diff = (bitrate > standardBitrates[i])
                            ? (bitrate - standardBitrates[i])
                            : (standardBitrates[i] - bitrate);

        if (diff < minDiff)
        {
            minDiff = diff;
            closest = standardBitrates[i];
        }
    }

    return closest;
}

/**
 * @brief Aktualisiert den Audio-Status durch Polling der Audio-Instanz
 */
void SdPlayer::updateAudioStatus()
{
    // Aktuellen Wiedergabestatus abfragen
    const bool running = m_audio->isRunning();
    m_audioStatus.isPlaying = running;

    // Basiswerte vom Decoder holen
    m_audioStatus.fileSize = m_audio->getFileSize();
    m_audioStatus.filePos = m_audio->getAudioFilePosition();

    // Wenn Audio läuft -> Werte aktualisieren
    if (running)
    {
        // State auf PLAYING setzen, sobald Audio tatsächlich läuft
        if (m_playbackState != PlaybackState::PLAYING && !m_playlistFinished)
        {
            m_playbackState = PlaybackState::PLAYING;
            updatePlayerButtonStates();
            LOG_INFO("Playback bestätigt - State auf PLAYING gesetzt");
        }

        // Playback-Start-Timeout zurücksetzen
        m_waitingForPlaybackStart = false;

        // CurrentTime aktualisieren
        m_audioStatus.currentTime = m_audio->getAudioCurrentTime();

        // Duration mit Library-Methode und Fallback aktualisieren
        if (m_audioStatus.duration == 0)
        {
            // 1. Priorität: Duration von Library holen (nutzt Header-Info oder VBR-Schätzung)
            uint32_t libDuration = m_audio->getAudioFileDuration();

            if (libDuration > 0 && libDuration >= 10 && libDuration <= 7200)
            {
                // Library liefert gültige Duration
                m_audioStatus.duration = libDuration;
                m_cachedDuration = libDuration;

                // Keine echte VBR-Erkennung: jede MP3 gilt als VBR-Kandidat, die Dauer wird während der Wiedergabe nachgeführt
                if (m_currentAudioFile[0] != '\0')
                {
                    const char *ext = strrchr(m_currentAudioFile, '.');
                    if (ext && strcasecmp(ext + 1, "mp3") == 0)
                        m_isVbrTrack = true; // Wird während Playback verfeinert
                }

                LOG_INFO_F("Duration von Library: %u Sekunden (VBR: %s)",
                           libDuration, m_isVbrTrack ? "ja" : "nein");
            }
            else
            {
                // 2. Priorität (Fallback): Duration aus Bitrate + Dateigröße berechnen
                uint32_t rawBitrate = m_audio->getBitRate();
                uint64_t fileSize = m_audioStatus.fileSize;

                // Nur fortfahren wenn Bitrate realistisch ist
                if (rawBitrate > 10000 && fileSize > 0)
                {
                    uint32_t usedBitrate = rawBitrate;

                    // Dateityp prüfen: verlustfreie Formate (FLAC/WAV/ALAC/APE) verwenden die rohe Bitrate, alle anderen werden auf Standard-Bitraten gerundet
                    bool isLossless = false;
                    if (m_currentAudioFile[0] != '\0')
                    {
                        const char *ext = strrchr(m_currentAudioFile, '.');
                        if (ext)
                        {
                            ext++; // Skip '.'
                            if (strcasecmp(ext, "flac") == 0 ||
                                strcasecmp(ext, "wav") == 0 ||
                                strcasecmp(ext, "alac") == 0 ||
                                strcasecmp(ext, "ape") == 0)
                            {
                                isLossless = true;
                            }
                        }
                    }

                    // Bei verlustbehafteten Formaten (MP3) Bitrate auf Standard runden
                    if (!isLossless)
                        usedBitrate = snapToStandardBitrate(rawBitrate);

                    // Duration = (FileSize * 8 Bits) / Bitrate
                    uint64_t durationCalc = (fileSize * 8) / usedBitrate;
                    uint32_t calculatedDuration = static_cast<uint32_t>(durationCalc);

                    // Sanity Check: 10 Sekunden bis 2 Stunden ist realistisch für Musik
                    if (calculatedDuration >= 10 && calculatedDuration <= 7200)
                    {
                        m_audioStatus.duration = calculatedDuration;
                        m_cachedDuration = calculatedDuration;

                        LOG_INFO_F("Duration berechnet (Fallback): %u Sekunden (Bitrate: %u bps%s, Dateigröße: %llu Bytes)",
                                   calculatedDuration, usedBitrate,
                                   isLossless ? " [verlustfrei]" : " [gerundet]",
                                   fileSize);
                    }
                    else
                    {
                        // Unrealistischer Wert, später erneut versuchen
                        LOG_WARNING_F("Unrealistische Duration: %u Sekunden (Bitrate: %u bps)", calculatedDuration, usedBitrate);
                    }
                }
            }
        }

        // Bei VBR-Tracks: Duration während Playback verfeinern
        if (m_isVbrTrack)
            refineDurationForVbr();

        // Progress aus currentTime/duration berechnen
        if (m_audioStatus.duration > 0 && m_audioStatus.currentTime <= m_audioStatus.duration)
            m_audioStatus.progress = (m_audioStatus.currentTime * 100) / m_audioStatus.duration;
        else if (m_audioStatus.duration > 0)
            m_audioStatus.progress = 100;
        else
            m_audioStatus.progress = 0;
    }
    else
    {
        // Warten auf Playback-Start nach onPlayFileRequested
        if (m_waitingForPlaybackStart)
        {
            // Eine Pause im Startfenster belegt den erfolgten Start: pauseResume wirkt nur auf eine offene Datei, der Poll sähe sonst nie ein laufendes Audio und der Timeout bräche den korrekt pausierten Titel ab
            if (m_playbackState == PlaybackState::PAUSED)
            {
                m_waitingForPlaybackStart = false;
                return;
            }

            uint32_t elapsed = millis() - m_playbackStartWaitMs;
            if (elapsed > PLAYBACK_START_TIMEOUT_MS)
            {
                // Timeout überschritten -> Playback-Start fehlgeschlagen
                m_waitingForPlaybackStart = false;
                m_playbackState = PlaybackState::STOPPED;
                updatePlayerButtonStates();

                LOG_ERROR_F("Playback-Start fehlgeschlagen (Timeout nach %u ms)", elapsed);

                // Fehlerhaft gestarteten Track nicht als abgespielt markieren
                if (!m_playlistFinished)
                    finalizeCurrentTrack(TrackFinishType::ABORTED, false);
            }
            // Sonst weiter warten (Audio startet evtl. noch)
            return;
        }

        // EOF-Erkennung
        if (m_playbackState == PlaybackState::PLAYING && !m_playlistFinished)
        {
            LOG_INFO("Audio gestoppt -> Fallback EOF-Trigger");
            onAudioTrackFinished();
        }
    }

    // Progress-Bar nur bei tatsächlicher Änderung neu zeichnen; die Laufzeitanzeige zählt sekündlich, der Prozentwert allein spränge bei langen Titeln
    if (m_audioStatus.progress != m_lastDrawnProgress || m_audioStatus.currentTime != m_lastDrawnTimeSec)
        m_needsProgressRedraw = true;
}

// ===================================================================
// Uhren
// ===================================================================

/**
 * @brief Analoge Uhr mit Timer
 */
void SdPlayer::drawAnalogClock(bool init, int32_t x, int32_t y, int remainingMinutes, int remainingSeconds)
{
    // Sprite-Initialisierung
    if (init)
    {
        if (!m_tft->safeCreateSprite(m_sprRetroAnalogClock, 70, 70))
        {
            LOG_ERROR("Konnte m_sprRetroAnalogClock Sprite nicht erstellen!");
            return;
        }

        m_sprRetroAnalogClock->setFont(&fonts::DejaVu9);
        m_lastDrawnSecond = -1;

        return;
    }

    // Rendering an Clocks-Klasse delegieren
    Clocks::drawAnalogClock(m_sprRetroAnalogClock, m_rtc, m_lastDrawnSecond, x, y, remainingMinutes, remainingSeconds);
}

/**
 * @brief Digitale Uhr mit Timer
 */
void SdPlayer::drawDigitalClock(bool init, int32_t x, int32_t y, int remainingMinutes, int remainingSeconds)
{
    // Sprite-Initialisierung
    if (init)
    {
        if (!m_tft->safeCreateSprite(m_sprRetroDigitalClock, 150, 45))
        {
            LOG_ERROR("Konnte m_sprRetroDigitalClock Sprite nicht erstellen!");
            return;
        }

        m_sprRetroDigitalClock->clear(TFT_BLACK);
        m_lastDrawnSecond = -1;

        return;
    }

    // Rendering an Clocks-Klasse delegieren
    Clocks::drawDigitalClock(m_sprRetroDigitalClock, m_rtc, m_lastDrawnSecond, x, y, remainingMinutes, remainingSeconds);
}

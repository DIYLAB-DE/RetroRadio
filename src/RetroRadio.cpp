#include "RetroRadio.h"

#include <time.h>

#include "Clocks.h"              // Uhren
#include "I18n.h"                // Lokalisierte Wochentag-/Monatsnamen
#include "Strings_retroradio.h"  // Modul-eigene Texte (RetroRadioKey)
#include "Logger.h"              // LOG_* Makros
#include "Routes.h"      // isStationsReloadPending()
#include "StringUtils.h" // String-Helper
#include "Theme.h"       // Farben/Konstanten

// Fonts
#include "fonts/ComfortaaBold10.h"
#include "fonts/ComfortaaBold14.h"
#include "fonts/ComfortaaBold16.h"
#include "fonts/ComfortaaBold18.h"
#include "fonts/LeagueGothicCond34.h"

// Images
#include "images/logo_90.h"
#include "IconShapes.h"
#include "images/retro_vu_meter.h"

// ===================================================================
// Interne Kennwerte
// ===================================================================

/**
 * @brief Namensraum für interne Implementierungsdetails
 */
namespace
{
    // Farbe des gezeichneten Stummschaltsymbols, an der früheren Bitmap abgelesen
    constexpr uint16_t ICON_MUTE_COLOR = 0x9CF3;      ///< Strichfarbe des Stummschaltsymbols
    constexpr uint16_t ICON_MUTE_BACKGROUND = 0x0000; ///< Freie Fläche des Stummschaltsymbols

    constexpr unsigned long MAGIC_EYE_MIN_REDRAW_MS = 8;  ///< Mindestpause zwischen zwei Aufbauten des magischen Auges, begrenzt es auf 125 Bilder je Sekunde; die EM80 rechnet dabei rund die doppelte Fläche der runden Bauformen
    constexpr unsigned long MAGIC_EYE_LONG_PRESS_MS = 800; ///< Haltedauer, ab der ein Druck auf das Auge die Darstellung umschaltet
}

// ===================================================================
// Button-Definitionen
// ===================================================================

// Statische Member für SleepTimer
const char *RetroRadio::m_sleepLabels[] = {"5", "15", "25", "45", "60", "^POWER:32:D206^"};
int RetroRadio::m_sleepValues[] = {5, 15, 25, 45, 60, 0};

// Button-Definitionen: Favoriten, Zurück, Vor, Reset, Sleep Timer
const Buttons RetroRadio::m_buttonDefs[] = {
    {nullptr, 513, 435, 45, 45, 4, 0x39E8, TFT_BLACK, TFT_DARKGREY, "^STAR:32:FC60^", nullptr}, // Favoriten
    {nullptr, 568, 435, 45, 45, 4, 0x39E8, TFT_BLACK, TFT_DARKGREY, "^BCKW:32:5370^", nullptr}, // Zurück
    {nullptr, 623, 435, 45, 45, 4, 0x39E8, TFT_BLACK, TFT_DARKGREY, "^FORW:32:5370^", nullptr}, // Vor
    {nullptr, 678, 435, 67, 45, 4, 0x39E8, TFT_BLACK, TFT_DARKGREY, "reset", nullptr},          // Reset
    {nullptr, 755, 435, 45, 45, 4, 0x39E8, TFT_BLACK, TFT_DARKGREY, "^POWER:32:D206^", nullptr} // Power/SleepTimer
};

// ===================================================================
// Konstruktor, Destruktor, Init, Draw
// ===================================================================

/**
 * @brief Konstruktor
 */
RetroRadio::RetroRadio()
{
}

/**
 * @brief Destruktor
 */
RetroRadio::~RetroRadio()
{
    // Sicherstellen, dass bei der Zerstörung kein Playback weiterläuft
    if (m_callbacks.requestStopPlayback)
        m_callbacks.requestStopPlayback();

    APP_STATE.setStationSetDone(false);

    // Visualizer aufräumen
    m_visualizer.cleanup();

    // UI-Sprites und Fonts aufräumen
    if (m_tft)
    {
        m_tft->safeDeleteSprite(m_sprRetroDiv);
        m_tft->safeDeleteSprite(m_sprRetroScale);
        m_tft->safeDeleteSprite(m_sprRetroNeedle);
        m_tft->safeDeleteSprite(m_sprRetroStereo);
        m_tft->safeDeleteSprite(m_sprRetroFooter);
        m_tft->safeDeleteSprite(m_sprRetroKnob);
        m_tft->safeDeleteSprite(m_sprRetroTracks);
        m_tft->safeDeleteSprite(m_sprMagicEye);
        m_tft->safeDeleteSprite(m_sprRetroAnalogClock);
        m_tft->safeDeleteSprite(m_sprRetroDigitalClock);

        m_tft->unloadFont();
        m_tft = nullptr;
    }

    m_stationList = nullptr;
}

/**
 * @brief Gibt den Modul-Typ zurück
 */
AppModule RetroRadio::type() const
{
    return AppModule::RETRO_RADIO;
}

/**
 * @brief Initialisierung
 */
bool RetroRadio::init(LGFX &tft, DS1307_RTC &rtc, StationManager &stationManager, const RetroRadioCallbacks &callbacks)
{
    // Referenzen sind per Definition nie null - Aufrufer ist verantwortlich für gültige Objekte
    m_tft = &tft;
    m_rtc = &rtc;
    m_stationManager = &stationManager;
    m_callbacks = callbacks;

    // Display-Variablen initialisieren
    memset(m_finalStation, 0, sizeof(m_finalStation));
    memset(m_finalBitrate, 0, sizeof(m_finalBitrate));
    memset(m_showstation, 0, sizeof(m_showstation));
    memset(m_showstreamtitle, 0, sizeof(m_showstreamtitle));

    // Touch-Variablen initialisieren
    m_touchX = 0;
    m_touchY = 0;
    m_touched = false;
    m_wasTouching = false;

    // Nadel-Variablen initialisieren
    memset(m_col0, 0, sizeof(m_col0));
    memset(m_col5, 0, sizeof(m_col5));
    m_handCounter = -1;
    m_lastMoveTime = 0;

    // Footer initialisieren
    memset(m_truncatedTitle, 0, sizeof(m_truncatedTitle));

    // Lautstärke-Regler initialisieren
    m_totalDegrees = 0.0f;
    m_lastAngle = 0.0f;
    m_prevVol = 0;

    // Equalizer-Regler initialisieren
    m_balance_prev = -128;
    m_gainLowPass_prev = -128;
    m_gainBandPass_prev = -128;
    m_gainHighPass_prev = -128;

    // Uhr initialisieren
    m_lastDrawnSecond = -1;

    // Sender-Auswahl initialisieren
    m_isTouched = false;

    // Buttons initialisieren
    for (int i = 0; i < 5; i++)
    {
        m_wasPressed[i] = false;
        m_longPressed[i] = false;
        m_pressTime[i] = 0;
    }

    // Diverse Variablen initialisieren
    m_animating = false;
    m_stopSent = false;
    m_lastStartedStation = 0xFF;
    m_lastProcessedStation = APP_STATE.getCurrentRetroStation();
    m_resetRequested = false;

    // Dialoge erstellen
    m_sleepDlg = std::make_unique<SleepTimer>(m_tft);
    m_msg = std::make_unique<MessageBox>(m_tft);
    m_presetDlg = std::make_unique<PresetManager>(m_tft);

    // Display mit schwarz leeren
    m_tft->clear(TFT_BLACK);

    // Senderliste laden und holen
    if (m_stationManager->loadStations())
    {
        m_stationList = m_stationManager->getStations();
        m_stationCounter = constrain(m_stationManager->getStationCount(), 0, RRConfig::Stations::MAX_COUNT);

        // Buttons erstellen und zeichnen
        for (int i = 0; i < 5; i++)
        {
            m_btn[i].initButton(
                m_tft,                                     // Displayobjekt
                m_buttonDefs[i].x + m_buttonDefs[i].w / 2, // X-Position
                m_buttonDefs[i].y + m_buttonDefs[i].h / 2, // Y-Position
                m_buttonDefs[i].w,                         // Breite
                m_buttonDefs[i].h,                         // Höhe
                m_buttonDefs[i].r,                         // Eckenradius
                m_buttonDefs[i].outlinecolor,              // Rahmenfarbe
                m_buttonDefs[i].fillcolor,                 // Füllfarbe
                m_buttonDefs[i].textcolor,                 // Textfarbe
                m_buttonDefs[i].label,                     // Text
                m_buttonDefs[i].icon);                     // Icon

            m_btn[i].setFont(LeagueGothicCond34);
            m_btn[i].drawButton();
        }

        // Sprite für Skalenzeiger erstellen
        if (!m_tft->safeCreateSprite(m_sprRetroNeedle, RRLayout::Scale::NEEDLE_WIDTH, RRLayout::Scale::HEIGHT, false))
        {
            LOG_ERROR("Konnte m_sprRetroNeedle Sprite nicht erstellen!");
            return false;
        }

        m_sprRetroNeedle->clear(TFT_RED);

        // Skala
        drawScale();

        // magisches Auge
        drawMagicEye(true, 0, 0, 0);

        // analoge Uhr
        drawAnalogClock(true, 0, 0, 0, 0);

        // digitale Uhr
        drawDigitalClock(true, 0, 0, 0, 0);

        // Footer
        drawFooterText(true);

        // Stereo-Indikator Sprite einmalig erstellen (wird in updateStereoIndicator wiederverwendet)
        if (!m_tft->safeCreateSprite(m_sprRetroStereo, RRLayout::Footer::STEREO_INDICATOR_W, RRLayout::Footer::STEREO_INDICATOR_H))
        {
            LOG_ERROR("Konnte m_sprRetroStereo Sprite nicht erstellen!");
            return false;
        }

        // Datum im Footer aus der Systemzeit bilden. Bei ungültiger Zeit steht der Platzhalter, bis NTP oder RTC die Zeit setzen.
        char buffer[64];
        if (DS1307_RTC::isSystemTimeValid())
        {
            time_t nowEpoch = time(nullptr);
            struct tm nowLocal;
            localtime_r(&nowEpoch, &nowLocal);
            char monBuf[16];
            I18n::copyMonth(monBuf, sizeof(monBuf), nowLocal.tm_mon, /*longForm=*/true);
            // COMMON_DT_FORMAT_DATE verwendet {1}=Monat (%s), {2}=Tag (%d), {4}=Jahr (%d)
            STRF(buffer, CommonKey::COMMON_DT_FORMAT_DATE,
                 monBuf,
                 nowLocal.tm_mday,
                 nowLocal.tm_year + 1900);
        }
        else
        {
            STRF(buffer, CommonKey::COMMON_DT_PLACEHOLDER_DATE);
        }
        drawFooterText(false, " ", buffer, " ", TFT_CYAN, TFT_CYAN, TFT_CYAN, TFT_CYAN, " - ");

        // Lautstärke-Regler; skipCallbacks, weil das Anzeigen beim Moduleintritt den geteilten AppState nicht verändern darf und die Audio-Lautstärke bereits gesetzt ist
        drawKnob(true, APP_STATE.getIsMuted() ? 0 : APP_STATE.getCurrentVolume(), /*skipCallbacks=*/true);

        // Balance, Tiefen, Mitten, Höhen Regler
        drawTracks(true,
                   APP_STATE.getCurrentBalance(),
                   APP_STATE.getCurrentGainLowPass(),
                   APP_STATE.getCurrentGainBandPass(),
                   APP_STATE.getCurrentGainHighPass(),
                   /*skipCallbacks=*/true);

        // Visualizer initialisieren
        if (!m_visualizer.init(*m_tft, *m_rtc))
        {
            LOG_ERROR("Visualizer-Initialisierung fehlgeschlagen!");
            return false;
        }

        // Dem Visualizer das aktive Modul mitteilen
        m_visualizer.setActiveModule(AppModule::RETRO_RADIO);

        // Visualizer initialen Modus setzen und zeichnen
        m_visualizer.setMode(static_cast<VisMode>(APP_STATE.getVisMode()));
        m_visualizer.draw(true);

        m_initialized = true;
        LOG_SUCCESS("RetroRadio erfolgreich initialisiert.");

        return true;
    }
    else
    {
        m_initialized = false;
        LOG_ERROR("Fehler beim Laden der Senderliste!");

        return false;
    }
}

/**
 * @brief Hauptschleife für die Darstellung
 */
void RetroRadio::draw()
{
    if (!m_initialized)
        return;

    // Sprachwechsel: alle lokalisierten Labels neu rendern
    if (I18n::consumeLanguageChanged())
        redrawAll();

    // Dialoge zuerst prüfen und mit return behandeln
    if (m_presetDlg->isActive())
    {
        m_presetDlg->update();
        return;
    }

    // MessageBox: seit API-Änderung kann nach OK ein "ConfirmationPending" Zustand auftreten, in dem isActive() bereits false ist. Daher auch isBusy() berücksichtigen und die Confirmation explizit abschließen.
    if (m_msg && (m_msg->isActive() || m_msg->isBusy()))
    {
        m_msg->update();

        // Completion-Check für einfache Dialoge
        if (m_msg->isBusy())
        {
            m_msg->completeConfirmation(true);
            return;
        }

        return;
    }

    // SleepTimer immer updaten
    m_sleepDlg->update();

    // Wenn der Dialog aktiv ist, den Rest der UI nicht zeichnen
    if (m_sleepDlg->isActive())
        return;

    // GFX-Status prüfen
    const bool gfxBlocked = APP_STATE.getBlockGFX();

    // Stations-Reload per WebUI prüfen
    if (!gfxBlocked && isStationsReloadPending())
    {
        clearStationsReloadPending();
        reloadStations();
    }

    // Touch nur wenn nicht blockiert; der Vorframe-Zustand trägt die Flankenerkennung der Buttons
    if (!gfxBlocked)
    {
        m_wasTouching = m_touched;
        m_touched = m_tft->getTouch(&m_touchX, &m_touchY);
        m_visualizer.handleTouch(m_touchX, m_touchY, m_touched);
        handleMagicEyeTouch();
        getPressedRetroButton();
        getPressedStation();
    }
    else
    {
        // Touch-State explizit zurücksetzen wenn blockiert
        m_touched = false;
        m_wasTouching = false;
    }

    // Lautstärke-Regler: Externe Änderungen erkennen (z.B. Web-Fernsteuerung)
    const uint8_t currentVol = APP_STATE.getCurrentVolume();
    const uint8_t displayVol = APP_STATE.getIsMuted() ? 0 : currentVol;
    const int expectedKnobVol = constrain((int)(m_totalDegrees / 36.0f), 0, 100);

    if (m_knobTouchState == KnobTouchState::IDLE && expectedKnobVol != displayVol)
    {
        // Externe Änderung -> Knob neu zeichnen ohne Callbacks (Audio ist bereits gesetzt)
        drawKnob(true, displayVol, true);
    }
    else
    {
        drawKnob(false, currentVol);
    }

    // Equalizer-Regler: Externe Änderungen erkennen
    const int8_t curBal = APP_STATE.getCurrentBalance();
    const int8_t curLow = APP_STATE.getCurrentGainLowPass();
    const int8_t curMid = APP_STATE.getCurrentGainBandPass();
    const int8_t curHigh = APP_STATE.getCurrentGainHighPass();

    // Pixel-Positionen für Änderungserkennung berechnen
    const int8_t balPixel = map(constrain(curBal, -16, 16), -16, 16, 104, 24);
    auto mapDbPx = [](int8_t db) -> int8_t
    {
        db = constrain(db, -12, 12);
        return (db <= 0) ? map(db, -12, 0, 104, 64) : map(db, 0, 12, 64, 24);
    };

    const int8_t lowPixel = mapDbPx(curLow);
    const int8_t midPixel = mapDbPx(curMid);
    const int8_t highPixel = mapDbPx(curHigh);

    const bool tracksChanged = !m_touched && (balPixel != m_balance_prev || lowPixel != m_gainLowPass_prev || midPixel != m_gainBandPass_prev || highPixel != m_gainHighPass_prev);

    if (tracksChanged)
    {
        // Externe Änderung -> Tracks neu zeichnen ohne Callbacks
        drawTracks(true, curBal, curLow, curMid, curHigh, true);
    }
    else
    {
        drawTracks(false, curBal, curLow, curMid, curHigh);
    }

    // Magisches Auge nur bei Wertänderung aufbauen; der Stilvergleich fängt einen Wechsel ab, bei dem sich sonst kein Eingangswert ändert. Die Mindestpause begrenzt die Bildrate, weil beim Einlaufen auf einen Sender jeder Nadelschritt einen neuen Wert liefert.
    const int32_t magicEyeValue = calculateMagicEyeValue();
    const int16_t magicEyeStyle = static_cast<int16_t>(APP_STATE.getMagicEyeStyle());
    const bool magicEyeChanged = (magicEyeValue != m_lastMagicEyeValue || magicEyeStyle != m_lastMagicEyeStyle);
    const bool magicEyeThrottled = (millis() - m_magicEyeLastDraw < MAGIC_EYE_MIN_REDRAW_MS);

    if (magicEyeChanged && !magicEyeThrottled)
    {
        m_lastMagicEyeStyle = magicEyeStyle;
        m_magicEyeLastDraw = millis();
        drawMagicEye(false, RRLayout::MagicEye::X, RRLayout::MagicEye::Y, magicEyeValue);
        m_lastMagicEyeValue = magicEyeValue;
    }

    // Uhren aktualisieren
    if (m_sleepDlg->isTimerActive())
    {
        int remaining = 0, seconds = 0;
        m_sleepDlg->getRemainingTime(remaining, seconds);
        (m_visualizer.getMode() == VisMode::VIS_GONIO)
            ? drawAnalogClock(false, RRLayout::Clock::ANALOG_X, RRLayout::Clock::ANALOG_Y, remaining, seconds)
            : drawDigitalClock(false, RRLayout::Clock::DIGITAL_X, RRLayout::Clock::DIGITAL_Y, remaining, seconds);
    }
    else
    {
        (m_visualizer.getMode() == VisMode::VIS_GONIO)
            ? drawAnalogClock(false, RRLayout::Clock::ANALOG_X, RRLayout::Clock::ANALOG_Y, -1, 0)
            : drawDigitalClock(false, RRLayout::Clock::DIGITAL_X, RRLayout::Clock::DIGITAL_Y, -1, 0);
    }

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

    // Audio-Event-Queue verarbeiten (Metadaten)
    bool footerNeedsUpdate = false;
    {
        // Senderwechsel erkennen (Station-ID hat sich geändert)
        uint8_t currentStation = APP_STATE.getCurrentRetroStation();
        if (m_lastProcessedStation != currentStation)
        {
            // Gepufferte Events des alten Senders verwerfen, sie würden sonst dem neuen zugeordnet
            AudioManagerEventType staleType;
            while (g_audioManager.dequeueEvent(staleType, m_eventText, sizeof(m_eventText)))
                ;

            // Senderwechsel -> alte Metadaten löschen
            m_showstreamtitle[0] = '\0';
            m_finalBitrate[0] = '\0';
            m_finalStation[0] = '\0';
            m_lastProcessedStation = currentStation;
            footerNeedsUpdate = true; // Sofort UI aktualisieren
        }

        AudioManagerEventType eventType;
        int eventCount = 0;
        constexpr int MAX_EVENTS_PER_FRAME = 3;

        while (eventCount < MAX_EVENTS_PER_FRAME && g_audioManager.dequeueEvent(eventType, m_eventText, sizeof(m_eventText)))
        {
            eventCount++;

            switch (eventType)
            {
            case AudioManagerEventType::STATION_NAME:
                // Nur Name aktualisieren, NICHT Titel/Bitrate löschen (das passiert oben bei echtem Senderwechsel via Station-ID)
                stringutils::safeCopy(m_finalStation, m_eventText);
                footerNeedsUpdate = true;
                break;

            case AudioManagerEventType::STREAM_TITLE:
                stringutils::safeCopy(m_showstreamtitle, m_eventText);
                footerNeedsUpdate = true;
                break;

            case AudioManagerEventType::BITRATE:
                if (m_eventText[0] != '\0' && strcmp(m_eventText, "0") != 0)
                {
                    // Bitrate von bps nach kbps umrechnen
                    int bitrateValue = atoi(m_eventText);
                    int bitrateKbps = bitrateValue / 1000;
                    stringutils::safeFormat(m_finalBitrate, "%d kbps", bitrateKbps);
                    footerNeedsUpdate = true;
                }

                break;
            default:
                break;
            }
        }

        // bestStation immer berechnen wenn Metadaten sich geändert haben
        if (footerNeedsUpdate)
        {
            const char *presetName = m_stationList[APP_STATE.getCurrentRetroStation()].name;
            const char *bestStation = (m_finalStation[0] != '\0' && strlen(m_finalStation) >= strlen(presetName))
                                          ? m_finalStation
                                          : presetName;

            // Footer auf Display aktualisieren (nur wenn GFX nicht blockiert)
            if (!APP_STATE.getBlockGFX())
            {
                drawFooterText(false, bestStation, m_showstreamtitle, m_finalBitrate);

                // Stereo/Mono-Symbol automatisch aktualisieren basierend auf Audiosignal
                updateStereoIndicator(APP_STATE.getForceMono());

                stringutils::safeCopy(m_showstation, bestStation);
            }

            // Fernsteuerung IMMER informieren (unabhängig von GFX-Block)
            extern void updateRadioMetadata(const char *stationName, const char *streamTitle, const char *bitrate);
            updateRadioMetadata(bestStation, m_showstreamtitle, m_finalBitrate);
        }
    }

    // Audio-Daten für Visualisierung abrufen
    if (g_audioManager.isAudioDataReady())
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

        // Audio-Daten als verarbeitet markieren
        { extern volatile uint32_t g_visDataCount; g_visDataCount = g_visDataCount + 1; } // Messzähler Visualisierungs-Datenrate
        g_audioManager.consumeAudioData();
    }

    // Visualisierungen zeichnen
    if (!gfxBlocked)
        m_visualizer.draw(false);

    // Zeigerbewegung
    const uint8_t target = APP_STATE.getCurrentRetroStation();
    const bool atTarget = this->setStation(target);

    if (!atTarget)
    {
        if (!m_animating)
        {
            m_animating = true;
            if (m_callbacks.requestStopPlayback && !m_stopSent)
            {
                m_callbacks.requestStopPlayback();
                m_stopSent = true;

                // Der Stopp ist synchron: jetzt noch gepufferte Events stammen vom alten Sender und werden verworfen
                AudioManagerEventType staleType;
                while (g_audioManager.dequeueEvent(staleType, m_eventText, sizeof(m_eventText)))
                    ;
            }
        }

        APP_STATE.setStationSetDone(false);
    }
    else
    {
        if (m_animating)
        {
            m_animating = false;
            m_stopSent = false;
            // Position speichern für graduelles Schließen beim Wegfahren
            m_lastGoodPosition = m_handCounter;
        }
        if (!APP_STATE.getStationSetDone())
        {
            if (m_callbacks.requestPlayStation)
            {
                if (m_lastStartedStation != target)
                {
                    m_callbacks.requestPlayStation(target);
                    m_lastStartedStation = target;
                }
            }

            APP_STATE.setStationSetDone(true);
        }
    }
}

// ===================================================================
// Buttons
// ===================================================================

/**
 * @brief Übergibt den gedrückten Button an die Callback-Funktion
 */
void RetroRadio::getPressedRetroButton()
{
    bool buttonState = false;
    const unsigned long LONG_PRESS_THRESHOLD = 2000;

    // Prüfen, ob bereits ein Dialog aktiv ist
    const bool dialogActive = (m_presetDlg && m_presetDlg->isActive()) || (m_msg && (m_msg->isActive() || m_msg->isBusy())) || (m_sleepDlg && m_sleepDlg->isActive());

    if (dialogActive)
        return;

    for (int i = 0; i < 5; i++)
    {
        bool isPressed = m_touched && m_btn[i].contains(m_touchX, m_touchY);

        m_btn[i].press(isPressed);

        // Nur ein frisches Aufsetzen macht die Taste scharf, eine hereinwandernde laufende Berührung nicht
        if (isPressed && !m_wasPressed[i] && !m_wasTouching)
        {
            m_btn[i].drawButton(true);
            m_wasPressed[i] = true;
            m_longPressed[i] = false;
            m_pressTime[i] = millis();
        }

        if (isPressed && m_wasPressed[i] && !m_longPressed[i])
        {
            if (millis() - m_pressTime[i] > LONG_PRESS_THRESHOLD)
                m_longPressed[i] = true;
        }

        if (!isPressed && m_wasPressed[i])
        {
            m_btn[i].drawButton(false);

            // Rutscht der Finger von der Taste, ist das ein Abbruch: nur echtes Abheben löst die Aktion aus
            if (m_touched)
            {
                m_wasPressed[i] = false;
                m_longPressed[i] = false;
                continue;
            }

            if (!m_longPressed[i])
            {
                // Button 0: Favoriten
                if (i == 0)
                {
                    m_lastStartedStation = 0xFF;
                    APP_STATE.setBlockGFX(true);

                    m_presetDlg->setBounds((m_tft->width() - 600) / 2, (m_tft->height() - 440) / 2, 600, 440);
                    char presetTitle[16];
                    STRF(presetTitle, RetroRadioKey::RETRO_PRESET_DLG_TITLE);
                    m_presetDlg->setTitle(presetTitle);
                    m_presetDlg->setTimeout(10000);
                    m_presetDlg->setCallback([this](int selectedIndex)
                                             {
                        if (selectedIndex >= 0) {
                            if (m_callbacks.onStationPressed)
                                m_callbacks.onStationPressed(selectedIndex);
                        }

                        APP_STATE.setBlockGFX(false); });
                    m_presetDlg->start(m_stationList, m_stationCounter);
                }

                // Button 3: Reset
                if (i == 3)
                {
                    m_lastStartedStation = 0xFF;
                    APP_STATE.setBlockGFX(true);

                    m_msg->setBounds(
                        (m_tft->width() - RRLayout::Dialog::MESSAGE_WIDTH) / 2,
                        (m_tft->height() - RRLayout::Dialog::MESSAGE_HEIGHT) / 2,
                        RRLayout::Dialog::MESSAGE_WIDTH,
                        RRLayout::Dialog::MESSAGE_HEIGHT);
                    char resetTitle[24], resetMsg[96], btnOk[16], btnCancel[16];
                    STRF(resetTitle, RetroRadioKey::RETRO_RESET_DLG_TITLE);
                    STRF(resetMsg, RetroRadioKey::RETRO_RESET_DLG_MESSAGE);
                    STRF(btnOk, CommonKey::COMMON_BTN_CONTINUE);
                    STRF(btnCancel, CommonKey::COMMON_BTN_CANCEL);
                    m_msg->setTitle(resetTitle);
                    m_msg->setMessage(resetMsg);
                    m_msg->setButtonLabels(btnOk, btnCancel);
                    m_msg->setTimeout(10000);

                    m_msg->setCallback([this](bool confirmed)
                                       { onMessageBoxResult(confirmed); });
                    m_msg->start();
                }

                // Button 4: Sleep Timer / Ausschalten
                if (i == 4)
                {
                    m_lastStartedStation = 0xFF;
                    APP_STATE.setBlockGFX(true);

                    m_sleepDlg->setBounds(
                        (m_tft->width() - RRLayout::Dialog::SLEEP_SIZE) / 2,
                        (m_tft->height() - RRLayout::Dialog::SLEEP_SIZE) / 2,
                        RRLayout::Dialog::SLEEP_SIZE,
                        RRLayout::Dialog::SLEEP_SIZE);
                    char sleepTitle[16], sleepMsg[32], sleepCancel[16], sleepStop[16];
                    STRF(sleepTitle, RetroRadioKey::RETRO_SLEEP_DLG_TITLE);
                    STRF(sleepMsg, RetroRadioKey::RETRO_SLEEP_DLG_MESSAGE);
                    STRF(sleepCancel, CommonKey::COMMON_BTN_CANCEL);
                    STRF(sleepStop, CommonKey::COMMON_BTN_STOP);
                    m_sleepDlg->setTitle(sleepTitle);
                    m_sleepDlg->setMessage(sleepMsg);
                    m_sleepDlg->setCancelLabel(sleepCancel);
                    m_sleepDlg->setStopLabel(sleepStop);
                    m_sleepDlg->setOptions(m_sleepLabels, m_sleepValues, 6);
                    m_sleepDlg->setTimeout(10000);

                    m_sleepDlg->setCallback([this](int minutes)
                                            {
                        if (minutes == 0) {
                            // Sofort ausschalten (ohne Sleep-Mode)
                            stopRadioImmediately();
                        } else if (minutes > 0) {
                            // Timer starten - nur bei Ablauf Sleep-Mode aktivieren
                            m_sleepDlg->startSleepCountdown(minutes, [this]() 
                            { 
                            // Timer ist abgelaufen -> Sleep-Mode aktivieren
                            APP_STATE.setSleepModeTriggered(true);
                            stopRadioImmediately(); 
                        });
                        }

                        // Bei -1 (Abbruch) oder -2 (Stop): nichts tun, Dialog schließt sich nur
                        APP_STATE.setBlockGFX(false); });

                    m_sleepDlg->start();
                }

                if (m_callbacks.onButtonPressed)
                    m_callbacks.onButtonPressed(i, buttonState);
            }

            m_wasPressed[i] = false;
            m_longPressed[i] = false;
        }
    }
}

/**
 * @brief Messagebox Ok-Button Callback
 */
void RetroRadio::onMessageBoxResult(bool confirmed)
{
    if (confirmed)
        m_resetRequested = true;

    APP_STATE.setBlockGFX(false);
}

/**
 * @brief Stoppt das Radio sofort
 */
void RetroRadio::stopRadioImmediately()
{
    // Radio zuverlässig stoppen und Status zurücksetzen
    if (m_callbacks.requestStopPlayback)
        m_callbacks.requestStopPlayback();

    APP_STATE.setStationSetDone(false);

    // Modul beenden
    if (m_callbacks.onExitRequested)
        m_callbacks.onExitRequested();
}

/**
 * @brief Komplette UI neu zeichnen (Sprachwechsel)
 */
void RetroRadio::redrawAll()
{
    if (!m_initialized || !m_tft)
        return;

    // Display leeren, damit alte Labels nicht durchscheinen
    m_tft->clear(TFT_BLACK);

    // Buttons neu zeichnen
    for (int i = 0; i < 5; i++)
        m_btn[i].drawButton();

    // Skala neu zeichnen
    drawScale();

    // Magisches Auge; das Skala-Sprite hat seine Fläche überdeckt, deshalb muss der Zwischenspeicher den nächsten Aufbau erzwingen.
    drawMagicEye(true, 0, 0, 0);
    m_lastMagicEyeValue = -1;

    // Uhren
    drawAnalogClock(true, 0, 0, 0, 0);
    drawDigitalClock(true, 0, 0, 0, 0);

    // Footer
    drawFooterText(true);

    // Datum im Footer aus der Systemzeit bilden
    char buffer[64];
    if (DS1307_RTC::isSystemTimeValid())
    {
        time_t nowEpoch = time(nullptr);
        struct tm nowLocal;
        localtime_r(&nowEpoch, &nowLocal);
        char monBuf[16];
        I18n::copyMonth(monBuf, sizeof(monBuf), nowLocal.tm_mon, /*longForm=*/true);
        STRF(buffer, CommonKey::COMMON_DT_FORMAT_DATE,
             monBuf,
             nowLocal.tm_mday,
             nowLocal.tm_year + 1900);
    }
    else
    {
        STRF(buffer, CommonKey::COMMON_DT_PLACEHOLDER_DATE);
    }
    drawFooterText(false, " ", buffer, " ", TFT_CYAN, TFT_CYAN, TFT_CYAN, TFT_CYAN, " - ");

    // Lautstärke-Regler
    drawKnob(true, APP_STATE.getIsMuted() ? 0 : APP_STATE.getCurrentVolume(), /*skipCallbacks=*/true);

    // Equalizer-Regler
    drawTracks(true,
               APP_STATE.getCurrentBalance(),
               APP_STATE.getCurrentGainLowPass(),
               APP_STATE.getCurrentGainBandPass(),
               APP_STATE.getCurrentGainHighPass(),
               /*skipCallbacks=*/true);

    // Visualizer (zeichnet auch sein Goniometer-Info-Label neu)
    m_visualizer.draw(true);
}

/**
 * @brief Schaltet die Stummschaltung ein/aus
 */
void RetroRadio::setMute(bool mute)
{
    if (mute)
    {
        // Lautstärke vor Mute nur sichern, wenn sie hörbar war: ein Mute bei 0 würde sonst den Wiederherstellwert mit 0 überschreiben
        if (APP_STATE.getCurrentVolume() > 0)
            APP_STATE.setVolumeBeforeMute(APP_STATE.getCurrentVolume());
        APP_STATE.setCurrentVolume(0);
        APP_STATE.setIsMuted(true);

        if (m_callbacks.onVolumeBeforeMute)
            m_callbacks.onVolumeBeforeMute(APP_STATE.getVolumeBeforeMute());

        drawKnob(true, 0);
    }
    else
    {
        // Lautstärke vor Mute wiederherstellen
        uint8_t previousVol = APP_STATE.getVolumeBeforeMute();
        APP_STATE.setCurrentVolume(previousVol);
        APP_STATE.setIsMuted(false);

        drawKnob(true, previousVol);

        if (m_callbacks.onVolumeChanged)
            m_callbacks.onVolumeChanged(previousVol);
    }

    if (m_callbacks.onMuteChanged)
        m_callbacks.onMuteChanged(mute);
}

// ===================================================================
// Skala
// ===================================================================

/**
 * @brief Skala zeichnen
 */
void RetroRadio::drawScale()
{
    if (!m_tft->safeCreateSprite(m_sprRetroScale, RRLayout::Scale::WIDTH, RRLayout::Scale::HEIGHT) || !m_tft->safeCreateSprite(m_sprRetroDiv, RRLayout::Scale::WIDTH, RRLayout::Scale::DIVIDER_HEIGHT))
    {
        LOG_ERROR("Konnte Skala-Sprites nicht erstellen");
        return;
    }

    m_sprRetroScale->clear(RRColors::BACKGROUND);
    m_sprRetroScale->loadFont(ComfortaaBold14);
    m_sprRetroScale->setTextColor(TFT_SILVER, RRColors::BACKGROUND);
    m_sprRetroScale->setTextWrap(false);

    m_sprRetroDiv->clear(0x79E2);

    uint16_t scalaPosX = 10;
    uint16_t scalaPosY = 102;

    for (uint16_t i = 0; i < m_stationCounter; i++)
    {
        // Position immer zuweisen (auch für leere Slots -> Touch-Mapping)
        m_stationList[i].x = scalaPosX;
        m_stationList[i].y = scalaPosY;

        // Nur nicht-leere Stationen zeichnen (Marker + Name)
        if (m_stationList[i].url[0] != '\0')
        {
            m_sprRetroScale->fillRect(scalaPosX - 4, scalaPosY + 5, 14, 6, (m_stationList[i].preset) ? RRColors::PRESET_MARKER : RRColors::STATION_MARKER);
            m_sprRetroScale->drawString(m_stationList[i].shortName, scalaPosX + 16, scalaPosY);

            if (m_stationList[i].preset)
            {
                int16_t textWidth = m_sprRetroScale->textWidth(m_stationList[i].shortName);
                m_sprRetroScale->drawFastHLine(scalaPosX + 16, scalaPosY + m_sprRetroScale->fontHeight(), textWidth, RRColors::PRESET_MARKER);
            }
        }
        else
        {
            // Leerer Slot: dezenter Marker als Platzhalter
            m_sprRetroScale->fillRect(scalaPosX - 4, scalaPosY + 5, 14, 6, RRColors::BORDER);
        }

        scalaPosX += 10;
        scalaPosY += 20;

        if (scalaPosY > RRLayout::Scale::HEIGHT)
            scalaPosY = 2;
    }

    m_sprRetroDiv->pushSprite(0, 0);
    m_sprRetroScale->pushImage(15, 30, 90, 53, logo_90);
    m_sprRetroScale->pushSprite(0, 5);
    m_sprRetroDiv->pushSprite(0, 307);
}

/**
 * @brief Lädt die Senderliste neu und zeichnet die Skala neu
 *
 * Wird aus der draw()-Schleife aufgerufen wenn das Reload-Flag durch die WebUI gesetzt wurde. Aktualisiert den gecachten Zeiger und Zähler und rendert die Skala komplett neu.
 */
void RetroRadio::reloadStations()
{
    if (!m_stationManager)
        return;

    // Senderliste aus dem RAM-Cache holen (loadStations() wurde bereits extern aufgerufen)
    m_stationList = m_stationManager->getStations();
    m_stationCounter = constrain(m_stationManager->getStationCount(), 0, RRConfig::Stations::MAX_COUNT);

    // Skala komplett neu zeichnen
    drawScale();

    // Aktuellen Sender-Index validieren: falls geleerter Slot -> nächsten gültigen suchen
    const uint8_t currentStation = APP_STATE.getCurrentRetroStation();
    if (currentStation >= m_stationCounter || m_stationList[currentStation].url[0] == '\0')
    {
        // Vorwärts suchen
        bool found = false;
        for (int i = 0; i < m_stationCounter; i++)
        {
            if (m_stationList[i].url[0] != '\0')
            {
                extern void setNewStation(uint8_t id);
                setNewStation(i);
                found = true;
                break;
            }
        }

        if (!found)
            LOG_WARNING("Keine gültigen Sender mehr vorhanden!");
    }

    // Nadel-State zurücksetzen damit sie sich zum aktuellen Sender bewegt
    m_handCounter = -1;
    m_lastGoodPosition = -1;
    m_lastMagicEyeValue = -1;
    m_lastStartedStation = 0xFF; // Duplikat-Schutz aufheben -> erzwingt requestPlayStation
    m_animating = false;         // Animationszustand
    m_stopSent = false;          // Stop-Flag zurücksetzen
    APP_STATE.setStationSetDone(false);

    LOG_SUCCESS_F("Senderliste per WebUI aktualisiert: %d Sender", m_stationCounter);
}

/**
 * @brief Übergibt die auf der Skala gewählte Station an die Callback-Funktion
 */
void RetroRadio::getPressedStation()
{
    if (APP_STATE.getBlockGFX())
        return;

    // Nur ein frisch in der Skala aufgesetzter Finger wählt einen Sender, eine hereinwandernde Geste nicht
    if (m_touched && !m_isTouched && !m_wasTouching && m_touchY > 5 && m_touchY < 305)
    {
        int16_t station = (m_touchX / 10);
        if (station < m_stationCounter && m_stationList[station].url[0] != '\0')
        {
            if (m_callbacks.onStationPressed)
                m_callbacks.onStationPressed(station);
        }

        m_isTouched = true;
    }
    else if (!m_touched)
    {
        m_isTouched = false;
    }
}

/**
 * @brief Bewegt den Skalenzeiger zur Zielposition einer Station
 */
bool RetroRadio::setStation(uint8_t id)
{
    if (APP_STATE.getBlockGFX())
        return true;

    if (id >= m_stationCounter)
        return false;

    if (m_handCounter == -1 || m_stationList[id].lastPos < 0 || m_stationList[id].lastPos > 794)
    {
        if (m_stationList[id].lastPos < 0 || m_stationList[id].lastPos > 794)
            m_stationList[id].lastPos = m_stationList[id].x;

        m_handCounter = m_stationList[id].lastPos;
        m_handCounter = constrain(m_handCounter, 0, 794);
        m_lastMoveTime = 0;
    }

    if (m_handCounter == m_stationList[id].x)
    {
        m_stationList[id].lastPos = m_handCounter;
        return true;
    }

    unsigned long currentTime = millis();
    float timePerPixel = 1000.0f / 166.60f;

    if (currentTime - m_lastMoveTime >= timePerPixel)
    {
        m_handCounter += (m_handCounter < m_stationList[id].x) ? 1 : -1;

        if (m_handCounter < 0 || m_handCounter > (RRConfig::Display::WIDTH - RRLayout::Scale::NEEDLE_WIDTH))
            return false;

        m_sprRetroScale->readRect(m_handCounter, 0, 1, RRLayout::Scale::HEIGHT, m_col0);
        m_sprRetroScale->readRect(m_handCounter + 5, 0, 1, RRLayout::Scale::HEIGHT, m_col5);

        m_sprRetroNeedle->pushImage(0, 0, 1, RRLayout::Scale::HEIGHT, m_col0);
        m_sprRetroNeedle->pushImage(5, 0, 1, RRLayout::Scale::HEIGHT, m_col5);
        m_sprRetroNeedle->pushSprite(m_handCounter, 5);

        m_lastMoveTime = currentTime;
    }

    return false;
}

/**
 * @brief Aktualisiert das Stereo/Mono-Symbol basierend auf dem Audiosignal
 */
void RetroRadio::updateStereoIndicator(bool isMono)
{
    // Sprite wurde in init() erstellt - hier nur prüfen ob verfügbar
    if (!m_sprRetroStereo)
    {
        LOG_ERROR("m_sprRetroStereo nicht initialisiert");
        return;
    }

    // Symbol zeichnen: gedimmte Farbe für Mono, helle Farbe für Stereo
    m_sprRetroStereo->clear(RRColors::BACKGROUND);
    m_sprRetroStereo->drawSmoothCircle(13, 12, 11.0f, 2.0f, isMono ? RRColors::DIMMED : TFT_SILVER, RRColors::BACKGROUND);
    m_sprRetroStereo->drawSmoothCircle(28, 12, 11.0f, 2.0f, isMono ? RRColors::DIMMED : TFT_SILVER, RRColors::BACKGROUND);
    m_sprRetroStereo->pushSprite(RRConfig::Display::WIDTH - m_sprRetroStereo->width() - 1, RRLayout::Footer::STEREO_INDICATOR_Y);
}

// ===================================================================
// Footer
// ===================================================================

/**
 * @brief Footer mit drei Segmenten ausgeben
 */
void RetroRadio::drawFooterText(bool init, const char *sender, const char *title, const char *bitrate, uint16_t senderColor, uint16_t titleColor, uint16_t bitrateColor, uint16_t separatorColor, const char *separator, int reservedPixels, uint16_t backgroundColor)
{
    static const uint8_t *fonts[] = {ComfortaaBold18, ComfortaaBold16, ComfortaaBold14};
    static const int fontCount = sizeof(fonts) / sizeof(fonts[0]);
    static const char *ellipsis = "...";

    // Initialisierung
    if (init)
    {
        if (!m_tft->safeCreateSprite(m_sprRetroFooter, RRLayout::Footer::WIDTH, RRLayout::Footer::HEIGHT))
        {
            LOG_ERROR("Konnte m_sprRetroFooter Sprite nicht erstellen!");
            return;
        }

        m_sprRetroFooter->setTextWrap(false);
        m_sprRetroFooter->setTextDatum(ML_DATUM);

        m_sprRetroFooter->clear(backgroundColor);
        m_sprRetroFooter->drawFastHLine(0, 0, RRLayout::Footer::WIDTH, RRColors::BORDER);
        m_sprRetroFooter->drawFastHLine(0, RRLayout::Footer::HEIGHT - 1, RRLayout::Footer::WIDTH, RRColors::BORDER);
        m_sprRetroFooter->pushSprite(0, RRLayout::Footer::Y);

        return;
    }

    if (!sender || !title || !bitrate || !separator)
        return;

    const bool hasSender = sender[0] != '\0';
    const bool hasTitle = title[0] != '\0';
    const bool hasBitrate = bitrate[0] != '\0';

    if (!hasSender && !hasTitle && !hasBitrate)
        return;

    m_sprRetroFooter->fillRect(0, 1, RRLayout::Footer::WIDTH, RRLayout::Footer::HEIGHT - 2, backgroundColor);

    const int availableWidth = RRLayout::Footer::WIDTH - reservedPixels;
    const int centerY = RRLayout::Footer::HEIGHT / 2;

    auto calcFooterWidth = [&](const char *snd, const char *ttl, const char *brt) -> int
    {
        int width = 0;
        bool hasPrevious = false;

        const int separatorWidth = m_sprRetroFooter->textWidth(separator);

        if (snd[0] != '\0')
        {
            width += m_sprRetroFooter->textWidth(snd);
            hasPrevious = true;
        }

        if (ttl[0] != '\0')
        {
            if (hasPrevious)
                width += separatorWidth;

            width += m_sprRetroFooter->textWidth(ttl);
            hasPrevious = true;
        }

        if (brt[0] != '\0')
        {
            if (hasPrevious)
                width += separatorWidth;

            width += m_sprRetroFooter->textWidth(brt);
        }

        return width;
    };

    auto drawFooter = [&](const char *snd, const char *ttl, const char *brt, int startX)
    {
        int cursorX = startX;
        bool hasPrevious = false;

        if (snd[0] != '\0')
        {
            m_sprRetroFooter->setTextColor(senderColor, backgroundColor);
            m_sprRetroFooter->drawString(snd, cursorX, centerY);
            cursorX += m_sprRetroFooter->textWidth(snd);
            hasPrevious = true;
        }

        if (ttl[0] != '\0')
        {
            if (hasPrevious)
            {
                m_sprRetroFooter->setTextColor(separatorColor, backgroundColor);
                m_sprRetroFooter->drawString(separator, cursorX, centerY);
                cursorX += m_sprRetroFooter->textWidth(separator);
            }

            m_sprRetroFooter->setTextColor(titleColor, backgroundColor);
            m_sprRetroFooter->drawString(ttl, cursorX, centerY);
            cursorX += m_sprRetroFooter->textWidth(ttl);
            hasPrevious = true;
        }

        if (brt[0] != '\0')
        {
            if (hasPrevious)
            {
                m_sprRetroFooter->setTextColor(separatorColor, backgroundColor);
                m_sprRetroFooter->drawString(separator, cursorX, centerY);
                cursorX += m_sprRetroFooter->textWidth(separator);
            }

            m_sprRetroFooter->setTextColor(bitrateColor, backgroundColor);
            m_sprRetroFooter->drawString(brt, cursorX, centerY);
        }
    };

    for (int fontIndex = 0; fontIndex < fontCount; fontIndex++)
    {
        m_sprRetroFooter->unloadFont();
        m_sprRetroFooter->loadFont(fonts[fontIndex]);

        int totalWidth = calcFooterWidth(sender, title, bitrate);

        if (totalWidth <= availableWidth)
        {
            int startX = (RRLayout::Footer::WIDTH - totalWidth) / 2;
            drawFooter(sender, title, bitrate, startX);
            m_sprRetroFooter->unloadFont();
            m_sprRetroFooter->pushSprite(0, RRLayout::Footer::Y);
            return;
        }
    }

    m_sprRetroFooter->unloadFont();
    m_sprRetroFooter->loadFont(fonts[fontCount - 1]);

    m_truncatedTitle[0] = '\0';

    if (hasTitle && availableWidth > 0)
    {
        int fixedWidth = 0;
        const int separatorWidth = m_sprRetroFooter->textWidth(separator);

        if (hasSender)
        {
            fixedWidth += m_sprRetroFooter->textWidth(sender);
            if (hasTitle)
                fixedWidth += separatorWidth;
        }

        if (hasBitrate)
        {
            fixedWidth += m_sprRetroFooter->textWidth(bitrate);
            if (hasTitle)
                fixedWidth += separatorWidth;
        }

        const int ellipsisWidth = m_sprRetroFooter->textWidth(ellipsis);
        const int availableForTitle = availableWidth - fixedWidth - ellipsisWidth;

        if (availableForTitle > 0)
        {
            stringutils::safeCopy(m_truncatedTitle, title);

            int titleWidth = m_sprRetroFooter->textWidth(m_truncatedTitle);
            if (titleWidth > availableForTitle)
            {
                // Grober Vorschnitt nach Breitenverhältnis; landet er auf einem UTF-8-Fortsetzungsbyte, wandert er auf den Anfang des angeschnittenen Zeichens zurück
                size_t estimatedLen = (strlen(m_truncatedTitle) * availableForTitle) / titleWidth;
                if (estimatedLen < strlen(m_truncatedTitle))
                {
                    while (estimatedLen > 0 && (m_truncatedTitle[estimatedLen] & 0xC0) == 0x80)
                        --estimatedLen;
                    m_truncatedTitle[estimatedLen] = '\0';
                }

                // Zeichenweise statt byteweise kürzen: Fortsetzungsbytes fallen mit ihrem Startbyte zusammen weg
                while (m_sprRetroFooter->textWidth(m_truncatedTitle) > availableForTitle && m_truncatedTitle[0] != '\0')
                {
                    size_t len = strlen(m_truncatedTitle) - 1;
                    while (len > 0 && (m_truncatedTitle[len] & 0xC0) == 0x80)
                        --len;
                    m_truncatedTitle[len] = '\0';
                }

                if (m_truncatedTitle[0] != '\0' && strlen(m_truncatedTitle) < strlen(title))
                    strncat(m_truncatedTitle, ellipsis, sizeof(m_truncatedTitle) - strlen(m_truncatedTitle) - 1);
            }
        }
    }

    const char *finalTitle = (m_truncatedTitle[0] != '\0') ? m_truncatedTitle : "";
    int finalWidth = calcFooterWidth(sender, finalTitle, bitrate);
    int startX = (RRLayout::Footer::WIDTH - finalWidth) / 2;

    drawFooter(sender, finalTitle, bitrate, startX);
    m_sprRetroFooter->unloadFont();
    m_sprRetroFooter->pushSprite(0, RRLayout::Footer::Y);
}

// ===================================================================
// Controls
// ===================================================================

/**
 * @brief Hilfsfunktion für den Lautstärkeregler
 */
void RetroRadio::calcMarkerPos(float angleDeg, int center, int radiusOffset, int &outX, int &outY)
{
    float rad = (angleDeg - 90.0f) * PI_F / 180.0f;
    outX = center + cosf(rad) * (center - radiusOffset);
    outY = center + sinf(rad) * (center - radiusOffset);
}

/**
 * @brief Lautstärkeregler Control mit integrierter Mute-Funktion
 */
void RetroRadio::drawKnob(bool init, uint8_t vol, bool skipCallbacks)
{
    int markerX = 0, markerY = 0;
    vol = constrain(vol, 0, 100);

    // Layout aus Namespace
    constexpr int x = RRLayout::Knob::X;
    constexpr int y = RRLayout::Knob::Y;
    constexpr int wh = RRLayout::Knob::SIZE;
    constexpr int mid = wh / 2;

    // Touch-Parameter
    constexpr int minTouchDistance = 20;
    constexpr int padding = 20;
    constexpr int muteAreaRadius = 16;

    // Marker-Parameter
    constexpr int markerRadiusOffset = 20;
    constexpr int markerCircleRadius = 13;
    constexpr int markerCircleThickness = 3;

    // Farben aus Namespace
    constexpr uint16_t colorActive = RRColors::KNOB_ACTIVE;
    constexpr uint16_t colorTrack = RRColors::BORDER;
    constexpr uint16_t colorMarker = RRColors::ACCENT;
    constexpr uint16_t colorText = TFT_LIGHTGREY;
    constexpr uint16_t colorTextBg = TFT_BLACK;
    constexpr uint16_t colorTextVal = TFT_DARKGREY;

    // Initialisierung
    if (init)
    {
        m_knobTouchState = KnobTouchState::IDLE;
        m_muteTriggered = false;
        m_totalDegrees = vol * 36.0f;
        m_prevVol = vol; // Rotations-Vergleichswert mitziehen, sonst prüft der erste Dreh-Tick gegen einen veralteten Wert

        // Label direkt auf TFT zeichnen (bei jedem init=true, damit Sprachwechsel das Label aktualisiert)
        m_tft->fillRect(x, y - 13, wh, 13, TFT_BLACK);
        m_tft->loadFont(ComfortaaBold10);
        m_tft->setTextColor(colorText, colorTextBg);
        m_tft->setTextDatum(TC_DATUM);
        m_tft->drawString(STR(RetroRadioKey::RETRO_KNOB_LABEL), x + wh / 2, y - 13);
        m_tft->unloadFont();
        m_tft->setTextDatum(TL_DATUM);

        // Sprite nur beim ersten Init erstellen (nicht bei Reset/Mute)
        if (!m_sprRetroKnob)
        {
            // Sprite erstellen
            if (!m_tft->safeCreateSprite(m_sprRetroKnob, wh, wh))
            {
                LOG_ERROR("Konnte m_sprRetroKnob Sprite nicht erstellen!");
                return;
            }

            // Font nur beim ersten Mal laden (bleibt im Sprite erhalten)
            // Ohne zweite Farbe bleibt der Zeichenhintergrund durchsichtig. Mit ihr malt drawString die ganze Zeichenzelle aus, und die ist höher als die Ziffern selbst; sie löschte dadurch ein Stück des Markierungskreises.
            m_sprRetroKnob->loadFont(LeagueGothicCond34);
            m_sprRetroKnob->setTextColor(colorTextVal);
        }

        // Ab hier: Sprite existiert, Inhalt neu zeichnen
        m_sprRetroKnob->clear(TFT_BLACK);

        m_sprRetroKnob->drawSmoothGauge(mid, mid, mid - 1, 3.0f, 0, 360, 0, 100, vol, colorActive, colorTrack, true, TFT_BLACK);

        calcMarkerPos(m_totalDegrees, mid, markerRadiusOffset, markerX, markerY);
        m_sprRetroKnob->drawSmoothCircle(markerX, markerY, markerCircleRadius, markerCircleThickness, colorMarker, TFT_BLACK);

        if (vol > 0)
        {
            char volStr[5];
            stringutils::safeFormat(volStr, "%d", vol);

            // Der Versatz hängt nur an der Stellenzahl und der Breite der ersten Ziffer, nicht an der Breite der ganzen Zahl. Dadurch steht sie still, statt bei jedem Wechsel um einen halben Bildpunkt zu springen, wie es eine echte Zentrierung täte.
            char firstDigit[2] = {volStr[0], '\0'};
            int32_t volTextW = m_sprRetroKnob->textWidth(firstDigit);
            int xOffset = (vol < 10) ? (volTextW / 2) : (vol < 100) ? volTextW
                                                                    : (2 * volTextW);
            m_sprRetroKnob->drawString(volStr, mid - xOffset, mid - 15);
        }
        else
        {
            m_sprRetroKnob->pushImage(mid - 14, mid - 14, 28, 28,
                                    IconShapes::cached(IconId::Mute, ICON_MUTE_COLOR, ICON_MUTE_BACKGROUND));
        }

        m_sprRetroKnob->pushSprite(x, y);

        if (!skipCallbacks && m_callbacks.onVolumeChanged)
            m_callbacks.onVolumeChanged(vol);

        return;
    }

    // Touch-Verarbeitung
    if (m_touched &&
        m_touchX > (x - padding) && m_touchX < (x + wh + padding) &&
        m_touchY > (y - padding) && m_touchY < (y + wh + padding))
    {
        int deltaX = m_touchX - (x + mid);
        int deltaY = m_touchY - (y + mid);
        int touchDistance = deltaX * deltaX + deltaY * deltaY;

        // Touch-Mode nur bei frisch aufgesetztem Finger festlegen, eine hereinwandernde laufende Berührung macht weder Mute noch Rotation scharf
        if (m_knobTouchState == KnobTouchState::IDLE && !m_wasTouching)
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
                // Stumm ist auch, wer nur auf Lautstärke 0 steht (z.B. per SD-Player gemutet): der Toggle richtet sich nach dem sichtbaren Zustand, nicht allein nach dem Flag
                const bool effectivelyMuted = APP_STATE.getIsMuted() || APP_STATE.getCurrentVolume() == 0;
                setMute(!effectivelyMuted);
                m_muteTriggered = true;
            }

            // Kein weiterer Code - warten bis User loslässt
            return;
        }

        // Rotations-Logik (nur wenn im Rotations-Modus und nicht gemutet)
        if (m_knobTouchState == KnobTouchState::ROTATING)
        {
            if (APP_STATE.getIsMuted())
                return;

            calcMarkerPos(m_totalDegrees, mid, markerRadiusOffset, markerX, markerY);

            float rad = atan2f(deltaY, deltaX);
            float currentAngle = rad * 57.29578f;
            float deltaAngle = fmod(currentAngle - m_lastAngle + 540.0f, 360.0f) - 180.0f;
            if (fabs(deltaAngle) < 2.0f)
                return;

            m_totalDegrees = constrain(m_totalDegrees + deltaAngle, 0.0f, 3600.0f);
            m_lastAngle = currentAngle;

            uint8_t newVol = (uint8_t)(m_totalDegrees / 36.0f);
            newVol = constrain(newVol, 0, 100);

            if (newVol != m_prevVol)
            {
                m_prevVol = newVol;

                // Wert in AppState persistieren
                APP_STATE.setCurrentVolume(newVol);

                calcMarkerPos(m_totalDegrees, mid, markerRadiusOffset, markerX, markerY);

                m_sprRetroKnob->clear(TFT_BLACK);
                m_sprRetroKnob->drawSmoothGauge(mid, mid, mid - 1, 3.0f, 0, 360, 0, 100, newVol, colorActive, colorTrack, true, TFT_BLACK);
                m_sprRetroKnob->drawSmoothCircle(markerX, markerY, markerCircleRadius, markerCircleThickness, colorMarker, TFT_BLACK);

                char volStr[5];
                stringutils::safeFormat(volStr, "%d", newVol);
                char firstDigit[2] = {volStr[0], '\0'};
                int32_t volTextW = m_sprRetroKnob->textWidth(firstDigit);
                int xOffset = (newVol < 10) ? (volTextW / 2) : (newVol < 100) ? volTextW
                                                                              : (2 * volTextW);
                m_sprRetroKnob->drawString(volStr, mid - xOffset, mid - 15);
                m_sprRetroKnob->pushSprite(x, y);

                if (m_callbacks.onVolumeChanged)
                    m_callbacks.onVolumeChanged(newVol);
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

/**
 * @brief Zeichnet die Equalizer-Regler (Balance, Tiefen, Mitten, Höhen)
 */
void RetroRadio::drawTracks(bool init, int8_t balance, int8_t gainLowPass, int8_t gainBandPass, int8_t gainHighPass, bool skipCallbacks)
{
    // Position und Größe aus Namespace
    constexpr int sliderPosX = RRLayout::Tracks::X;
    constexpr int sliderPosY = RRLayout::Tracks::Y;
    constexpr int sliderWeight = RRLayout::Tracks::WIDTH;
    constexpr int sliderHeight = RRLayout::Tracks::HEIGHT;

    // Slider-interne Konstanten
    constexpr int SLIDER_X[] = {5, 55, 105, 155};
    constexpr int TOUCH_WIDTH = 31;
    constexpr int PIXEL_MIN = 104;
    constexpr int PIXEL_ZERO = 64;
    constexpr int PIXEL_MAX = 24;

    auto mapDbToPixel = [](int8_t db) -> int8_t
    {
        return (db <= 0)
                   ? map(db, -12, 0, PIXEL_MIN, PIXEL_ZERO)
                   : map(db, 0, 12, PIXEL_ZERO, PIXEL_MAX);
    };

    auto mapPixelToDb = [](int y) -> int8_t
    {
        return (y > PIXEL_ZERO)
                   ? map(y, PIXEL_MIN, PIXEL_ZERO, -12, 0)
                   : map(y, PIXEL_ZERO, PIXEL_MAX, 0, 12);
    };

    auto drawSlider = [this](int xPos, const __FlashStringHelper *label, int8_t pixelValue, bool withLabel = true)
    {
        m_sprRetroTracks->fillGradientRect(xPos + 14, 2, 3, 116, RRColors::BACKGROUND, RRColors::BORDER, lgfx::v1::gradient_fill_styles::VLINEAR);

        if (withLabel && label)
            m_sprRetroTracks->drawString(label, (xPos == 5) ? xPos - 5 : xPos, 0);

        m_sprRetroTracks->drawFastHLine(xPos, PIXEL_ZERO, TOUCH_WIDTH, TFT_DARKGREY);
        m_sprRetroTracks->drawSmoothCircle(xPos + 15, pixelValue, 13, 3, RRColors::ACCENT, TFT_BLACK);
        m_sprRetroTracks->fillSmoothCircle(xPos + 15, pixelValue, 10, TFT_BLACK);
    };

    balance = constrain(balance, -16, 16);
    gainLowPass = constrain(gainLowPass, -12, 12);
    gainBandPass = constrain(gainBandPass, -12, 12);
    gainHighPass = constrain(gainHighPass, -12, 12);

    int8_t balancePixel = map(balance, -16, 16, PIXEL_MIN, PIXEL_MAX);
    int8_t lowPixel = mapDbToPixel(gainLowPass);
    int8_t midPixel = mapDbToPixel(gainBandPass);
    int8_t highPixel = mapDbToPixel(gainHighPass);

    // Initialisierung
    if (init)
    {
        // Sprite nur beim ersten Init erstellen (nicht bei Reset)
        if (!m_sprRetroTracks)
        {
            if (!m_tft->safeCreateSprite(m_sprRetroTracks, sliderWeight, sliderHeight))
            {
                LOG_ERROR("Konnte m_sprRetroTracks Sprite nicht erstellen!");
                return;
            }
        }

        // Ab hier: Sprite existiert, Inhalt neu zeichnen
        m_sprRetroTracks->clear(TFT_BLACK);
        m_sprRetroTracks->loadFont(ComfortaaBold10);
        m_sprRetroTracks->setTextColor(TFT_LIGHTGREY, TFT_BLACK);

        drawSlider(SLIDER_X[0], STR(RetroRadioKey::RETRO_TRACK_BALANCE), balancePixel);
        drawSlider(SLIDER_X[1], STR(RetroRadioKey::RETRO_TRACK_LOW), lowPixel);
        drawSlider(SLIDER_X[2], STR(RetroRadioKey::RETRO_TRACK_MID), midPixel);
        drawSlider(SLIDER_X[3], STR(RetroRadioKey::RETRO_TRACK_HIGH), highPixel);

        m_sprRetroTracks->pushSprite(sliderPosX, sliderPosY);
        m_sprRetroTracks->unloadFont();

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

    // Touch-Verarbeitung
    if (!m_touched)
        return;

    int deltaX = m_touchX - sliderPosX;
    int deltaY = m_touchY - sliderPosY;

    if (deltaY <= 23 || deltaY >= 105)
        return;

    typedef void (*CallbackFunc)(int8_t);
    typedef void (*AppStateSetter)(int8_t);

    auto updateSlider = [&](int sliderIndex, int8_t &prevValue, int8_t currentPixel, CallbackFunc callback, AppStateSetter appStateSetter, bool isDbSlider = false)
    {
        if (deltaX >= SLIDER_X[sliderIndex] && deltaX <= SLIDER_X[sliderIndex] + TOUCH_WIDTH)
        {
            deltaY = constrain(deltaY, PIXEL_MAX, PIXEL_MIN);

            if (prevValue != deltaY)
            {
                m_sprRetroTracks->setClipRect(SLIDER_X[sliderIndex], 11, TOUCH_WIDTH, 107);
                m_sprRetroTracks->clear(TFT_BLACK);
                drawSlider(SLIDER_X[sliderIndex], nullptr, deltaY, false);
                m_sprRetroTracks->pushSprite(sliderPosX, sliderPosY);
                m_sprRetroTracks->clearClipRect();

                int8_t value = isDbSlider
                                   ? mapPixelToDb(deltaY)
                                   : map(deltaY, PIXEL_MIN, PIXEL_MAX, -16, 16);

                // In AppState persistieren
                if (appStateSetter)
                    appStateSetter(value);

                // Callback aufrufen
                if (callback)
                    callback(value);

                prevValue = deltaY;
            }

            return true;
        }

        return false;
    };

    if (updateSlider(0, m_balance_prev, balancePixel, m_callbacks.onBalanceChanged, [](int8_t val)
                     { APP_STATE.setCurrentBalance(val); }))
        return;

    if (updateSlider(1, m_gainLowPass_prev, lowPixel, m_callbacks.onGainLowPassChanged, [](int8_t val)
                     { APP_STATE.setCurrentGainLowPass(val); }, true))
        return;

    if (updateSlider(2, m_gainBandPass_prev, midPixel, m_callbacks.onGainBandPassChanged, [](int8_t val)
                     { APP_STATE.setCurrentGainBandPass(val); }, true))
        return;

    if (updateSlider(3, m_gainHighPass_prev, highPixel, m_callbacks.onGainHighPassChanged, [](int8_t val)
                     { APP_STATE.setCurrentGainHighPass(val); }, true))
        return;
}

// ===================================================================
// Magisches Auge
// ===================================================================

/**
 * @brief Berechnet den Wert für das magische Auge basierend auf Nadelposition
 */
int32_t RetroRadio::calculateMagicEyeValue()
{
    constexpr int32_t VALUE_MIN = 35;                 // Minimaler Wert (kein Signal/Bewegung)
    constexpr int32_t VALUE_MAX = 100;                // Maximaler Wert (perfekte Abstimmung)
    constexpr int32_t TRANSITION_RANGE_APPROACH = 30; // Übergangsbereich beim Ankommen (graduell)
    constexpr int32_t TRANSITION_RANGE_LEAVE = 6;     // Übergangsbereich beim Wegfahren (schnell)

    // Kein gültiger Zeigerwert
    if (m_handCounter < 0)
        return VALUE_MIN;

    // Aktuelle Zielposition ermitteln
    const uint8_t target = APP_STATE.getCurrentRetroStation();
    if (target >= m_stationCounter || m_stationList == nullptr)
        return VALUE_MIN;

    const int32_t targetPos = m_stationList[target].x;
    const int32_t currentPos = m_handCounter;

    // Distanz zur neuen Zielposition berechnen
    const int32_t distanceToTarget = abs(currentPos - targetPos);

    // Distanz zur letzten guten Position berechnen (falls vorhanden)
    int32_t distanceToOldPos = INT32_MAX;
    if (m_lastGoodPosition >= 0)
        distanceToOldPos = abs(currentPos - m_lastGoodPosition);

    // Prüfen ob wir uns der neuen Station nähern oder wegfahren
    const bool approaching = (distanceToTarget <= distanceToOldPos);

    int32_t distance;
    int32_t transitionRange;

    if (approaching)
    {
        // Beim Ankommen: schöner gradueller Übergang
        distance = distanceToTarget;
        transitionRange = TRANSITION_RANGE_APPROACH;
    }
    else
    {
        // Beim Wegfahren: schneller Übergang (fast sofort zu)
        distance = distanceToOldPos;
        transitionRange = TRANSITION_RANGE_LEAVE;
    }

    // Nadel exakt am Ziel
    if (distance == 0)
        return VALUE_MAX;

    // Außerhalb des Übergangsbereichs
    if (distance >= transitionRange)
        return VALUE_MIN;

    // Innerhalb des Übergangsbereichs: linear interpolieren
    const int32_t valueRange = VALUE_MAX - VALUE_MIN;
    const int32_t interpolatedValue = VALUE_MIN + (valueRange * (transitionRange - distance)) / transitionRange;

    return interpolatedValue;
}

/**
 * @brief Simulation eines magischen Auges
 *
 * Alle vier Bauformen teilen sich dasselbe quadratische Sprite. Die runden füllen es aus, die hochkant stehenden EM80 und EM84 stehen kleiner und mittig darin.
 */
void RetroRadio::drawMagicEye(bool init, int32_t x, int32_t y, int32_t value)
{
    if (APP_STATE.getBlockGFX())
        return;

    // Dimensionen aus Namespace
    constexpr int SPRITE_WIDTH = RRLayout::MagicEye::WIDTH;
    constexpr int SPRITE_HEIGHT = RRLayout::MagicEye::HEIGHT;
    constexpr float RING_THICKNESS = 5.0f;
    constexpr float EYE_VALUE_MIN = 35.0f;
    constexpr float EYE_VALUE_MAX = 100.0f;
    const float roundCenter = SPRITE_WIDTH / 2.0f;
    const float roundRadius = roundCenter - 1.0f;

    // Initialisierung
    if (init)
    {
        if (!m_tft->safeCreateSprite(m_sprMagicEye, SPRITE_WIDTH, SPRITE_HEIGHT))
        {
            LOG_ERROR("Konnte m_sprMagicEye Sprite nicht erstellen!");
            return;
        }

        return;
    }

    m_sprMagicEye->clear(RRColors::BACKGROUND);

    // calculateMagicEyeValue liefert nur den oberen Teil des Zahlenbereichs, deshalb wird er auf den vollen Pegelhub der Röhre gespreizt.
    float level = (static_cast<float>(value) - EYE_VALUE_MIN) / (EYE_VALUE_MAX - EYE_VALUE_MIN);
    level = constrain(level, 0.0f, 1.0f);

    const uint8_t style = APP_STATE.getMagicEyeStyle();

    if (style == static_cast<uint8_t>(MagicEyeStyle::Em80))
    {
        // Der Rundbogen steht mittig im Sprite und bringt seinen Rahmen selbst mit.
        m_sprMagicEye->drawEm80MagicEye(SPRITE_WIDTH / 2, SPRITE_HEIGHT / 2,
                                        RRLayout::MagicEye::EM80_WIDTH / 2.0f, RRLayout::MagicEye::EM80_HEIGHT / 2.0f,
                                        level, RRColors::MAGIC_EYE_RING, RRColors::BACKGROUND);
    }
    else if (style == static_cast<uint8_t>(MagicEyeStyle::Em84))
    {
        // Das Leuchtband steht hochkant und mittig im Sprite und bringt seinen Rahmen ebenfalls selbst mit.
        m_sprMagicEye->drawEm84MagicEye(SPRITE_WIDTH / 2, SPRITE_HEIGHT / 2,
                                        RRLayout::MagicEye::EM84_WIDTH / 2.0f, RRLayout::MagicEye::EM84_HEIGHT / 2.0f,
                                        level, RRColors::MAGIC_EYE_RING, RRColors::BACKGROUND);
    }
    else
    {
        m_sprMagicEye->drawSmoothCircle(roundCenter, roundCenter, roundRadius, RING_THICKNESS, RRColors::MAGIC_EYE_RING, RRColors::BACKGROUND);

        const float screenRadius = roundRadius - RING_THICKNESS;
        if (style == static_cast<uint8_t>(MagicEyeStyle::Em34))
            m_sprMagicEye->drawEm34MagicEye(roundCenter, roundCenter, screenRadius, level, RRColors::BACKGROUND);
        else
            m_sprMagicEye->drawEm11MagicEye(roundCenter, roundCenter, screenRadius, level, RRColors::BACKGROUND);
    }

    m_sprMagicEye->pushSprite(x, y);
}

/**
 * @brief Schaltet die Darstellung des magischen Auges bei langem Druck um
 */
void RetroRadio::handleMagicEyeTouch()
{
    if (!m_touched)
    {
        m_magicEyeTouchStart = 0;
        m_magicEyeLongPressDone = false;
        return;
    }

    // Alle Bauformen liegen im selben Sprite, deshalb genügt dessen Rechteck als Trefferfläche.
    const bool insideEye = m_touchX >= RRLayout::MagicEye::X &&
                           m_touchX < RRLayout::MagicEye::X + RRLayout::MagicEye::WIDTH &&
                           m_touchY >= RRLayout::MagicEye::Y &&
                           m_touchY < RRLayout::MagicEye::Y + RRLayout::MagicEye::HEIGHT;

    if (!insideEye)
    {
        m_magicEyeTouchStart = 0;
        return;
    }

    if (m_magicEyeTouchStart == 0)
    {
        m_magicEyeTouchStart = millis();
        return;
    }

    if (m_magicEyeLongPressDone || millis() - m_magicEyeTouchStart < MAGIC_EYE_LONG_PRESS_MS)
        return;

    // Reihum durch alle Darstellungen weiterschalten
    constexpr uint8_t STYLE_COUNT = static_cast<uint8_t>(MagicEyeStyle::Count);
    const uint8_t nextStyle = static_cast<uint8_t>((APP_STATE.getMagicEyeStyle() + 1) % STYLE_COUNT);
    APP_STATE.setMagicEyeStyle(nextStyle);
    m_magicEyeLongPressDone = true;

    LOG_INFO_F("Magisches Auge gewechselt zu: %d", static_cast<int>(nextStyle));

    // SSE-Event an Web-Clients senden
    extern void sendRadioSSEEvent();
    sendRadioSSEEvent();
}

// ===================================================================
// Uhren
// ===================================================================

/**
 * @brief Analoge Uhr mit Timer
 */
void RetroRadio::drawAnalogClock(bool init, int32_t x, int32_t y, int remainingMinutes, int remainingSeconds)
{
    // Sprite-Initialisierung (modulspezifisch)
    if (init)
    {
        if (!m_tft->safeCreateSprite(m_sprRetroAnalogClock, RRLayout::Clock::ANALOG_SIZE, RRLayout::Clock::ANALOG_SIZE))
        {
            LOG_ERROR("Konnte m_sprRetroAnalogClock Sprite nicht erstellen!");
            return;
        }

        m_sprRetroAnalogClock->setFont(&fonts::DejaVu9);
        m_lastDrawnSecond = -1;

        return;
    }

    // Rendering delegieren
    Clocks::drawAnalogClock(m_sprRetroAnalogClock, m_rtc, m_lastDrawnSecond, x, y, remainingMinutes, remainingSeconds);
}

/**
 * @brief Digitale Uhr mit Timer
 */
void RetroRadio::drawDigitalClock(bool init, int32_t x, int32_t y, int remainingMinutes, int remainingSeconds)
{
    // Sprite-Initialisierung (modulspezifisch)
    if (init)
    {
        if (!m_tft->safeCreateSprite(m_sprRetroDigitalClock, RRLayout::Clock::DIGITAL_WIDTH, RRLayout::Clock::DIGITAL_HEIGHT))
        {
            LOG_ERROR("Konnte m_sprRetroDigitalClock Sprite nicht erstellen!");
            return;
        }

        m_sprRetroDigitalClock->clear(TFT_BLACK);
        m_lastDrawnSecond = -1;

        return;
    }

    // Rendering delegieren
    Clocks::drawDigitalClock(m_sprRetroDigitalClock, m_rtc, m_lastDrawnSecond, x, y, remainingMinutes, remainingSeconds);
}

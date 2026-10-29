#include "WorkshopTool_Metronome.h"

#include <algorithm>
#include <cmath>

#include "Gfx.h"
#include "I18n.h"
#include "StringUtils.h"
#include "Workshop.h"
#include "fonts/ComfortaaBold14.h"
#include "fonts/ComfortaaBold30.h"
#include "fonts/LeagueGothicCond34.h"

// ===================================================================
// Anonymer Namespace - file-local Helfer und Konstanten
// ===================================================================

namespace {

// Layout - vertikale Positionen relativ zum Inhaltsbereich-Ursprung
constexpr int BEATS_CENTER_Y = 80;       ///< Y-Mitte der Beat-Lichter-Reihe
constexpr int BPM_CENTER_Y = 180;        ///< Y-Mitte der BPM-Zahl
constexpr int TEMPO_CENTER_Y = 215;      ///< Y-Mitte des Tempo-Namens
constexpr int FOOTER_TEXT_Y = 290;       ///< Y-Mitte für Taktart bzw. Tipphinweis

// Beat-Lichter
constexpr int BEAT_LIGHT_SPACING = 90;   ///< Horizontaler Abstand zwischen Lichter-Mittelpunkten
constexpr int BEAT_LIGHT_R = 28;         ///< Radius eines inaktiven Lichts
constexpr int BEAT_LIGHT_R_ACTIVE = 36;  ///< Radius des aktiven Lichts

// Farben
constexpr uint16_t C_BG = 0x1082;              ///< Hintergrund des Inhaltsbereichs
constexpr uint16_t C_BPM_TEXT = 0xFFFF;        ///< BPM-Zahl (Weiß)
constexpr uint16_t C_TEMPO_NAME = 0xB5F7;      ///< Tempo-Bezeichnung (Hellgrau)
constexpr uint16_t C_TIMESIG = 0xAD75;         ///< Taktart-Beschriftung
constexpr uint16_t C_STOPPED_HINT = 0x7BEF;    ///< Tipphinweis bei Stillstand
constexpr uint16_t C_BEAT_DIM = 0x2945;        ///< Inaktives Beat-Licht (Dunkelgrau)
constexpr uint16_t C_BEAT_ACTIVE = 0x07F4;     ///< Aktives normales Beat-Licht (Petrol)
constexpr uint16_t C_BEAT_ACCENT = 0xFD20;     ///< Aktives Akzent-Licht (Orange)
constexpr uint16_t C_BEAT_ACCENT_DIM = 0x6920; ///< Inaktives Akzent-Licht (gedämpftes Orange)

// Footer-Buttons
constexpr int FOOTER_BTN_W = 62;
constexpr int FOOTER_BTN_H = 44;
constexpr int FOOTER_BTN_R = 6;
constexpr int FOOTER_BTN_GAP = 4;
constexpr int FOOTER_BTN_MARGIN = 2;

constexpr uint16_t TOGGLE_ACTIVE_FILL = 0x328A;
constexpr uint16_t TOGGLE_ACTIVE_TEXT = 0xFFFF;
constexpr uint16_t TOGGLE_ACTIVE_OUTLINE = 0x328A;
constexpr uint16_t TOGGLE_INACTIVE_FILL = 0x0000;
constexpr uint16_t TOGGLE_INACTIVE_TEXT = 0x7BEF;
constexpr uint16_t TOGGLE_INACTIVE_OUTLINE = 0x39E8;

// Taktarten
struct TimeSig
{
    uint8_t num;
    uint8_t den;
};

constexpr TimeSig TIME_SIGS[] = {
    {2, 4}, {3, 4}, {4, 4}, {6, 8}
};
constexpr int NUM_TIME_SIGS = 4;

const char *const TIMESIG_LABELS[] = {"2/4", "3/4", "4/4", "6/8"};

// Hilfe-Dialog
constexpr int DIALOG_HELP_W = 700;
constexpr int DIALOG_HELP_X = (800 - DIALOG_HELP_W) / 2;

// Tempo-Bezeichnungen
struct TempoRange
{
    int maxBpm;
    const char *name;
};

constexpr TempoRange TEMPO_NAMES[] = {
    {39, "Grave"},
    {59, "Largo"},
    {65, "Larghetto"},
    {75, "Adagio"},
    {107, "Andante"},
    {119, "Moderato"},
    {139, "Allegro moderato"},
    {155, "Allegro"},
    {175, "Vivace"},
    {199, "Presto"},
    {300, "Prestissimo"}
};
constexpr int NUM_TEMPO_NAMES = sizeof(TEMPO_NAMES) / sizeof(TEMPO_NAMES[0]);

// Sound-Pfade
const char *const SOUND_HI = "/sounds/metro_hi.wav";
const char *const SOUND_LO = "/sounds/metro_lo.wav";

} // namespace

// ===================================================================
// Konstruktor
// ===================================================================

/**
 * @brief Konstruktor mit Standardwerten.
 */
WorkshopTool_Metronome::WorkshopTool_Metronome()
{
    updateSubtitle();
}

// ===================================================================
// Lebenszyklus
// ===================================================================

/**
 * @brief Initialisiert Zustand und Footer-Buttons.
 */
void WorkshopTool_Metronome::onEnter(LGFX *tft)
{
    clearAll();

    if (!m_msg)
        m_msg.reset(new MessageBox(tft));

    if (!m_buttonsInitialized)
    {
        constexpr int cy = WorkshopCfg::TOOL_FOOTER_Y + WorkshopCfg::TOOL_FOOTER_H / 2;
        constexpr int x0 = WorkshopCfg::TOOL_FOOTER_X + FOOTER_BTN_MARGIN;
        constexpr int cx1 = x0 + FOOTER_BTN_W / 2;
        constexpr int cx2 = cx1 + FOOTER_BTN_W + FOOTER_BTN_GAP;
        constexpr int cx3 = cx2 + FOOTER_BTN_W + FOOTER_BTN_GAP;
        constexpr int cx4 = cx3 + FOOTER_BTN_W + FOOTER_BTN_GAP;
        constexpr int cx5 = cx4 + FOOTER_BTN_W + FOOTER_BTN_GAP;
        constexpr int cx6 = cx5 + FOOTER_BTN_W + FOOTER_BTN_GAP;

        m_btnStartStop.initButton(
            tft, cx1, cy, FOOTER_BTN_W, FOOTER_BTN_H, FOOTER_BTN_R,
            TOGGLE_INACTIVE_OUTLINE, TOGGLE_INACTIVE_FILL, TOGGLE_INACTIVE_TEXT,
            "start", nullptr);
        m_btnStartStop.setFont(LeagueGothicCond34);

        m_btnMinus.initButton(
            tft, cx2, cy, FOOTER_BTN_W, FOOTER_BTN_H, FOOTER_BTN_R,
            TOGGLE_INACTIVE_OUTLINE, TOGGLE_INACTIVE_FILL, TOGGLE_INACTIVE_TEXT,
            "-", nullptr);
        m_btnMinus.setFont(LeagueGothicCond34);

        m_btnPlus.initButton(
            tft, cx3, cy, FOOTER_BTN_W, FOOTER_BTN_H, FOOTER_BTN_R,
            TOGGLE_INACTIVE_OUTLINE, TOGGLE_INACTIVE_FILL, TOGGLE_INACTIVE_TEXT,
            "+", nullptr);
        m_btnPlus.setFont(LeagueGothicCond34);

        m_btnTap.initButton(
            tft, cx4, cy, FOOTER_BTN_W, FOOTER_BTN_H, FOOTER_BTN_R,
            TOGGLE_INACTIVE_OUTLINE, TOGGLE_INACTIVE_FILL, TOGGLE_INACTIVE_TEXT,
            "tap", nullptr);
        m_btnTap.setFont(LeagueGothicCond34);

        m_btnTimeSig.initButton(
            tft, cx5, cy, FOOTER_BTN_W, FOOTER_BTN_H, FOOTER_BTN_R,
            TOGGLE_ACTIVE_OUTLINE, TOGGLE_ACTIVE_FILL, TOGGLE_ACTIVE_TEXT,
            TIMESIG_LABELS[m_timeSigIdx], nullptr);
        m_btnTimeSig.setFont(LeagueGothicCond34);

        m_btnHelp.initButton(
            tft, cx6, cy, FOOTER_BTN_W, FOOTER_BTN_H, FOOTER_BTN_R,
            TOGGLE_INACTIVE_OUTLINE, TOGGLE_INACTIVE_FILL, TOGGLE_INACTIVE_TEXT,
            "?", nullptr);
        m_btnHelp.setFont(LeagueGothicCond34);

        m_buttonsInitialized = true;
    }

    updateStartStopButton();
    updateTimeSigButton();
}

/**
 * @brief Räumt den Werkzeugzustand auf.
 */
void WorkshopTool_Metronome::onExit()
{
    m_running = false;
    m_msg.reset();
    m_buttonsInitialized = false;
    m_touchActive = false;
}

/**
 * @brief Setzt alle Member auf definierte Ausgangswerte.
 */
void WorkshopTool_Metronome::clearAll()
{
    m_bpm = BPM_DEFAULT;
    m_timeSigIdx = 2;
    m_running = false;
    m_startMs = 0;
    m_lastBeatIndex = -1;
    m_tapCount = 0;
    m_touchActive = false;
    updateSubtitle();
}

// ===================================================================
// Hauptzeichenfunktion
// ===================================================================

/**
 * @brief Zeichnet den kompletten Metronom-Inhalt.
 */
void WorkshopTool_Metronome::draw(Gfx_Sprite *sprite, int ox, int oy, int w, int h)
{
    (void)h;

    if (m_msg && m_msg->isVisible())
    {
        m_msg->update();
        if (m_msg->isBusy())
            m_msg->completeConfirmation(true);
        return;
    }

    const int cx = ox + w / 2;
    const int beatsY = oy + BEATS_CENTER_Y;
    const int bpmY = oy + BPM_CENTER_Y;
    const int tempoY = oy + TEMPO_CENTER_Y;
    const int footerY = oy + FOOTER_TEXT_Y;

    int currentBeat = -1;
    const int beats = beatsPerMeasure();

    if (m_running)
    {
        const uint32_t now = millis();
        const uint32_t elapsedMs = now - m_startMs;
        const uint32_t beatMs = 60000UL / static_cast<uint32_t>(m_bpm);
        const uint32_t totalBeats = elapsedMs / beatMs;
        currentBeat = static_cast<int>(totalBeats % static_cast<uint32_t>(beats));

        // Beat-Erkennung für Sound-Trigger
        if (currentBeat != m_lastBeatIndex)
        {
            m_lastBeatIndex = currentBeat;
            if (owner())
            {
                if (isAccentBeat(currentBeat))
                    owner()->playSound(SOUND_HI);
                else
                    owner()->playSound(SOUND_LO);
            }
        }
    }

    drawBeatLights(sprite, cx, beatsY, currentBeat);
    drawCenter(sprite, cx, bpmY, tempoY);
    drawFooterText(sprite, cx, footerY);
}

// ===================================================================
// Zeichnen - Beat-Lichter
// ===================================================================

/**
 * @brief Zeichnet die Beat-Lichter waagrecht zentriert.
 */
void WorkshopTool_Metronome::drawBeatLights(Gfx_Sprite *sprite, int cx, int cy, int currentBeat)
{
    const int beats = beatsPerMeasure();
    const int totalW = (beats - 1) * BEAT_LIGHT_SPACING;
    const int startX = cx - totalW / 2;

    for (int i = 0; i < beats; ++i)
    {
        const int dotX = startX + i * BEAT_LIGHT_SPACING;
        const bool accent = isAccentBeat(i);
        const bool active = (i == currentBeat);

        uint16_t color;
        int r;

        if (active)
        {
            color = accent ? C_BEAT_ACCENT : C_BEAT_ACTIVE;
            r = BEAT_LIGHT_R_ACTIVE;
        }
        else
        {
            color = accent ? C_BEAT_ACCENT_DIM : C_BEAT_DIM;
            r = BEAT_LIGHT_R;
        }

        sprite->fillSmoothCircle(dotX, cy, r, color);
    }
}

// ===================================================================
// Zeichnen - Mitte
// ===================================================================

/**
 * @brief Zeichnet BPM-Wert und Tempo-Bezeichnung zentriert.
 */
void WorkshopTool_Metronome::drawCenter(Gfx_Sprite *sprite, int cx, int bpmCenterY, int tempoCenterY)
{
    char bpmBuf[8];
    stringutils::safeFormat(bpmBuf, "%d", m_bpm);

    sprite->loadFont(ComfortaaBold30);
    sprite->setTextDatum(MC_DATUM);
    sprite->setTextColor(C_BPM_TEXT, C_BG);
    sprite->drawString(bpmBuf, cx, bpmCenterY);
    sprite->unloadFont();

    sprite->loadFont(ComfortaaBold14);
    sprite->setTextDatum(MC_DATUM);
    sprite->setTextColor(C_TEMPO_NAME, C_BG);
    sprite->drawString(getTempoName(), cx, tempoCenterY);
    sprite->unloadFont();
}

// ===================================================================
// Zeichnen - Fußzeile
// ===================================================================

/**
 * @brief Zeichnet Taktart oder Tipphinweis in der Fußzeile.
 */
void WorkshopTool_Metronome::drawFooterText(Gfx_Sprite *sprite, int cx, int cy)
{
    sprite->loadFont(ComfortaaBold14);
    sprite->setTextDatum(MC_DATUM);

    if (m_running)
    {
        sprite->setTextColor(C_TIMESIG, C_BG);
        sprite->drawString(TIMESIG_LABELS[m_timeSigIdx], cx, cy);
    }
    else
    {
        sprite->setTextColor(C_STOPPED_HINT, C_BG);
        sprite->drawString(I18n::str(WorkshopKey::WORKSHOP_METRONOME_HINT_TAP_TO_START), cx, cy);
    }

    sprite->unloadFont();
}

// ===================================================================
// Touch-Handling
// ===================================================================

/**
 * @brief Verarbeitet Touch-Ereignisse im Inhaltsbereich.
 */
bool WorkshopTool_Metronome::handleTouch(int32_t tx, int32_t ty, bool touched)
{
    (void)tx;
    (void)ty;

    if (m_msg && m_msg->isVisible())
    {
        m_msg->update();
        if (m_msg->isBusy())
            m_msg->completeConfirmation(true);
        return false;
    }

    if (touched)
    {
        m_touchActive = true;
        return false;
    }

    if (!m_touchActive)
        return false;

    m_touchActive = false;

    // Tap auf die Anzeige startet/stoppt das Metronom
    m_running = !m_running;

    if (m_running)
    {
        m_startMs = millis();
        m_lastBeatIndex = -1;
    }

    updateStartStopButton();
    return true;
}

// ===================================================================
// Footer
// ===================================================================

/**
 * @brief Zeichnet die Footer-Buttons.
 */
void WorkshopTool_Metronome::drawFooter(LGFX *tft, int x, int y, int w, int h)
{
    (void)tft;
    (void)x;
    (void)y;
    (void)w;
    (void)h;

    if (!m_buttonsInitialized)
        return;

    m_btnStartStop.drawButton();
    m_btnMinus.drawButton();
    m_btnPlus.drawButton();
    m_btnTap.drawButton();
    m_btnTimeSig.drawButton();
    m_btnHelp.drawButton();
}

/**
 * @brief Verarbeitet Touch-Ereignisse auf den Footer-Buttons.
 */
bool WorkshopTool_Metronome::handleFooterTouch(int32_t tx, int32_t ty, bool touched)
{
    if (!m_buttonsInitialized)
        return false;

    if (m_msg && m_msg->isVisible())
        return false;

    const bool onStart = touched && m_btnStartStop.contains(tx, ty);
    const bool onMinus = touched && m_btnMinus.contains(tx, ty);
    const bool onPlus = touched && m_btnPlus.contains(tx, ty);
    const bool onTap = touched && m_btnTap.contains(tx, ty);
    const bool onSig = touched && m_btnTimeSig.contains(tx, ty);
    const bool onHelp = touched && m_btnHelp.contains(tx, ty);

    const bool wasStart = m_btnStartStop.isPressed();
    const bool wasMinus = m_btnMinus.isPressed();
    const bool wasPlus = m_btnPlus.isPressed();
    const bool wasTap = m_btnTap.isPressed();
    const bool wasSig = m_btnTimeSig.isPressed();
    const bool wasHelp = m_btnHelp.isPressed();

    m_btnStartStop.press(onStart);
    m_btnMinus.press(onMinus);
    m_btnPlus.press(onPlus);
    m_btnTap.press(onTap);
    m_btnTimeSig.press(onSig);
    m_btnHelp.press(onHelp);

    if (m_btnStartStop.justReleased())
    {
        m_running = !m_running;
        if (m_running)
        {
            m_startMs = millis();
            m_lastBeatIndex = -1;
        }
        updateStartStopButton();
        return true;
    }

    if (m_btnMinus.justReleased())
    {
        if (m_bpm > BPM_MIN)
        {
            m_bpm--;
            updateSubtitle();
            return true;
        }
        return false;
    }

    if (m_btnPlus.justReleased())
    {
        if (m_bpm < BPM_MAX)
        {
            m_bpm++;
            updateSubtitle();
            return true;
        }
        return false;
    }

    if (m_btnTap.justReleased())
    {
        processTapTempo();
        return true;
    }

    if (m_btnTimeSig.justReleased())
    {
        m_timeSigIdx = (m_timeSigIdx + 1) % NUM_TIME_SIGS;
        m_lastBeatIndex = -1;
        if (m_running)
            m_startMs = millis();
        updateTimeSigButton();
        return true;
    }

    if (m_btnHelp.justReleased())
    {
        showHelpDialog();
        return false;
    }

    // Visuelle Press-Rückmeldung
    if (onStart != wasStart)
        m_btnStartStop.drawButton(onStart);

    if (onMinus != wasMinus)
        m_btnMinus.drawButton(onMinus);

    if (onPlus != wasPlus)
        m_btnPlus.drawButton(onPlus);

    if (onTap != wasTap)
        m_btnTap.drawButton(onTap);

    if (onSig != wasSig)
        m_btnTimeSig.drawButton(onSig);

    if (onHelp != wasHelp)
        m_btnHelp.drawButton(onHelp);

    return false;
}

// ===================================================================
// Footer-Helfer
// ===================================================================

/**
 * @brief Aktualisiert Start/Stop-Button.
 */
void WorkshopTool_Metronome::updateStartStopButton()
{
    if (!m_buttonsInitialized)
        return;

    if (m_running)
    {
        m_btnStartStop.setLabelText("stop");
        m_btnStartStop.setOutlineColor(TOGGLE_ACTIVE_OUTLINE);
        m_btnStartStop.setFillColor(TOGGLE_ACTIVE_FILL);
        m_btnStartStop.setTextColor(TOGGLE_ACTIVE_TEXT);
    }
    else
    {
        m_btnStartStop.setLabelText("start");
        m_btnStartStop.setOutlineColor(TOGGLE_INACTIVE_OUTLINE);
        m_btnStartStop.setFillColor(TOGGLE_INACTIVE_FILL);
        m_btnStartStop.setTextColor(TOGGLE_INACTIVE_TEXT);
    }
}

/**
 * @brief Aktualisiert Taktart-Button-Label.
 */
void WorkshopTool_Metronome::updateTimeSigButton()
{
    if (!m_buttonsInitialized)
        return;

    m_btnTimeSig.setLabelText(TIMESIG_LABELS[m_timeSigIdx]);
}

/**
 * @brief Setzt alle Footer-Buttons in den Ruhezustand und zeichnet sie neu.
 */
void WorkshopTool_Metronome::resetFooterButtons()
{
    if (!m_buttonsInitialized)
        return;

    m_btnStartStop.press(false);
    m_btnMinus.press(false);
    m_btnPlus.press(false);
    m_btnTap.press(false);
    m_btnTimeSig.press(false);
    m_btnHelp.press(false);

    m_btnStartStop.drawButton();
    m_btnMinus.drawButton();
    m_btnPlus.drawButton();
    m_btnTap.drawButton();
    m_btnTimeSig.drawButton();
    m_btnHelp.drawButton();
}

// ===================================================================
// Helfer
// ===================================================================

/**
 * @brief Liefert die italienische Tempo-Bezeichnung.
 */
const char *WorkshopTool_Metronome::getTempoName() const
{
    for (int i = 0; i < NUM_TEMPO_NAMES; ++i)
    {
        if (m_bpm <= TEMPO_NAMES[i].maxBpm)
            return TEMPO_NAMES[i].name;
    }
    return "Prestissimo";
}

/**
 * @brief Liefert die Schlaganzahl pro Takt.
 */
int WorkshopTool_Metronome::beatsPerMeasure() const
{
    return TIME_SIGS[m_timeSigIdx].num;
}

/**
 * @brief Prüft ob ein Beat ein Akzent ist (Beat 0 immer, Beat 3 bei 6/8).
 */
bool WorkshopTool_Metronome::isAccentBeat(int beatIdx) const
{
    if (beatIdx == 0)
        return true;

    if (TIME_SIGS[m_timeSigIdx].num == 6 && TIME_SIGS[m_timeSigIdx].den == 8 && beatIdx == 3)
        return true;

    return false;
}

/**
 * @brief Aktualisiert den Untertitel-Puffer.
 */
void WorkshopTool_Metronome::updateSubtitle()
{
    stringutils::safeFormat(m_subtitleBuf, "%d BPM - %s", m_bpm, getTempoName());
}

/**
 * @brief Liefert den dynamischen Untertitel.
 */
const char *WorkshopTool_Metronome::getSubtitle() const
{
    return m_subtitleBuf;
}

/**
 * @brief Liefert true solange das Metronom läuft.
 */
bool WorkshopTool_Metronome::needsAnimationFrame() const
{
    return m_running;
}

// ===================================================================
// Tap-Tempo
// ===================================================================

/**
 * @brief Registriert einen Tap und berechnet BPM aus den letzten Taps.
 */
void WorkshopTool_Metronome::processTapTempo()
{
    const uint32_t now = millis();

    if (m_tapCount > 0)
    {
        const uint32_t lastTap = m_tapTimes[(m_tapCount - 1) % TAP_HISTORY];
        if ((now - lastTap) > 2000)
            m_tapCount = 0;
    }

    m_tapTimes[m_tapCount % TAP_HISTORY] = now;
    m_tapCount++;

    if (m_tapCount >= 2)
    {
        const int usable = std::min(m_tapCount, TAP_HISTORY);
        const int firstIdx = (m_tapCount - usable) % TAP_HISTORY;
        const int lastIdx = (m_tapCount - 1) % TAP_HISTORY;
        const uint32_t totalMs = m_tapTimes[lastIdx] - m_tapTimes[firstIdx];
        const uint32_t avgMs = totalMs / static_cast<uint32_t>(usable - 1);

        if (avgMs > 0)
        {
            int bpm = static_cast<int>(60000.0f / static_cast<float>(avgMs) + 0.5f);
            m_bpm = std::max(BPM_MIN, std::min(BPM_MAX, bpm));
            updateSubtitle();
        }

        if (!m_running)
        {
            m_running = true;
            m_startMs = millis();
            m_lastBeatIndex = -1;
            updateStartStopButton();
        }
    }
}

// ===================================================================
// Dialog
// ===================================================================

/**
 * @brief Öffnet die Bedienungsanleitung.
 */
void WorkshopTool_Metronome::showHelpDialog()
{
    if (!m_msg)
        return;

    I18n::format(m_helpTitleBuf, sizeof(m_helpTitleBuf), WorkshopKey::WORKSHOP_METRONOME_HELP_TITLE);
    I18n::format(m_helpTextBuf, sizeof(m_helpTextBuf), WorkshopKey::WORKSHOP_METRONOME_HELP_TEXT);
    I18n::format(m_dialogCloseBuf, sizeof(m_dialogCloseBuf), WorkshopKey::WORKSHOP_DIALOG_CLOSE);

    m_msg->setTitle(m_helpTitleBuf);
    m_msg->setMessage(m_helpTextBuf);
    m_msg->setButtonLabel(m_dialogCloseBuf);
    m_msg->setTimeout(0);
    m_msg->setAutoBounds(DIALOG_HELP_X, 0, DIALOG_HELP_W, 480);
    m_msg->setCallback([this](bool)
    {
        resetFooterButtons();
        if (owner())
            owner()->setWorkContentPaused(false);
    });
    m_msg->start();

    if (owner())
        owner()->setWorkContentPaused(true);
}

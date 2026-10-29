#include "Clocks.h"

#include <cmath>
#include <utility>

#include "AppState.h"
#include "DS1307_RTC.h"
#include "Gfx.h"
#include "Logger.h"
#include "StringUtils.h"

/**
 * @brief Zeichnet eine analoge Uhr mit optionalem Sleep-Timer
 */
void Clocks::drawAnalogClock(Gfx_Sprite *sprite, DS1307_RTC *rtc, int &lastDrawnSecond, int32_t x, int32_t y, int remainingMinutes, int remainingSeconds)
{
    if (APP_STATE.getBlockGFX() || !sprite || !rtc)
        return;

    constexpr float RADIUS = 30.0f;
    constexpr float HOUR_HAND_RATIO = 0.5f;
    constexpr float MINUTE_HAND_RATIO = 0.7f;
    constexpr float SECOND_HAND_RATIO = 0.9f;
    constexpr float HOUR_MARK_LENGTH = 6.0f;
    constexpr float HAND_WIDTH = 2.0f;
    constexpr float SECOND_HAND_WIDTH = 1.5f;
    constexpr float CENTER_CIRCLE_RADIUS = 4.0f;
    constexpr int MAX_TIMER_MINUTES = 60;
    constexpr int DEGREES_PER_HOUR = 30;
    constexpr int DEGREES_PER_MINUTE = 6;
    constexpr int BUFFER_SIZE = 4;
    constexpr float DEG2RAD = 0.0174532925f;

    constexpr uint16_t COLOR_BACKGROUND = 0x1082;
    constexpr uint16_t COLOR_TIMER = 0x8010;
    constexpr uint16_t COLOR_TRACK = 0x39E8;
    constexpr uint16_t COLOR_TEXT = 0xdb9a;
    constexpr uint16_t COLOR_MARKS = 0x528a;
    constexpr uint16_t COLOR_HANDS = TFT_SILVER;
    constexpr uint16_t COLOR_SECOND_HAND = TFT_RED;

    constexpr int SPRITE_WIDTH = 70;
    constexpr int SPRITE_HEIGHT = 70;

    const float cx = SPRITE_WIDTH / 2.0f;
    const float cy = SPRITE_HEIGHT / 2.0f;

    constexpr uint8_t HOUR_MARKS[] = {1, 2, 4, 5, 7, 8, 10, 11};

    // Parameter-Validierung
    if (remainingMinutes < -1 || remainingMinutes > MAX_TIMER_MINUTES)
        remainingMinutes = -1;

    if (remainingSeconds < 0 || remainingSeconds >= 60)
        remainingSeconds = 0;

    const bool timeValid = DS1307_RTC::isSystemTimeValid();
    struct tm localTime = DS1307_RTC::getCurrentLocalTime();

    // Nur neu zeichnen, wenn sich die Sekunde geändert hat
    if (lastDrawnSecond == localTime.tm_sec)
        return;

    lastDrawnSecond = localTime.tm_sec;

    const float hLen = RADIUS * HOUR_HAND_RATIO;
    const float mLen = RADIUS * MINUTE_HAND_RATIO;
    const float sLen = RADIUS * SECOND_HAND_RATIO;

    sprite->clear(COLOR_BACKGROUND);

    auto getCoord = [](float cx, float cy, float angleDeg, float length) -> std::pair<float, float>
    {
        const float rad = (angleDeg - 90.0f) * DEG2RAD;
        return {cx + cosf(rad) * length, cy + sinf(rad) * length};
    };

    // Sleep-Timer Ring-Gauge
    if (remainingMinutes != -1)
    {
        sprite->drawSmoothGauge(cx, cy,
                                34.0f, 3.5f,
                                0, 360,
                                0, MAX_TIMER_MINUTES, remainingMinutes,
                                COLOR_TIMER,
                                COLOR_TRACK,
                                false,
                                COLOR_BACKGROUND);

        sprite->setTextColor(COLOR_TEXT);

        char remainingMinutesBuffer[BUFFER_SIZE];
        stringutils::safeFormat(remainingMinutesBuffer, BUFFER_SIZE, "%02d", remainingMinutes);
        sprite->setTextDatum(top_right);
        sprite->drawString(remainingMinutesBuffer, SPRITE_WIDTH, 0);

        char remainingSecondsBuffer[BUFFER_SIZE];
        stringutils::safeFormat(remainingSecondsBuffer, BUFFER_SIZE, "%02d", remainingSeconds);
        sprite->setTextDatum(bottom_right);
        sprite->drawString(remainingSecondsBuffer, SPRITE_WIDTH, SPRITE_HEIGHT);
    }

    // Stunden-Markierungen
    for (uint8_t hourMark : HOUR_MARKS)
    {
        const float angleDeg = hourMark * DEGREES_PER_HOUR;
        const float angleRad = angleDeg * DEG2RAD;
        const float sinAngle = sinf(angleRad);
        const float cosAngle = cosf(angleRad);

        const float x1 = cx + sinAngle * RADIUS;
        const float y1 = cy - cosAngle * RADIUS;
        const float x2 = cx + sinAngle * (RADIUS - HOUR_MARK_LENGTH);
        const float y2 = cy - cosAngle * (RADIUS - HOUR_MARK_LENGTH);

        sprite->drawWideLine(roundf(x1), roundf(y1), roundf(x2), roundf(y2), 1.0f, COLOR_MARKS);
    }

    // Zahlen 12, 3, 6, 9
    sprite->setTextColor(COLOR_HANDS);
    sprite->setTextDatum(middle_center);
    sprite->drawString("12", cx, cy - RADIUS + 5);
    sprite->drawString("3", cx + RADIUS - 3, cy);
    sprite->drawString("6", cx, cy + RADIUS - 3);
    sprite->drawString("9", cx - RADIUS + 3, cy);

    // Zeiger nur zeichnen, wenn die Systemzeit plausibel ist. Ohne gültige Zeit bleibt das Zifferblatt leer und der Anwender sieht sofort, dass aktuell keine Uhrzeit verfügbar ist. Der Sleep-Timer-Gauge oben bleibt davon unberührt.
    if (timeValid)
    {
        const float hourAngle = (localTime.tm_hour % 12 + localTime.tm_min / 60.0f) * DEGREES_PER_HOUR;
        const float minuteAngle = (localTime.tm_min + localTime.tm_sec / 60.0f) * DEGREES_PER_MINUTE;
        const float secondAngle = localTime.tm_sec * DEGREES_PER_MINUTE;

        auto hourPos = getCoord(cx, cy, hourAngle, hLen);
        sprite->drawWideLine(cx, cy, hourPos.first, hourPos.second, HAND_WIDTH, COLOR_HANDS);

        auto minPos = getCoord(cx, cy, minuteAngle, mLen);
        sprite->drawWideLine(cx, cy, minPos.first, minPos.second, HAND_WIDTH, COLOR_HANDS);

        auto secPos = getCoord(cx, cy, secondAngle, sLen);
        sprite->drawWedgeLine(cx, cy, secPos.first, secPos.second, SECOND_HAND_WIDTH, 0.7f, COLOR_SECOND_HAND);

        sprite->fillSmoothCircle(cx, cy, CENTER_CIRCLE_RADIUS, COLOR_SECOND_HAND);
    }

    sprite->pushSprite(x, y);
}

/**
 * @brief Zeichnet eine digitale Uhr mit optionalem Sleep-Timer
 */
void Clocks::drawDigitalClock(Gfx_Sprite *sprite, DS1307_RTC *rtc, int &lastDrawnSecond, int32_t x, int32_t y, int remainingMinutes, int remainingSeconds)
{
    if (APP_STATE.getBlockGFX() || !sprite || !rtc)
        return;

    constexpr int MAX_TIMER_MINUTES = 60;

    constexpr uint16_t COLOR_BACKGROUND = TFT_BLACK;
    constexpr uint16_t COLOR_TIMER = 0x8010;
    constexpr uint16_t COLOR_TRACK = 0x2104;
    constexpr uint16_t COLOR_TEXT = TFT_DARKGREY;

    const int SPRITE_WIDTH = sprite->width();
    const int SPRITE_HEIGHT = sprite->height();
    const float cx = SPRITE_WIDTH - 26;
    const float cy = SPRITE_HEIGHT / 2.0f;

    // Parameter-Validierung
    if (remainingMinutes < -1 || remainingMinutes > MAX_TIMER_MINUTES)
        remainingMinutes = -1;

    if (remainingSeconds < 0 || remainingSeconds >= 60)
        remainingSeconds = 0;

    const bool timeValid = DS1307_RTC::isSystemTimeValid();
    struct tm localTime = DS1307_RTC::getCurrentLocalTime();

    // Nur neu zeichnen, wenn sich die Sekunde geändert hat
    if (lastDrawnSecond == localTime.tm_sec)
        return;

    lastDrawnSecond = localTime.tm_sec;

    // Digitaluhr mit blinkendem Doppelpunkt
    sprite->setFont(&fonts::Font7);
    sprite->setTextSize(.75f);
    sprite->setTextDatum(middle_left);

    // Hintergrund "88:88" in COLOR_TRACK - zeigt die inaktiven 7-Segmente. Wird auch bei ungültiger Zeit dargestellt, damit die Uhrform sichtbar bleibt.
    sprite->setTextColor(COLOR_TRACK);
    sprite->drawString("88:88", 0, cy);

    // Uhrzeit und Doppelpunkt nur bei plausibler Systemzeit
    if (timeValid)
    {
        // Zahlen ohne Doppelpunkt in COLOR_TEXT
        char timeBuffer[6];
        stringutils::safeFormat(timeBuffer, sizeof(timeBuffer), "%02d %02d", localTime.tm_hour, localTime.tm_min);
        sprite->setTextColor(COLOR_TEXT);
        sprite->drawString(timeBuffer, 0, cy);

        // Doppelpunkt separat in wechselnder Farbe (EIN=COLOR_TEXT, AUS=COLOR_TRACK)
        const uint16_t colonColor = (localTime.tm_sec % 2 == 0) ? COLOR_TEXT : COLOR_TRACK;
        sprite->setTextColor(colonColor);
        const int16_t colonX = sprite->textWidth("88");
        sprite->drawString(":", colonX, cy);
    }

    // Sleep-Timer Ring-Gauge
    if (remainingMinutes != -1)
    {
        sprite->drawSmoothGauge(cx, cy,
                                cy - 6.0f, 4.5f,
                                0, 360,
                                0, MAX_TIMER_MINUTES, remainingMinutes,
                                COLOR_TIMER,
                                COLOR_TRACK,
                                true,
                                COLOR_BACKGROUND);

        // Statische Variablen für Font-Maße (werden nur einmal berechnet)
        static int timer_text_max_width = 0;
        static int timer_font_height = 0;
        sprite->setFont(&fonts::DejaVu9);
        sprite->setTextSize(1.0f);

        if (timer_text_max_width == 0)
        {
            timer_text_max_width = sprite->textWidth("88");
            timer_font_height = sprite->fontHeight();
        }

        // Timer-Minuten: Gezielt den Bereich löschen, dann zeichnen
        sprite->fillRect(SPRITE_WIDTH - timer_text_max_width, 0, timer_text_max_width, timer_font_height, COLOR_BACKGROUND);
        char remainingMinutesBuffer[4];
        stringutils::safeFormat(remainingMinutesBuffer, 4, "%02d", remainingMinutes);
        sprite->setTextDatum(top_right);
        sprite->setTextColor(0xdb9a);
        sprite->drawString(remainingMinutesBuffer, SPRITE_WIDTH, 0);

        // Timer-Sekunden: Gezielt den Bereich löschen, dann zeichnen
        sprite->fillRect(SPRITE_WIDTH - timer_text_max_width, SPRITE_HEIGHT - timer_font_height, timer_text_max_width, timer_font_height, COLOR_BACKGROUND);
        char remainingSecondsBuffer[4];
        stringutils::safeFormat(remainingSecondsBuffer, 4, "%02d", remainingSeconds);
        sprite->setTextDatum(bottom_right);
        sprite->drawString(remainingSecondsBuffer, SPRITE_WIDTH, SPRITE_HEIGHT);
    }

    sprite->pushSprite(x, y);
}

/**
 * @brief Zeichnet eine einfache digitale Uhr ohne Timer-Features
 */
void Clocks::drawDigitalClockPlain(Gfx_Sprite *sprite, DS1307_RTC *rtc, int &lastDrawnSecond, int32_t x, int32_t y, bool showSeconds, bool blinkingColons)
{
    if (APP_STATE.getBlockGFX() || !sprite || !rtc)
        return;

    constexpr uint16_t COLOR_TRACK = 0x2104;
    constexpr uint16_t COLOR_TEXT = TFT_DARKGREY;
    const float cy = sprite->height() / 2.0f;

    const bool timeValid = DS1307_RTC::isSystemTimeValid();
    struct tm localTime = DS1307_RTC::getCurrentLocalTime();

    // Nur neu zeichnen, wenn sich die Sekunde geändert hat
    if (lastDrawnSecond == localTime.tm_sec)
        return;

    lastDrawnSecond = localTime.tm_sec;

    sprite->setFont(&fonts::Font7);
    sprite->setTextSize(.75f);
    sprite->setTextDatum(middle_left);

    if (showSeconds)
    {
        // Mit Sekunden: HH:MM:SS - Hintergrund "88:88:88" in COLOR_TRACK. Wird auch bei ungültiger Zeit gezeichnet, damit die Uhrform sichtbar bleibt.
        sprite->setTextColor(COLOR_TRACK);
        sprite->drawString("88:88:88", 0, cy);

        // Zahlen und Doppelpunkte nur bei plausibler Systemzeit
        if (timeValid)
        {
            // Zahlen ohne Doppelpunkte in COLOR_TEXT
            char timeBuffer[9];
            stringutils::safeFormat(timeBuffer, sizeof(timeBuffer), "%02d %02d %02d", localTime.tm_hour, localTime.tm_min, localTime.tm_sec);
            sprite->setTextColor(COLOR_TEXT);
            sprite->drawString(timeBuffer, 0, cy);

            // Doppelpunkte separat zeichnen (blinkend oder statisch je nach Parameter)
            const uint16_t colonColor = (blinkingColons && (localTime.tm_sec % 2 == 0)) ? COLOR_TEXT : (blinkingColons ? COLOR_TRACK : COLOR_TEXT);
            sprite->setTextColor(colonColor);

            // Erste Doppelpunkt-Position (nach HH)
            const int16_t colonX1 = sprite->textWidth("88");
            sprite->drawString(":", colonX1, cy);

            // Zweite Doppelpunkt-Position (nach MM)
            const int16_t colonX2 = sprite->textWidth("88 88");
            sprite->drawString(":", colonX2, cy);
        }
    }
    else
    {
        // Ohne Sekunden: HH:MM - Hintergrund "88:88" in COLOR_TRACK. Wird auch bei ungültiger Zeit gezeichnet, damit die Uhrform sichtbar bleibt.
        sprite->setTextColor(COLOR_TRACK);
        sprite->drawString("88:88", 0, cy);

        // Zahlen und Doppelpunkt nur bei plausibler Systemzeit
        if (timeValid)
        {
            // Zahlen ohne Doppelpunkt in COLOR_TEXT
            char timeBuffer[6];
            stringutils::safeFormat(timeBuffer, sizeof(timeBuffer), "%02d %02d", localTime.tm_hour, localTime.tm_min);
            sprite->setTextColor(COLOR_TEXT);
            sprite->drawString(timeBuffer, 0, cy);

            // Doppelpunkt separat zeichnen (blinkend oder statisch je nach Parameter)
            const uint16_t colonColor = (blinkingColons && (localTime.tm_sec % 2 == 0)) ? COLOR_TEXT : (blinkingColons ? COLOR_TRACK : COLOR_TEXT);
            sprite->setTextColor(colonColor);
            const int16_t colonX = sprite->textWidth("88");
            sprite->drawString(":", colonX, cy);
        }
    }

    sprite->pushSprite(x, y);
}

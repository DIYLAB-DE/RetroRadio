#include "AudioSliderTouch.h"

/** @brief Ermittelt die nächste Position des berührten Reglers. */
AudioSliderTouch::Target AudioSliderTouch::update(bool touched, int x, int y, const int8_t (&positions)[COUNT], uint32_t now)
{
    constexpr uint32_t HOLD_MS = 800;
    constexpr int MOVE_TOLERANCE = 6;

    if (!touched)
    {
        const bool tap = m_phase == Phase::Holding && m_startY >= TOP && m_startY <= BOTTOM;
        m_phase = Phase::Idle;
        return tap ? Target{m_slider, m_startY} : Target{};
    }

    if (m_phase == Phase::Consumed)
        return {};

    int index = -1;
    for (int i = 0; i < COUNT; ++i)
    {
        if (x >= LEFT[i] && x <= LEFT[i] + WIDTH)
        {
            index = i;
            break;
        }
    }

    if (m_phase == Phase::Idle)
    {
        m_phase = Phase::Sliding;
        if (index >= 0)
        {
            const int dx = x - (LEFT[index] + CENTER_X);
            const int dy = y - positions[index];
            if (dx * dx + dy * dy <= RADIUS * RADIUS)
            {
                m_slider = index;
                m_startX = x;
                m_startY = y;
                m_startTime = now;
                m_phase = Phase::Holding;
            }
        }
    }

    if (m_phase == Phase::Holding)
    {
        const int dx = x - m_startX;
        const int dy = y - m_startY;
        if (dx * dx + dy * dy <= MOVE_TOLERANCE * MOVE_TOLERANCE)
        {
            if (now - m_startTime < HOLD_MS)
                return {};

            m_phase = Phase::Consumed;
            return {m_slider, NEUTRAL, true};
        }
        m_phase = Phase::Sliding;
    }

    if (index < 0 || y < TOP || y > BOTTOM)
        return {};

    return {index, y};
}

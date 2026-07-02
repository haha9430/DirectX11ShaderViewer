#include "core/Timer.h"

namespace dxsv
{
    void Timer::reset()
    {
        m_startTime = Clock::now();
        m_previousTime = m_startTime;
        m_deltaSeconds = 0.0f;
        m_totalSeconds = 0.0f;
    }

    void Timer::tick()
    {
        const auto currentTime = Clock::now();
        m_deltaSeconds = std::chrono::duration<float>(currentTime - m_previousTime).count();
        m_totalSeconds = std::chrono::duration<float>(currentTime - m_startTime).count();
        m_previousTime = currentTime;
    }
}

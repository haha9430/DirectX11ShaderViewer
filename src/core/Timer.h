#pragma once

#include <chrono>

namespace dxsv
{
    class Timer
    {
    public:
        void reset();
        void tick();

        float deltaSeconds() const { return m_deltaSeconds; }
        float totalSeconds() const { return m_totalSeconds; }

    private:
        using Clock = std::chrono::steady_clock;

        Clock::time_point m_startTime = Clock::now();
        Clock::time_point m_previousTime = m_startTime;
        float m_deltaSeconds = 0.0f;
        float m_totalSeconds = 0.0f;
    };
}

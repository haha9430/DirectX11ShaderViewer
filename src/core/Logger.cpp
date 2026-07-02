#include "core/Logger.h"

#include <Windows.h>

namespace dxsv
{
    void Logger::info(const std::wstring& message)
    {
        OutputDebugStringW((L"[Info] " + message + L"\n").c_str());
    }

    void Logger::error(const std::wstring& message)
    {
        OutputDebugStringW((L"[Error] " + message + L"\n").c_str());
    }
}

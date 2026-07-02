#pragma once

#include <string>

namespace dxsv
{
    class Logger
    {
    public:
        static void info(const std::wstring& message);
        static void error(const std::wstring& message);
    };
}

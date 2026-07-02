#pragma once

#include <filesystem>
#include <string>

namespace dxsv
{
    class FileUtils
    {
    public:
        static bool exists(const std::filesystem::path& path);
        static std::wstring readTextFile(const std::filesystem::path& path);
    };
}

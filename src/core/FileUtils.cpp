#include "core/FileUtils.h"

#include <fstream>
#include <sstream>

namespace dxsv
{
    bool FileUtils::exists(const std::filesystem::path& path)
    {
        return std::filesystem::exists(path);
    }

    std::wstring FileUtils::readTextFile(const std::filesystem::path& path)
    {
        std::wifstream file(path);
        std::wstringstream buffer;
        buffer << file.rdbuf();
        return buffer.str();
    }
}

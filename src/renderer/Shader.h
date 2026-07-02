#pragma once

#include <filesystem>
#include <string>

namespace dxsv
{
    class Shader
    {
    public:
        Shader() = default;
        explicit Shader(std::filesystem::path path);

        const std::filesystem::path& path() const { return m_path; }
        const std::wstring& name() const { return m_name; }

    private:
        std::filesystem::path m_path;
        std::wstring m_name;
    };
}

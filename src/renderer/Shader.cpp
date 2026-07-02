#include "renderer/Shader.h"

namespace dxsv
{
    Shader::Shader(std::filesystem::path path)
        : m_path(std::move(path)), m_name(m_path.filename().wstring())
    {
    }
}

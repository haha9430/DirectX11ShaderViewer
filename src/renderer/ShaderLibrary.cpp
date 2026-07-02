#include "renderer/ShaderLibrary.h"

namespace dxsv
{
    void ShaderLibrary::add(const std::string& key, Shader shader)
    {
        m_shaders[key] = std::move(shader);
    }

    const Shader* ShaderLibrary::find(const std::string& key) const
    {
        const auto it = m_shaders.find(key);
        return it == m_shaders.end() ? nullptr : &it->second;
    }
}

#pragma once

#include "renderer/Shader.h"

#include <map>

namespace dxsv
{
    class ShaderLibrary
    {
    public:
        void add(const std::string& key, Shader shader);
        const Shader* find(const std::string& key) const;

    private:
        std::map<std::string, Shader> m_shaders;
    };
}

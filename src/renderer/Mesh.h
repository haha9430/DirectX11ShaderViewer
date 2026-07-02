#pragma once

#include "core/MathTypes.h"

#include <vector>

namespace dxsv
{
    struct Vertex
    {
        Vector3 position;
        Vector3 normal;
        Vector3 tangent;
        Vector4 color;
        Vector2 texcoord;
    };

    class Mesh
    {
    public:
        static Mesh createCube();
        static Mesh createSphere(float radius = 1.0f, unsigned int sliceCount = 64, unsigned int stackCount = 32);

        const std::vector<Vertex>& vertices() const { return m_vertices; }
        const std::vector<unsigned int>& indices() const { return m_indices; }

    private:
        std::vector<Vertex> m_vertices;
        std::vector<unsigned int> m_indices;
    };
}

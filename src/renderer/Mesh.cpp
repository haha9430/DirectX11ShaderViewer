#include "renderer/Mesh.h"

#include <algorithm>
#include <cmath>

namespace dxsv
{
    namespace
    {
        constexpr float kPi = 3.14159265359f;

        Vector3 normalize(const Vector3& value)
        {
            const float length = std::sqrt(value.x * value.x + value.y * value.y + value.z * value.z);
            if (length <= 0.00001f)
            {
                return { 0.0f, 1.0f, 0.0f };
            }

            return { value.x / length, value.y / length, value.z / length };
        }
    }

    Mesh Mesh::createCube()
    {
        Mesh mesh;

        auto addFace = [&mesh](Vector3 a, Vector3 b, Vector3 c, Vector3 d, Vector3 normal, Vector3 tangent, Vector4 color)
        {
            const unsigned int start = static_cast<unsigned int>(mesh.m_vertices.size());
            mesh.m_vertices.push_back({ a, normal, tangent, color, {0.0f, 1.0f} });
            mesh.m_vertices.push_back({ b, normal, tangent, color, {0.0f, 0.0f} });
            mesh.m_vertices.push_back({ c, normal, tangent, color, {1.0f, 0.0f} });
            mesh.m_vertices.push_back({ d, normal, tangent, color, {1.0f, 1.0f} });

            mesh.m_indices.insert(mesh.m_indices.end(), {
                start + 0, start + 1, start + 2,
                start + 0, start + 2, start + 3,
            });
        };

        addFace(
            {-1.0f, -1.0f, -1.0f}, {-1.0f,  1.0f, -1.0f}, { 1.0f,  1.0f, -1.0f}, { 1.0f, -1.0f, -1.0f},
            { 0.0f,  0.0f, -1.0f}, { 1.0f,  0.0f,  0.0f}, {0.95f, 0.95f, 0.95f, 1.0f});
        addFace(
            { 1.0f, -1.0f,  1.0f}, { 1.0f,  1.0f,  1.0f}, {-1.0f,  1.0f,  1.0f}, {-1.0f, -1.0f,  1.0f},
            { 0.0f,  0.0f,  1.0f}, {-1.0f,  0.0f,  0.0f}, {0.95f, 0.95f, 0.95f, 1.0f});
        addFace(
            {-1.0f, -1.0f,  1.0f}, {-1.0f,  1.0f,  1.0f}, {-1.0f,  1.0f, -1.0f}, {-1.0f, -1.0f, -1.0f},
            {-1.0f,  0.0f,  0.0f}, { 0.0f,  0.0f, -1.0f}, {0.95f, 0.95f, 0.95f, 1.0f});
        addFace(
            { 1.0f, -1.0f, -1.0f}, { 1.0f,  1.0f, -1.0f}, { 1.0f,  1.0f,  1.0f}, { 1.0f, -1.0f,  1.0f},
            { 1.0f,  0.0f,  0.0f}, { 0.0f,  0.0f,  1.0f}, {0.95f, 0.95f, 0.95f, 1.0f});
        addFace(
            {-1.0f,  1.0f, -1.0f}, {-1.0f,  1.0f,  1.0f}, { 1.0f,  1.0f,  1.0f}, { 1.0f,  1.0f, -1.0f},
            { 0.0f,  1.0f,  0.0f}, { 1.0f,  0.0f,  0.0f}, {0.95f, 0.95f, 0.95f, 1.0f});
        addFace(
            {-1.0f, -1.0f,  1.0f}, {-1.0f, -1.0f, -1.0f}, { 1.0f, -1.0f, -1.0f}, { 1.0f, -1.0f,  1.0f},
            { 0.0f, -1.0f,  0.0f}, { 1.0f,  0.0f,  0.0f}, {0.95f, 0.95f, 0.95f, 1.0f});

        return mesh;
    }

    Mesh Mesh::createSphere(float radius, unsigned int sliceCount, unsigned int stackCount)
    {
        Mesh mesh;
        sliceCount = std::max(3u, sliceCount);
        stackCount = std::max(2u, stackCount);

        const Vector4 color{ 1.0f, 1.0f, 1.0f, 1.0f };

        for (unsigned int stack = 0; stack <= stackCount; ++stack)
        {
            const float v = static_cast<float>(stack) / static_cast<float>(stackCount);
            const float phi = v * kPi;
            const float sinPhi = std::sin(phi);
            const float cosPhi = std::cos(phi);

            for (unsigned int slice = 0; slice <= sliceCount; ++slice)
            {
                const float u = static_cast<float>(slice) / static_cast<float>(sliceCount);
                const float theta = u * kPi * 2.0f;
                const float sinTheta = std::sin(theta);
                const float cosTheta = std::cos(theta);

                const Vector3 normal = normalize({
                    sinPhi * cosTheta,
                    cosPhi,
                    sinPhi * sinTheta,
                });

                Vector3 tangent = {
                    -sinTheta,
                    0.0f,
                    cosTheta,
                };
                tangent = normalize(tangent);

                const Vector3 position = {
                    normal.x * radius,
                    normal.y * radius,
                    normal.z * radius,
                };

                mesh.m_vertices.push_back({
                    position,
                    normal,
                    tangent,
                    color,
                    { u, v },
                });
            }
        }

        const unsigned int rowVertexCount = sliceCount + 1;
        for (unsigned int stack = 0; stack < stackCount; ++stack)
        {
            for (unsigned int slice = 0; slice < sliceCount; ++slice)
            {
                const unsigned int a = stack * rowVertexCount + slice;
                const unsigned int b = (stack + 1) * rowVertexCount + slice;
                const unsigned int c = (stack + 1) * rowVertexCount + slice + 1;
                const unsigned int d = stack * rowVertexCount + slice + 1;

                mesh.m_indices.insert(mesh.m_indices.end(), {
                    a, b, d,
                    d, b, c,
                });
            }
        }

        return mesh;
    }
}

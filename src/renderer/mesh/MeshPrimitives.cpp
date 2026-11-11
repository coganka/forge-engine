#include "MeshPrimitives.hpp"
#include <cmath>
#include <numbers>
#include <stdexcept>
#include <vector>


std::shared_ptr<Mesh> MeshPrimitives::plane(MTL::Device* device, float size)
{
    if (!std::isfinite(size) || size <= 0)
    {
        throw std::invalid_argument("Plane size must be positive");
    }

    const float h = size * 0.5f;
    const Vertex vertices[] = {
        {-h, 0, -h, 1, 1, 1, 0, 0, 0, 1, 0},
        {h, 0, -h, 1, 1, 1, 1, 0, 0, 1, 0},
        {h, 0, h, 1, 1, 1, 1, 1, 0, 1, 0},
        {-h, 0, h, 1, 1, 1, 0, 1, 0, 1, 0}
    };
    const uint32_t indices[] = {0, 2, 1, 0, 3, 2};

    return std::make_shared<Mesh>(device, vertices, indices);
}


std::shared_ptr<Mesh> MeshPrimitives::sphere(MTL::Device* device)
{
    constexpr uint32_t rings = 12;
    constexpr uint32_t sectors = 24;
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;

    for (uint32_t y = 0; y <= rings; ++y)
    {
        const float v = float(y) / rings;
        const float phi = v * std::numbers::pi_v<float>;

        for (uint32_t x = 0; x <= sectors; ++x)
        {
            const float u = float(x) / sectors;
            const float theta = u * 2 * std::numbers::pi_v<float>;
            const float nx = std::sin(phi) * std::cos(theta);
            const float ny = std::cos(phi);
            const float nz = std::sin(phi) * std::sin(theta);
            vertices.push_back({nx, ny, nz, 1, 1, 1, u, v, nx, ny, nz});

            if (y < rings && x < sectors)
            {
                const uint32_t a = y * (sectors + 1) + x;
                const uint32_t b = a + sectors + 1;
                indices.insert(indices.end(), {a, b, a + 1, a + 1, b, b + 1});
            }
        }
    }

    return std::make_shared<Mesh>(device, vertices, indices);
}

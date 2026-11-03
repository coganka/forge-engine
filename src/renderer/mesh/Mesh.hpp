#pragma once

#include <Metal/Metal.hpp>
#include <span>
#include <cstdint>

#include "renderer/Types.hpp"


class Mesh
{
public:
    Mesh(
        MTL::Device* device,
        std::span<const Vertex> vertices,
        std::span<const uint32_t> indices
    );

    ~Mesh();

    Mesh(const Mesh&) = delete;
    Mesh& operator=(const Mesh&) = delete;

    MTL::Buffer* vertexBuffer() const { return vertexBuffer_; }
    MTL::Buffer* indexBuffer() const { return indexBuffer_; }

    NS::UInteger indexCount() const { return indexCount_; }

private:
    MTL::Buffer* vertexBuffer_ = nullptr;
    MTL::Buffer* indexBuffer_ = nullptr;

    NS::UInteger indexCount_ = 0;
};
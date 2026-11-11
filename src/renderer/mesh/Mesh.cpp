#include "Mesh.hpp"
#include <stdexcept>


Mesh::Mesh(
    MTL::Device* device,
    std::span<const Vertex> vertices,
    std::span<const uint32_t> indices
)
{
    if (!device || vertices.empty() || indices.empty())
    {
        throw std::invalid_argument("Invalid mesh data");
    }

    vertexBuffer_ = device->newBuffer(
        vertices.data(), vertices.size_bytes(), MTL::ResourceStorageModeShared
    );

    indexBuffer_ = device->newBuffer(
        indices.data(), indices.size_bytes(), MTL::ResourceStorageModeShared
    );

    if (!vertexBuffer_ || !indexBuffer_)
    {
        if (vertexBuffer_)
        {
            vertexBuffer_->release();
        }

        if (indexBuffer_)
        {
            indexBuffer_->release();
        }

        vertexBuffer_ = nullptr;
        indexBuffer_ = nullptr;

        throw std::runtime_error("failed to create mesh buffer");
    }

    indexCount_ = indices.size();
}


Mesh::~Mesh()
{
    if (vertexBuffer_)
    {
        vertexBuffer_->release();
    }

    if (indexBuffer_)
    {
        indexBuffer_->release();
    }
}

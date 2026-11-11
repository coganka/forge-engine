#pragma once
#include <Metal/Metal.hpp>
#include <string>


class Texture
{
public:
    Texture(
        MTL::Device* device,
        MTL::CommandQueue* queue,
        const std::string& path,
        bool srgb
    );
    Texture(MTL::Device* device, uint32_t rgba, bool srgb = false);
    ~Texture();
    Texture(const Texture&) = delete;
    Texture& operator=(const Texture&) = delete;

    MTL::Texture* gpu() const
    {
        return texture_;
    }

private:
    MTL::Texture* texture_ = nullptr;
};

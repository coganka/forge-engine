#pragma once

#include <memory>
#include <string>
#include <unordered_map>
#include "renderer/Texture.hpp"
#include "renderer/mesh/Mesh.hpp"


class AssetLibrary
{
public:
    explicit AssetLibrary(MTL::Device* device);
    ~AssetLibrary();
    AssetLibrary(const AssetLibrary&) = delete;
    AssetLibrary& operator=(const AssetLibrary&) = delete;

    std::shared_ptr<Texture> texture(const std::string& path, bool srgb);
    std::shared_ptr<Mesh> mesh(const std::string& path);

private:
    MTL::Device* device_;
    MTL::CommandQueue* uploadQueue_;
    std::unordered_map<std::string, std::shared_ptr<Texture>> textures_;
    std::unordered_map<std::string, std::shared_ptr<Mesh>> meshes_;
};

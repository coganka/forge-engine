#include "AssetLibrary.hpp"
#include "ObjLoader.hpp"
#include <filesystem>
#include <stdexcept>


AssetLibrary::AssetLibrary(MTL::Device* device)
    : device_(device), uploadQueue_(device->newCommandQueue())
{
    if (!uploadQueue_)
    {
        throw std::runtime_error("Cannot create asset upload queue");
    }
}


AssetLibrary::~AssetLibrary()
{
    uploadQueue_->release();
}


std::shared_ptr<Texture> AssetLibrary::texture(const std::string& path, bool srgb)
{
    const auto normalized = std::filesystem::path(path).lexically_normal().string();
    const auto key = normalized + (srgb ? ":srgb" : ":linear");

    if (auto found = textures_.find(key); found != textures_.end())
    {
        return found->second;
    }

    auto resource = std::make_shared<Texture>(device_, uploadQueue_, normalized, srgb);
    textures_.emplace(key, resource);

    return resource;
}


std::shared_ptr<Mesh> AssetLibrary::mesh(const std::string& path)
{
    const auto key = std::filesystem::path(path).lexically_normal().string();

    if (auto found = meshes_.find(key); found != meshes_.end())
    {
        return found->second;
    }

    const auto data = ObjLoader::load(key);
    auto resource = std::make_shared<Mesh>(device_, data.vertices, data.indices);
    meshes_.emplace(key, resource);

    return resource;
}

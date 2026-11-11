#pragma once

#include <cstdint>
#include <string>
#include <vector>


#include "renderer/Types.hpp"


struct MeshData
{
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;
};


class ObjLoader
{
public:
    static MeshData load(const std::string& path);
};

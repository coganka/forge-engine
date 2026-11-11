#pragma once

#include <memory>
#include <string>


#include "math/Transform.hpp"
#include "renderer/Material.hpp"
#include "renderer/mesh/Mesh.hpp"


struct Entity
{
    std::string name;
    Transform transform;
    Material material;
    std::shared_ptr<Mesh> mesh;
};

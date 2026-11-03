#pragma once

#include <memory>

#include "math/Transform.hpp"
#include "renderer/mesh/Mesh.hpp"


struct Entity
{
    Transform transform;
    std::shared_ptr<Mesh> mesh;
};
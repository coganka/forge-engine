#pragma once

#include "Mesh.hpp"
#include <memory>


namespace MeshPrimitives
{
    std::shared_ptr<Mesh> plane(MTL::Device* device, float size);
    std::shared_ptr<Mesh> sphere(MTL::Device* device);
}

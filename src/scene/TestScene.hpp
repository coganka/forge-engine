#pragma once
#include "Scene.hpp"
#include <Metal/Metal.hpp>
#include <memory>


std::unique_ptr<Scene> createTestScene(MTL::Device* device);

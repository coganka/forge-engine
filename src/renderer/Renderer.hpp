#pragma once

#include <memory>
#include <Metal/Metal.hpp>
#include <QuartzCore/QuartzCore.hpp>
#include "math/Transform.hpp"
#include "scene/Camera.hpp"
#include "renderer/mesh/Mesh.hpp"
#include "scene/Scene.hpp"

class Renderer 
{
public:
    Renderer(MTL::Device* device, CA::MetalLayer* layer);
    ~Renderer();
    void draw(const Scene& scene);

private:
    MTL::Device* device = nullptr;
    CA::MetalLayer* layer = nullptr;
    
    MTL::CommandQueue* commandQueue = nullptr;
    MTL::RenderPipelineState* pipeline = nullptr;
    MTL::Texture* depthTexture = nullptr;
    MTL::DepthStencilState* depthState = nullptr;

    void createPipeline();
    void createDepthResources();
    void resizeDepthTexture(NS::UInteger width, NS::UInteger height);
};
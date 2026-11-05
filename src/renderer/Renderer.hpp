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
    MTL::RenderPipelineState* unlitPipeline = nullptr;
    MTL::RenderPipelineState* tonemapPipeline = nullptr;
    MTL::Texture* depthTexture = nullptr;
    MTL::Texture* hdrTexture = nullptr;
    MTL::DepthStencilState* depthState = nullptr;
    std::unique_ptr<Mesh> lightSphereMesh;

    void createPipeline();
    void createDepthResources();
    void resizeDepthTexture(NS::UInteger width, NS::UInteger height);
    void resizeHDRTexture(NS::UInteger width, NS::UInteger height);
};
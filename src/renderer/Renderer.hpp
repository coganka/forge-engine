#pragma once

#include <memory>
#include <Metal/Metal.hpp>
#include <QuartzCore/QuartzCore.hpp>
#include "math/Transform.hpp"
#include "scene/Camera.hpp"
#include "renderer/mesh/Mesh.hpp"

class Renderer 
{
public:
    Renderer(MTL::Device* device, CA::MetalLayer* layer);
    ~Renderer();
    void draw();

private:
    MTL::Device* device = nullptr;
    CA::MetalLayer* layer = nullptr;

    std::unique_ptr<Mesh> mesh;
    
    MTL::CommandQueue* commandQueue = nullptr;
    MTL::RenderPipelineState* pipeline = nullptr;
    MTL::Buffer* uniformBuffer = nullptr;
    MTL::Texture* depthTexture = nullptr;
    MTL::DepthStencilState* depthState = nullptr;

    Transform triangleTransform;
    Camera camera;

    float time = 0.0f;

    void createPipeline();
    void createVertexBuffer();
    void createDepthResources();
    void resizeDepthTexture(NS::UInteger width, NS::UInteger height);
};
#pragma once

#include <memory>
#include <array>
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
    MTL::RenderPipelineState* bloomExtractPipeline = nullptr;
    MTL::Texture* depthTexture = nullptr;
    MTL::Texture* hdrTexture = nullptr;
    static constexpr size_t BloomLevels = 4;
    std::array<MTL::Texture*, BloomLevels> bloomTextures{};
    std::array<MTL::Texture*, BloomLevels> bloomScratch{};
    MTL::RenderPipelineState* bloomBlurPipeline = nullptr;
    MTL::RenderPipelineState* bloomDownsamplePipeline = nullptr;
    MTL::DepthStencilState* depthState = nullptr;
    std::shared_ptr<Mesh> lightSphereMesh;

    MTL::RenderPipelineState* shadowPipeline = nullptr;
    MTL::Texture* shadowTexture = nullptr;
    std::unique_ptr<Texture> whiteTexture, flatNormalTexture, blackTexture;
    void createShadowResources();
    void createPipeline();
    void createDepthResources();
    void resizeDepthTexture(NS::UInteger width, NS::UInteger height);
    void resizeHDRTexture(NS::UInteger width, NS::UInteger height);
    void resizeBloomTexture(NS::UInteger width, NS::UInteger height);
};

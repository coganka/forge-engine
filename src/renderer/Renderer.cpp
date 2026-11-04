#include <cmath>
#include <memory>
#include <algorithm>
#include <array>
#include <cmath>
#include <iostream>
#include <Foundation/Foundation.hpp>
#include <numbers>

#include "Renderer.hpp"
#include "renderer/Types.hpp"
#include "renderer/mesh/Mesh.hpp"
#include "assets/ObjLoader.hpp"



Renderer::Renderer(MTL::Device* device, CA::MetalLayer* layer)
    : device(device), layer(layer) 
{
    commandQueue = device->newCommandQueue();
    createPipeline();
    createDepthResources();
}


Renderer::~Renderer()
{
    if (pipeline) pipeline->release();
    if (commandQueue) commandQueue->release();
    if (depthTexture) depthTexture->release();
    if (depthState) depthState->release();
}


void Renderer::createPipeline()
{
    NS::Error* error = nullptr;

    auto* path = NS::String::string(
        "build/MetalLab.app/Contents/Resources/mesh.metallib",
        NS::UTF8StringEncoding
    );

    auto* library = device->newLibrary(path, &error);

    if (!library)
    {
        std::cerr
            << "Shader library error: "
            << (error ? error->localizedDescription()->utf8String() : "unknown")
            << '\n';
        return;
    }

    auto* vertexFunction = library->newFunction(
        NS::String::string(
            "vertex_main",
            NS::UTF8StringEncoding
        )
    );

    auto* fragmentFunction = library->newFunction(
        NS::String::string(
            "fragment_main",
            NS::UTF8StringEncoding
        )
    );

    if (!vertexFunction || !fragmentFunction)
    {
        std::cerr << "Shader function not found\n";
        library->release();
        return;
    }

    auto* descriptor =
        MTL::RenderPipelineDescriptor::alloc()->init();

    descriptor->setVertexFunction(vertexFunction);
    descriptor->setFragmentFunction(fragmentFunction);

    descriptor
        ->colorAttachments()
        ->object(0)
        ->setPixelFormat(MTL::PixelFormatBGRA8Unorm);

    auto* vertexDescriptor = MTL::VertexDescriptor::alloc()->init();

    auto setAttribute = [&](NS::UInteger index, MTL::VertexFormat format, NS::UInteger offset)
    {
        auto* attribute = vertexDescriptor->attributes()->object(index);
        attribute->setFormat(format);
        attribute->setOffset(offset);
        attribute->setBufferIndex(0);
    };

    setAttribute(0, MTL::VertexFormatFloat3, 0);
    setAttribute(1, MTL::VertexFormatFloat3, 12);
    setAttribute(2, MTL::VertexFormatFloat2, 24);
    setAttribute(3, MTL::VertexFormatFloat2, 32);

    vertexDescriptor
        ->layouts()
        ->object(0)
        ->setStride(sizeof(Vertex));

    descriptor->setVertexDescriptor(vertexDescriptor);
    
    descriptor->setDepthAttachmentPixelFormat(MTL::PixelFormatDepth32Float);

    pipeline = device->newRenderPipelineState(
        descriptor,
        &error
    );

    if (!pipeline)
    {
        std::cerr
            << "Pipeline creation failed: "
            << (error ? error->localizedDescription()->utf8String() : "unknown")
            << '\n';
    }

    descriptor->release();
    vertexDescriptor->release();
    vertexFunction->release();
    fragmentFunction->release();
    library->release();
}


void Renderer::draw(const Scene& scene)
{
    if (!pipeline || !commandQueue || !depthState)
        return;

    auto* drawable = layer->nextDrawable();
            
    if (!drawable) {
        return;
    }

    auto width = drawable->texture()->width();
    auto height = drawable->texture()->height();

    resizeDepthTexture(width, height);

    auto* pass = MTL::RenderPassDescriptor::renderPassDescriptor();
    auto* color = pass->colorAttachments()->object(0);

    color->setTexture(drawable->texture());
    color->setLoadAction(MTL::LoadActionClear);
    color->setStoreAction(MTL::StoreActionStore);

    color->setClearColor(
        MTL::ClearColor(0.5, 0.2, 0.7, 1.0)
    );

    auto* commandBuffer = commandQueue->commandBuffer();

    auto* depth = pass->depthAttachment();

    depth->setTexture(depthTexture);
    depth->setLoadAction(MTL::LoadActionClear);
    depth->setStoreAction(MTL::StoreActionDontCare);
    depth->setClearDepth(1.0);

    auto*encoder = commandBuffer->renderCommandEncoder(pass);

    float aspectRatio = static_cast<float>(width) / static_cast<float>(height);

    const auto view = scene.camera().viewMatrix();
    const auto projection = scene.camera().projectionMatrix(aspectRatio);

    encoder->setRenderPipelineState(pipeline);
    encoder->setDepthStencilState(depthState);

    constexpr size_t MaxLights = 16;

    std::array<GPULight, MaxLights> gpuLights{};

    const size_t lightCount =
        std::min(scene.lights().size(), MaxLights);

    for (size_t i = 0; i < lightCount; ++i)
    {
        const Light& light = scene.lights()[i];
        GPULight& gpu = gpuLights[i];

        gpu.position = {
            light.position[0],
            light.position[1],
            light.position[2],
            1.0f
        };

        gpu.direction = {
            light.direction[0],
            light.direction[1],
            light.direction[2],
            0.0f
        };

        gpu.color = {
            light.color[0],
            light.color[1],
            light.color[2],
            1.0f
        };

        gpu.intensity = light.intensity;
        gpu.range = light.range;

        gpu.innerConeCos =
            std::cos(light.innerConeAngle * std::numbers::pi_v<float> / 180.0f);

        gpu.outerConeCos =
            std::cos(light.outerConeAngle * std::numbers::pi_v<float> / 180.0f);

        gpu.type = static_cast<uint32_t>(light.type);
    }
    
    for (const auto& entity : scene.entities())
    {
        Uniforms uniforms{};
        uniforms.model = entity.transform.modelMatrix();
        uniforms.view = view;
        uniforms.projection = projection;

        uniforms.cameraPosition = {
            scene.camera().position[0],
            scene.camera().position[1],
            scene.camera().position[2],
            1.0f
        };

        encoder->setVertexBytes(
            &uniforms,
            sizeof(Uniforms),
            1
        );

        encoder->setFragmentBytes(
            &uniforms,
            sizeof(Uniforms),
            1
        );

        encoder->setFragmentBytes(
            gpuLights.data(),
            sizeof(GPULight) * lightCount,
            2
        );

        uint32_t gpuLightCount = static_cast<uint32_t>(lightCount);

        encoder->setFragmentBytes(
            &gpuLightCount,
            sizeof(gpuLightCount),
            3
        );

        encoder->setVertexBuffer(
            entity.mesh->vertexBuffer(),
            0,
            0
        );

        encoder->drawIndexedPrimitives(
            MTL::PrimitiveTypeTriangle,
            entity.mesh->indexCount(),
            MTL::IndexTypeUInt32,
            entity.mesh->indexBuffer(),
            NS::UInteger(0)
        );
    }

    encoder->endEncoding();
    commandBuffer->presentDrawable(drawable);
    commandBuffer->commit();
}


void Renderer::createDepthResources()
{
    auto* depthDesc = MTL::DepthStencilDescriptor::alloc()->init();
    
    depthDesc->setDepthCompareFunction(MTL::CompareFunctionLess);
    depthDesc->setDepthWriteEnabled(true);

    depthState = device->newDepthStencilState(depthDesc);

    depthDesc->release();
}


void Renderer::resizeDepthTexture(NS::UInteger width, NS::UInteger height)
{
    if (depthTexture)
    {
        if (depthTexture->width() == width && depthTexture->height() == height)
        {
            return;
        }
        depthTexture->release();
        depthTexture = nullptr;
    }

    auto* desc = MTL::TextureDescriptor::texture2DDescriptor(
        MTL::PixelFormatDepth32Float,
        width,
        height,
        false
    );

    desc->setUsage(MTL::TextureUsageRenderTarget);
    desc->setStorageMode(MTL::StorageModePrivate);

    depthTexture = device->newTexture(desc);
}
#include <cmath>
#include <memory>
#include <algorithm>
#include <array>
#include <cmath>
#include <iostream>
#include <Foundation/Foundation.hpp>
#include <numbers>
#include <stdexcept>

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

    MeshData sphereData =
        ObjLoader::load("resources/models/sphere.obj");

    lightSphereMesh = std::make_unique<Mesh>(
        device,
        std::span<const Vertex>(sphereData.vertices),
        std::span<const uint32_t>(sphereData.indices)
    );
}


Renderer::~Renderer()
{
    if (pipeline) pipeline->release();
    if (unlitPipeline) unlitPipeline->release();
    if (tonemapPipeline) tonemapPipeline->release();
    if (bloomExtractPipeline) bloomExtractPipeline->release();
    if (commandQueue) commandQueue->release();
    if (depthTexture) depthTexture->release();
    if (depthState) depthState->release();
    if (hdrTexture) hdrTexture->release();
    if (bloomTexture) bloomTexture->release();
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
        ->setPixelFormat(MTL::PixelFormatRGBA16Float);

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
    setAttribute(3, MTL::VertexFormatFloat3, 32);

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

    auto* unlitVertexFn = library->newFunction(
        NS::String::string("unlit_vertex", NS::UTF8StringEncoding)
    );

    auto* unlitFragmentFn = library->newFunction(
        NS::String::string("unlit_fragment", NS::UTF8StringEncoding)
    );

    if (!unlitVertexFn || !unlitFragmentFn)
    {
        std::cerr << "unlit shader funcs not found\n";
    } else 
    {
        descriptor->setVertexFunction(unlitVertexFn);
        descriptor->setFragmentFunction(unlitFragmentFn);

        unlitPipeline = device->newRenderPipelineState(
            descriptor,
            &error
        );

        if (!unlitPipeline)
        {
            std::cerr << "Unlit pipeline creation failed: "
                    << (error ? error->localizedDescription()->utf8String() : "unknown")
                    << '\n';
        }
    }

    if(unlitVertexFn) unlitVertexFn->release();
    if(unlitFragmentFn) unlitFragmentFn->release();

    auto* fullscreenVertexFn = library->newFunction(
        NS::String::string("fullscreen_vertex", NS::UTF8StringEncoding)
    );

    auto* tonemapFragmentFn = library->newFunction(
        NS::String::string("tonemap_fragment", NS::UTF8StringEncoding)
    );

    if (!fullscreenVertexFn || !tonemapFragmentFn)
        throw std::runtime_error("Tone mapping shaders not found");

    auto* tonemapDesc =
        MTL::RenderPipelineDescriptor::alloc()->init();

    tonemapDesc->setVertexFunction(fullscreenVertexFn);
    tonemapDesc->setFragmentFunction(tonemapFragmentFn);

    tonemapDesc->colorAttachments()
        ->object(0)
        ->setPixelFormat(MTL::PixelFormatBGRA8Unorm);

    // No vertex descriptor or depth attachment needed
    tonemapPipeline = device->newRenderPipelineState(
        tonemapDesc,
        &error
    );

    if (!tonemapPipeline)
    {
        std::cerr << "Tone mapping pipeline failed: "
                << (error ? error->localizedDescription()->utf8String() : "unknown")
                << '\n';
    }

    tonemapDesc->release();
    fullscreenVertexFn->release();
    tonemapFragmentFn->release();

    auto* bloomVertexFn = library->newFunction(
        NS::String::string("bloom_vertex", NS::UTF8StringEncoding)
    );

    auto* bloomFragmentFn = library->newFunction(
        NS::String::string("bloom_extract", NS::UTF8StringEncoding)
    );

    if (!bloomVertexFn || !bloomFragmentFn)
        throw std::runtime_error("Bloom shader functions missing");

    auto* bloomDesc =
        MTL::RenderPipelineDescriptor::alloc()->init();

    bloomDesc->setVertexFunction(bloomVertexFn);
    bloomDesc->setFragmentFunction(bloomFragmentFn);

    bloomDesc->colorAttachments()
        ->object(0)
        ->setPixelFormat(MTL::PixelFormatRGBA16Float);

    bloomExtractPipeline =
        device->newRenderPipelineState(bloomDesc, &error);

    if (!bloomExtractPipeline)
        throw std::runtime_error("Bloom extraction pipeline failed");

    bloomDesc->release();
    bloomVertexFn->release();
    bloomFragmentFn->release();

    descriptor->release();
    vertexDescriptor->release();
    vertexFunction->release();
    fragmentFunction->release();
    library->release();
}


void Renderer::draw(const Scene& scene)
{
    if (!pipeline || !unlitPipeline || !commandQueue || !depthState)
        return;

    auto* drawable = layer->nextDrawable();
            
    if (!drawable) {
        return;
    }

    auto width = drawable->texture()->width();
    auto height = drawable->texture()->height();

    resizeDepthTexture(width, height);
    resizeHDRTexture(width, height);

    auto* pass = MTL::RenderPassDescriptor::renderPassDescriptor();
    auto* color = pass->colorAttachments()->object(0);

    color->setTexture(hdrTexture);
    color->setLoadAction(MTL::LoadActionClear);
    color->setStoreAction(MTL::StoreActionStore);

    color->setClearColor(
        MTL::ClearColor(0.0, 0.0, 0.0, 1.0)
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

    encoder->setRenderPipelineState(unlitPipeline);

    for (const auto& light : scene.lights())
    {
        if (light.type != LightType::Point)
            continue;

        Transform transform;
        transform.position = light.position;
        transform.scale = {0.12f, 0.12f, 0.12f};

        Uniforms uniforms{};
        uniforms.model = transform.modelMatrix();
        uniforms.view = view;
        uniforms.projection = projection;

        encoder->setVertexBytes(
            &uniforms,
            sizeof(Uniforms),
            1
        );

        encoder->setVertexBuffer(
            lightSphereMesh->vertexBuffer(),
            0,
            0
        );

        encoder->drawIndexedPrimitives(
            MTL::PrimitiveTypeTriangle,
            lightSphereMesh->indexCount(),
            MTL::IndexTypeUInt32,
            lightSphereMesh->indexBuffer(),
            NS::UInteger(0)
        );
    }

    encoder->endEncoding();

    // Second pass: HDR texture -> display drawable
    auto* displayPass =
        MTL::RenderPassDescriptor::renderPassDescriptor();

    auto* displayColor =
        displayPass->colorAttachments()->object(0);

    displayColor->setTexture(drawable->texture());
    displayColor->setLoadAction(MTL::LoadActionDontCare);
    displayColor->setStoreAction(MTL::StoreActionStore);

    auto* displayEncoder =
        commandBuffer->renderCommandEncoder(displayPass);

    displayEncoder->setRenderPipelineState(tonemapPipeline);

    displayEncoder->setFragmentTexture(hdrTexture, 0);
    displayEncoder->setFragmentTexture(bloomTexture, 1);

    displayEncoder->drawPrimitives(
        MTL::PrimitiveTypeTriangle,
        NS::UInteger(0),
        NS::UInteger(3)
    );

    displayEncoder->endEncoding();

    resizeBloomTexture(width, height);

    auto* bloomPass =
        MTL::RenderPassDescriptor::renderPassDescriptor();

    auto* bloomColor =
        bloomPass->colorAttachments()->object(0);

    bloomColor->setTexture(bloomTexture);
    bloomColor->setLoadAction(MTL::LoadActionDontCare);
    bloomColor->setStoreAction(MTL::StoreActionStore);

    auto* bloomEncoder =
        commandBuffer->renderCommandEncoder(bloomPass);

    bloomEncoder->setRenderPipelineState(bloomExtractPipeline);
    bloomEncoder->setFragmentTexture(hdrTexture, 0);

    bloomEncoder->drawPrimitives(
        MTL::PrimitiveTypeTriangle,
        NS::UInteger(0),
        NS::UInteger(3)
    );

    bloomEncoder->endEncoding();

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


void Renderer::resizeHDRTexture(NS::UInteger width, NS::UInteger height)
{
    if (hdrTexture &&
        hdrTexture->width() == width &&
        hdrTexture->height() == height)
    {
        return;
    }

    auto* descriptor =
        MTL::TextureDescriptor::texture2DDescriptor(
            MTL::PixelFormatRGBA16Float,
            width,
            height,
            false
        );

    descriptor->setUsage(
        MTL::TextureUsageRenderTarget |
        MTL::TextureUsageShaderRead
    );

    descriptor->setStorageMode(
        MTL::StorageModePrivate
    );

    auto* newTexture = device->newTexture(descriptor);

    if (!newTexture)
        throw std::runtime_error("Failed to create HDR texture");

    if (hdrTexture)
        hdrTexture->release();

    hdrTexture = newTexture;
}


void Renderer::resizeBloomTexture(
    NS::UInteger width,
    NS::UInteger height
)
{
    if (bloomTexture &&
        bloomTexture->width() == width &&
        bloomTexture->height() == height)
        return;

    auto* desc = MTL::TextureDescriptor::texture2DDescriptor(
        MTL::PixelFormatRGBA16Float,
        width,
        height,
        false
    );

    desc->setUsage(
        MTL::TextureUsageRenderTarget |
        MTL::TextureUsageShaderRead
    );

    desc->setStorageMode(MTL::StorageModePrivate);

    auto* newTexture = device->newTexture(desc);

    if (!newTexture)
        throw std::runtime_error("Bloom texture allocation failed");

    if (bloomTexture)
        bloomTexture->release();

    bloomTexture = newTexture;
}
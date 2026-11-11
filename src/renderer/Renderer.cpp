#include "math/View.hpp"
#include <cmath>
#include <memory>
#include <algorithm>
#include <array>
#include <iostream>
#include <Foundation/Foundation.hpp>
#include <numbers>
#include <stdexcept>


#include "Renderer.hpp"
#include "renderer/Types.hpp"
#include "renderer/mesh/Mesh.hpp"
#include "renderer/mesh/MeshPrimitives.hpp"


Renderer::Renderer(MTL::Device* device, CA::MetalLayer* layer)
    : device(device), layer(layer)
{
    commandQueue = device->newCommandQueue();
    createPipeline();
    createDepthResources();
    createShadowResources();
    whiteTexture = std::make_unique<Texture>(device, 0xffffffff);
    flatNormalTexture = std::make_unique<Texture>(device, 0xffff8080);
    blackTexture = std::make_unique<Texture>(device, 0xff000000);

    lightSphereMesh = MeshPrimitives::sphere(device);
}


Renderer::~Renderer()
{
    if (shadowPipeline)
    {
        shadowPipeline->release();
    }

    if (shadowTexture)
    {
        shadowTexture->release();
    }

    if (pipeline)
    {
        pipeline->release();
    }

    if (unlitPipeline)
    {
        unlitPipeline->release();
    }

    if (tonemapPipeline)
    {
        tonemapPipeline->release();
    }

    if (bloomExtractPipeline)
    {
        bloomExtractPipeline->release();
    }

    if (commandQueue)
    {
        commandQueue->release();
    }

    if (depthTexture)
    {
        depthTexture->release();
    }

    if (depthState)
    {
        depthState->release();
    }

    if (hdrTexture)
    {
        hdrTexture->release();
    }

    if (bloomBlurPipeline)
    {
        bloomBlurPipeline->release();
    }

    if (bloomDownsamplePipeline)
    {
        bloomDownsamplePipeline->release();
    }

    for (auto* texture : bloomTextures)
    {
        if (texture)
        {
            texture->release();
        }
    }

    for (auto* texture : bloomScratch)
    {
        if (texture)
        {
            texture->release();
        }
    }
}


void Renderer::createPipeline()
{
    NS::Error* error = nullptr;

    auto* path = NS::String::string(
        "build/MetalLab.app/Contents/Resources/mesh.metallib", NS::UTF8StringEncoding
    );

    auto* library = device->newLibrary(path, &error);

    if (!library)
    {
        std::cerr << "Shader library error: "
                  << (error ? error->localizedDescription()->utf8String() : "unknown")
                  << '\n';
        return;
    }

    auto* vertexFunction =
        library->newFunction(NS::String::string("vertex_main", NS::UTF8StringEncoding));

    auto* fragmentFunction = library->newFunction(
        NS::String::string("fragment_main", NS::UTF8StringEncoding)
    );

    if (!vertexFunction || !fragmentFunction)
    {
        std::cerr << "Shader function not found\n";
        library->release();
        return;
    }

    auto* descriptor = MTL::RenderPipelineDescriptor::alloc()->init();

    descriptor->setVertexFunction(vertexFunction);
    descriptor->setFragmentFunction(fragmentFunction);

    descriptor->colorAttachments()->object(0)->setPixelFormat(
        MTL::PixelFormatRGBA16Float
    );

    auto* vertexDescriptor = MTL::VertexDescriptor::alloc()->init();

    auto setAttribute =
        [&](NS::UInteger index, MTL::VertexFormat format, NS::UInteger offset)
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

    vertexDescriptor->layouts()->object(0)->setStride(sizeof(Vertex));

    descriptor->setVertexDescriptor(vertexDescriptor);

    descriptor->setDepthAttachmentPixelFormat(MTL::PixelFormatDepth32Float);

    pipeline = device->newRenderPipelineState(descriptor, &error);

    if (!pipeline)
    {
        std::cerr << "Pipeline creation failed: "
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
    }
    else
    {
        descriptor->setVertexFunction(unlitVertexFn);
        descriptor->setFragmentFunction(unlitFragmentFn);

        unlitPipeline = device->newRenderPipelineState(descriptor, &error);

        if (!unlitPipeline)
        {
            std::cerr << "Unlit pipeline creation failed: "
                      << (error ? error->localizedDescription()->utf8String()
                                : "unknown")
                      << '\n';
        }
    }

    if (unlitVertexFn)
    {
        unlitVertexFn->release();
    }

    if (unlitFragmentFn)
    {
        unlitFragmentFn->release();
    }

    auto* fullscreenVertexFn = library->newFunction(
        NS::String::string("fullscreen_vertex", NS::UTF8StringEncoding)
    );

    auto* tonemapFragmentFn = library->newFunction(
        NS::String::string("tonemap_fragment", NS::UTF8StringEncoding)
    );

    if (!fullscreenVertexFn || !tonemapFragmentFn)
    {
        throw std::runtime_error("Tone mapping shaders not found");
    }

    auto* tonemapDesc = MTL::RenderPipelineDescriptor::alloc()->init();

    tonemapDesc->setVertexFunction(fullscreenVertexFn);
    tonemapDesc->setFragmentFunction(tonemapFragmentFn);

    tonemapDesc->colorAttachments()->object(0)->setPixelFormat(
        MTL::PixelFormatBGRA8Unorm
    );

    tonemapPipeline = device->newRenderPipelineState(tonemapDesc, &error);

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
    {
        throw std::runtime_error("Bloom shader functions missing");
    }

    auto* bloomDesc = MTL::RenderPipelineDescriptor::alloc()->init();

    bloomDesc->setVertexFunction(bloomVertexFn);
    bloomDesc->setFragmentFunction(bloomFragmentFn);

    bloomDesc->colorAttachments()->object(0)->setPixelFormat(
        MTL::PixelFormatRGBA16Float
    );

    bloomExtractPipeline = device->newRenderPipelineState(bloomDesc, &error);

    if (!bloomExtractPipeline)
    {
        throw std::runtime_error("Bloom extraction pipeline failed");
    }

    auto makeBloomPipeline = [&](const char* name)
    {
        auto* function =
            library->newFunction(NS::String::string(name, NS::UTF8StringEncoding));

        if (!function)
        {
            throw std::runtime_error(name);
        }

        bloomDesc->setFragmentFunction(function);
        auto* state = device->newRenderPipelineState(bloomDesc, &error);
        function->release();

        if (!state)
        {
            throw std::runtime_error(name);
        }

        return state;
    };
    bloomBlurPipeline = makeBloomPipeline("bloom_blur");
    bloomDownsamplePipeline = makeBloomPipeline("bloom_downsample");

    bloomDesc->release();
    bloomVertexFn->release();
    bloomFragmentFn->release();

    auto* shadowFn = library->newFunction(
        NS::String::string("shadow_vertex", NS::UTF8StringEncoding)
    );
    descriptor->setVertexFunction(shadowFn);
    descriptor->setFragmentFunction(nullptr);
    descriptor->colorAttachments()->object(0)->setPixelFormat(MTL::PixelFormatInvalid);
    shadowPipeline = device->newRenderPipelineState(descriptor, &error);

    if (shadowFn)
    {
        shadowFn->release();
    }

    if (!shadowPipeline)
    {
        throw std::runtime_error(
            error ? error->localizedDescription()->utf8String()
                  : "Shadow pipeline failed"
        );
    }

    descriptor->release();
    vertexDescriptor->release();
    vertexFunction->release();
    fragmentFunction->release();
    library->release();
}


void Renderer::draw(const Scene& scene)
{
    if (!pipeline || !unlitPipeline || !tonemapPipeline || !bloomExtractPipeline ||
        !bloomBlurPipeline || !bloomDownsamplePipeline || !commandQueue || !depthState)
    {
        return;
    }

    auto* drawable = layer->nextDrawable();

    if (!drawable)
    {
        return;
    }

    auto width = drawable->texture()->width();
    auto height = drawable->texture()->height();

    resizeDepthTexture(width, height);
    resizeHDRTexture(width, height);
    resizeBloomTexture(width, height);

    auto* pass = MTL::RenderPassDescriptor::renderPassDescriptor();
    auto* color = pass->colorAttachments()->object(0);

    color->setTexture(hdrTexture);
    color->setLoadAction(MTL::LoadActionClear);
    color->setStoreAction(MTL::StoreActionStore);

    color->setClearColor(MTL::ClearColor(0.0, 0.0, 0.0, 1.0));

    auto* commandBuffer = commandQueue->commandBuffer();

    struct ShadowData
    {
        simd_float4x4 viewProjection;
        simd_float4 settings;
    };

    ShadowData shadow{matrix_identity_float4x4, {-1, 0.0005f, 1.0f / 2048, 0}};

    for (size_t i = 0; i < std::min(scene.lights().size(), size_t(16)); ++i)
    {
        if (scene.lights()[i].type != LightType::Directional)
        {
            continue;
        }

        auto direction = simd_normalize(scene.lights()[i].direction);
        simd_float3 center{0, 1, 0};
        shadow.viewProjection = simd_mul(
            orthographic(6, 0.1f, 24), lookAt(center - direction * 10, center)
        );
        shadow.settings.x = float(i);
        shadow.settings.w = 1;
        break;
    }

    auto* shadowPass = MTL::RenderPassDescriptor::renderPassDescriptor();
    auto* sd = shadowPass->depthAttachment();
    sd->setTexture(shadowTexture);
    sd->setLoadAction(MTL::LoadActionClear);
    sd->setStoreAction(MTL::StoreActionStore);
    sd->setClearDepth(1);
    auto* se = commandBuffer->renderCommandEncoder(shadowPass);
    se->setRenderPipelineState(shadowPipeline);
    se->setDepthStencilState(depthState);
    se->setDepthBias(0.001f, 1.5f, 0.01f);

    if (shadow.settings.w > 0)
    {
        for (const auto& entity : scene.entities())
        {
            auto mvp = simd_mul(shadow.viewProjection, entity.transform.modelMatrix());
            se->setVertexBytes(&mvp, sizeof(mvp), 1);
            se->setVertexBuffer(entity.mesh->vertexBuffer(), 0, 0);
            se->drawIndexedPrimitives(
                MTL::PrimitiveTypeTriangle,
                entity.mesh->indexCount(),
                MTL::IndexTypeUInt32,
                entity.mesh->indexBuffer(),
                NS::UInteger(0)
            );
        }
    }

    se->endEncoding();

    auto* depth = pass->depthAttachment();

    depth->setTexture(depthTexture);
    depth->setLoadAction(MTL::LoadActionClear);
    depth->setStoreAction(MTL::StoreActionDontCare);
    depth->setClearDepth(1.0);

    auto* encoder = commandBuffer->renderCommandEncoder(pass);

    float aspectRatio = static_cast<float>(width) / static_cast<float>(height);

    const auto view = scene.camera().viewMatrix();
    const auto projection = scene.camera().projectionMatrix(aspectRatio);

    encoder->setRenderPipelineState(pipeline);
    encoder->setDepthStencilState(depthState);

    constexpr size_t MaxLights = 16;

    std::array<GPULight, MaxLights> gpuLights{};

    const size_t lightCount = std::min(scene.lights().size(), MaxLights);

    for (size_t i = 0; i < lightCount; ++i)
    {
        const Light& light = scene.lights()[i];
        GPULight& gpu = gpuLights[i];

        gpu.position = {light.position[0], light.position[1], light.position[2], 1.0f};

        gpu.direction = {
            light.direction[0], light.direction[1], light.direction[2], 0.0f
        };

        gpu.color = {light.color[0], light.color[1], light.color[2], 1.0f};

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
        auto normalMatrix = simd_transpose(simd_inverse(uniforms.model));
        encoder->setVertexBytes(&normalMatrix, sizeof(normalMatrix), 5);
        encoder->setVertexBytes(&shadow, sizeof(shadow), 6);
        encoder->setFragmentBytes(&shadow, sizeof(shadow), 6);
        auto params = entity.material.params;
        params.separateRoughness = entity.material.roughness ? 1.0f : 0.0f;
        encoder->setFragmentBytes(&params, sizeof(params), 4);
        const auto& material = entity.material;
        encoder->setFragmentTexture(
            material.baseColor ? material.baseColor->gpu() : whiteTexture->gpu(), 0
        );
        encoder->setFragmentTexture(
            material.normal ? material.normal->gpu() : flatNormalTexture->gpu(), 1
        );
        encoder->setFragmentTexture(
            material.metallicRoughness ? material.metallicRoughness->gpu()
                                       : whiteTexture->gpu(),
            2
        );
        encoder->setFragmentTexture(
            material.emissive ? material.emissive->gpu() : blackTexture->gpu(), 3
        );
        encoder->setFragmentTexture(shadowTexture, 4);
        encoder->setFragmentTexture(
            material.roughness ? material.roughness->gpu() : whiteTexture->gpu(), 5
        );

        uniforms.view = view;
        uniforms.projection = projection;

        uniforms.cameraPosition = {
            scene.camera().position[0],
            scene.camera().position[1],
            scene.camera().position[2],
            1.0f
        };

        encoder->setVertexBytes(&uniforms, sizeof(Uniforms), 1);

        encoder->setFragmentBytes(&uniforms, sizeof(Uniforms), 1);

        encoder->setFragmentBytes(gpuLights.data(), sizeof(gpuLights), 2);

        uint32_t gpuLightCount = static_cast<uint32_t>(lightCount);

        encoder->setFragmentBytes(&gpuLightCount, sizeof(gpuLightCount), 3);

        encoder->setVertexBuffer(entity.mesh->vertexBuffer(), 0, 0);

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
        if (light.type != LightType::Point || !light.showMarker)
        {
            continue;
        }

        Transform transform;
        transform.position = light.position;
        transform.scale = {0.12f, 0.12f, 0.12f};

        Uniforms uniforms{};
        uniforms.model = transform.modelMatrix();
        uniforms.view = view;
        uniforms.projection = projection;
        uniforms.cameraPosition = {
            scene.camera().position[0],
            scene.camera().position[1],
            scene.camera().position[2],
            1.0f
        };
        const simd_float4 emission = {
            light.color[0], light.color[1], light.color[2], light.intensity * 2.0f
        };
        encoder->setFragmentBytes(&uniforms, sizeof(uniforms), 1);
        encoder->setFragmentBytes(&emission, sizeof(emission), 4);

        encoder->setVertexBytes(&uniforms, sizeof(Uniforms), 1);

        encoder->setVertexBuffer(lightSphereMesh->vertexBuffer(), 0, 0);

        encoder->drawIndexedPrimitives(
            MTL::PrimitiveTypeTriangle,
            lightSphereMesh->indexCount(),
            MTL::IndexTypeUInt32,
            lightSphereMesh->indexBuffer(),
            NS::UInteger(0)
        );
    }

    encoder->endEncoding();

    auto fullscreenPass = [&](MTL::Texture* target,
                              MTL::Texture* source,
                              MTL::RenderPipelineState* state,
                              simd_float2 direction)
    {
        auto* pass = MTL::RenderPassDescriptor::renderPassDescriptor();
        auto* attachment = pass->colorAttachments()->object(0);
        attachment->setTexture(target);
        attachment->setLoadAction(MTL::LoadActionDontCare);
        attachment->setStoreAction(MTL::StoreActionStore);
        auto* encoder = commandBuffer->renderCommandEncoder(pass);
        encoder->setRenderPipelineState(state);
        encoder->setFragmentTexture(source, 0);
        encoder->setFragmentBytes(&direction, sizeof(direction), 0);
        encoder->drawPrimitives(
            MTL::PrimitiveTypeTriangle, NS::UInteger(0), NS::UInteger(3)
        );
        encoder->endEncoding();
    };

    for (size_t i = 0; i < BloomLevels; ++i)
    {
        auto* source = i == 0 ? hdrTexture : bloomTextures[i - 1];
        fullscreenPass(
            bloomTextures[i],
            source,
            i == 0 ? bloomExtractPipeline : bloomDownsamplePipeline,
            simd_float2{0.0f, 0.0f}
        );
        fullscreenPass(
            bloomScratch[i],
            bloomTextures[i],
            bloomBlurPipeline,
            simd_float2{1.0f, 0.0f}
        );
        fullscreenPass(
            bloomTextures[i],
            bloomScratch[i],
            bloomBlurPipeline,
            simd_float2{0.0f, 1.0f}
        );
    }

    auto* displayPass = MTL::RenderPassDescriptor::renderPassDescriptor();
    auto* displayColor = displayPass->colorAttachments()->object(0);
    displayColor->setTexture(drawable->texture());
    displayColor->setLoadAction(MTL::LoadActionDontCare);
    displayColor->setStoreAction(MTL::StoreActionStore);
    auto* displayEncoder = commandBuffer->renderCommandEncoder(displayPass);
    displayEncoder->setRenderPipelineState(tonemapPipeline);
    displayEncoder->setFragmentTexture(hdrTexture, 0);

    for (size_t i = 0; i < BloomLevels; ++i)
    {
        displayEncoder->setFragmentTexture(bloomTextures[i], i + 1);
    }

    displayEncoder->drawPrimitives(
        MTL::PrimitiveTypeTriangle, NS::UInteger(0), NS::UInteger(3)
    );
    displayEncoder->endEncoding();

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
        MTL::PixelFormatDepth32Float, width, height, false
    );

    desc->setUsage(MTL::TextureUsageRenderTarget);
    desc->setStorageMode(MTL::StorageModePrivate);

    depthTexture = device->newTexture(desc);
}


void Renderer::resizeHDRTexture(NS::UInteger width, NS::UInteger height)
{
    if (hdrTexture && hdrTexture->width() == width && hdrTexture->height() == height)
    {
        return;
    }

    auto* descriptor = MTL::TextureDescriptor::texture2DDescriptor(
        MTL::PixelFormatRGBA16Float, width, height, false
    );

    descriptor->setUsage(MTL::TextureUsageRenderTarget | MTL::TextureUsageShaderRead);

    descriptor->setStorageMode(MTL::StorageModePrivate);

    auto* newTexture = device->newTexture(descriptor);

    if (!newTexture)
    {
        throw std::runtime_error("Failed to create HDR texture");
    }

    if (hdrTexture)
    {
        hdrTexture->release();
    }

    hdrTexture = newTexture;
}


void Renderer::resizeBloomTexture(NS::UInteger width, NS::UInteger height)
{
    width = std::max(NS::UInteger(1), (width + 1) / 2);
    height = std::max(NS::UInteger(1), (height + 1) / 2);

    if (bloomTextures[0] && bloomTextures[0]->width() == width &&
        bloomTextures[0]->height() == height)
    {
        return;
    }

    for (size_t i = 0; i < BloomLevels; ++i)
    {
        auto* desc = MTL::TextureDescriptor::texture2DDescriptor(
            MTL::PixelFormatRGBA16Float, width, height, false
        );
        desc->setUsage(MTL::TextureUsageRenderTarget | MTL::TextureUsageShaderRead);
        desc->setStorageMode(MTL::StorageModePrivate);
        auto* level = device->newTexture(desc);
        auto* scratch = device->newTexture(desc);

        if (!level || !scratch)
        {
            if (level)
            {
                level->release();
            }

            if (scratch)
            {
                scratch->release();
            }

            throw std::runtime_error("Bloom texture allocation failed");
        }

        if (bloomTextures[i])
        {
            bloomTextures[i]->release();
        }

        if (bloomScratch[i])
        {
            bloomScratch[i]->release();
        }

        bloomTextures[i] = level;
        bloomScratch[i] = scratch;
        width = std::max(NS::UInteger(1), (width + 1) / 2);
        height = std::max(NS::UInteger(1), (height + 1) / 2);
    }
}


void Renderer::createShadowResources()
{
    auto* desc = MTL::TextureDescriptor::texture2DDescriptor(
        MTL::PixelFormatDepth32Float, 2048, 2048, false
    );
    desc->setStorageMode(MTL::StorageModePrivate);
    desc->setUsage(MTL::TextureUsageRenderTarget | MTL::TextureUsageShaderRead);
    shadowTexture = device->newTexture(desc);

    if (!shadowTexture)
    {
        throw std::runtime_error("Shadow map allocation failed");
    }
}

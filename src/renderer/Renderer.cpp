#include "Renderer.hpp"
#include "renderer/Types.hpp"
#include <iostream>
#include <Foundation/Foundation.hpp>
#include <cmath>
#include <memory>
#include "renderer/mesh/Mesh.hpp"
#include "assets/ObjLoader.hpp"


Renderer::Renderer(MTL::Device* device, CA::MetalLayer* layer)
    : device(device), layer(layer) 
{
    commandQueue = device->newCommandQueue();
    createPipeline();
    createDepthResources();
    createVertexBuffer();
}


Renderer::~Renderer()
{
    if (pipeline) pipeline->release();
    if (commandQueue) commandQueue->release();
    if (uniformBuffer) uniformBuffer->release();
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


void Renderer::createVertexBuffer()
{
    MeshData data = ObjLoader::load("resources/models/test.obj");

    Uniforms uniforms = {};
    uniforms.model = matrix_identity_float4x4;
    uniforms.view = matrix_identity_float4x4;
    uniforms.projection = matrix_identity_float4x4;

    mesh = std::make_unique<Mesh>(
        device, 
        std::span<const Vertex>(data.vertices),
        std::span<const uint32_t>(data.indices)
    );

    uniformBuffer = device->newBuffer(
        &uniforms,
        sizeof(Uniforms),
        MTL::ResourceStorageModeShared
    );
}


void Renderer::draw()
{
    if (!pipeline || !uniformBuffer || !commandQueue || !depthState)
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

    time += 0.01f;

    triangleTransform.rotation[1] = time;
    triangleTransform.rotation[2] = time * 0.4f;

    camera.position[0] = 0.0f;
    camera.position[1] = 0.0f;
    camera.position[2] = -3.0f;

    // “Give me the buffer’s raw memory address, and interpret that memory as a Uniforms struct.”
    auto* uniforms = static_cast<Uniforms*>(uniformBuffer->contents());

    uniforms->model = triangleTransform.modelMatrix();
    uniforms->view = camera.viewMatrix();
    uniforms->projection = camera.projectionMatrix(aspectRatio);

    encoder->setRenderPipelineState(pipeline);

    encoder->setVertexBuffer(mesh->vertexBuffer(), 0, 0);
    encoder->setVertexBuffer(uniformBuffer, 0, 1);

    encoder->setDepthStencilState(depthState);
    
    encoder->drawIndexedPrimitives(
        MTL::PrimitiveTypeTriangle,
        mesh->indexCount(),
        MTL::IndexTypeUInt32,
        mesh->indexBuffer(),
        NS::UInteger(0)
    );    

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
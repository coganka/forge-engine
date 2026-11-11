#include "Texture.hpp"
#include <ImageIO/ImageIO.h>
#include <CoreGraphics/CoreGraphics.h>
#include <vector>
#include <stdexcept>


Texture::Texture(
    MTL::Device* device, MTL::CommandQueue* queue, const std::string& path, bool srgb
)
{
    auto url = CFURLCreateFromFileSystemRepresentation(
        nullptr, reinterpret_cast<const UInt8*>(path.data()), path.size(), false
    );

    auto source = CGImageSourceCreateWithURL(url, nullptr);
    CFRelease(url);

    if (!source)
    {
        throw std::runtime_error("Cannot load texture: " + path);
    }

    auto image = CGImageSourceCreateImageAtIndex(source, 0, nullptr);
    CFRelease(source);

    if (!image)
    {
        throw std::runtime_error("Cannot decode texture: " + path);
    }

    const size_t w = CGImageGetWidth(image);
    const size_t h = CGImageGetHeight(image);
    std::vector<uint8_t> pixels(w * h * 4);

    auto colorSpace = CGColorSpaceCreateWithName(kCGColorSpaceSRGB);

    auto context = CGBitmapContextCreate(
        pixels.data(),
        w,
        h,
        8,
        w * 4,
        colorSpace,
        CGBitmapInfo(kCGImageAlphaPremultipliedLast) | kCGBitmapByteOrder32Big
    );
    CGColorSpaceRelease(colorSpace);

    if (!context)
    {
        CGImageRelease(image);
        throw std::runtime_error("Texture decode context failed");
    }

    CGContextDrawImage(context, CGRectMake(0, 0, w, h), image);
    CGContextRelease(context);
    CGImageRelease(image);

    auto desc = MTL::TextureDescriptor::texture2DDescriptor(
        srgb ? MTL::PixelFormatRGBA8Unorm_sRGB : MTL::PixelFormatRGBA8Unorm, w, h, true
    );
    desc->setStorageMode(MTL::StorageModeShared);
    desc->setUsage(MTL::TextureUsageShaderRead);
    texture_ = device->newTexture(desc);

    if (!texture_)
    {
        throw std::runtime_error("Texture allocation failed: " + path);
    }

    texture_->replaceRegion(MTL::Region(0, 0, w, h), 0, pixels.data(), w * 4);

    auto command = queue->commandBuffer();
    auto blit = command->blitCommandEncoder();

    blit->generateMipmaps(texture_);
    blit->endEncoding();

    command->commit();
    command->waitUntilCompleted();
}


Texture::Texture(MTL::Device* device, uint32_t rgba, bool srgb)
{

    auto desc = MTL::TextureDescriptor::texture2DDescriptor(
        srgb ? MTL::PixelFormatRGBA8Unorm_sRGB : MTL::PixelFormatRGBA8Unorm, 1, 1, false
    );
    desc->setStorageMode(MTL::StorageModeShared);
    texture_ = device->newTexture(desc);

    if (!texture_)
    {
        throw std::runtime_error("Fallback texture allocation failed");
    }

    texture_->replaceRegion(MTL::Region(0, 0, 1, 1), 0, &rgba, 4);
}


Texture::~Texture()
{
    if (texture_)
    {
        texture_->release();
    }
}

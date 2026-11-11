#pragma once

#include "Texture.hpp"
#include <simd/simd.h>
#include <cstddef>
#include <memory>


struct MaterialParams
{
    simd_float4 baseColor{1, 1, 1, 1};
    simd_float4 emissive{0, 0, 0, 0};
    float metallic = 0;
    float roughness = 0.6f;
    float normalStrength = 1;
    float separateRoughness = 0;
    simd_float4 uvTransform{1, 1, 0, 0};
};


struct Material
{
    MaterialParams params;
    std::shared_ptr<Texture> baseColor;
    std::shared_ptr<Texture> normal;
    std::shared_ptr<Texture> metallicRoughness;
    std::shared_ptr<Texture> roughness;
    std::shared_ptr<Texture> emissive;
};


static_assert(sizeof(MaterialParams) == 64);
static_assert(offsetof(MaterialParams, metallic) == 32);
static_assert(offsetof(MaterialParams, uvTransform) == 48);

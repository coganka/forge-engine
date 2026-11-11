#pragma once

#include <simd/simd.h>
#include <cstdint>


enum class LightType : uint32_t
{
    Directional = 0,
    Point = 1,
    Spot = 2
};


struct Light
{
    LightType type = LightType::Directional;

    simd_float3 position{0.0f, 0.0f, 0.0f};
    simd_float3 direction{0.0f, -1.0f, 0.0f};
    simd_float3 color{1.0f, 1.0f, 1.0f};

    bool showMarker = true;
    float intensity = 1.0f;
    float range = 10.0f;

    float innerConeAngle = 20.0f;
    float outerConeAngle = 30.0f;
};

#pragma once

#include <simd/simd.h>


struct Transform
{
    simd_float3 position { 0.0f, 0.0f, 0.0f };
    simd_float3 rotation { 0.0f, 0.0f, 0.0f };
    simd_float3 scale { 1.0f, 1.0f, 1.0f };
    simd_float4x4 modelMatrix() const;
};
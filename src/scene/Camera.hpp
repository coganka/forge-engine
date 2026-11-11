#pragma once

#include <simd/simd.h>


class Camera
{
public:
    float fovY = 60.0f;
    float nearPlane = 0.1f;
    float farPlane = 100.0f;

    simd_float3 position{0.0f, 0.0f, 0.0f};
    simd_float3 target{0, 0, 1};
    simd_float4x4 viewMatrix() const;
    simd_float4x4 projectionMatrix(float aspectRatio) const;
};

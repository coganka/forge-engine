#pragma once
#include <simd/simd.h>
#include <cmath>


inline simd_float4x4 lookAt(simd_float3 eye, simd_float3 target)
{
    auto z = simd_normalize(target - eye);
    simd_float3 up =
        std::abs(z.y) > 0.99f ? simd_float3{0, 0, 1} : simd_float3{0, 1, 0};
    auto x = simd_normalize(simd_cross(up, z));
    auto y = simd_cross(z, x);

    return {
        {{x.x, y.x, z.x, 0},
         {x.y, y.y, z.y, 0},
         {x.z, y.z, z.z, 0},
         {-simd_dot(x, eye), -simd_dot(y, eye), -simd_dot(z, eye), 1}}
    };
}


inline simd_float4x4 orthographic(float radius, float nearZ, float farZ)
{
    auto m = matrix_identity_float4x4;
    m.columns[0].x = 1 / radius;
    m.columns[1].y = 1 / radius;
    m.columns[2].z = 1 / (farZ - nearZ);
    m.columns[3].z = -nearZ / (farZ - nearZ);

    return m;
}

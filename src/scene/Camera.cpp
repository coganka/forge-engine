#include "Camera.hpp"
#include <cmath>
#include <numbers>


simd_float4x4 Camera::viewMatrix() const
{
    
    simd_float4x4 view = matrix_identity_float4x4;
    view.columns[3] = {
        -position[0],
        -position[1],
        -position[2],
        1.0f
    };
    
    return view;
}

simd_float4x4 Camera::projectionMatrix(float aspectRatio) const
{
    float fovRadians = fovY * (std::numbers::pi_v<float> / 180.0f);
    float yScale = 1.0f / std::tan(fovRadians * 0.5f);
    float xScale = yScale / aspectRatio;

    float zRange = farPlane - nearPlane;

    simd_float4x4 projection = {};

    projection.columns[0] = {
        xScale,
        0.0f,
        0.0f,
        0.0f
    };

    projection.columns[1] = {
        0.0f,
        yScale,
        0.0f,
        0.0f
    };

    projection.columns[2] = {
        0.0f,
        0.0f,
        farPlane / zRange,
        1.0f
    };
 
    projection.columns[3] = {
        0.0f,
        0.0f,
        -(nearPlane * farPlane) / zRange,
        0.0f
    };

    return projection;
}
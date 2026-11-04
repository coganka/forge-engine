#pragma once

#include <simd/simd.h>


struct Vertex 
{
    float x, y, z;
    float r, g, b;
    float u, v;
    float nx, ny, nz;
};


struct Uniforms
{
    simd_float4x4 model;
    simd_float4x4 view;
    simd_float4x4 projection;
    
    simd_float4 cameraPosition;
};


struct GPULight
{
    simd_float4 position;
    simd_float4 direction;
    simd_float4 color;

    float intensity;
    float range;
    float innerConeCos;
    float outerConeCos;

    uint32_t type;
};
#include "Transform.hpp"
#include <cmath>


static simd_float4x4 makeTranslation(const simd_float3& t)
{
    simd_float4x4 m = matrix_identity_float4x4;

    m.columns[3] = {
        t[0],
        t[1],
        t[2],
        1.0f
    };

    return m;
}


static simd_float4x4 makeScale(const simd_float3& s)
{
    simd_float4x4 m = matrix_identity_float4x4;

    m.columns[0][0] = s[0];
    m.columns[1][1] = s[1];
    m.columns[2][2] = s[2];

    return m;
}


static simd_float4x4 makeRotationZ(float angle)
{
    float c = std::cos(angle);
    float s = std::sin(angle);

    simd_float4x4 m = matrix_identity_float4x4;

    m.columns[0] = { c, -s, 0.0f, 0.0f };
    m.columns[1] = { s, c, 0.0f, 0.0f };

    return m;
}

static simd_float4x4 makeRotationY(float angle)
{
    float c = std::cos(angle);
    float s = std::sin(angle);

    simd_float4x4 m = matrix_identity_float4x4;

    m.columns[0] = { c, 0.0f, -s, 0.0f };
    m.columns[2] = { s, 0.0f, c, 0.0f };
    
    return m;
}


simd_float4x4 Transform::modelMatrix() const
{
    auto T = makeTranslation(position);
    auto Ry = makeRotationY(rotation[1]);
    auto Rz = makeRotationZ(rotation[2]);
    auto S = makeScale(scale);

    return simd_mul(
        T,
        simd_mul(Ry, simd_mul(Rz, S))
    );
}
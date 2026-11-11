#include <metal_stdlib>
using namespace metal;

struct FullscreenOut
{
    float4 position [[position]];
    float2 uv;
};


vertex FullscreenOut bloom_vertex(uint id [[vertex_id]])
{
    float2 positions[3] = {{-1.0, -1.0}, {3.0, -1.0}, {-1.0, 3.0}};

    FullscreenOut out;
    float2 pos = positions[id];

    out.position = float4(pos, 0.0, 1.0);
    out.uv = float2(pos.x * 0.5 + 0.5, 1.0 - (pos.y * 0.5 + 0.5));

    return out;
}


float3 downsample(texture2d<float> image, float2 uv)
{
    constexpr sampler s(filter::linear, address::clamp_to_edge);
    float2 t = 0.5 / float2(image.get_width(), image.get_height());

    return 0.25 * (image.sample(s, uv + float2(-t.x, -t.y)).rgb +
                   image.sample(s, uv + float2(t.x, -t.y)).rgb +
                   image.sample(s, uv + float2(-t.x, t.y)).rgb +
                   image.sample(s, uv + float2(t.x, t.y)).rgb);
}


fragment float4
bloom_extract(FullscreenOut in [[stage_in]], texture2d<float> hdr [[texture(0)]])
{
    float3 color = downsample(hdr, in.uv);
    float brightness = max(color.r, max(color.g, color.b));

    constexpr float threshold = 2.2;
    constexpr float knee = 0.6;
    float soft = clamp(brightness - threshold + knee, 0.0, 2.0 * knee);
    soft = soft * soft / (4.0 * knee);
    float contribution = max(brightness - threshold, soft);

    return float4(color * contribution / max(brightness, 0.0001), 1.0);
}


fragment float4
bloom_downsample(FullscreenOut in [[stage_in]], texture2d<float> image [[texture(0)]])
{
    return float4(downsample(image, in.uv), 1.0);
}


fragment float4 bloom_blur(
    FullscreenOut in [[stage_in]],
    texture2d<float> image [[texture(0)]],
    constant float2& direction [[buffer(0)]]
)
{
    constexpr sampler s(filter::linear, address::clamp_to_edge);
    float2 step = direction / float2(image.get_width(), image.get_height());

    float3 color = image.sample(s, in.uv).rgb * 0.227027;
    color += image.sample(s, in.uv + step * 1.384615).rgb * 0.316216;
    color += image.sample(s, in.uv - step * 1.384615).rgb * 0.316216;
    color += image.sample(s, in.uv + step * 3.230769).rgb * 0.070270;
    color += image.sample(s, in.uv - step * 3.230769).rgb * 0.070270;

    return float4(color, 1.0);
}

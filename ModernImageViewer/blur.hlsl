Texture2D tex : register(t0);
Texture2D tex_original : register(t2);
SamplerState smp : register(s0);

cbuffer cb1 : register(b0)
{
    bool use_color_managment; //use color managment
    float2 axis; //x or y axis, (1, 0) or (0, 1)
    float blur_sigma;
    float blur_radius;
    float unsharp_amount;
};

struct Vertex_shader_output
{
    float4 position : SV_Position; //xyzw
    float2 texcoord : TEXCOORD; //uv
};

#define get_weight(x) (exp(-x * x / (2.0 * blur_sigma * blur_sigma)))

float4 main(Vertex_shader_output vs_out) : SV_Target
{
    if (blur_sigma == 0.0 || blur_radius == 0.0)
        return tex.SampleLevel(smp, vs_out.texcoord, 0.0);
    float2 dims;
    tex.GetDimensions(dims.x, dims.y);
    dims = 1.0 / dims * axis;
    float weight;
    float4 csum = tex.SampleLevel(smp, vs_out.texcoord, 0.0);
    float wsum = 1.0;
    for (float i = 1.0; i <= blur_radius; ++i) {
        weight = get_weight(i);
        csum += (tex.SampleLevel(smp, vs_out.texcoord + dims * -i, 0.0) + tex.SampleLevel(smp, vs_out.texcoord + dims * i, 0.0)) * weight;
        wsum += 2.0 * weight;
    }
    if (unsharp_amount > 0.0 && axis.x > 0.0) {
        float4 original = tex_original.SampleLevel(smp, vs_out.texcoord, 0.0);
        return original + (original - csum / wsum) * unsharp_amount;
    }
    return csum / wsum;
}
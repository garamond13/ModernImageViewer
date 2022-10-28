Texture2D tex : register(t0);
Texture3D lut : register(t1);
SamplerState smp : register(s0);

cbuffer cb1 : register(b0)
{
    float2 wh; //image width and height
    bool cm; //use color managment
};

struct Vertex_shader_output
{
    float4 position : SV_Position; //xyzw
    float2 texcoord : TEXCOORD; //uv
};

//based on https://doi.org/10.2312/egp.20211031 and on https://github.com/ledoge/dwm_lut/blob/master/dwm_lut.c
float3 color_transform(float3 rgb)
{
    
#define LUT_SIZE 64.0
    
    float3 index = rgb * (LUT_SIZE - 1);
    float3 r = frac(index);
    int3 vert2 = 0;
    int3 vert3 = 1;
    bool3 c = r.xyz >= r.yzx;
    bool c_xy = c.x, c_yz = c.y, c_zx = c.z;
    bool c_yx = !c.x, c_zy = !c.y, c_xz = !c.z;
    bool cond;
    float3 s = 0.0;
    
#define ORDER(X,Y,Z) \
    cond = c_ ## X ## Y && c_ ## Y ## Z; \
    s = cond ? r.X ## Y ## Z : s; \
    vert2.X = cond ? 1 : vert2.X; \
    vert3.Z = cond ? 0 : vert3.Z;
    ORDER(x, y, z)ORDER(x, z, y)ORDER(z, x, y)
    ORDER(z, y, x)ORDER(y, z, x)ORDER(y, x, z)

#define SAMPLE_LUT(INDEX) lut.Sample(smp, (INDEX + 0.5) / LUT_SIZE).rgb
    
    float3 base = floor(index);
    return (1.0 - s.x) * SAMPLE_LUT(base) + s.z * SAMPLE_LUT(base + 1.0) + (s.x - s.y) * SAMPLE_LUT(base + vert2) + (s.y - s.z) * SAMPLE_LUT(base + vert3);
}

//bsed on https://gist.github.com/TheRealMJP/c83b8c0f46b63f3a88a5986f4fa982b1 and on https://vec3.ca/bicubic-filtering-in-fewer-taps/
float4 catmull_rom(float2 uv)
{
    uv *= wh;
    
    //texel center
    float2 tc1 = floor(uv - 0.5) + 0.5;
    
    //fractional offset
    float2 f = uv - tc1;
    
    //weights
    float2 w0 = f * (-0.5 + f * (1.0 - 0.5 * f));
    float2 w1 = 1.0 + f * f * (-2.5 + 1.5 * f);
    float2 w3 = f * f * (-0.5 + 0.5 * f);
    float2 w12 = 1.0 - w0 - w3;
    
    //sampling coordinates
    float2 tc0 = (tc1 - 1.0) / wh;
    float2 tc3 = (tc1 + 2.0) / wh;
    float2 tc12 = (tc1 + (w12 - w1) / w12) / wh;
    
    return
        tex.Sample(smp, float2(tc0.x, tc0.y)) * w0.x * w0.y +
        tex.Sample(smp, float2(tc12.x, tc0.y)) * w12.x * w0.y +
        tex.Sample(smp, float2(tc3.x, tc0.y)) * w3.x * w0.y +
        tex.Sample(smp, float2(tc0.x, tc12.y)) * w0.x * w12.y +
        tex.Sample(smp, float2(tc12.x, tc12.y)) * w12.x * w12.y +
        tex.Sample(smp, float2(tc3.x, tc12.y)) * w3.x * w12.y +
        tex.Sample(smp, float2(tc0.x, tc3.y)) * w0.x * w3.y +
        tex.Sample(smp, float2(tc12.x, tc3.y)) * w12.x * w3.y +
        tex.Sample(smp, float2(tc3.x, tc3.y)) * w3.x * w3.y;
}

float4 main(Vertex_shader_output input) : SV_Target
{
    //apply color managment
    if (cm)
        return float4(color_transform(catmull_rom(input.texcoord).rgb), 1.0);
    else
        return catmull_rom(input.texcoord);
}
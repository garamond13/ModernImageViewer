Texture2D tex : register(t0);
Texture3D lut : register(t1);
SamplerState smp : register(s0);

cbuffer cb1 : register(b0)
{
    bool use_color_managment; //use color managment
    float2 axis; //x or y axis, (1, 0) or (0, 1)
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

float4 main(Vertex_shader_output vs_out) : SV_Target
{
    float4 color = tex.Sample(smp, vs_out.texcoord);
    if (use_color_managment)
        return float4(color_transform(color.rgb), color.a);
    return color;
}
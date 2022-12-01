Texture2D tex : register(t0);
Texture3D lut : register(t1);
SamplerState smp : register(s0);

cbuffer cb1 : register(b0)
{
    bool use_color_managment; //use color managment
    float2 axis; //x or y axis, (1, 0) or (0, 1)
};

cbuffer cb2 : register(b1)
{
    int kernel_index;
    float radius; //kernel radius
    float2 kparam; //kernel specific parameters, kparam.x == kparam1, kparam.y == kparam2
    float antiringing; //antiringing strenght
    float widening_factor; //if downsampling, > 1.0 else == 1.0
};

struct Vertex_shader_output
{
    float4 position : SV_Position; //xyzw
    float2 texcoord : TEXCOORD; //uv
};

//source corecrt_math_defines.h
#define M_PI 3.14159265358979323846 //pi
#define M_PI_2 1.57079632679489661923 //pi/2

//based on https://www.boost.org/doc/libs/1_54_0/libs/math/doc/html/math_toolkit/bessel/mbessel.html
float bessel_i0(float x)
{
    static const float P1[] = {
        -2.2335582639474375249e+15,
        -5.5050369673018427753e+14,
        -3.2940087627407749166e+13,
        -8.4925101247114157499e+11,
        -1.1912746104985237192e+10,
        -1.0313066708737980747e+08,
        -5.9545626019847898221e+05,
        -2.4125195876041896775e+03,
        -7.0935347449210549190e+00,
        -1.5453977791786851041e-02,
        -2.5172644670688975051e-05,
        -3.0517226450451067446e-08,
        -2.6843448573468483278e-11,
        -1.5982226675653184646e-14,
        -5.2487866627945699800e-18,
    };
    static const float Q1[] = {
        -2.2335582639474375245e+15,
        7.8858692566751002988e+12,
        -1.2207067397808979846e+10,
        1.0377081058062166144e+07,
        -4.8527560179962773045e+03,
        1.0,
    };
    static const float P2[] = {
        -2.2210262233306573296e-04,
        1.3067392038106924055e-02,
        -4.4700805721174453923e-01,
        5.5674518371240761397e+00,
        -2.3517945679239481621e+01,
        3.1611322818701131207e+01,
        -9.6090021968656180000e+00,
    };
    static const float Q2[] = {
        -5.5194330231005480228e-04,
        3.2547697594819615062e-02,
        -1.1151759188741312645e+00,
        1.3982595353892851542e+01,
        -6.0228002066743340583e+01,
        8.5539563258012929600e+01,
        -3.1446690275135491500e+01,
        1.0,
    };
    if (x < 0.0)
        x = -x;
    if (x == 0.0)
        return 1.0;
    else if (x <= 15.0) {
        float y = x * x;
        float p1sum = P1[14];
        for (int i = 13; i >= 0; --i) {
            p1sum *= y;
            p1sum += P1[i];
        }
        float q1sum = Q1[5];
        for (i = 4; i >= 0; --i) {
            q1sum *= y;
            q1sum += Q1[i];
        }
        return p1sum / q1sum;
    }
    else {
        float y = 1.0 / x - 1.0 / 15.0;
        float p2sum = P2[6];
        for (int i = 5; i >= 0; --i) {
            p2sum *= y;
            p2sum += P2[i];
        }
        float q2sum = Q2[7];
        for (i = 6; i >= 0; --i) {
            q2sum *= y;
            q2sum += Q2[i];
        }
        return exp(x) / sqrt(x) * p2sum / q2sum;
    }
}

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

float sinc(float x)
{
    //should be (x == 0), but it can cause unexplainable artifacts
    if (x < 1e-7)
        return 1.0;
    return sin(M_PI * x) / (M_PI * x);
}

float cosine(float x)
{
    return cos(M_PI_2 * x);
}

float hann(float x)
{
    return 0.5 + 0.5 * cos(M_PI * x);
}

float hamming(float x)
{
    return 0.54 + 0.46 * cos(M_PI * x);
}

float blackman(float x)
{
    //precalculated for alpha = 0.16
    return 0.42 + 0.5 * cos(M_PI * x) + 0.08 * cos(2.0 * M_PI * x);
}

float kaiser(float x)
{
    //kparam.x == beta == pi * alpha
    return bessel_i0(kparam.x * sqrt(1.0 - x * x)) / bessel_i0(kparam.x);
}

float welch(float x)
{
    return 1.0 - x * x;
}

//source https://www.hpl.hp.com/techreports/2007/HPL-2007-179.pdf
float said(float x)
{
    //kparam.x == chi
    //kparam.y == eta
    return sinc(x) * cosh(sqrt(2.0 * kparam.y) * M_PI * kparam.x * x / (2.0 - kparam.y)) * exp(-((M_PI * kparam.x * x / (2.0 - kparam.y)) * (M_PI * kparam.x * x / (2.0 - kparam.y))));
}

float bc_spline(float x)
{
    //kparam.x == b
    //kparam.y == c
    if (x < 1.0)
        return ((12.0 - 9.0 * kparam.x - 6.0 * kparam.y) * x * x * x + (-18.0 + 12.0 * kparam.x + 6.0 * kparam.y) * x * x + (6.0 - 2.0 * kparam.x)) / 6.0;
    else //x < 2.0
        return ((-kparam.x - 6.0 * kparam.y) * x * x * x + (6.0 * kparam.x + 30.0 * kparam.y) * x * x + (-12.0 * kparam.x - 48.0 * kparam.y) * x + (8.0 * kparam.x + 24.0 * kparam.y)) / 6.0;
}

float bicubic(float x)
{
    //kparam.x == a
    if (x < 1.0)
        return (kparam.x + 2.0) * x * x * x - (kparam.x + 3.0) * x * x + 1.0;
    else // x < 2.0
        return kparam.x * x * x * x - 5.0 * kparam.x * x * x + 8.0 * kparam.x * x - 4.0 * kparam.x;
}

float nearest_neighbor(float x)
{
    return x < 0.5 ? 1.0 : 0.0;
}

float get_weight(float x)
{
    x = abs(x) / widening_factor;
    if (x < radius) {
        switch(kernel_index) {
        case 1:
            return sinc(x) * sinc(x / radius);
        case 2:
            return sinc(x) * cosine(x / radius);
        case 3:
            return sinc(x) * hann(x / radius);
        case 4:
            return sinc(x) * hamming(x / radius);
        case 5:
            return sinc(x) * blackman(x / radius);
        case 6:
            return sinc(x) * kaiser(x / radius);
        case 7:
            return sinc(x) * welch(x / radius);
        case 8:
            return said(x);
        case 9:
            return bc_spline(x);
        case 10:
            return bicubic(x);
        default: //11
            return nearest_neighbor(x);
        }
    }
    else //x >= radius
        return 0.0;
}

//samples one axis (x or y) at a time
float4 sample_1d(float2 uv)
{
    float2 dims;
    tex.GetDimensions(dims.x, dims.y);  
    float fcoord = dot(frac(uv * dims - 0.5), axis);
    dims = 1.0 / dims * axis;
    float2 base = uv - fcoord * dims;
    float4 color;
    float4 csum = 0.0; //weighted color sum
    float weight;
    float wsum = 0.0; //weight sum
    
    //antiringing
    bool ar = antiringing > 0.0 && widening_factor == 1.0; //enable antiringing
    float4 low = 1.0;
    float4 high = 0.0;
    
    float N2 = ceil(radius * widening_factor); //number of samples / 2
    for (float n = 1.0 - N2; n <= N2; n++) {
        color = tex.Sample(smp, base + dims * n);
        weight = get_weight(n - fcoord);
        csum += weight * color;
        wsum += weight;
        
        //antiringing
        if (ar && n >= 0.0 && n <= 1.0) {
            low = min(low, color);
            high = max(high, color);
        }
    }
    csum /= wsum; //normalize color values
    
    //antiringing
    if (ar)
        csum = lerp(csum, clamp(csum, low, high), antiringing);
    
    return csum;
}

float4 main(Vertex_shader_output vs_out) : SV_Target
{
    float4 color;
    if(kernel_index != 0)
        color = sample_1d(vs_out.texcoord);
    else //linear
        color = tex.Sample(smp, vs_out.texcoord);
    if (use_color_managment)
        return float4(color_transform(color.rgb), color.a);
    return color;
}
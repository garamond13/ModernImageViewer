Texture2D tex : register(t0);
SamplerState smp : register(s0);

cbuffer cb1 : register(b0)
{
    bool use_color_managment; //use color managment
    float2 axis; //x or y axis, (1, 0) or (0, 1)
    float blur_sigma;
    float blur_radius;
};

cbuffer cb2 : register(b1)
{
    int kernel_index;
    float radius; //kernel radius
    float blur; //kernel blur
    float2 kparam; //kernel specific parameters, kparam.x == kparam1, kparam.y == kparam2
    float antiringing; //antiringing strenght
    float scale; //if downsampling, > 1.0 else == 1.0
};

struct Vertex_shader_output
{
    float4 position : SV_Position; //xyzw
    float2 texcoord : TEXCOORD; //uv
};

//source corecrt_math_defines.h
#define M_PI 3.14159265358979323846 //pi
#define M_PI_2 1.57079632679489661923 //pi/2

//source float.h
#define FLT_EPSILON 1.192092896e-07 // smallest such that 1.0+FLT_EPSILON != 1.0

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

//should be (x == 0.0), but it can cause unexplainable artifacts
#define sinc(x) (x < FLT_EPSILON ? M_PI : sin(M_PI * x) / x)

#define cosine(x) (cos(M_PI_2 * x))

#define hann(x) (0.5 + 0.5 * cos(M_PI * x))

#define hamming(x) (0.54 + 0.46 * cos(M_PI * x))

#define blackman(x) (0.42 + 0.5 * cos(M_PI * x) + 0.08 * cos(2.0 * M_PI * x))

#define kaiser(x, beta) (bessel_i0(beta * sqrt(1.0 - x * x)))

#define welch(x) (1.0 - x * x)

//source https://www.hpl.hp.com/techreports/2007/HPL-2007-179.pdf
#define said(x, chi, eta) (cosh(sqrt(2.0 * eta) * M_PI * chi / (2.0 - eta) * x) * exp(-(M_PI * M_PI * chi * chi / ((2.0 - eta) * (2.0 - eta)) * x * x)))

#define bc_spline(x, b, c) (x < 1.0 ? (12.0 - 9.0 * b - 6.0 * c) * x * x * x + (-18.0 + 12.0 * b + 6.0 * c) * x * x + (6.0 - 2.0 * b) : (-b - 6.0 * c) * x * x * x + (6.0 * b + 30.0 * c) * x * x + (-12.0 * b - 48.0 * c) * x + (8.0 * b + 24.0 * c))

#define bicubic(x, alpha) (x < 1.0 ? (alpha + 2.0) * x * x * x - (alpha + 3.0) * x * x + 1.0 : alpha * x * x * x - 5.0 * alpha * x * x + 8.0 * alpha * x - 4.0 * alpha)

#define nearest_neighbor(x) (x < 0.5 ? 1.0 : 0.0)

float get_weight(float x)
{
    if (x < radius) {
        switch(kernel_index) {
        case 1:
            return sinc(x / blur) * sinc(x / radius);
        case 2:
            return sinc(x / blur) * cosine(x / radius);
        case 3:
            return sinc(x / blur) * hann(x / radius);
        case 4:
            return sinc(x / blur) * hamming(x / radius);
        case 5:
            return sinc(x / blur) * blackman(x / radius);
        case 6:
            return sinc(x / blur) * kaiser(x / radius, kparam.x);
        case 7:
            return sinc(x / blur) * welch(x / radius);
        case 8:
            return sinc(x / blur) * said(x, kparam.x, kparam.y);
        case 9:
            return bc_spline(x, kparam.x, kparam.y);
        case 10:
            return bicubic(x, kparam.x);
        default: //11
            return nearest_neighbor(x);
        }
    }
    else //x >= radius
        return 0.0;
}

//samples one axis (x or y) at a time
float4 main(Vertex_shader_output vs_out) : SV_Target
{
    float2 dims;
    tex.GetDimensions(dims.x, dims.y);  
    float fcoord = dot(frac(vs_out.texcoord * dims - 0.5), axis);
    dims = 1.0 / dims * axis;
    float2 base = vs_out.texcoord - fcoord * dims;
    float4 color;
    float4 csum = 0.0; //weighted color sum
    float weight;
    float wsum = 0.0; //weight sum
    
    //antiringing
    bool ar = antiringing > 0.0 && scale == 1.0; //enable antiringing
    float4 low = 1.0;
    float4 high = 0.0;
    
    float sampling_radius = ceil(radius * scale); //number of samples / 2
    for (float i = 1.0 - sampling_radius; i <= sampling_radius; ++i) {
        color = tex.SampleLevel(smp, base + dims * i, 0.0);
        weight = get_weight(abs((i - fcoord) / scale));
        csum += color * weight;
        wsum += weight;
        
        //antiringing
        if (ar && i >= 0.0 && i <= 1.0) {
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
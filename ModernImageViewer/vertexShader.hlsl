struct Vertex_shader_output
{
    float4 position : SV_POSITION; //xyzw
    float2 texcoord : TEXCOORD; //uv
};

//fullscreen triangle
Vertex_shader_output main(uint id : SV_VertexID)
{
    Vertex_shader_output vs_out;
    vs_out.texcoord = float2((id << 1) & 2, id & 2);
    vs_out.position = float4(vs_out.texcoord.x * 2.0 - 1.0, vs_out.texcoord.y * -2.0 + 1.0, 0.0, 1.0);
    return vs_out;
}
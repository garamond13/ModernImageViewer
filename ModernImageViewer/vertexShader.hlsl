struct Vertex_shader_output
{
    float4 position : SV_POSITION; //xyzw
    float2 texcoord : TEXCOORD; //uv
};

//fullscreen triangle
//based on https://www.slideshare.net/DevCentralAMD/vertex-shader-tricks-bill-bilodeau
Vertex_shader_output main(uint id : SV_VertexID)
{
    Vertex_shader_output output;
    
    //position
    output.position.x = (float) (id / 2) * 4.0 - 1.0;
    output.position.y = (float) (id % 2) * 4.0 - 1.0;
    output.position.z = 0.0;
    output.position.w = 1.0;
    
    //texcoord
    output.texcoord.x = (float) (id / 2) * 2.0;
    output.texcoord.y = 1.0 - (float) (id % 2) * 2.0;
    
    return output;
}
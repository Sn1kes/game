cbuffer cbPerObject
{
    float4x4 wvp_matrix;
};

struct VS_OUT
{
    float4 o_pos : SV_POSITION;
    float4 o_color : COLOR;
};

VS_OUT main(float3 i_pos : POSITION, float4 i_color : COLOR)
{
	VS_OUT vs_out;
    vs_out.o_pos = mul(float4(i_pos, 1.0f), wvp_matrix);
    vs_out.o_color = i_color;
    return vs_out;
}

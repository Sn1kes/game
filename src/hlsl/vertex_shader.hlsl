cbuffer cbPerObject
{
    float4x4 wvp_matrix;
};

struct VS_OUT
{
    float4 o_pos : SV_POSITION;
};

VS_OUT main(float2 i_pos : POSITION, uint i_instance_id : SV_InstanceID)
{
	i_pos.x += (i_instance_id & 63) * 2 - 64.f;
	i_pos.y += (i_instance_id >> 6) * 2 - 64.f;
	VS_OUT vs_out;
    vs_out.o_pos = mul(float4(i_pos.x, -1.f, i_pos.y, 1.0f), wvp_matrix);
    return vs_out;
}

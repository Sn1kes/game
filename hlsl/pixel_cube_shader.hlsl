struct PS_IN
{
    float4 i_pos : SV_POSITION;
    float4 i_color : COLOR;
};

float4 main(PS_IN ps_in) : SV_TARGET
{
    return ps_in.i_color;
}

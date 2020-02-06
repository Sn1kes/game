struct PS_IN
{
    float4 i_pos : SV_POSITION;
};

float4 main(PS_IN ps_in) : SV_TARGET
{
    return float4(0.f, 0.f, 1.f, 1.f);
}

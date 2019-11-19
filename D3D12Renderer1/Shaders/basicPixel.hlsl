struct VS_OUT
{
	float4 position : SV_POSITION;
	float3 normal : NORMAL;
};

float4 main(VS_OUT input) : SV_Target
{
	return float4(input.normal,1.0f);
}
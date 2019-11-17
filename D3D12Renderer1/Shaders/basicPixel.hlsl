struct VS_OUT
{
	float4 position : SV_POSITION;
};

cbuffer test : register(b0)
{
	float3 test;
}

float4 main(VS_OUT input) : SV_Target
{
	return float4(1,1,1,1);
}
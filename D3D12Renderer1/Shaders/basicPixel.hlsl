struct VS_OUT
{
	float4 position : SV_POSITION;
};

cbuffer ColorConstBuffer : register(b0)
{
	float3 color;
}

float4 main(VS_OUT input) : SV_Target
{
	return float4(color,1);
}
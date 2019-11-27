struct VS_INPUT
{
	float3 position : POSITION;
	float3 normal : NORMAL;
	float2 texcoord : TEXCOORD;
};

struct VS_OUT
{
	float4 position : SV_POSITION;
};

cbuffer lightSpaceCamera : register(b0)
{
	matrix projection;
	matrix view;
}

VS_OUT main(VS_INPUT input)
{
	VS_OUT output;

	matrix projectionView = mul(projection, view);

	output.position = mul(projectionView, float4(input.position, 1.0f));
	return output;
}
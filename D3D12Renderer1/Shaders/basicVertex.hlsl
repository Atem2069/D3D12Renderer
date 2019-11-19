struct VS_INPUT
{
	float3 position : POSITION;
	float3 normal : NORMAL;
};

struct VS_OUT
{
	float4 position : SV_POSITION;
	float3 normal : NORMAL;
};

cbuffer camera : register(b0)
{
	matrix projection;
	matrix view;
}

VS_OUT main(VS_INPUT input)
{
	VS_OUT output;

	matrix projView = mul(projection, view);;

	output.position = mul(projView,float4(input.position, 1.0f));
	output.normal = input.normal;
	return output;
}
struct VS_INPUT
{
	float3 position : POSITION;
	float3 normal : NORMAL;
	float2 texcoord : TEXCOORD;
};

struct VS_OUT
{
	float4 position : SV_POSITION;
	float3 normal : NORMAL;
	float2 texcoord : TEXCOORD;
	float4 camerapos : CAMERAPOS;
	float3 fragpos : FRAGPOS;
};

cbuffer camera : register(b0)
{
	matrix projection;
	matrix view;
	float4 position;
}

cbuffer object : register(b1)
{
	matrix model;
}

VS_OUT main(VS_INPUT input)
{
	VS_OUT output;

	matrix projView = mul(projection, view);;

	output.position = mul(projView,float4(input.position, 1.0f));
	output.normal = input.normal;
	output.texcoord = input.texcoord;
	output.camerapos = position;
	output.fragpos = input.position.xyz;
	return output;
}
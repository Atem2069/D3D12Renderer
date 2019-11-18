struct VS_INPUT
{
	float3 position : POSITION;
};

struct VS_OUT
{
	float4 position : SV_POSITION;
};

cbuffer vertexscale : register(b0)
{
	float4 scale;
}

VS_OUT main(VS_INPUT input)
{
	VS_OUT output;
	output.position = float4(input.position * scale.xyz, 1.0f);
	output.position.z += 1;
	return output;
}
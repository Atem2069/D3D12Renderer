struct VS_INPUT
{
	float3 position : POSITION;
};

struct VS_OUT
{
	float4 position : SV_POSITION;
};

VS_OUT main(VS_INPUT input)
{
	VS_OUT output;
	output.position = float4(input.position, 1.0f);
	return output;
}
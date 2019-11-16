struct VS_OUT
{
	float4 position : SV_POSITION;
};

float4 main(VS_OUT input) : SV_Target
{
	return float4(1,1,1,1);
}
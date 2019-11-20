struct VS_OUT
{
	float4 position : SV_POSITION;
	float3 normal : NORMAL;
};

float4 main(VS_OUT input) : SV_Target
{
	//return float4(input.normal,1.0f);

	//Basic lighting
	float3 lightColor = float3(1,1,1);
	float3 lightDirection = float3(-50, -1000, 0);
	float ambientTerm = 0.3f * lightColor;

	float3 lightDir = normalize(-lightDirection);

	float diffuseTerm = max(dot(normalize(input.normal), lightDir), 0.0f);

	float3 result = (ambientTerm + diffuseTerm) * lightColor;;
	return float4(result, 1.0f);
}
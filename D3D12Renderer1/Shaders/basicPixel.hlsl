struct VS_OUT
{
	float4 position : SV_POSITION;
	float3 normal : NORMAL;
	float2 texcoord : TEXCOORD;
};

cbuffer light : register(b0)
{
	float4 lightDirection;
};

Texture2D albedoTex : register(t0);
SamplerState samplerState : register(s0);

float4 main(VS_OUT input) : SV_Target
{
	//Basic lighting
	float width,height;
	albedoTex.GetDimensions(width, height);
	float3 texColor = albedoTex.Sample(samplerState,input.texcoord).xyz;
	if (!width || !height)
		texColor = float3(1, 1, 1);
	float3 lightColor = float3(1,1,1);
	float ambientTerm = 0.3f * lightColor;

	float3 lightDir = normalize(-lightDirection.xyz);

	float diffuseTerm = max(dot(normalize(input.normal), lightDir), 0.0f);

	float3 result = (ambientTerm + diffuseTerm) * texColor;
	return float4(result, 1.0f);
}
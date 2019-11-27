struct VS_OUT
{
	float4 position : SV_POSITION;
	float3 normal : NORMAL;
	float2 texcoord : TEXCOORD;
	float4 camerapos : CAMERAPOS;
	float3 fragpos : FRAGPOS;
	float4 fragposlightspace : FRAGPOSLIGHTSPACE;
};

cbuffer light : register(b0)
{
	float4 lightDirection;
};

Texture2D albedoTex : register(t0);
Texture2D shadowMapTex : register(t1);
SamplerState samplerState : register(s0);
SamplerComparisonState shadowSamplerState : register(s1);

#define BIAS 0.000006

float shadowCalculation(float4 fragPosLightSpace, float3 normal, float3 lightDir)
{
	float3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
	projCoords.x = projCoords.x / 2 + 0.5;
	projCoords.y = projCoords.y / -2 + 0.5;	//Flipped because Direct3D does UV flipping.
	//The Z is not transformed given that unlike OpenGL, the Z is already 0-1. No need unless you don't like shadows..

	float currentDepth = projCoords.z;

	float bias = max((BIAS * 10) * (1.0 - dot(normal, lightDir)), BIAS);

	float shadow = 0.0;
	float2 texelSize;
	shadowMapTex.GetDimensions(texelSize.x, texelSize.y);
	texelSize = 1.0f / texelSize;
	for (int x = -1; x <= 1; ++x)
	{
		for (int y = -1; y <= 1; ++y)
		{
			//float pcfDepth = shadowTex.Sample(shadowSampler, projCoords.xy + float2(x, y) * texelSize).r;
			//shadow += currentDepth - bias > pcfDepth ? 1.0 : 0.0;
			shadow += shadowMapTex.SampleCmpLevelZero(shadowSamplerState, projCoords.xy + float2(x, y)*texelSize, currentDepth - bias).r;
		}
	}
	shadow /= 9.0;
	return shadow;
}

float4 main(VS_OUT input) : SV_Target
{
	//Basic lighting
	float width,height;
	albedoTex.GetDimensions(width, height);
	float4 texColor = albedoTex.Sample(samplerState,input.texcoord).xyzw;
	if (!width || !height)
		texColor.xyz = float3(1, 1, 1);
	if (texColor.w < 0.5)
		clip(-1);
	float3 lightColor = float3(1,1,1);
	float ambientTerm = 0.3f * lightColor;

	float3 lightDir = normalize(-lightDirection.xyz);

	float diffuseTerm = max(dot(normalize(input.normal), lightDir), 0.0f);

	float3 viewDir = normalize(input.camerapos.xyz - input.fragpos);
	float3 halfwayDir = normalize(lightDir + viewDir);
	float specularTerm = pow(max(dot(normalize(input.normal), halfwayDir), 0.0f), 128.0f);

	float shadowFactor = shadowCalculation(input.fragposlightspace, normalize(input.normal), lightDir);

	float3 result = (ambientTerm + (1.0f - shadowFactor) * (diffuseTerm + specularTerm)) * texColor.xyz;
	return float4(result, 1.0f);
}
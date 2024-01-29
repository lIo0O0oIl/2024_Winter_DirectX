#ifndef _PARAMS_HLSL_
#define _PARAMS_HLSL_

#define MAXLIGHTS 16

struct Material
{
	float4 DiffuseAlbedo;
	float3 FresnelR0;
	float Shininess;
};

struct Light
{
	int LightType;		// 0 : directional, 1 : Point, 2 : Spot
	float3 LightPadding;
	float3 Strength;
	float FalloffStart;
	float3 Direction;
	float FalloffEnd;
	float3 Position;
	float SpotPower;
};

cbuffer cbPerObject : register(b0)
{
	float4x4 gWorld;
	float4x4 gTexTransform;
}

cbuffer cbMaterial : register(b1)
{
	float4 gDiffuseAlbedo;
	float3 gFresnelR0;
	float gRoughness;
	int gTexture_On;
	int gNormal_On;
	float2 gTexturePadding;
}

cbuffer cbPass : register(b2)
{
	float4x4 gView;
	float4x4 gInvView;
	float4x4 gProj;
	float4x4 gInvProj;
	float4x4 gViewProj;

	float4 gAmbientLight;
	float3 gEyePosW;
	int gLightCount;
	Light gLights[MAXLIGHTS];

	float4 gFogColor;
	float gFogStart;
	float gFogRange;
	float2 fogpadding;
}

TextureCube gCubeMap : register(t0);

Texture2D gTexture_0 : register(t1);
Texture2D gNormal_0 : register(t2);

SamplerState gSampler_0 : register(s0);

// normal map sample to world space
float3 NormalSampleToWorldSpace(float3 normalMapSample, float3 unitNormalW, float tangentW)
{
	// 0, 1 -> -1, 1
	float3 normalT = 2.0f * normalMapSample - 1.0f;

	float3 N = unitNormalW;
	float3 T = normalize(tangentW - dot(tangentW, N) * N);
	float3 B = cross(N, T);

	float3x3 TBN = float3x3(T, B, N);

	float3 bumpedNormalW = mul(normalT, TBN);

	return bumpedNormalW;
}

#endif
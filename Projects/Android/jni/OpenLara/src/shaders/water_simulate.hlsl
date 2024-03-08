#include "common.hlsl"

struct VS_OUTPUT {
	float4 pos        : POSITION;
	float2 texCoord   : TEXCOORD0;
	half2  maskCoord  : TEXCOORD1;
	float2 texCoordL  : TEXCOORD2;
	float2 texCoordR  : TEXCOORD3;
	float2 texCoordT  : TEXCOORD4;
	float2 texCoordB  : TEXCOORD5;
	half2  noiseCoord : TEXCOORD6;
};

#ifdef VERTEX

VS_OUTPUT main(VS_INPUT In) {
	VS_OUTPUT Out;

	float3 coord = In.aCoord.xyz * INV_SHORT_HALF;
	float4 uv = float4(coord.x, coord.y, coord.x, -coord.y) * 0.5 + 0.5;

	Out.pos       = float4(coord.xyz, 1.0);
	Out.maskCoord = uv.xy * uRoomSize.zw;
	Out.texCoord  = uv.zw * uTexParam.zw;
	#ifdef _GAPI_D3D9
		Out.texCoord += 0.5 * uTexParam.xy;
	#endif

	float3 d = float3(uTexParam.x, uTexParam.y, 0.0);
	Out.texCoordL  = Out.texCoord - d.xz;
	Out.texCoordR  = Out.texCoord + d.xz;
	Out.texCoordT  = Out.texCoord - d.zy;
	Out.texCoordB  = Out.texCoord + d.zy;
	Out.noiseCoord = Out.maskCoord + uParam.zw;

	return Out;
}

#else // PIXEL

#define WATER_VEL	1.4
#define WATER_VIS	0.995

half4 main(VS_OUTPUT In) : COLOR0 {
	half4 v = SAMPLE_2D_POINT(sNormal, In.texCoord.xy); // height, speed

	half average = (
		SAMPLE_2D_POINT(sNormal, In.texCoordL).x +
		SAMPLE_2D_POINT(sNormal, In.texCoordR).x +
		SAMPLE_2D_POINT(sNormal, In.texCoordT).x +
		SAMPLE_2D_POINT(sNormal, In.texCoordB).x) * 0.25;

// integrate
	v.y += (average - v.x) * WATER_VEL;
	v.y *= WATER_VIS;
	v.x += v.y;
	v.x += (SAMPLE_2D_LINEAR_WRAP(sDiffuse, In.noiseCoord).x * 2.0 - 1.0) * 0.00025;

	v *= SAMPLE_2D_POINT(sMask, In.maskCoord).r;

	return v;
}

#endif

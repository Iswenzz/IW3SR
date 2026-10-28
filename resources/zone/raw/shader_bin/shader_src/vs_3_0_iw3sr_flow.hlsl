#define PC
#define IS_VERTEX_SHADER 1
#define IS_PIXEL_SHADER 0
#include <shader.hlsl>

// The wind's direction (xy), the finest layer's repeats a second (z) and the
// units one of its repeats spans (w).
float4 waterFlow;

struct VertexShaderInput
{
	float4 position : POSITION;
};

struct PixelShaderInput
{
	float4 position : POSITION;
	float4 waves : TEXCOORD0;
	float4 swell : TEXCOORD1;
	float4 eyeFog : TEXCOORD2;
	float4 clip : TEXCOORD3;
};

PixelShaderInput vs_main(VertexShaderInput input)
{
	PixelShaderInput output;
	float4 world = mul(float4(input.position.xyz, 1.0f), worldMatrix);
	output.position = mul(float4(input.position.xyz, 1.0f), worldViewProjectionMatrix);

	// worldMatrix carries the renderer's eyeOffset, so world is relative to the
	// eye; the waves and the height come off the vertex, which is absolute.
	// Offsets wrap every repeat, so they never grow past float precision.
	float2 p = input.position.xy / waterFlow.w;
	float t = gameTime.w * waterFlow.z;
	float2 wind = waterFlow.xy;
	float2 side = float2(-wind.y, wind.x);
	output.waves.xy = p + frac(wind * t);
	float2 turned = float2(dot(p, float2(0.7986f, 0.6018f)), dot(p, float2(-0.6018f, 0.7986f))) * 1.37f;
	output.waves.zw = turned + frac((wind * 0.7f + side * 0.5f) * t * 1.21f);
	output.swell.xy = p * 0.29f + frac(wind * t * 0.37f);
	output.swell.zw = p * 2.3f + frac(side * t * 0.6f);

	output.eyeFog.xyz = inverseViewMatrix[3].xyz - world.xyz;
	output.eyeFog.w = saturate(exp2(length(output.eyeFog.xyz) * fogConsts.z + fogConsts.w));
	output.clip = float4(output.position.xy, output.position.w, input.position.z);
	return output;
}

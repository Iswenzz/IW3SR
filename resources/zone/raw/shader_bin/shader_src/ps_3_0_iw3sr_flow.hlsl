#define PC
#define IS_VERTEX_SHADER 0
#define IS_PIXEL_SHADER 1
#include <shader.hlsl>

// IzFF's iz_flow, for the iw3sr zone that stock water borrows from: a stock map's vertex alpha says
// nothing about its water, so the opacity comes from the depth alone.

// The shallows' colour (rgb) and the units of water that fade the ground under
// it to 1/e (w).
float4 waterShallow;
// The depths' colour (rgb) and the depth it is reached by, in units (w).
float4 waterDeep;
// Toward the sun (xyz) and its strength (w), nothing where the map has none.
float4 waterSun;
// The sun's colour (rgb) and how bright the foam is (w).
float4 waterSunColor;
// Wave strength (x), the distance they flatten out by (y), the depth foam
// reaches to (z), and how far the waves bend the planar reflection (w).
float4 waterShape;
// The height IW3SR drew its planar reflection for (x), and whether it did (y).
float4 waterPlanar;

struct PixelShaderInput
{
	float4 position : POSITION;
	float4 waves : TEXCOORD0;
	float4 swell : TEXCOORD1;
	float4 eyeFog : TEXCOORD2;
	float4 clip : TEXCOORD3;
};

float4 ps_main(PixelShaderInput input) : COLOR
{
	float distance = length(input.eyeFog.xyz);
	float3 eye = input.eyeFog.xyz / distance;

	float4 a = tex2D(detailMapSampler, input.waves.xy);
	float4 b = tex2D(detailMapSampler, input.waves.zw);
	float4 s = tex2D(detailMapSampler, input.swell.xy);
	float4 f = tex2D(detailMapSampler, input.swell.zw);
	float near = saturate(1.0f - distance / waterShape.y);
	float2 slope = (a.xy + b.xy - 1.0f) * 2.0f * (0.25f + 0.75f * near) + (s.xy * 2.0f - 1.0f) * 0.8f;
	float3 normal = normalize(float3(slope * waterShape.x, 1.0f));

	// Seen from under the surface, only the depths' colour.
	if (eye.z < 0.0f)
		return float4(lerp(fogColor.rgb, waterDeep.rgb * 1.5f, input.eyeFog.w), 0.85f);

	// What lies under this pixel, and how much water is in front of it. floatZ also
	// holds the view model: where that covers the water, the water further up the
	// screen stands in. Ground level with the surface just touches it; taken for the
	// hand, the depth test's tie there drew a flickering line.
	float2 screen = input.clip.xy / input.clip.z * float2(0.5f, -0.5f) + 0.5f + 0.5f * float2(texelSizeX, texelSizeY);
	float own = input.clip.z;
	float here = tex2Dlod(floatZSampler, float4(screen, 0.0f, 0.0f)).r;
	float above = tex2Dlod(floatZSampler, float4(screen.x, screen.y - 0.15f, 0.0f, 0.0f)).r;
	float further = tex2Dlod(floatZSampler, float4(screen.x, screen.y - 0.35f, 0.0f, 0.0f)).r;
	float hand = above > own ? above : further > own ? further : own + waterShallow.w * 1.5f;
	float thickness = max((here < min(own * 0.5f, 64.0f) ? hand : here) - own, 0.0f);
	float depth = thickness * eye.z;
	float through = exp(-thickness / waterShallow.w);
	float3 body = lerp(waterDeep.rgb, waterShallow.rgb, exp(-depth / waterDeep.w));

	float facing = saturate(dot(normal, eye));
	float fresnel = 0.02f + 0.98f * pow(1.0f - facing, 5.0f);
	float3 bounce = reflect(-eye, normal);
	bounce.z = abs(bounce.z);
	float3 sky = texCUBE(skyMapSampler, bounce).rgb;
	float3 planar = tex2D(specularMapSampler, float2(screen.x, 1.0f - screen.y) + normal.xy * waterShape.w).rgb;
	float mirrored = waterPlanar.y * saturate(1.0f - abs(input.clip.w - waterPlanar.x) * 0.125f);
	float3 reflection = lerp(sky, planar, mirrored);

	float3 halfway = normalize(waterSun.xyz + eye);
	float lobe = saturate(dot(normal, halfway));
	float glint = (pow(lobe, 900.0f) * 4.0f + pow(lobe, 90.0f) * 0.2f) * waterSun.w;

	float shore = saturate(1.0f - depth / waterShape.z);
	float foam = saturate((f.a * 0.6f + a.a * 0.4f - (1.0f - shore)) * 4.0f) * shore;

	float3 color = lerp(body, reflection, fresnel) + glint * waterSunColor.rgb;
	color = lerp(color, waterSunColor.rgb * waterSunColor.w, foam);
	// A stock map's water often lies a few units over a bed never meant to be seen, which IzFF's clear
	// shallows would show; it stays mostly opaque, and the shore only softens its edge.
	float alpha = lerp(max(1.0f - through, 0.85f), 1.0f, fresnel);
	alpha = saturate(max(alpha, foam) + glint) * lerp(0.5f, 1.0f, saturate(depth * 0.5f));
	color = lerp(fogColor.rgb, color, input.eyeFog.w);
	return float4(color, alpha);
}

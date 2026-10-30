#include "SSAO.hpp"

#include "Game/Renderer/Drawing/Text.hpp"
#include "Game/Renderer/Portal/Portal.hpp"
#include "Game/Renderer/Renderer.hpp"
#include "Game/System/Dvar.hpp"

namespace IW3SR
{
	// Nearer than this is the view model, which would only darken itself.
	constexpr float NEAR_LIMIT = 24.0f;

	// The occlusion fades out over the last quarter of sr_ssao_distance.
	constexpr float FAR_FADE = 0.75f;

	// InfinitePerspectiveMatrix scales both axes by this, so it comes back out with the field of view.
	constexpr float PROJECTION_EPSILON = 0.99951172f;

	struct QuadVertex
	{
		float X, Y, Z, W;
	};

	constexpr const char* VertexSource = R"(
float4 main(float4 position : POSITION) : POSITION
{
	return position;
}
)";

	// Horizon-based, as NVIDIA's HBAO: along each direction the highest point within the radius, over the surface's
	// own tangent raised by a bias. Float-Z holds each pixel's distance along the view axis, so a view space position
	// comes back from where the pixel sits on screen. c0: tan of the half fov (xy), a depth texel (zw). c1: radius
	// (x), strength (y), fade start and end (zw). c2: a texel of this pass (xy), the view model's reach (z).
	constexpr const char* OcclusionSource = R"(
sampler2D Depth : register(s0);
float4 View : register(c0);
float4 Shape : register(c1);
float4 Frame : register(c2);

static const int DIRECTIONS = 8;
static const int STEPS = 6;

// Raises every horizon above the surface's own tangent, so a flat surface never occludes itself.
static const float ANGLE_BIAS = 0.35;

// The position comes from where the depth was actually read: off a texel's centre, a steep floor puts it off its
// own plane by half a texel's change in depth, and that reads as occlusion in a grid.
float3 Position(float2 uv)
{
	uv = (floor(uv / View.zw) + 0.5) * View.zw;
	float z = tex2Dlod(Depth, float4(uv, 0, 0)).r;
	return float3((uv.x * 2 - 1) * View.x * z, (1 - uv.y * 2) * View.y * z, z);
}

// How far above the view plane v rises, as a tangent. z grows away from the eye.
float Tangent(float3 v)
{
	return -v.z * rsqrt(dot(v.xy, v.xy) + 0.0001);
}

float TangentToSine(float t)
{
	return t * rsqrt(t * t + 1);
}

// A 4x4 Bayer value: the composite's 4x4 blur covers one whole tile, so the pattern averages out.
float Bayer4(float2 pixel)
{
	float2 a = floor(pixel);
	float2 b = floor(a * 0.5);
	return frac(b.x * 0.5 + b.y * b.y * 0.75) * 0.25 + frac(a.x * 0.5 + a.y * a.y * 0.75);
}

float4 main(float2 pixel : VPOS) : COLOR0
{
	// The first depth texel of the 2x2 under this pixel, rather than the corner they share.
	float2 uv = (pixel * 2 + 0.5) * View.zw;
	float3 p = Position(uv);
	if (p.z < Frame.z || p.z > Shape.w)
		return float4(1, 1, 1, 1);

	// The surface's slope per depth texel, from the nearer neighbour on each axis so a silhouette does not bend it.
	float3 right = Position(uv + float2(View.z, 0)) - p;
	float3 left = p - Position(uv - float2(View.z, 0));
	float3 down = Position(uv + float2(0, View.w)) - p;
	float3 up = p - Position(uv - float2(0, View.w));
	float3 du = dot(right, right) < dot(left, left) ? right : left;
	float3 dv = dot(down, down) < dot(up, up) ? down : up;

	// The radius on screen, split in steps; under a texel a step finds nothing.
	float2 reach = min(Shape.x / (p.z * View.xy) * 0.5, 0.15);
	float2 step = reach / (STEPS + 1);
	if (max(step.x / View.z, step.y / View.w) < 1)
		return float4(1, 1, 1, 1);

	float rotation = Bayer4(pixel);
	float jitter = Bayer4(pixel + float2(1, 2));
	float radius2 = Shape.x * Shape.x;

	float occlusion = 0;
	[loop] for (int d = 0; d < DIRECTIONS; d++)
	{
		float angle = (d + rotation) * (6.2831853 / DIRECTIONS);
		float2 delta = float2(cos(angle), sin(angle)) * step;

		float3 tangent = du * (delta.x / View.z) + dv * (delta.y / View.w);
		float tanH = tan(min(atan(Tangent(tangent)) + ANGLE_BIAS, 1.5));
		float sinH = TangentToSine(tanH);

		float2 at = uv + delta * jitter;
		[loop] for (int s = 0; s < STEPS; s++)
		{
			at += delta;
			float3 v = Position(at) - p;
			float distance2 = dot(v, v);
			float tanS = Tangent(v);
			if (distance2 < radius2 && tanS > tanH)
			{
				float sinS = TangentToSine(tanS);
				occlusion += (1 - distance2 / radius2) * (sinS - sinH);
				tanH = tanS;
				sinH = sinS;
			}
		}
	}

	float ao = saturate(1 - Shape.y * occlusion / DIRECTIONS);
	float weight = saturate((p.z - Frame.z) / Frame.z) * saturate((Shape.w - p.z) / (Shape.w - Shape.z));
	ao = lerp(1, ao, weight);
	return float4(ao, ao, ao, 1);
}
)";

	// A 4x4 blur over the half size occlusion, each tap weighted by how close its depth is to this pixel's plane,
	// so the shadow of one surface does not bleed onto another while a floor seen at a grazing angle still blurs
	// across. c2 here is a texel of the occlusion, c3 the projection's depth terms.
	constexpr const char* CompositeSource = R"(
sampler2D Occlusion : register(s0);
sampler2D Depth : register(s1);
float4 View : register(c0);
float4 Shape : register(c1);
float4 Frame : register(c2);
float4 Projection : register(c3);

float4 main(float2 pixel : VPOS, out float depth : DEPTH) : COLOR0
{
	float2 uv = (pixel + 0.5) * View.zw;
	float z = tex2Dlod(Depth, float4(uv, 0, 0)).r;
	float tolerance = max(z * 0.02, 1.0);

	// Placed just in front of the surface float-Z holds, so the scene's depth test drops it wherever something
	// float-Z leaves out was drawn nearer: water, and anything else without a depth prepass.
	float front = max(z - max(z * 0.001, 0.5), 1.0);
	depth = Projection.x + Projection.y / front;

	// The depth's slope per texel, from the nearer neighbour on each axis so a silhouette does not tilt it.
	float right = tex2Dlod(Depth, float4(uv + float2(View.z, 0), 0, 0)).r - z;
	float left = z - tex2Dlod(Depth, float4(uv - float2(View.z, 0), 0, 0)).r;
	float down = tex2Dlod(Depth, float4(uv + float2(0, View.w), 0, 0)).r - z;
	float up = z - tex2Dlod(Depth, float4(uv - float2(0, View.w), 0, 0)).r;
	float2 slope = float2(abs(right) < abs(left) ? right : left, abs(down) < abs(up) ? down : up);

	float sum = 0;
	float weight = 0;
	[unroll] for (int y = 0; y < 4; y++)
	{
		[unroll] for (int x = 0; x < 4; x++)
		{
			float2 offset = (float2(x, y) - 1.5) * Frame.xy;
			float2 tap = uv + offset;
			float expected = z + dot(slope, offset / View.zw);
			float w = exp(-abs(tex2Dlod(Depth, float4(tap, 0, 0)).r - expected) / tolerance);
			sum += tex2Dlod(Occlusion, float4(tap, 0, 0)).r * w;
			weight += w;
		}
	}
	float ao = sum / max(weight, 0.0001);
	return float4(ao, ao, ao, 1);
}
)";

	template <typename T>
	static void Release(T*& object)
	{
		if (object)
			object->Release();
		object = nullptr;
	}

	// Compiled once on the render thread. A failure is logged and turns the effect off until the next reset.
	static ID3DBlob* Compile(const char* source, const char* target)
	{
		ID3DBlob* bytecode = nullptr;
		ID3DBlob* errors = nullptr;
		const HRESULT hr = D3DCompile(source, std::strlen(source), "SSAO", nullptr, nullptr, "main", target,
			D3DCOMPILE_OPTIMIZATION_LEVEL3, 0, &bytecode, &errors);

		if (errors)
		{
			if (FAILED(hr))
				Log::WriteLine(Channel::Error, "SSAO: {}", static_cast<const char*>(errors->GetBufferPointer()));
			errors->Release();
		}
		return SUCCEEDED(hr) ? bytecode : nullptr;
	}

	static IDirect3DPixelShader9* CreatePixelShader(IDirect3DDevice9* device, const char* source)
	{
		ID3DBlob* bytecode = Compile(source, "ps_3_0");
		IDirect3DPixelShader9* shader = nullptr;
		if (bytecode)
		{
			device->CreatePixelShader(static_cast<const DWORD*>(bytecode->GetBufferPointer()), &shader);
			bytecode->Release();
		}
		return shader;
	}

	static void DrawQuad(IDirect3DDevice9* device)
	{
		static constexpr QuadVertex quad[4] = {
			{ -1.0f, 1.0f, 0.0f, 1.0f },
			{ 1.0f, 1.0f, 0.0f, 1.0f },
			{ -1.0f, -1.0f, 0.0f, 1.0f },
			{ 1.0f, -1.0f, 0.0f, 1.0f },
		};
		device->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, quad, sizeof(QuadVertex));
	}

	static void PointSampler(IDirect3DDevice9* device, DWORD sampler)
	{
		device->SetSamplerState(sampler, D3DSAMP_MINFILTER, D3DTEXF_POINT);
		device->SetSamplerState(sampler, D3DSAMP_MAGFILTER, D3DTEXF_POINT);
		device->SetSamplerState(sampler, D3DSAMP_MIPFILTER, D3DTEXF_NONE);
		device->SetSamplerState(sampler, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP);
		device->SetSamplerState(sampler, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP);
		device->SetSamplerState(sampler, D3DSAMP_SRGBTEXTURE, FALSE);
	}

	void GSSAO::Initialize()
	{
		// Off unless asked for: it draws the float-Z pass every frame, a second depth draw of the scene.
		Enabled = Dvar::RegisterBool("sr_ssao", DVAR_SAVED, "Screen-space ambient occlusion", false);
		Strength = Dvar::RegisterFloat("sr_ssao_strength", DVAR_SAVED, "Ambient occlusion strength", 1.0f, 0.0f, 3.0f);
		Radius = Dvar::RegisterFloat("sr_ssao_radius", DVAR_SAVED, "Ambient occlusion radius, in units", 24.0f, 4.0f,
			128.0f);
		Distance = Dvar::RegisterFloat("sr_ssao_distance", DVAR_SAVED, "Ambient occlusion fades out by this distance",
			4000.0f, 500.0f, 20000.0f);
		Debug = Dvar::RegisterBool("sr_ssao_debug", DVAR_TEMP,
			"Show the ambient occlusion buffer instead of the scene, and how far the pass got", false);
	}

	void GSSAO::DrawDebug()
	{
		if (!Debug || !Debug->current.enabled)
			return;

		static GText text{ "", FONT_NORMAL, 10, 260, 1.2f, vec4(0, 1, 1, 1) };
		text.Value = std::format("ssao request: {}\nssao pass: {}", RequestStage.load(), RenderStage.load());
		text.Render();
	}

	// Everything the device owns goes before a reset; the next frame that draws builds it again.
	void GSSAO::Shutdown()
	{
		Release(TargetSurface);
		Release(Target);
		Release(State);
		Release(VertexShader);
		Release(OcclusionShader);
		Release(CompositeShader);
		TargetSize = {};
		Failed = false;
	}

	// The occlusion is read from float-Z, which the engine does not draw unless asked.
	void GSSAO::RequestDepth()
	{
		if (!Enabled || !Enabled->current.enabled)
			return void(RequestStage = "off");
		if (GPortal::Rendering)
			return;

		RequestStage = GRenderer::RequestFloatZ() ? "float-z requested" : "no float-z target or no views";
	}

	// Run on the render thread once the scene is drawn, with the scene's target still bound.
	void GSSAO::Render(const GfxViewInfo* viewInfo)
	{
		if (!Enabled || !Enabled->current.enabled)
			return void(RenderStage = "off");
		if (Failed)
			return void(RenderStage = "shaders failed to compile (IW3SR console)");
		if (!viewInfo || !viewInfo->needsFloatZ)
			return void(RenderStage = "the view drew no float-z");

		IDirect3DTexture9* depth = Depth();
		IDirect3DDevice9* device = dx ? dx->device : nullptr;
		if (!depth || !device || dx->deviceLost)
			return void(RenderStage = "no depth texture or device");

		if (!Create(device))
		{
			Failed = true;
			return void(RenderStage = "shaders failed to compile (IW3SR console)");
		}

		const GfxRenderTarget& source = gfx_renderTargets[R_RENDERTARGET_FLOAT_Z];
		const vec2 size(static_cast<float>(source.width), static_cast<float>(source.height));
		if (size.x < 2.0f || size.y < 2.0f || !Resize(device, glm::ceil(size * 0.5f)))
			return void(RenderStage = "no occlusion target");

		const GfxMatrix& projection = viewInfo->viewParms.projectionMatrix;
		if (projection.m[0][0] <= 0.0f || projection.m[1][1] <= 0.0f)
			return void(RenderStage = "no projection");
		RenderStage = "drawn";

		const float distance = Distance->current.value;
		const vec4 view(PROJECTION_EPSILON / projection.m[0][0], PROJECTION_EPSILON / projection.m[1][1], 1.0f / size.x,
			1.0f / size.y);
		const vec4 shape(Radius->current.value, Strength->current.value, distance * FAR_FADE, distance);
		const vec4 depthTerms(projection.m[2][2], projection.m[3][2], 0.0f, 0.0f);
		Draw(device, depth, view, shape, depthTerms);
	}

	IDirect3DTexture9* GSSAO::Depth()
	{
		const GfxRenderTarget& target = gfx_renderTargets[R_RENDERTARGET_FLOAT_Z];
		if (!target.surface.color || !target.image)
			return nullptr;
		return target.image->texture.map;
	}

	bool GSSAO::Create(IDirect3DDevice9* device)
	{
		if (State)
			return true;

		if (ID3DBlob* bytecode = Compile(VertexSource, "vs_3_0"))
		{
			device->CreateVertexShader(static_cast<const DWORD*>(bytecode->GetBufferPointer()), &VertexShader);
			bytecode->Release();
		}
		OcclusionShader = CreatePixelShader(device, OcclusionSource);
		CompositeShader = CreatePixelShader(device, CompositeSource);

		if (!VertexShader || !OcclusionShader || !CompositeShader || FAILED(device->CreateStateBlock(D3DSBT_ALL, &State)))
		{
			Release(State);
			Release(VertexShader);
			Release(OcclusionShader);
			Release(CompositeShader);
			return false;
		}
		return true;
	}

	bool GSSAO::Resize(IDirect3DDevice9* device, const vec2& size)
	{
		if (Target && TargetSize == size)
			return true;

		Release(TargetSurface);
		Release(Target);
		TargetSize = {};

		if (FAILED(device->CreateTexture(static_cast<UINT>(size.x), static_cast<UINT>(size.y), 1, D3DUSAGE_RENDERTARGET,
				D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT, &Target, nullptr))
			|| FAILED(Target->GetSurfaceLevel(0, &TargetSurface)))
		{
			Release(TargetSurface);
			Release(Target);
			return false;
		}
		TargetSize = size;
		return true;
	}

	// The occlusion goes to its own target, then multiplies the scene through blending, so the scene is never
	// read back. The device comes back as the engine left it: its targets by hand, the rest through the state
	// block, which keeps the engine's own record of the device true.
	void GSSAO::Draw(IDirect3DDevice9* device, IDirect3DTexture9* depth, const vec4& view, const vec4& shape,
		const vec4& projection)
	{
		IDirect3DSurface9* scene = nullptr;
		IDirect3DSurface9* stencil = nullptr;
		D3DVIEWPORT9 sceneViewport{};
		if (FAILED(device->GetRenderTarget(0, &scene)) || !scene)
			return;
		device->GetDepthStencilSurface(&stencil);
		device->GetViewport(&sceneViewport);
		State->Capture();

		device->SetVertexShader(VertexShader);
		device->SetFVF(D3DFVF_XYZW);
		device->SetRenderState(D3DRS_ZENABLE, D3DZB_FALSE);
		device->SetRenderState(D3DRS_ZWRITEENABLE, FALSE);
		device->SetRenderState(D3DRS_STENCILENABLE, FALSE);
		device->SetRenderState(D3DRS_ALPHATESTENABLE, FALSE);
		device->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
		device->SetRenderState(D3DRS_FOGENABLE, FALSE);
		device->SetRenderState(D3DRS_SCISSORTESTENABLE, FALSE);
		device->SetRenderState(D3DRS_CLIPPLANEENABLE, 0);
		device->SetRenderState(D3DRS_FILLMODE, D3DFILL_SOLID);
		device->SetRenderState(D3DRS_SRGBWRITEENABLE, FALSE);
		device->SetPixelShaderConstantF(0, &view.x, 1);
		device->SetPixelShaderConstantF(1, &shape.x, 1);

		const vec4 texel(1.0f / TargetSize.x, 1.0f / TargetSize.y, NEAR_LIMIT, 0.0f);
		device->SetPixelShaderConstantF(2, &texel.x, 1);

		const D3DVIEWPORT9 half = { 0, 0, static_cast<DWORD>(TargetSize.x), static_cast<DWORD>(TargetSize.y), 0.0f, 1.0f };
		device->SetRenderTarget(0, TargetSurface);
		device->SetDepthStencilSurface(nullptr);
		device->SetViewport(&half);
		device->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
		device->SetRenderState(D3DRS_COLORWRITEENABLE, 0xF);
		device->SetPixelShader(OcclusionShader);
		device->SetTexture(0, depth);
		PointSampler(device, 0);
		DrawQuad(device);

		device->SetRenderTarget(0, scene);
		device->SetDepthStencilSurface(stencil);
		device->SetViewport(&sceneViewport);
		device->SetRenderState(D3DRS_ZENABLE, stencil ? D3DZB_TRUE : D3DZB_FALSE);
		device->SetRenderState(D3DRS_ZFUNC, D3DCMP_LESSEQUAL);
		device->SetPixelShaderConstantF(3, &projection.x, 1);
		device->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
		device->SetRenderState(D3DRS_SEPARATEALPHABLENDENABLE, FALSE);
		device->SetRenderState(D3DRS_BLENDOP, D3DBLENDOP_ADD);
		// sr_ssao_debug shows the occlusion itself in place of the scene.
		const bool debug = Debug && Debug->current.enabled;
		device->SetRenderState(D3DRS_SRCBLEND, debug ? D3DBLEND_ONE : D3DBLEND_ZERO);
		device->SetRenderState(D3DRS_DESTBLEND, debug ? D3DBLEND_ZERO : D3DBLEND_SRCCOLOR);
		device->SetRenderState(
			D3DRS_COLORWRITEENABLE, D3DCOLORWRITEENABLE_RED | D3DCOLORWRITEENABLE_GREEN | D3DCOLORWRITEENABLE_BLUE);
		device->SetPixelShader(CompositeShader);
		device->SetTexture(0, Target);
		device->SetTexture(1, depth);
		PointSampler(device, 0);
		PointSampler(device, 1);
		DrawQuad(device);

		scene->Release();
		if (stencil)
			stencil->Release();
		State->Apply();
	}
}

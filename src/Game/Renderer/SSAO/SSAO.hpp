#pragma once
#include "Game/Base.hpp"

namespace IW3SR
{
	// Screen-space ambient occlusion read from the engine's float-Z, drawn at half size, then blurred and
	// multiplied into the scene ahead of IW3SR's own 3D overlays.
	class API GSSAO
	{
	public:
		static void Initialize();
		static void Shutdown();
		static void RequestDepth();
		static void Render(const GfxViewInfo* viewInfo);
		static void DrawDebug();

	private:
		static inline dvar_s* Enabled = nullptr;
		static inline dvar_s* Strength = nullptr;
		static inline dvar_s* Radius = nullptr;
		static inline dvar_s* Distance = nullptr;
		static inline dvar_s* Debug = nullptr;

		// Where the main thread's request and the render thread's pass got to, for sr_ssao_debug.
		static inline std::atomic<const char*> RequestStage = "off";
		static inline std::atomic<const char*> RenderStage = "off";

		static inline IDirect3DVertexShader9* VertexShader = nullptr;
		static inline IDirect3DPixelShader9* OcclusionShader = nullptr;
		static inline IDirect3DPixelShader9* CompositeShader = nullptr;
		static inline IDirect3DStateBlock9* State = nullptr;
		static inline IDirect3DTexture9* Target = nullptr;
		static inline IDirect3DSurface9* TargetSurface = nullptr;
		static inline vec2 TargetSize{};
		static inline bool Failed = false;

		static IDirect3DTexture9* Depth();
		static bool Create(IDirect3DDevice9* device);
		static bool Resize(IDirect3DDevice9* device, const vec2& size);
		static void Draw(IDirect3DDevice9* device, IDirect3DTexture9* depth, const vec4& view, const vec4& shape);
	};
}

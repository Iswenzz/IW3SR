#pragma once
#include "Game/Base.hpp"

namespace IW3SR
{
	// A placed portal, read back from the flat quad entity the mod spawns on the surface.
	// The entity's forward axis is the surface normal, matching trace["angles"] in GSC.
	struct PortalEndpoint
	{
		vec3 Origin{};
		vec3 Forward{};
		vec3 Right{};
		vec3 Up{};
		Material* Surface = nullptr;
		int Entity = -1;
	};

	// Offscreen colour buffer holding one portal's view of the far side.
	struct PortalTarget
	{
		Ref<Texture> Color = nullptr;
		vec2 Size{};
	};

	// A material in the loaded world that draws through the portal_view technique set. Original is
	// whatever colour map the image was holding when this frame's swap went in, read fresh each
	// frame rather than kept from Discover: a zone can be unloaded and its images recycled while
	// the list still stands, and handing back a pointer out of the old one would have the engine
	// release a texture that is already gone.
	struct PortalSurface
	{
		Material* Material = nullptr;
		GfxImage* Image = nullptr;
		IDirect3DTexture9* Original = nullptr;
	};

	class API GPortal
	{
	public:
		static inline std::atomic<bool> Rendering = false;

		static void Initialize();
		static void Shutdown();
		static void BeginFrame();
		static void EndFrame();
		static void DrawDebug();

	private:
		static inline PortalTarget Targets[2];
		static inline std::vector<PortalSurface> Surfaces;
		static inline GfxWorld* KnownWorld = nullptr;
		static inline bool Swapped = false;
		static inline int Missed = 0;
		static inline int Rendered[2] = { -1, -1 };

		static inline dvar_s* Enabled = nullptr;
		static inline dvar_s* Scale = nullptr;
		static inline dvar_s* Distance = nullptr;
		static inline dvar_s* Debug = nullptr;
		static inline dvar_s* Threaded = nullptr;

		static inline int DebugFrames = 0;
		static inline int DebugModels = 0;
		static inline int DebugMatched = 0;
		static inline int DebugCaptured = 0;
		static inline int DebugViews = 0;
		static inline std::string DebugStage;
		static inline std::string DebugMaterial = "-";
		static inline std::string DebugTechniques = "-";

		static bool Ready();
		static void Discover();
		static bool Collect(PortalEndpoint (&pair)[2]);
		static bool Paired();
		static bool Visible(const PortalEndpoint& endpoint);
		static void BeginCommandList();
		static void Render(int index, const PortalEndpoint& into, const PortalEndpoint& out);
		static bool Resize(int index, PortalTarget& target, const vec2& size);
		static bool Capture(PortalTarget& target);
		static uint32_t Unbind(IDirect3DTexture9* texture);
		static void Blank();
		static void Bind(const PortalTarget& target, const PortalEndpoint& endpoint);
		static void Assign(Material* material, IDirect3DTexture9* texture);
		static void Swap();
		static void Restore();
		static vec3 Through(const PortalEndpoint& into, const PortalEndpoint& out, const vec3& v, bool position);
	};
}

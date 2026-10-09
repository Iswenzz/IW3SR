#pragma once
#include "Game/Base.hpp"

namespace IW3SR
{
	// One height of flat water in the loaded world, and the box its surfaces cover.
	struct WaterPlane
	{
		float Height = 0.0f;
		vec3 Mins{};
		vec3 Maxs{};
	};

	// A material drawing through IzFF's iz_flow: the image it reads the planar reflection from, and
	// the constant that tells its shader which height that reflection was drawn for.
	struct WaterSurface
	{
		Material* Material = nullptr;
		GfxImage* Image = nullptr;
		IDirect3DTexture9* Original = nullptr;
		vec4* Plane = nullptr;
	};

	class API GWater
	{
	public:
		static void Initialize();
		static void Shutdown();
		static void BeginFrame();
		static void EndFrame();
		static void DrawDebug();

	private:
		static inline Ref<Texture> Target = nullptr;
		static inline vec2 TargetSize{};
		static inline std::vector<WaterSurface> Surfaces;
		static inline std::vector<WaterPlane> Planes;
		static inline GfxWorld* KnownWorld = nullptr;
		static inline int KnownCount = -1;
		static inline int SettledScans = 0;
		static inline bool Swapped = false;

		static inline dvar_s* Enabled = nullptr;
		static inline dvar_s* Scale = nullptr;
		static inline dvar_s* Distance = nullptr;
		static inline dvar_s* Threaded = nullptr;
		static inline dvar_s* Debug = nullptr;

		static inline std::string DebugStage = "-";
		static inline float DebugHeight = 0.0f;
		static inline int DebugRendered = 0;

		static bool Ready();
		static void Discover();
		static void FindPlanes();
		static const WaterPlane* Pick();
		static bool Render(const WaterPlane& plane);
		static bool Resize(const vec2& size);
		static bool Capture();
		static void Bind(float height);
		static void Restore();
	};
}

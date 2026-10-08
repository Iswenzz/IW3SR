#pragma once
#include "Game/Base.hpp"

namespace IW3SR
{
	// How far the sun's shadow map reaches, in world units: the near partition's width and the far
	// one's. Zero keeps the engine's own.
	struct SunShadowRange
	{
		float Near = 0;
		float Far = 0;
		bool Force = false;
	};

	class API GShadows
	{
	public:
		static inline SunShadowRange Map;
		static inline SunShadowRange Override;
		static inline bool UseOverride = false;

		static void Initialize();
		static void LoadMap();
		static void BeginFrame();
		static void ChooseShadowedLights(GfxViewInfo* viewInfo);
		static std::array<int, 6> CasterCounts();
		static std::array<int, 2> DrawBuffers();

	private:
		static inline double Ratio = 4.0;
		static inline float RatioFixed = 4.0f;
		static inline bool Patched = false;
		static inline bool Applied = false;
		static inline float PlayerSampleSize = 0.25f;

		static const SunShadowRange& Current();
		static std::string WorldspawnKey(const std::string& entities, const char* key);
	};
}

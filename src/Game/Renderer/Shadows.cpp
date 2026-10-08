#include "Shadows.hpp"

namespace IW3SR
{
	// R_GenerateSortedDrawSurfs sets the partition ratio each frame: 4 / sm_sunShadowScale on shader
	// model 3 and a flat 4 otherwise, each loaded from a constant the whole binary shares. The two
	// loads are pointed at values owned here, so nothing else that reads those constants moves.
	constexpr uintptr_t RatioSite = 0x5F9AD8;
	constexpr uintptr_t RatioFixedSite = 0x5F9AF8;
	constexpr uint32_t StockRatio = 0x70B340;
	constexpr uint32_t StockRatioFixed = 0x6BDF00;

	// R_AddStaticModelSurfacesForShadow draws a static model into the sun shadow map only once it holds
	// one of the 3072 static model lighting entries a frame has, which the camera's own models take
	// first and the far partition asks for last. A map built of models runs out, and its far shadows
	// lose a share of their casters that grows and shrinks as the view turns. Casting reads no lighting,
	// so the call is replaced by its success: mov al, 1.
	constexpr uintptr_t CasterLightingSite = 0x63B2B4;
	constexpr uint8_t CasterLightingCall[5] = { 0xE8, 0xF7, 0x38, 0xFF, 0xFF };
	constexpr uint8_t CasterLightingSkip[5] = { 0xB0, 0x01, 0x90, 0x90, 0x90 };

	// The near partition is 1024 texels of sm_sunSampleSizeNear units, and the far one ratio times
	// as wide.
	constexpr float PartitionTexels = 1024.0f;

	// The scene's draw surface stages for the sun shadow map: world, static models and entities for
	// the near partition, then the same three for the far one.
	constexpr int SunShadowStages = 15;

	static_assert(offsetof(GfxScene, shadowableLightIsUsed) == 0xE4000);
	static_assert(offsetof(r_global_permanent_t, world) == 0x20A0);
	static_assert(offsetof(GfxWorld, sunPrimaryLightIndex) == 0xD8);

	// Both loads are checked before either is touched, so a build that moved them keeps the stock
	// range rather than half of a patch.
	void GShadows::Initialize()
	{
		if (Memory::Get<uint16_t>(RatioSite) != 0x3DDC || Memory::Get<uint32_t>(RatioSite + 2) != StockRatio
			|| Memory::Get<uint16_t>(RatioFixedSite) != 0x05D9
			|| Memory::Get<uint32_t>(RatioFixedSite + 2) != StockRatioFixed)
			return;

		Memory::Set<uint32_t>(RatioSite + 2, static_cast<uint32_t>(reinterpret_cast<uintptr_t>(&Ratio)));
		Memory::Set<uint32_t>(RatioFixedSite + 2, static_cast<uint32_t>(reinterpret_cast<uintptr_t>(&RatioFixed)));
		Patched = true;

		bool stock = true;
		for (size_t i = 0; i < sizeof(CasterLightingCall); i++)
			stock = stock && Memory::Get<uint8_t>(CasterLightingSite + i) == CasterLightingCall[i];
		if (stock)
		{
			for (size_t i = 0; i < sizeof(CasterLightingSkip); i++)
				Memory::Set<uint8_t>(CasterLightingSite + i, CasterLightingSkip[i]);
		}
	}

	// A map states its own range in worldspawn, for one whose world is mostly models: CoD4 shadows a
	// model through the map alone, and the stock 1024 units leave a city in full sun a street away.
	void GShadows::LoadMap()
	{
		Map = {};
		if (!cm || !cm->mapEnts || !cm->mapEnts->entityString)
			return;

		const std::string entities = cm->mapEnts->entityString;
		const auto number = [&](const char* key)
		{
			const std::string value = WorldspawnKey(entities, key);
			return value.empty() ? 0.0f : std::max(0.0f, static_cast<float>(std::atof(value.c_str())));
		};
		Map.Near = number("sunshadownear");
		Map.Far = number("sunshadowfar");
		Map.Force = WorldspawnKey(entities, "sunshadowforce") == "1";
	}

	// sm_sunSampleSizeNear is written straight into the dvar while a range is asked for, and handed back
	// to whatever the player had set once nothing asks for one.
	void GShadows::BeginFrame()
	{
		static const auto sm_sunSampleSizeNear = Dvar::Find("sm_sunSampleSizeNear");
		if (!Patched || !sm_sunSampleSizeNear)
			return;

		const SunShadowRange& range = Current();
		if (range.Near <= 0 && range.Far <= 0)
		{
			if (Applied)
			{
				sm_sunSampleSizeNear->current.value = PlayerSampleSize;
				Ratio = 4.0;
				RatioFixed = 4.0f;
				Applied = false;
			}
			return;
		}

		if (!Applied)
			PlayerSampleSize = sm_sunSampleSizeNear->current.value;
		const float sample =
			range.Near > 0 ? std::clamp(range.Near / PartitionTexels, 0.0625f, 32.0f) : PlayerSampleSize;
		const float width = sample * PartitionTexels;
		const float reach = range.Far > 0 ? std::max(range.Far, width) : width * 4.0f;

		// A whole ratio keeps the near texel grid on the far one's, which the view origin snaps to;
		// otherwise near shadows crawl as the view moves.
		sm_sunSampleSizeNear->current.value = sample;
		Ratio = std::max(1.0, std::round(static_cast<double>(reach / width)));
		RatioFixed = static_cast<float>(Ratio);
		Applied = true;
	}

	// The sun gets its shadow map only in a frame that draws a lit world surface naming it, and a map
	// built of models may draw none: its models then fall back to the light grid's one sun value, so
	// a room under a roof is sunlit. Naming it here is what such a surface would have done, and is
	// still undone by sm_sunEnable.
	void GShadows::ChooseShadowedLights(GfxViewInfo* viewInfo)
	{
		static const auto sm_sunEnable = Dvar::Find("sm_sunEnable");

		if (Current().Force && sm_sunEnable && sm_sunEnable->current.enabled && rgp->world)
		{
			const uint32_t sun = rgp->world->sunPrimaryLightIndex;
			if (sun && sun < 128)
				scene->shadowableLightIsUsed[sun >> 5] |= 1u << (sun & 31);
		}
		R_ChooseShadowedLights_h(viewInfo);
	}

	// How many surfaces each partition drew into the sun shadow map this frame, near then far. The
	// near partition holds up to 4096 of each kind and the far one 8192.
	std::array<int, 6> GShadows::CasterCounts()
	{
		std::array<int, 6> counts{};
		for (int i = 0; i < 6; i++)
			counts[i] = scene->drawSurfCount[SunShadowStages + i];
		return counts;
	}

	// What the frame being drawn used of the lists every view shares: 65536 words naming each draw
	// surface's models (one run per static model surface), and 32768 draw surfaces.
	std::array<int, 2> GShadows::DrawBuffers()
	{
		const GfxBackEndData* data = gfx_backEndData ? *gfx_backEndData : nullptr;
		return data ? std::array<int, 2>{ data->primDrawSurfPos, data->drawSurfCount } : std::array<int, 2>{};
	}

	const SunShadowRange& GShadows::Current()
	{
		return UseOverride ? Override : Map;
	}

	// The value of a key of the first entity, which a compiled map always opens with worldspawn.
	std::string GShadows::WorldspawnKey(const std::string& entities, const char* key)
	{
		const size_t open = entities.find('{');
		const size_t close = entities.find('}', open);
		if (open == std::string::npos || close == std::string::npos)
			return {};

		const std::string block = entities.substr(open + 1, close - open - 1);
		const std::string quoted = std::string("\"") + key + "\"";
		for (size_t at = block.find('"'); at != std::string::npos;)
		{
			const size_t keyEnd = block.find('"', at + 1);
			const size_t valueStart = keyEnd == std::string::npos ? keyEnd : block.find('"', keyEnd + 1);
			const size_t valueEnd = valueStart == std::string::npos ? valueStart : block.find('"', valueStart + 1);
			if (valueEnd == std::string::npos)
				break;
			if (_stricmp(block.substr(at, keyEnd - at + 1).c_str(), quoted.c_str()) == 0)
				return block.substr(valueStart + 1, valueEnd - valueStart - 1);
			at = block.find('"', valueEnd + 1);
		}
		return {};
	}
}

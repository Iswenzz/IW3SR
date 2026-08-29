#pragma once
// Stands in for the game's Game/Base.hpp so the shipping movement sources compile into the
// simulator untouched. The simulator's include path puts this directory ahead of src/, so
// Movements/*.cpp pick this up instead of dragging in the renderer, hooks and engine bindings.
#include "Engine/Types.hpp"

#include <algorithm>
#include <cstring>
#include <string>

namespace IW3SR
{
	// Enough of a dvar for the movement code, which only ever reads a value out of one.
	struct dvar_value
	{
		float value;
		int integer;
		bool enabled;
	};

	struct dvar_s
	{
		const char* name;
		dvar_value current;
	};

	class Dvar
	{
	public:
		static dvar_s* Find(const char* name);
		static void Set(const char* name, float value);
	};

	// Trace and landing hooks the movement code calls into. The simulator supplies a world
	// through Engine/Trace.hpp rather than a BSP.
	void PM_PlayerTrace(pmove_t* pm, trace_t* results, const vec3& start, const vec3& mins, const vec3& maxs,
		const vec3& end, int pass_entity_num, int content_mask);
	bool PM_CorrectAllSolid(pmove_t* pm, pml_t* pml, trace_t* trace);
	void PM_GroundTraceMissed(pmove_t* pm, pml_t* pml);
	void PM_CrashLand(playerState_s* ps, pml_t* pml);
	void PM_AddTouchEnt(pmove_t* pm, int entity_num);
}

using namespace IW3SR;

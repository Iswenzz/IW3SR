#include "Game/Base.hpp"

#include <string>
#include <unordered_map>

namespace IW3SR
{
	// CoD4 multiplayer defaults for the handful of dvars the movement code reads.
	static std::unordered_map<std::string, dvar_s>& Table()
	{
		static std::unordered_map<std::string, dvar_s> table = {
			{ "jump_height", { "jump_height", { 39.0f, 39, true } } },
			{ "g_gravity", { "g_gravity", { 800.0f, 800, true } } },
			{ "bg_bounces", { "bg_bounces", { 0.0f, 0, false } } },
			{ "player_meleeChargeFriction", { "player_meleeChargeFriction", { 1200.0f, 1200, true } } },
			{ "friction", { "friction", { 5.5f, 5, true } } },
			{ "bg_fallDamageMinHeight", { "bg_fallDamageMinHeight", { 128.0f, 128, true } } },
			{ "bg_fallDamageMaxHeight", { "bg_fallDamageMaxHeight", { 300.0f, 300, true } } },
		};
		return table;
	}

	dvar_s* Dvar::Find(const char* name)
	{
		auto it = Table().find(name);
		return it == Table().end() ? nullptr : &it->second;
	}

	void Dvar::Set(const char* name, float value)
	{
		dvar_s& dvar = Table()[name];
		dvar.name = name;
		dvar.current.value = value;
		dvar.current.integer = static_cast<int>(value);
		dvar.current.enabled = value != 0.0f;
	}
}

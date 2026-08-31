#include "Dvar.hpp"
#include "Patch.hpp"

namespace IW3SR
{
	std::vector<StringOverride> Dvar::Overrides;
	cmd_function_s Dvar::UnsetCommand = {};

	static const char* Argument(int index)
	{
		if (!cmd_args || cmd_args->nesting < 0 || cmd_args->nesting >= 8)
			return "";

		const int count = cmd_args->argc[cmd_args->nesting];
		const char** argv = cmd_args->argv[cmd_args->nesting];

		return index < count && argv ? argv[index] : "";
	}

	static void UnsetCommand_f()
	{
		Dvar::Unset(Argument(1));
	}

	static void SetColorDefault(const char* name, const vec4& color)
	{
		dvar_s* dvar = Dvar::Find(name);
		if (!dvar || dvar->type != DvarType::VEC4)
			return;

		// Every console colour is DVAR_ARCHIVE, so a config written by a stock build already carries the
		// retail defaults and the theme would never show. Moving current with reset while the two still
		// agree covers that, and leaves anything the player picked themselves alone.
		const bool untouched = dvar->current.vector == dvar->reset.vector;

		dvar->reset.vector = color;
		if (untouched)
		{
			dvar->current.vector = color;
			dvar->latched.vector = color;
		}
	}

	void Dvar::Initialize()
	{
		Register();
		InitializeConsole();

		RegisterString("sr_version", DvarFlags(DVAR_READONLY | DVAR_SERVERINFO), "Client version", APPLICATION_VERSION);
		RegisterString("cef_url", DvarFlags(DVAR_TEMP), "CEF URL", "about:blank");

		if (!Patch::UseCoD4X)
			RegisterBool("debug_show_viewpos", DVAR_NONE, "Draw the view position and angles", false);

		if (const auto developer_script = Find("developer_script"))
			developer_script->flags = DVAR_NONE;

		if (const auto rate = Find("rate"))
			rate->domain.integer.max = 100000;

		if (const auto snaps = Find("snaps"))
			snaps->domain.integer.max = 75;
	}

	void Dvar::InitializeConsole()
	{
		SetColorDefault("con_inputBoxColor", { 0.0f, 0.0f, 0.0f, 0.9f });
		SetColorDefault("con_inputHintBoxColor", { 0.0f, 0.0f, 0.0f, 0.9f });
		SetColorDefault("con_outputWindowColor", { 0.0f, 0.0f, 0.0f, 0.8f });
		SetColorDefault("con_outputBarColor", { 0.0f, 0.0f, 0.0f, 0.8f });
		SetColorDefault("con_outputSliderColor", { 0.0f, 0.62f, 0.7f, 0.8f });
	}

	void Dvar::InitializeRenderer()
	{
		if (const auto r_filmusetweaks = Find("r_filmusetweaks"))
			r_filmusetweaks->flags = DVAR_NONE;

		if (const auto r_fullbright = Find("r_fullbright"))
			r_fullbright->flags = DVAR_SAVED;

		if (const auto r_glowusetweaks = Find("r_glowusetweaks"))
			r_glowusetweaks->flags = DVAR_NONE;

		if (const auto r_lodBiasRigid = Find("r_lodBiasRigid"))
			r_lodBiasRigid->domain.value.min = -1000000;

		if (const auto r_lodBiasSkinned = Find("r_lodBiasSkinned"))
			r_lodBiasSkinned->domain.value.min = -1000000;

		if (const auto r_zfar = Find("r_zfar"))
			r_zfar->flags = DVAR_SAVED;
	}

	void Dvar::InitializeGame()
	{
		if (const auto bg_bobmax = Find("bg_bobmax"))
			bg_bobmax->flags = DVAR_SAVED;

		if (const auto cg_draw2d = Find("cg_draw2d"))
			cg_draw2d->flags = DVAR_NONE;

		if (const auto cg_drawgun = Find("cg_drawgun"))
			cg_drawgun->flags = DVAR_NONE;

		if (const auto cg_fovscale = Find("cg_fovscale"))
			cg_fovscale->flags = DVAR_SAVED;

		if (const auto cg_gun_x = Find("cg_gun_x"))
			cg_gun_x->flags = DVAR_SAVED;

		if (const auto cg_gun_y = Find("cg_gun_y"))
			cg_gun_y->flags = DVAR_SAVED;

		if (const auto cg_gun_z = Find("cg_gun_z"))
			cg_gun_z->flags = DVAR_SAVED;
	}

	// Points a string dvar at a buffer we own. Dvar_Shutdown resolves every string dvar value back
	// to a script string and drops a reference on it, so the engine's own pointers are kept here and
	// put back in Shutdown; releasing a reference against a buffer the string table never issued
	// corrupts the script memory tree and the next free spins forever.
	void Dvar::OverrideString(dvar_s* dvar, const char* value)
	{
		if (!dvar || dvar->type != DvarType::STRING || !value)
			return;

		if (std::ranges::none_of(Overrides, [dvar](const StringOverride& saved) { return saved.Var == dvar; }))
			Overrides.emplace_back(dvar, dvar->current.string, dvar->latched.string, dvar->reset.string);

		dvar->current.string = value;
		dvar->latched.string = value;
		dvar->reset.string = value;
	}

	// Linked into the engine's command list by hand: every Cmd_AddCommandInternal call site is inlined,
	// and the console only completes names it finds walking this list. The node lives in this module,
	// so Unregister has to take it back out before the module goes.
	void Dvar::Register()
	{
		if (!cmds || UnsetCommand.name)
			return;

		UnsetCommand.name = "unset";
		UnsetCommand.function = UnsetCommand_f;
		UnsetCommand.next = *cmds;

		*cmds = &UnsetCommand;
	}

	void Dvar::Unregister()
	{
		if (!cmds || !UnsetCommand.name)
			return;

		for (cmd_function_s** at = cmds; *at; at = &(*at)->next)
		{
			if (*at != &UnsetCommand)
				continue;

			*at = UnsetCommand.next;
			break;
		}
		UnsetCommand = {};
	}

	void Dvar::Unset(const std::string& name)
	{
		if (name.empty())
		{
			Com_PrintMessage(CON_CHANNEL_CLIENT, "USAGE: unset <variable>\n", 0);
			return;
		}
		dvar_s* dvar = Find(name);
		if (!dvar)
		{
			Com_PrintMessage(CON_CHANNEL_CLIENT, std::format("^3'{}' is not a dvar.\n", name).c_str(), 0);
			return;
		}
		if (!(dvar->flags & DVAR_SAVED))
		{
			Com_PrintMessage(CON_CHANNEL_CLIENT,
				std::format("^3'{}' is not saved to the config.\n", dvar->name).c_str(), 0);
			return;
		}
		const bool created = dvar->flags & DVAR_USERCREATED;
		dvar->flags = DvarFlags(dvar->flags & ~DVAR_SAVED);
		dvar_modifiedFlags |= DVAR_SAVED;

		Com_PrintMessage(CON_CHANNEL_CLIENT,
			std::format("'{}' will be dropped from the config on exit{}.\n", dvar->name,
				created ? "" : ", and comes back at its default")
				.c_str(),
			0);
	}

	void Dvar::Shutdown()
	{
		for (const StringOverride& saved : Overrides)
		{
			saved.Var->current.string = saved.Current;
			saved.Var->latched.string = saved.Latched;
			saved.Var->reset.string = saved.Reset;
		}
		Overrides.clear();

		Dvar_Shutdown_h();
	}

	dvar_s* Dvar::RegisterInt(const char* name, DvarFlags flags, const char* description, int value, int min, int max)
	{
		return Dvar_RegisterVariantInt(name, DvarType::INTEGER, flags, description, value, 0, 0, 0, min, max);
	}

	dvar_s* Dvar::RegisterFloat(const char* name, DvarFlags flags, const char* description, float value, float min,
		float max)
	{
		return Dvar_RegisterVariantFloat(name, DvarType::VALUE, flags, description, value, 0, 0, 0, min, max);
	}

	dvar_s* Dvar::RegisterBool(const char* name, DvarFlags flags, const char* description, bool value)
	{
		return Dvar_RegisterVariantBool(name, DvarType::BOOLEAN, flags, description, value, 0, 0, 0, 0, 0);
	}

	dvar_s* Dvar::RegisterString(const char* name, DvarFlags flags, const char* description, const char* value)
	{
		return Dvar_RegisterVariantString(name, DvarType::STRING, flags, description, value, 0, 0, 0, 0, 0);
	}

	dvar_s* Dvar::RegisterEnum(const char* name, DvarFlags flags, const char* description, int value,
		const std::vector<const char*>& list)
	{
		return Dvar_RegisterVariantEnum(name, DvarType::ENUMERATION, flags, description, value, 0, 0, 0,
			static_cast<int>(list.size()), const_cast<const char**>(list.data()));
	}

	dvar_s* Dvar::RegisterVec2(const char* name, DvarFlags flags, const char* description, float x, float y, float min,
		float max)
	{
		return Dvar_RegisterVariantVec2(name, DvarType::VEC2, flags, description, x, y, 0, 0, min, max);
	}

	dvar_s* Dvar::RegisterVec3(const char* name, DvarFlags flags, const char* description, float x, float y, float z,
		float min, float max)
	{
		return Dvar_RegisterVariantVec3(name, DvarType::VEC3, flags, description, x, y, z, 0, min, max);
	}

	dvar_s* Dvar::RegisterVec4(const char* name, DvarFlags flags, const char* description, float x, float y, float z,
		float w, float min, float max)
	{
		return Dvar_RegisterVariantVec4(name, DvarType::VEC4, flags, description, x, y, z, w, min, max);
	}

	dvar_s* Dvar::RegisterColor(const char* name, DvarFlags flags, const char* description, float r, float g, float b,
		float a)
	{
		return Dvar_RegisterVariantColor(name, DvarType::COLOR, flags, description, r, g, b, a, 0, 0);
	}

	dvar_s* Dvar::Find(const std::string& name)
	{
		return Dvar_FindVar(name.c_str());
	}
}

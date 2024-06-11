#include "Dvar.hpp"

namespace IW3SR
{
	void Dvar::Initialize()
	{
		RegisterString("sr_version", DvarFlags(DVAR_READONLY | DVAR_SERVERINFO), "Client version", APPLICATION_VERSION);
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

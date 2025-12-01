#pragma once
#include "Game/Base.hpp"

namespace IW3SR
{
	class API Dvar
	{
	public:
		static void Initialize();

		static dvar_s* RegisterInt(const char* name, DvarFlags flags, const char* description, int value, int min,
			int max);
		static dvar_s* RegisterFloat(const char* name, DvarFlags flags, const char* description, float value, float min,
			float max);
		static dvar_s* RegisterBool(const char* name, DvarFlags flags, const char* description, bool value);
		static dvar_s* RegisterString(const char* name, DvarFlags flags, const char* description, const char* value);
		static dvar_s* RegisterEnum(const char* name, DvarFlags flags, const char* description, int value,
			const std::vector<const char*>& list);
		static dvar_s* RegisterVec2(const char* name, DvarFlags flags, const char* description, float x, float y,
			float min, float max);
		static dvar_s* RegisterVec3(const char* name, DvarFlags flags, const char* description, float x, float y,
			float z, float min, float max);
		static dvar_s* RegisterVec4(const char* name, DvarFlags flags, const char* description, float x, float y,
			float z, float w, float min, float max);
		static dvar_s* RegisterColor(const char* name, DvarFlags flags, const char* description, float r, float g,
			float b, float a);
		static dvar_s* Find(const std::string& name);

		template <typename T>
		static T Get(const std::string& name)
		{
			const auto dvar = Find(name);
			if (!dvar)
				return T{};
			return *reinterpret_cast<T*>(&dvar->current);
		}

		template <typename T>
		static void Set(const std::string& name, T value)
		{
			const auto dvar = Find(name);
			if (!dvar)
				return;

			*reinterpret_cast<T*>(&dvar->current) = value;
			*reinterpret_cast<T*>(&dvar->latched) = value;
		}

		template <typename T>
		static void SetLatched(const std::string& name, T value)
		{
			const auto dvar = Find(name);
			if (!dvar)
				throw std::runtime_error("Dvar not found");

			*reinterpret_cast<T*>(&dvar->latched) = value;
		}
	};
}

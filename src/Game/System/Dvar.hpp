#pragma once
#include "Game/Base.hpp"

namespace IW3SR
{
	/// <summary>
	/// Game dvars.
	/// </summary>
	class API Dvar
	{
	public:
		/// <summary>
		/// Initialize dvars.
		/// </summary>
		static void Initialize();

		/// <summary>
		/// Register int dvar.
		/// </summary>
		/// <param name="name">The dvar name.</param>
		/// <param name="flags">The dvar flags.</param>
		/// <param name="description">The description.</param>
		/// <param name="value">The default value.</param>
		/// <param name="min">The min value.</param>
		/// <param name="max">The max value.</param>
		/// <returns></returns>
		static dvar_s* RegisterInt(const char* name, DvarFlags flags, const char* description, int value, int min,
			int max);

		/// <summary>
		/// Register float dvar.
		/// </summary>
		/// <param name="name">The dvar name.</param>
		/// <param name="flags">The dvar flags.</param>
		/// <param name="description">The description.</param>
		/// <param name="value">The default value.</param>
		/// <param name="min">The min value.</param>
		/// <param name="max">The max value.</param>
		/// <returns></returns>
		static dvar_s* RegisterFloat(const char* name, DvarFlags flags, const char* description, float value, float min,
			float max);

		/// <summary>
		/// Register bool dvar.
		/// </summary>
		/// <param name="name">The dvar name.</param>
		/// <param name="flags">The dvar flags.</param>
		/// <param name="description">The description.</param>
		/// <param name="value">The default value.</param>
		/// <returns></returns>
		static dvar_s* RegisterBool(const char* name, DvarFlags flags, const char* description, bool value);

		/// <summary>
		/// Register string dvar.
		/// </summary>
		/// <param name="name">The dvar name.</param>
		/// <param name="flags">The dvar flags.</param>
		/// <param name="description">The description.</param>
		/// <param name="value">The default value.</param>
		/// <returns></returns>
		static dvar_s* RegisterString(const char* name, DvarFlags flags, const char* description, const char* value);

		/// <summary>
		/// Register enum dvar.
		/// </summary>
		/// <param name="name">The dvar name.</param>
		/// <param name="flags">The dvar flags.</param>
		/// <param name="description">The description.</param>
		/// <param name="value">The default value.</param>
		/// <param name="list">The enum list.</param>
		/// <returns></returns>
		static dvar_s* RegisterEnum(const char* name, DvarFlags flags, const char* description, int value,
			const std::vector<const char*>& list);

		/// <summary>
		/// Register vec2 dvar.
		/// </summary>
		/// <param name="name">The dvar name.</param>
		/// <param name="flags">The dvar flags.</param>
		/// <param name="description">The description.</param>
		/// <param name="x">The default x value.</param>
		/// <param name="y">The default y value.</param>
		/// <param name="min">The min value.</param>
		/// <param name="max">The max value.</param>
		/// <returns></returns>
		static dvar_s* RegisterVec2(const char* name, DvarFlags flags, const char* description, float x, float y,
			float min, float max);

		/// <summary>
		/// Register vec3 dvar.
		/// </summary>
		/// <param name="name">The dvar name.</param>
		/// <param name="flags">The dvar flags.</param>
		/// <param name="description">The description.</param>
		/// <param name="x">The default x value.</param>
		/// <param name="y">The default y value.</param>
		/// <param name="z">The default z value.</param>
		/// <param name="min">The min value.</param>
		/// <param name="max">The max value.</param>
		/// <returns></returns>
		static dvar_s* RegisterVec3(const char* name, DvarFlags flags, const char* description, float x, float y,
			float z, float min, float max);

		/// <summary>
		/// Register vec4 dvar.
		/// </summary>
		/// <param name="name">The dvar name.</param>
		/// <param name="flags">The dvar flags.</param>
		/// <param name="description">The description.</param>
		/// <param name="x">The default x value.</param>
		/// <param name="y">The default y value.</param>
		/// <param name="z">The default z value.</param>
		/// <param name="w">The default w value.</param>
		/// <param name="min">The min value.</param>
		/// <param name="max">The max value.</param>
		/// <returns></returns>
		static dvar_s* RegisterVec4(const char* name, DvarFlags flags, const char* description, float x, float y,
			float z, float w, float min, float max);

		/// <summary>
		/// Register color dvar.
		/// </summary>
		/// <param name="name">The dvar name.</param>
		/// <param name="flags">The dvar flags.</param>
		/// <param name="description">The description.</param>
		/// <param name="r">The default r value.</param>
		/// <param name="g">The default g value.</param>
		/// <param name="b">The default b value.</param>
		/// <param name="a">The default a value.</param>
		/// <returns></returns>
		static dvar_s* RegisterColor(const char* name, DvarFlags flags, const char* description, float r, float g,
			float b, float a);

		/// <summary>
		/// Find dvar.
		/// </summary>
		/// <param name="name">The dvar name.</param>
		static dvar_s* Find(const std::string& name);

		/// <summary>
		/// Get dvar value.
		/// </summary>
		/// <typeparam name="T">The value type.</typeparam>
		/// <param name="name">The dvar name.</param>
		template <typename T>
		static T Get(const std::string& name)
		{
			const auto dvar = Find(name);
			if (!dvar)
				throw std::runtime_error("Dvar not found");
			return *reinterpret_cast<T*>(&dvar->current);
		}

		/// <summary>
		/// Set dvar value.
		/// </summary>
		/// <typeparam name="T">The value type.</typeparam>
		/// <param name="name">The dvar name.</param>
		/// <param name="value">The value.</param>
		template <typename T>
		static void Set(const std::string& name, T value)
		{
			const auto dvar = Find(name);
			if (!dvar)
				throw std::runtime_error("Dvar not found");

			*reinterpret_cast<T*>(&dvar->current) = value;
			*reinterpret_cast<T*>(&dvar->latched) = value;
		}

		/// <summary>
		/// Set dvar latched value.
		/// </summary>
		/// <typeparam name="T">The value type.</typeparam>
		/// <param name="name">The dvar name.</param>
		/// <param name="value">The value.</param>
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

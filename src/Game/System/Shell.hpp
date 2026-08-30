#pragma once
#include "Game/Base.hpp"

namespace IW3SR
{
	struct ShellUrl
	{
		std::string address;
		std::string password;
	};

	enum class RegistryWrite
	{
		Failed,
		Unchanged,
		Updated
	};

	class GShell
	{
	public:
		static void Initialize();

		static bool Command(const std::string& command);

		static bool Register();
		static void Unregister();

		static std::optional<ShellUrl> Parse(const std::string& url);

	private:
		static inline dvar_s* AssociateDvar = nullptr;
		static inline dvar_s* ImportDvar = nullptr;

		static bool Connect(const std::string& url);
		static bool Play(const std::string& demo);

		static std::string Executable();
		static std::vector<std::filesystem::path> Demos();

		static std::optional<std::string> Read(const std::string& key, const char* name);
		static RegistryWrite Write(const std::string& key, const char* name, const std::string& value);
		static void Clear(const std::string& key, const char* name);
		static void Erase(const std::string& key);
	};
}

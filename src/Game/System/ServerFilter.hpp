#pragma once
#include "Game/Base.hpp"

#include "Game/System/Remote.hpp"

namespace IW3SR
{
	constexpr int FilterBrowser = 0;  // hidden from the server browser
	constexpr int FilterRedirect = 3; // rewrites the address rather than filtering it
	constexpr int FilterConnect = 8;  // refused when connecting to it

	struct FilterEntry
	{
		netadr_t Address = {};
		netadr_t Redirect = {};
		int Severity = 0;
	};

	// A downloaded list of servers to hide, refuse or redirect, cached so it still applies offline.
	class ServerFilter
	{
	public:
		static void Initialize();
		static void Refresh();
		static bool Check(netadr_t& address, int severity);
		static bool Command(const std::string& command);

	private:
		static void Apply(const RemoteResult& result);
		static std::vector<FilterEntry> Parse(const std::string& body);

		static inline dvar_s* Url = nullptr;
		static inline dvar_s* Enabled = nullptr;
		static inline std::mutex Guard;
		static inline std::vector<FilterEntry> Entries;
		static inline bool Busy = false;
	};
}

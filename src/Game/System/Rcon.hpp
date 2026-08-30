#pragma once
#include "Game/Base.hpp"

#include "Game/System/Net.hpp"

namespace IW3SR
{
	constexpr size_t MaxRconPassword = 60;

	// The password lives in this buffer instead of config_mp.cfg
	class GRcon
	{
	public:
		static void Initialize();
		static void Shutdown();
		static void Frame();

		static bool Command(const std::string& command);

	private:
		static inline std::array<char, MaxRconPassword + 1> Password = {};
		static inline netadr_t Host = {};
		static inline NetSocket Socket = InvalidSocket;
		static inline int Deadline = 0;

		static bool Login(const std::string& password);
		static void Logout();
		static bool SetHost(const std::string& value);
		static bool Send(const std::string& arguments);

		static bool Target(netadr_t& address);
		static void Receive();
		static void Store(const std::string& password);
		static void Wipe();
		static void Print(const std::string& text);
		static void Error(const std::string& text);
	};
}

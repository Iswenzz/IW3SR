#pragma once
#include "Game/Base.hpp"

namespace IW3SR
{
	constexpr std::array<LogColor, 10> Q3Colors = { LogColor::Default, LogColor::Red, LogColor::Green, LogColor::Yellow,
		LogColor::Blue, LogColor::Cyan, LogColor::Magenta, LogColor::Default, LogColor::Default, LogColor::Default };

	class GConsole
	{
	public:
		static void Initialize();
		static void Shutdown();

		static void Register(const char* name);
		static void Unregister();

		static void Write(ConChannel channel, const char* msg, int type);
		static std::string Q3(const std::string& msg);

		static void OnExecute(EventConsoleCommand& event);
		static void Dispatch(Event& event);

	private:
		static cmd_function_s* Find(const char* name);

		static std::list<cmd_function_s> Registered;
	};
}

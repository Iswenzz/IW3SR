#pragma once
#include "Game/Base.hpp"

namespace IW3SR
{
	// clang-format off
	constexpr std::array<LogColor, 10> Q3Colors = {
		LogColor::Default,
		LogColor::Red,
		LogColor::Green,
		LogColor::Yellow,
		LogColor::Blue,
		LogColor::Cyan,
		LogColor::Magenta,
		LogColor::Default,
		LogColor::Default,
		LogColor::Default
	};
	// clang-format on

	/// <summary>
	/// Console class.
	/// </summary>
	class GConsole
	{
	public:
		/// <summary>
		/// Initialize the console.
		/// </summary>
		static void Initialize();

		/// <summary>
		/// Shutdown the console.
		/// </summary>
		static void Shutdown();

		/// <summary>
		/// Write to the console.
		/// </summary>
		/// <param name="channel">The channel.</param>
		/// <param name="msg">The message.</param>
		/// <param name="type">The messsage type.</param>
		static void Write(ConChannel channel, const char* msg, int type);

	private:
		/// <summary>
		/// Converts Q3 colors to ANSI.
		/// </summary>
		/// <param name="msg">The message.</param>
		/// <returns></returns>
		static std::string Q3(const std::string& msg);
	};
}

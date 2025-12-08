#include "Console.hpp"

namespace IW3SR
{
	void GConsole::Initialize()
	{
		if (!System::IsDebug())
			return;

		Console::Initialize("IW3SR");
	}

	void GConsole::Shutdown()
	{
		if (!System::IsDebug())
			return;

		Console::Shutdown();
	}

	void GConsole::Write(ConChannel channel, const char* msg, int type)
	{
		Log::Write(Q3(msg));
		Com_PrintMessage_h(channel, msg, type);
	}

	std::string GConsole::Q3(const std::string& msg)
	{
		std::string result;
		auto size = msg.size();

		for (int i = 0; i < size; i++)
		{
			if (msg[i] == '^' && i + 1 < size && msg[i + 1] != '^')
			{
				int color = ((msg[i + 1]) - '0') & 7;
				result += std::format("\x1b[{}m", static_cast<int>(Q3Colors[color]));
				i++;
				continue;
			}
			result += msg[i];
		}
		return result;
	}
}

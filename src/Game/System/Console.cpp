#include "Console.hpp"

#include "Core/Console/Console.hpp"

namespace IW3SR
{
	void GConsole::Initialize()
	{
		Console::Initialize();
		Console::SetTile("IW3SR");

		for (int i = 0; i <= dvarCount - 1; i++)
			Console::AddCommand(dvars[i]->name);
	}

	void GConsole::Shutdown()
	{
		Console::Shutdown();
	}

	void GConsole::Write(ConChannel channel, const char* msg, int type)
	{
		Log::Write(Q3(msg));

		if (Com_PrintMessage_h)
			Com_PrintMessage_h(channel, msg, type);
	}

	void GConsole::Command(EventConsoleCommand& event)
	{
		Cmd_ExecuteSingleCommand(0, 0, event.command.c_str());
	}

	void GConsole::Dispatch(Event& event)
	{
		EventDispatcher dispatcher(event);

		dispatcher.Dispatch<EventConsoleCommand>(Command);
	}

	void GConsole::Frame()
	{
		Console::Frame();
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

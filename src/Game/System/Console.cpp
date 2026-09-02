#include "Console.hpp"
#include "System.hpp"

namespace IW3SR
{
	std::list<cmd_function_s> GConsole::Registered;

	// Nothing reaches this: Cmd_ExecuteSingleCommand hands every IW3SR command to its module before the
	// engine walks its own list. The nodes are there so the console has a name to complete, and a stub
	// beats a null the engine would call straight through.
	static void CommandStub()
	{
	}

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

	cmd_function_s* GConsole::Find(const char* name)
	{
		for (cmd_function_s* command = cmds ? *cmds : nullptr; command; command = command->next)
		{
			if (command->name && _stricmp(command->name, name) == 0)
				return command;
		}
		return nullptr;
	}

	// The game console completes nothing it cannot find walking the engine's command list, and every
	// Cmd_AddCommandInternal call site is inlined, so the node has to be linked in by hand the way
	// Dvar::Register does it for "unset". The name is not copied: every caller passes a literal.
	// The nodes live in this module, so Unregister has to take them back out before it goes.
	void GConsole::Register(const char* name)
	{
		if (!cmds || !name || !*name || Find(name))
			return;

		cmd_function_s& command = Registered.emplace_back();

		command.name = name;
		command.function = CommandStub;
		command.next = *cmds;

		*cmds = &command;
	}

	void GConsole::Unregister()
	{
		if (!cmds)
		{
			Registered.clear();
			return;
		}

		for (const cmd_function_s& node : Registered)
		{
			for (cmd_function_s** at = cmds; *at; at = &(*at)->next)
			{
				if (*at != &node)
					continue;

				*at = node.next;
				break;
			}
		}
		Registered.clear();
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

	void GConsole::OnExecute(EventConsoleCommand& event)
	{
		Cmd_ExecuteSingleCommand(0, 0, event.command.c_str());
	}

	void GConsole::Dispatch(Event& event)
	{
		EventDispatcher dispatcher(event);

		dispatcher.Dispatch<EventConsoleCommand>(OnExecute);
	}
}

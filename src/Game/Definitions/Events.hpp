#pragma once
#include "Core/Communication/Event.hpp"

namespace IW3SR
{
	class EventRenderer3D : public Event
	{
		EVENT_CLASS("renderer.3D")
		EventRenderer3D() = default;
	};

	class EventRenderer2D : public Event
	{
		EVENT_CLASS("renderer.2D")
		EventRenderer2D() = default;
	};

	class EventRendererRender : public Event
	{
		EVENT_CLASS("renderer.render")
		EventRendererRender() = default;
	};

	class EventPlayerConnect : public Event
	{
		EVENT_CLASS("player.connect")
		EventPlayerConnect() = default;
	};

	class EventPlayerDisconnect : public Event
	{
		EVENT_CLASS("player.disconnect")
		EventPlayerDisconnect() = default;
	};

	class EventPlayerSpawn : public Event
	{
		EVENT_CLASS("player.spawn")
		EventPlayerSpawn() = default;
	};

	class EventPlayerDeath : public Event
	{
		EVENT_CLASS("player.death")
		EventPlayerDeath() = default;
	};

	class EventPlayerDamage : public Event
	{
		EVENT_CLASS("player.damage")
		EventPlayerDamage() = default;
	};

	class EventPlayerCommand : public Event
	{
		EVENT_CLASS("player.command")
		EventPlayerCommand() = default;
	};

	class EventPlayerLoadPosition : public Event
	{
		EVENT_CLASS("player.load.position")
		EventPlayerLoadPosition() = default;
	};

	class EventPMoveWalk : public Event
	{
		EVENT_CLASS("pmove.walk")
		EventPMoveWalk(pmove_t* pm, pml_t* pml) : pm(pm), pml(pml) { }
		pmove_t* pm;
		pml_t* pml;
	};

	class EventPMoveAir : public Event
	{
		EVENT_CLASS("pmove.air")
		EventPMoveAir(pmove_t* pm, pml_t* pml) : pm(pm), pml(pml) { }
		pmove_t* pm;
		pml_t* pml;
	};

	class EventPMoveFinish : public Event
	{
		EVENT_CLASS("pmove.finish")
		EventPMoveFinish(usercmd_s* cmd) : cmd(cmd) { }
		usercmd_s* cmd;
	};

	class EventScriptMenuResponse : public Event
	{
		EVENT_CLASS("script.menu.response")
		EventScriptMenuResponse(std::string menu, std::string response) : menu(menu), response(response) { }
		std::string menu;
		std::string response;
	};

	class EventExecuteCommand : public Event
	{
		EVENT_CLASS("execute.command")
		EventExecuteCommand(std::string command) : command(command) { }
		std::string command;
	};
}

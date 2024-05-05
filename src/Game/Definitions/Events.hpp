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

	class EventClientConnect : public Event
	{
		EVENT_CLASS("client.connect")
		EventClientConnect() = default;
	};

	class EventClientDisconnect : public Event
	{
		EVENT_CLASS("client.disconnect")
		EventClientDisconnect() = default;
	};

	class EventClientSpawn : public Event
	{
		EVENT_CLASS("client.spawn")
		EventClientSpawn() = default;
	};

	class EventClientPredict : public Event
	{
		EVENT_CLASS("client.predict")
		EventClientPredict() = default;
	};

	class EventClientDeath : public Event
	{
		EVENT_CLASS("client.death")
		EventClientDeath() = default;
	};

	class EventClientDamage : public Event
	{
		EVENT_CLASS("client.damage")
		EventClientDamage() = default;
	};

	class EventClientCommand : public Event
	{
		EVENT_CLASS("client.command")
		EventClientCommand(std::string command) : command(command) { }
		std::string command;
	};

	class EventClientLoadPosition : public Event
	{
		EVENT_CLASS("client.load.position")
		EventClientLoadPosition() = default;
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

	class EventPMoveGroundTrace : public Event
	{
		EVENT_CLASS("pmove.ground.trace")
		EventPMoveGroundTrace(pmove_t* pm, pml_t* pml) : pm(pm), pml(pml) { }
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
}

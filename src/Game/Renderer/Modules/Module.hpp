#pragma once
#include "Game/Base.hpp"

namespace IW3SR
{
	class API Module : public IObject
	{
	public:
		std::string ID;
		std::string Name;
		std::string Group;
		Frame MenuFrame;
		bool IsEnabled = false;

		Module() = default;
		Module(const std::string& id, const std::string& group, const std::string& name);
		virtual ~Module();

		virtual void Initialize();
		virtual void Release();
		virtual void Menu();

		virtual void OnConnect(EventClientConnect& event);
		virtual void OnDisconnect(EventClientDisconnect& event);
		virtual void OnSpawn(EventClientSpawn& event);
		virtual void OnPredict(EventClientPredict& event);
		virtual void OnWalkMove(EventPMoveWalk& event);
		virtual void OnAirMove(EventPMoveAir& event);
		virtual void OnGroundTrace(EventPMoveGroundTrace& event);
		virtual void OnFinishMove(EventPMoveFinish& event);
		virtual void OnLoadPosition();
		virtual void OnExecuteCommand(EventClientCommand& event);
		virtual void OnMenuResponse(EventScriptMenuResponse& event);
		virtual void OnDraw3D(EventRenderer3D& event);
		virtual void OnDraw2D(EventRenderer2D& event);
		virtual void OnDrawText(EventRendererText& event);
		virtual void OnRender();
		virtual void OnEvent(Event& event) override;

		SERIALIZE_POLY_BASE(Module, IsEnabled, MenuFrame)
	};
}

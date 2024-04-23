#pragma once
#include "Game/Base.hpp"

namespace IW3SR
{
	/// <summary>
	/// Module class.
	/// </summary>
	class API Module : public IObject
	{
	public:
		std::string ID;
		std::string Name;
		std::string Group;
		Window Menu;
		bool IsEnabled = false;

		/// <summary>
		/// Initialize the module.
		/// </summary>
		Module() = default;

		/// <summary>
		/// Initialize the module.
		/// </summary>
		/// <param name="id">The ID.</param>
		/// <param name="group">The group.</param>
		/// <param name="name">The name.</param>
		Module(const std::string& id, const std::string& group, const std::string& name);

		/// <summary>
		/// Release the module.
		/// </summary>
		virtual ~Module();

		/// <summary>
		/// Initialize the module.
		/// </summary>
		virtual void Initialize();

		/// <summary>
		/// Release the module.
		/// </summary>
		virtual void Release();

		/// <summary>
		/// Event dispatch.
		/// </summary>
		/// <param name="event">The event.</param>
		virtual void OnEvent(Event& event) override;

		/// <summary>
		/// Menu drawing.
		/// </summary>
		virtual void OnMenu();

		/// <summary>
		/// Client connect.
		/// </summary>
		/// <param name="event">The event.</param>
		virtual void OnConnect(EventClientConnect& event);

		/// <summary>
		/// Client disconnect.
		/// </summary>
		/// <param name="event">The event.</param>
		virtual void OnDisconnect(EventClientDisconnect& event);

		/// <summary>
		/// Client spawn.
		/// </summary>
		/// <param name="event">The event.</param>
		virtual void OnSpawn(EventClientSpawn& event);

		/// <summary>
		/// Walk moving.
		/// </summary>
		/// <param name="event">The event.</param>
		virtual void OnWalkMove(EventPMoveWalk& event);

		/// <summary>
		/// Air moving.
		/// </summary>
		/// <param name="event">The event.</param>
		virtual void OnAirMove(EventPMoveAir& event);

		/// <summary>
		/// Finish moving.
		/// </summary>
		/// <param name="event">The event.</param>
		virtual void OnFinishMove(EventPMoveFinish& event);

		/// <summary>
		/// Load position.
		/// </summary>
		virtual void OnLoadPosition();

		/// <summary>
		/// Execute command.
		/// </summary>
		/// <param name="event">The event.</param>
		virtual void OnExecuteCommand(EventExecuteCommand& event);

		/// <summary>
		/// Menu response.
		/// </summary>
		/// <param name="event">The event.</param>
		virtual void OnMenuResponse(EventScriptMenuResponse& event);

		/// <summary>
		/// Draw 3D.
		/// </summary>
		/// <param name="event">The event.</param>
		virtual void OnDraw3D(EventRenderer3D& event);

		/// <summary>
		/// Draw 2D.
		/// </summary>
		/// <param name="event">The event.</param>
		virtual void OnDraw2D(EventRenderer2D& event);

		/// <summary>
		/// Render frame.
		/// </summary>
		virtual void OnRender();

		SERIALIZE_POLY_BASE(Module, IsEnabled, Menu)
	};
}

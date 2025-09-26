#pragma once
#include "Game/Base.hpp"

namespace IW3SR
{
	/// <summary>
	/// Setting class.
	/// </summary>
	class API Setting : public IObject
	{
	public:
		std::string ID;
		std::string Name;
		std::string Group;
		Frame MenuFrame;

		/// <summary>
		/// Initialize the setting.
		/// </summary>
		Setting() = default;
		virtual ~Setting();

		/// <summary>
		/// Initialize the setting.
		/// </summary>
		/// <param name="id">The ID.</param>
		/// <param name="group">The group.</param>
		/// <param name="name">The name.</param>
		Setting(const std::string& id, const std::string& group, const std::string& name);

		/// <summary>
		/// Initialize the setting.
		/// </summary>
		virtual void Initialize();

		/// <summary>
		/// Release the setting.
		/// </summary>
		virtual void Release();

		/// <summary>
		/// Menu drawing.
		/// </summary>
		virtual void Menu();

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

		/// <summary>
		/// Event dispatch.
		/// </summary>
		/// <param name="event">The event.</param>
		virtual void OnEvent(Event& event) override;

		SERIALIZE_POLY_BASE(Setting, MenuFrame)
	};
}

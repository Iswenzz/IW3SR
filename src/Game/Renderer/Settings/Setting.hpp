#pragma once
#include "Game/Base.hpp"

namespace IW3SR
{
	class API Setting : public IObject
	{
	public:
		std::string ID;
		std::string Name;
		std::string Group;
		Frame MenuFrame;

		Setting() = default;
		Setting(const std::string& id, const std::string& group, const std::string& name);
		virtual ~Setting();

		virtual void Initialize();
		virtual void Release();
		virtual void Menu();

		virtual void OnDraw3D(EventRenderer3D& event);
		virtual void OnDraw2D(EventRenderer2D& event);
		virtual void OnRender();
		virtual void OnEvent(Event& event) override;

		SERIALIZE_POLY_BASE(Setting, MenuFrame)
	};
}

#include "Setting.hpp"

namespace IW3SR
{
	Setting::Setting(const std::string& id, const std::string& group, const std::string& name)
	{
		ID = id;
		Name = name;
		Group = group;
		Menu = Window(name);
	}

	Setting::~Setting()
	{
		Release();
	}

	void Setting::Initialize() { }
	void Setting::Release() { }

	void Setting::OnEvent(Event& event)
	{
		EventDispatcher dispatcher(event);

		dispatcher.Dispatch<EventRenderer3D>(EVENT_BIND(OnDraw3D));
		dispatcher.Dispatch<EventRenderer2D>(EVENT_BIND(OnDraw2D));
		dispatcher.Dispatch<EventRendererRender>(EVENT_BIND_VOID(OnRender));
	}

	void Setting::OnMenu() { }
	void Setting::OnDraw3D(EventRenderer3D& event) { }
	void Setting::OnDraw2D(EventRenderer2D& event) { }
	void Setting::OnRender() { }
}

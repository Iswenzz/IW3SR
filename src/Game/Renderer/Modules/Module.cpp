#include "Module.hpp"

namespace IW3SR
{
	Module::Module(const std::string& id, const std::string& group, const std::string& name)
	{
		ID = id;
		Name = name;
		Group = group;
		MenuFrame = Frame(name);
	}

	Module::~Module()
	{
		Release();
	}

	void Module::Initialize() { }
	void Module::Release() { }
	void Module::Menu() { }

	void Module::OnConnect(EventClientConnect& event) { }
	void Module::OnDisconnect(EventClientDisconnect& event) { }
	void Module::OnSpawn(EventClientSpawn& event) { }
	void Module::OnPredict(EventClientPredict& event) { }

	void Module::OnWalkMove(EventPMoveWalk& event) { }
	void Module::OnAirMove(EventPMoveAir& event) { }
	void Module::OnGroundTrace(EventPMoveGroundTrace& event) { }
	void Module::OnFinishMove(EventPMoveFinish& event) { }
	void Module::OnLoadPosition() { }

	void Module::OnExecuteCommand(EventClientCommand& event) { }
	void Module::OnMenuResponse(EventScriptMenuResponse& event) { }

	void Module::OnDraw3D(EventRenderer3D& event) { }
	void Module::OnDraw2D(EventRenderer2D& event) { }
	void Module::OnDrawText(EventRendererText& event) {  }
	void Module::OnRender() { }

	void Module::OnEvent(Event& event)
	{
		if (!IsEnabled || client_ui->connectionState != CA_ACTIVE)
			return;

		EventDispatcher dispatcher(event);

		dispatcher.Dispatch<EventClientConnect>(EVENT_BIND(OnConnect));
		dispatcher.Dispatch<EventClientDisconnect>(EVENT_BIND(OnDisconnect));
		dispatcher.Dispatch<EventClientSpawn>(EVENT_BIND(OnSpawn));
		dispatcher.Dispatch<EventClientPredict>(EVENT_BIND(OnPredict));
		dispatcher.Dispatch<EventClientLoadPosition>(EVENT_BIND_VOID(OnLoadPosition));

		dispatcher.Dispatch<EventPMoveWalk>(EVENT_BIND(OnWalkMove));
		dispatcher.Dispatch<EventPMoveAir>(EVENT_BIND(OnAirMove));
		dispatcher.Dispatch<EventPMoveGroundTrace>(EVENT_BIND(OnGroundTrace));
		dispatcher.Dispatch<EventPMoveFinish>(EVENT_BIND(OnFinishMove));

		dispatcher.Dispatch<EventClientCommand>(EVENT_BIND(OnExecuteCommand));
		dispatcher.Dispatch<EventScriptMenuResponse>(EVENT_BIND(OnMenuResponse));

		dispatcher.Dispatch<EventRenderer3D>(EVENT_BIND(OnDraw3D));
		dispatcher.Dispatch<EventRenderer2D>(EVENT_BIND(OnDraw2D));
		dispatcher.Dispatch<EventRendererText>(EVENT_BIND(OnDrawText));
		dispatcher.Dispatch<EventRendererRender>(EVENT_BIND_VOID(OnRender));
	}
}

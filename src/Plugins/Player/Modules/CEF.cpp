#include "CEF.hpp"

namespace IW3SR::Addons
{
	CEF::CEF() : Module("sr.player.cef", "Player", "CEF") { }

	void CEF::Initialize()
	{
		Browser::Start();
		if (Browser::Active)
			Browser::SetURL("https://youtu.be/UgQFcvYg9Kk?t=90");
	}

	void CEF::Release()
	{
		Browser::Stop();
	}

	void CEF::Menu() { }

	void CEF::OnRender()
	{
		if (!Browser::Open || !Browser::Texture)
			return;

		std::scoped_lock lock(Browser::TextureMutex);

		Draw2D::Rect(Browser::Texture, { 0, 0 }, Window::Size);
	}
}

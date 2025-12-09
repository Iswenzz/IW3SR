#include "CEF.hpp"

namespace IW3SR::Addons
{
	CEF::CEF() : Module("sr.player.cef", "Player", "CEF") { }

	void CEF::Initialize()
	{
		Browser::Start();
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
		Material* mtl = Material_RegisterHandle("black", 3);
		if (!mtl)
			return;

		/*if (mtl && mtl->textureTable && mtl->textureTable->u.image)
		{
			auto browserTexture = reinterpret_cast<IDirect3DTexture9*>(Browser::Texture->Data);
			auto texture = mtl->textureTable->u.image->texture.map;
			IDirect3DSurface9* srcSurface = nullptr;
			IDirect3DSurface9* destSurface = nullptr;

			browserTexture->GetSurfaceLevel(0, &srcSurface);
			texture->GetSurfaceLevel(0, &destSurface);

			Device::D3Device->StretchRect(srcSurface, nullptr, destSurface, nullptr, D3DTEXF_LINEAR);

			srcSurface->Release();
			destSurface->Release();
		}*/
		Draw2D::Rect(Browser::Texture, { 0, 0 }, Window::Size);
	}
}

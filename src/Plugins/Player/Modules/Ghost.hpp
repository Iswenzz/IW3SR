#pragma once
#include "Player/Base.hpp"

#include "Game/Renderer/Ghost.hpp"
#include "Game/System/ServerGhost.hpp"

namespace IW3SR::Addons
{
	class Ghost : public Module
	{
	public:
		Text DeltaText;

		bool ShowDelta;
		bool ShowLabel;
		float HideDistance;
		float Opacity;

		Ghost();
		virtual ~Ghost() = default;

		void Menu() override;
		void OnRender() override;

	private:
		void DrawDelta(const ServerGhostState& ghost);
		void DrawLabel(const ServerGhostState& ghost);

		SERIALIZE_POLY(Ghost, Module, DeltaText, ShowDelta, ShowLabel, HideDistance, Opacity)
	};
}

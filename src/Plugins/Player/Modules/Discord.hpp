#pragma once
#include "Player/Base.hpp"

#include "Game/System/Discord.hpp"

namespace IW3SR::Addons
{
	class Discord : public Module
	{
	public:
		Text TitleText;
		Text RequestText;
		Text AcceptText;
		Text DenyText;

		vec2 BarPosition;
		vec2 BarSize;

		vec4 ColorHeader;
		vec4 ColorBackground;

		Bind KeyAccept;
		Bind KeyDeny;

		bool ShowOverlay;

		Discord();
		virtual ~Discord() = default;

		void Menu() override;
		void OnRender() override;

	private:
		void Draw(const DiscordJoinRequest& request);
		void DrawBar();

		SERIALIZE_POLY(Discord, Module, TitleText, RequestText, AcceptText, DenyText, BarPosition, BarSize, ColorHeader,
			ColorBackground, KeyAccept, KeyDeny, ShowOverlay)
	};
}

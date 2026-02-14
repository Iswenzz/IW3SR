#pragma once
#include "Base.hpp"

namespace IW3SR::Addons
{
	struct EmojiCommand
	{
		Ref<Texture> Emoji;
		vec2 Position;
		vec2 Size;
		vec4 Color;
	};

	class Texts : public Module
	{
	public:
		bool UseEmojis;

		Texts();
		virtual ~Texts() = default;

		void Menu() override;
		void ProcessText(std::string& text, Font_s* font, vec2 position, vec2 scale, const vec4& color);
		void OnDrawText(EventRendererText& event) override;
		void OnRender() override;

	private:
		static inline std::vector<EmojiCommand> EmojiCommands;
		static inline std::unordered_map<std::string, Ref<Texture>> EmojiMap;

		SERIALIZE_POLY(Texts, Module, UseEmojis)
	};
}

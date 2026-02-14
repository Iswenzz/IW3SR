#pragma once

namespace IW3SR
{
	struct EmojiCommand
	{
		Ref<Texture> Emoji;
		vec2 Position;
		vec2 Size;
		vec4 Color;
	};

	class Emojis
	{
	public:
		static inline std::vector<EmojiCommand> Commands;

		static void Initialize();
		static void ProcessText(std::string& text, Font_s* font, vec2 position, float scale, const vec4& color);
		static void Frame();

	private:
		static inline std::unordered_map<std::string, Ref<Texture>> EmojisMap;
	};
}

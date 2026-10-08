#pragma once
#include "Graphics/Base.hpp"

namespace IW3SR::Addons
{
	struct EmojiAsset
	{
		std::string Path;
		Ref<Texture> Image;
	};

	struct EmojiCommand
	{
		EmojiAsset* Emoji;
		vec2 Position;
		vec2 Size;
		vec4 Color;
	};

	struct EmojiQuery
	{
		size_t Start;
		size_t End;
		std::string Prefix;
	};

	class General : public Module
	{
	public:
		bool UseEmojis;

		General();
		virtual ~General() = default;

		void Menu() override;
		void ProcessText(std::string& text, Font_s* font, vec2 position, vec2 scale, const vec4& color);
		void OnDrawText(EventRendererText& event) override;
		void OnChatKey(EventClientChatKey& event) override;
		void OnRender() override;

	private:
		using EmojiEntry = std::map<std::string, EmojiAsset>::value_type;

		std::vector<EmojiCommand> EmojiCommands;
		std::mutex EmojiMutex;
		std::map<std::string, EmojiAsset> EmojiMap;

		std::string SuggestionPrefix;
		int Suggestion = 0;
		std::mutex SuggestionMutex;

		static const Ref<Texture>& GetImage(EmojiAsset& emoji);
		static std::optional<EmojiQuery> FindQuery(const field_t& field);
		std::vector<EmojiEntry*> FindSuggestions(const std::string& prefix);
		void DrawSuggestions();

		SERIALIZE_POLY(General, Module, UseEmojis)
	};
}

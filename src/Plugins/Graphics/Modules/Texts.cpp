#include "Texts.hpp"

namespace IW3SR::Addons
{
	Texts::Texts() : Module("sr.graphics.texts", "Graphics", "Texts")
	{
		UseEmojis = true;

		EmojiMap = {
			{ "africa", Texture::Load("Textures/Emojis/africa.png") },
			{ "angry", Texture::Load("Textures/Emojis/angry.png") },
			{ "bored", Texture::Load("Textures/Emojis/bored.png") },
			{ "cash", Texture::Load("Textures/Emojis/cash.png") },
			{ "clown", Texture::Load("Textures/Emojis/clown.png") },
			{ "cold", Texture::Load("Textures/Emojis/cold.png") },
			{ "cowboy", Texture::Load("Textures/Emojis/cowboy.png") },
			{ "down", Texture::Load("Textures/Emojis/down.png") },
			{ "feet", Texture::Load("Textures/Emojis/feet.png") },
			{ "fire", Texture::Load("Textures/Emojis/fire.png") },
			{ "happy", Texture::Load("Textures/Emojis/happy.png") },
			{ "heart", Texture::Load("Textures/Emojis/heart.png") },
			{ "huff", Texture::Load("Textures/Emojis/huff.png") },
			{ "huh", Texture::Load("Textures/Emojis/huh.png") },
			{ "imp", Texture::Load("Textures/Emojis/imp.png") },
			{ "joy", Texture::Load("Textures/Emojis/joy.png") },
			{ "monkey", Texture::Load("Textures/Emojis/monkey.png") },
			{ "neutral", Texture::Load("Textures/Emojis/neutral.png") },
			{ "noevil", Texture::Load("Textures/Emojis/noevil.png") },
			{ "omg", Texture::Load("Textures/Emojis/omg.png") },
			{ "palm", Texture::Load("Textures/Emojis/palm.png") },
			{ "pensive", Texture::Load("Textures/Emojis/pensive.png") },
			{ "pls", Texture::Load("Textures/Emojis/pls.png") },
			{ "rage", Texture::Load("Textures/Emojis/rage.png") },
			{ "rofl", Texture::Load("Textures/Emojis/rofl.png") },
			{ "rose", Texture::Load("Textures/Emojis/rose.png") },
			{ "sad", Texture::Load("Textures/Emojis/sad.png") },
			{ "sh", Texture::Load("Textures/Emojis/sh.png") },
			{ "shit", Texture::Load("Textures/Emojis/shit.png") },
			{ "skull", Texture::Load("Textures/Emojis/skull.png") },
			{ "skull2", Texture::Load("Textures/Emojis/skull2.png") },
			{ "skull3", Texture::Load("Textures/Emojis/skull3.png") },
			{ "sus", Texture::Load("Textures/Emojis/sus.png") },
			{ "think", Texture::Load("Textures/Emojis/think.png") },
			{ "uk", Texture::Load("Textures/Emojis/uk.png") },
			{ "up", Texture::Load("Textures/Emojis/up.png") },
			{ "weary", Texture::Load("Textures/Emojis/weary.png") },
			{ "wizard", Texture::Load("Textures/Emojis/wizard.png") },
			{ "wtf", Texture::Load("Textures/Emojis/wtf.png") },
			{ "wtf2", Texture::Load("Textures/Emojis/wtf2.png") },
			{ "yawn", Texture::Load("Textures/Emojis/yawn.png") },
		};
	}

	void Texts::Menu()
	{
		ImGui::Checkbox("Show Emojis", &UseEmojis);
	}

	void Texts::ProcessText(std::string& text, Font_s* font, vec2 position, vec2 scale, const vec4& color)
	{
		size_t i = 0;
		while (i < text.size())
		{
			if (text[i] != ':')
			{
				i++;
				continue;
			}
			size_t end = i + 1;
			while (end < text.size() && text[end] != ':' && text[end] != ' ')
				end++;

			if (end >= text.size() || text[end] != ':')
			{
				i++;
				continue;
			}
			size_t length = end - i - 1;
			if (length == 0 || length > 20)
			{
				i++;
				continue;
			}
			std::string emojiName(text.data() + i + 1, length);
			auto it = EmojiMap.find(emojiName);
			if (it != EmojiMap.end())
			{
				std::string textBefore(text.data(), i);
				vec2 textSize = GDraw2D::TextSize(textBefore, font) * scale;
				text.replace(i, length + 2, 8, ' ');

				EmojiCommand cmd;
				cmd.Emoji = it->second;
				cmd.Size = vec2(textSize.y * 1.1);
				cmd.Position = { position.x + textSize.x, position.y - cmd.Size.y };
				cmd.Color = vec4(1, 1, 1, color.w);
				EmojiCommands.push_back(cmd);

				i += 8;
			}
			else
			{
				i++;
			}
		}
	}

	void Texts::OnDrawText(EventRendererText& event)
	{
		if (UseEmojis)
			ProcessText(event.text, event.font, event.position, event.size, event.color);
	}

	void Texts::OnRender()
	{
		if (UseEmojis)
		{
			for (const auto& command : EmojiCommands)
				Draw2D::DrawQuad(vec3(command.Position, 0), command.Size, 0, command.Emoji, command.Color);
			EmojiCommands.clear();
		}
	}
}

#include "Texts.hpp"

namespace IW3SR::Addons
{
	Texts::Texts() : Module("sr.graphics.texts", "Graphics", "Texts")
	{
		UseEmojis = false;

		EmojiMap = {
			{ "afr", Texture::Load("Textures/Emojis/afr.png") },
			{ "ang", Texture::Load("Textures/Emojis/ang.png") },
			{ "cash", Texture::Load("Textures/Emojis/cash.png") },
			{ "clown", Texture::Load("Textures/Emojis/clown.png") },
			{ "cold", Texture::Load("Textures/Emojis/cold.png") },
			{ "cow", Texture::Load("Textures/Emojis/cow.png") },
			{ "feet", Texture::Load("Textures/Emojis/feet.png") },
			{ "fire", Texture::Load("Textures/Emojis/fire.png") },
			{ "heart", Texture::Load("Textures/Emojis/heart.png") },
			{ "imp", Texture::Load("Textures/Emojis/imp.png") },
			{ "joy", Texture::Load("Textures/Emojis/joy.png") },
			{ "mon", Texture::Load("Textures/Emojis/mon.png") },
			{ "neu", Texture::Load("Textures/Emojis/neu.png") },
			{ "noev", Texture::Load("Textures/Emojis/noev.png") },
			{ "om", Texture::Load("Textures/Emojis/om.png") },
			{ "pens", Texture::Load("Textures/Emojis/pens.png") },
			{ "pls", Texture::Load("Textures/Emojis/pls.png") },
			{ "rage", Texture::Load("Textures/Emojis/rage.png") },
			{ "rofl", Texture::Load("Textures/Emojis/rofl.png") },
			{ "rose", Texture::Load("Textures/Emojis/rose.png") },
			{ "sad", Texture::Load("Textures/Emojis/sad.png") },
			{ "sbo", Texture::Load("Textures/Emojis/sbo.png") },
			{ "sh", Texture::Load("Textures/Emojis/sh.png") },
			{ "sha", Texture::Load("Textures/Emojis/sha.png") },
			{ "shit", Texture::Load("Textures/Emojis/shit.png") },
			{ "sku", Texture::Load("Textures/Emojis/sku.png") },
			{ "skuv", Texture::Load("Textures/Emojis/skuv.png") },
			{ "skuw", Texture::Load("Textures/Emojis/skuw.png") },
			{ "spa", Texture::Load("Textures/Emojis/spa.png") },
			{ "sur", Texture::Load("Textures/Emojis/sur.png") },
			{ "sus", Texture::Load("Textures/Emojis/sus.png") },
			{ "tdown", Texture::Load("Textures/Emojis/tdown.png") },
			{ "think", Texture::Load("Textures/Emojis/think.png") },
			{ "tri", Texture::Load("Textures/Emojis/tri.png") },
			{ "tup", Texture::Load("Textures/Emojis/tup.png") },
			{ "uk", Texture::Load("Textures/Emojis/uk.png") },
			{ "wat", Texture::Load("Textures/Emojis/wat.png") },
			{ "weary", Texture::Load("Textures/Emojis/weary.png") },
			{ "wiz", Texture::Load("Textures/Emojis/wiz.png") },
			{ "wtf", Texture::Load("Textures/Emojis/wtf.png") },
			{ "yawn", Texture::Load("Textures/Emojis/yawn.png") },
		};
	}

	void Texts::Menu()
	{
		ImGui::Checkbox("Emojis", &UseEmojis);
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

#include "Texts.hpp"

namespace IW3SR::Addons
{
	Texts::Texts() : Module("sr.graphics.texts", "Graphics", "Texts")
	{
		UseEmojis = false;

		EmojiMap = {
			{ "afr", Texture::Create(Environment::Path(Directory::Images) / "Emojis" / "afr.png") },
			{ "ang", Texture::Create(Environment::Path(Directory::Images) / "Emojis" / "ang.png") },
			{ "cash", Texture::Create(Environment::Path(Directory::Images) / "Emojis" / "cash.png") },
			{ "clown", Texture::Create(Environment::Path(Directory::Images) / "Emojis" / "clown.png") },
			{ "cold", Texture::Create(Environment::Path(Directory::Images) / "Emojis" / "cold.png") },
			{ "cow", Texture::Create(Environment::Path(Directory::Images) / "Emojis" / "cow.png") },
			{ "feet", Texture::Create(Environment::Path(Directory::Images) / "Emojis" / "feet.png") },
			{ "fire", Texture::Create(Environment::Path(Directory::Images) / "Emojis" / "fire.png") },
			{ "heart", Texture::Create(Environment::Path(Directory::Images) / "Emojis" / "heart.png") },
			{ "imp", Texture::Create(Environment::Path(Directory::Images) / "Emojis" / "imp.png") },
			{ "joy", Texture::Create(Environment::Path(Directory::Images) / "Emojis" / "joy.png") },
			{ "mon", Texture::Create(Environment::Path(Directory::Images) / "Emojis" / "mon.png") },
			{ "neu", Texture::Create(Environment::Path(Directory::Images) / "Emojis" / "neu.png") },
			{ "noev", Texture::Create(Environment::Path(Directory::Images) / "Emojis" / "noev.png") },
			{ "om", Texture::Create(Environment::Path(Directory::Images) / "Emojis" / "om.png") },
			{ "pens", Texture::Create(Environment::Path(Directory::Images) / "Emojis" / "pens.png") },
			{ "pls", Texture::Create(Environment::Path(Directory::Images) / "Emojis" / "pls.png") },
			{ "rage", Texture::Create(Environment::Path(Directory::Images) / "Emojis" / "rage.png") },
			{ "rofl", Texture::Create(Environment::Path(Directory::Images) / "Emojis" / "rofl.png") },
			{ "rose", Texture::Create(Environment::Path(Directory::Images) / "Emojis" / "rose.png") },
			{ "sad", Texture::Create(Environment::Path(Directory::Images) / "Emojis" / "sad.png") },
			{ "sbo", Texture::Create(Environment::Path(Directory::Images) / "Emojis" / "sbo.png") },
			{ "sh", Texture::Create(Environment::Path(Directory::Images) / "Emojis" / "sh.png") },
			{ "sha", Texture::Create(Environment::Path(Directory::Images) / "Emojis" / "sha.png") },
			{ "shit", Texture::Create(Environment::Path(Directory::Images) / "Emojis" / "shit.png") },
			{ "sku", Texture::Create(Environment::Path(Directory::Images) / "Emojis" / "sku.png") },
			{ "skuv", Texture::Create(Environment::Path(Directory::Images) / "Emojis" / "skuv.png") },
			{ "skuw", Texture::Create(Environment::Path(Directory::Images) / "Emojis" / "skuw.png") },
			{ "spa", Texture::Create(Environment::Path(Directory::Images) / "Emojis" / "spa.png") },
			{ "sur", Texture::Create(Environment::Path(Directory::Images) / "Emojis" / "sur.png") },
			{ "sus", Texture::Create(Environment::Path(Directory::Images) / "Emojis" / "sus.png") },
			{ "tdown", Texture::Create(Environment::Path(Directory::Images) / "Emojis" / "tdown.png") },
			{ "think", Texture::Create(Environment::Path(Directory::Images) / "Emojis" / "think.png") },
			{ "tri", Texture::Create(Environment::Path(Directory::Images) / "Emojis" / "tri.png") },
			{ "tup", Texture::Create(Environment::Path(Directory::Images) / "Emojis" / "tup.png") },
			{ "uk", Texture::Create(Environment::Path(Directory::Images) / "Emojis" / "uk.png") },
			{ "wat", Texture::Create(Environment::Path(Directory::Images) / "Emojis" / "wat.png") },
			{ "weary", Texture::Create(Environment::Path(Directory::Images) / "Emojis" / "weary.png") },
			{ "wiz", Texture::Create(Environment::Path(Directory::Images) / "Emojis" / "wiz.png") },
			{ "wtf", Texture::Create(Environment::Path(Directory::Images) / "Emojis" / "wtf.png") },
			{ "yawn", Texture::Create(Environment::Path(Directory::Images) / "Emojis" / "yawn.png") },
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
				Draw2D::Rect(command.Emoji, command.Position, command.Size, command.Color);
			EmojiCommands.clear();
		}
	}
}

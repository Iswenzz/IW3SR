#include "Emojis.hpp"

namespace IW3SR
{
	void Emojis::Initialize()
	{
		EmojisMap = {
			{ "afr", Texture::Create(Environment::Path(Directory::Images) / "emojis" / "afr.png") },
			{ "ang", Texture::Create(Environment::Path(Directory::Images) / "emojis" / "ang.png") },
			{ "cash", Texture::Create(Environment::Path(Directory::Images) / "emojis" / "cash.png") },
			{ "clown", Texture::Create(Environment::Path(Directory::Images) / "emojis" / "clown.png") },
			{ "cold", Texture::Create(Environment::Path(Directory::Images) / "emojis" / "cold.png") },
			{ "cow", Texture::Create(Environment::Path(Directory::Images) / "emojis" / "cow.png") },
			{ "feet", Texture::Create(Environment::Path(Directory::Images) / "emojis" / "feet.png") },
			{ "fire", Texture::Create(Environment::Path(Directory::Images) / "emojis" / "fire.png") },
			{ "heart", Texture::Create(Environment::Path(Directory::Images) / "emojis" / "heart.png") },
			{ "imp", Texture::Create(Environment::Path(Directory::Images) / "emojis" / "imp.png") },
			{ "joy", Texture::Create(Environment::Path(Directory::Images) / "emojis" / "joy.png") },
			{ "mon", Texture::Create(Environment::Path(Directory::Images) / "emojis" / "mon.png") },
			{ "neu", Texture::Create(Environment::Path(Directory::Images) / "emojis" / "neu.png") },
			{ "noev", Texture::Create(Environment::Path(Directory::Images) / "emojis" / "noev.png") },
			{ "om", Texture::Create(Environment::Path(Directory::Images) / "emojis" / "om.png") },
			{ "pens", Texture::Create(Environment::Path(Directory::Images) / "emojis" / "pens.png") },
			{ "pls", Texture::Create(Environment::Path(Directory::Images) / "emojis" / "pls.png") },
			{ "rage", Texture::Create(Environment::Path(Directory::Images) / "emojis" / "rage.png") },
			{ "rofl", Texture::Create(Environment::Path(Directory::Images) / "emojis" / "rofl.png") },
			{ "rose", Texture::Create(Environment::Path(Directory::Images) / "emojis" / "rose.png") },
			{ "sad", Texture::Create(Environment::Path(Directory::Images) / "emojis" / "sad.png") },
			{ "sbo", Texture::Create(Environment::Path(Directory::Images) / "emojis" / "sbo.png") },
			{ "sh", Texture::Create(Environment::Path(Directory::Images) / "emojis" / "sh.png") },
			{ "sha", Texture::Create(Environment::Path(Directory::Images) / "emojis" / "sha.png") },
			{ "shit", Texture::Create(Environment::Path(Directory::Images) / "emojis" / "shit.png") },
			{ "sku", Texture::Create(Environment::Path(Directory::Images) / "emojis" / "sku.png") },
			{ "skuv", Texture::Create(Environment::Path(Directory::Images) / "emojis" / "skuv.png") },
			{ "skuw", Texture::Create(Environment::Path(Directory::Images) / "emojis" / "skuw.png") },
			{ "spa", Texture::Create(Environment::Path(Directory::Images) / "emojis" / "spa.png") },
			{ "sur", Texture::Create(Environment::Path(Directory::Images) / "emojis" / "sur.png") },
			{ "sus", Texture::Create(Environment::Path(Directory::Images) / "emojis" / "sus.png") },
			{ "tdown", Texture::Create(Environment::Path(Directory::Images) / "emojis" / "tdown.png") },
			{ "think", Texture::Create(Environment::Path(Directory::Images) / "emojis" / "think.png") },
			{ "tri", Texture::Create(Environment::Path(Directory::Images) / "emojis" / "tri.png") },
			{ "tup", Texture::Create(Environment::Path(Directory::Images) / "emojis" / "tup.png") },
			{ "uk", Texture::Create(Environment::Path(Directory::Images) / "emojis" / "uk.png") },
			{ "wat", Texture::Create(Environment::Path(Directory::Images) / "emojis" / "wat.png") },
			{ "weary", Texture::Create(Environment::Path(Directory::Images) / "emojis" / "weary.png") },
			{ "wiz", Texture::Create(Environment::Path(Directory::Images) / "emojis" / "wiz.png") },
			{ "wtf", Texture::Create(Environment::Path(Directory::Images) / "emojis" / "wtf.png") },
			{ "yawn", Texture::Create(Environment::Path(Directory::Images) / "emojis" / "yawn.png") },
		};
	}

	void Emojis::ProcessText(std::string& text, Font_s* font, vec2 position, float scale, const vec4& color)
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
			auto it = EmojisMap.find(emojiName);
			if (it != EmojisMap.end())
			{
				std::string textBefore(text.data(), i);
				vec2 textSize = GDraw2D::TextSize(textBefore, font) * scale;
				text.replace(i, length + 2, 8, ' ');

				EmojiCommand cmd;
				cmd.Emoji = it->second;
				cmd.Size = vec2(textSize.y * 1.1);
				cmd.Position = { position.x + textSize.x, position.y - cmd.Size.y };
				cmd.Color = vec4(1, 1, 1, color.w);
				Commands.push_back(cmd);

				i += 8;
			}
			else
			{
				i++;
			}
		}
	}

	void Emojis::Frame()
	{
		for (const auto& command : Commands)
			Draw2D::Rect(command.Emoji, command.Position, command.Size, command.Color);
		Commands.clear();
	}
}

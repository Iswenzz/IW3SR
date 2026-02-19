#include "Texts.hpp"

namespace IW3SR::Addons
{
	Texts::Texts() : Module("sr.graphics.texts", "Graphics", "Texts")
	{
		UseEmojis = false;

		EmojiMap = {
			{ "afr", Texture::Create(VFS::GetFile("Textures/Emojis/afr.png")) },
			{ "ang", Texture::Create(VFS::GetFile("Textures/Emojis/ang.png")) },
			{ "cash", Texture::Create(VFS::GetFile("Textures/Emojis/cash.png")) },
			{ "clown", Texture::Create(VFS::GetFile("Textures/Emojis/clown.png")) },
			{ "cold", Texture::Create(VFS::GetFile("Textures/Emojis/cold.png")) },
			{ "cow", Texture::Create(VFS::GetFile("Textures/Emojis/cow.png")) },
			{ "feet", Texture::Create(VFS::GetFile("Textures/Emojis/feet.png")) },
			{ "fire", Texture::Create(VFS::GetFile("Textures/Emojis/fire.png")) },
			{ "heart", Texture::Create(VFS::GetFile("Textures/Emojis/heart.png")) },
			{ "imp", Texture::Create(VFS::GetFile("Textures/Emojis/imp.png")) },
			{ "joy", Texture::Create(VFS::GetFile("Textures/Emojis/joy.png")) },
			{ "mon", Texture::Create(VFS::GetFile("Textures/Emojis/mon.png")) },
			{ "neu", Texture::Create(VFS::GetFile("Textures/Emojis/neu.png")) },
			{ "noev", Texture::Create(VFS::GetFile("Textures/Emojis/noev.png")) },
			{ "om", Texture::Create(VFS::GetFile("Textures/Emojis/om.png")) },
			{ "pens", Texture::Create(VFS::GetFile("Textures/Emojis/pens.png")) },
			{ "pls", Texture::Create(VFS::GetFile("Textures/Emojis/pls.png")) },
			{ "rage", Texture::Create(VFS::GetFile("Textures/Emojis/rage.png")) },
			{ "rofl", Texture::Create(VFS::GetFile("Textures/Emojis/rofl.png")) },
			{ "rose", Texture::Create(VFS::GetFile("Textures/Emojis/rose.png")) },
			{ "sad", Texture::Create(VFS::GetFile("Textures/Emojis/sad.png")) },
			{ "sbo", Texture::Create(VFS::GetFile("Textures/Emojis/sbo.png")) },
			{ "sh", Texture::Create(VFS::GetFile("Textures/Emojis/sh.png")) },
			{ "sha", Texture::Create(VFS::GetFile("Textures/Emojis/sha.png")) },
			{ "shit", Texture::Create(VFS::GetFile("Textures/Emojis/shit.png")) },
			{ "sku", Texture::Create(VFS::GetFile("Textures/Emojis/sku.png")) },
			{ "skuv", Texture::Create(VFS::GetFile("Textures/Emojis/skuv.png")) },
			{ "skuw", Texture::Create(VFS::GetFile("Textures/Emojis/skuw.png")) },
			{ "spa", Texture::Create(VFS::GetFile("Textures/Emojis/spa.png")) },
			{ "sur", Texture::Create(VFS::GetFile("Textures/Emojis/sur.png")) },
			{ "sus", Texture::Create(VFS::GetFile("Textures/Emojis/sus.png")) },
			{ "tdown", Texture::Create(VFS::GetFile("Textures/Emojis/tdown.png")) },
			{ "think", Texture::Create(VFS::GetFile("Textures/Emojis/think.png")) },
			{ "tri", Texture::Create(VFS::GetFile("Textures/Emojis/tri.png")) },
			{ "tup", Texture::Create(VFS::GetFile("Textures/Emojis/tup.png")) },
			{ "uk", Texture::Create(VFS::GetFile("Textures/Emojis/uk.png")) },
			{ "wat", Texture::Create(VFS::GetFile("Textures/Emojis/wat.png")) },
			{ "weary", Texture::Create(VFS::GetFile("Textures/Emojis/weary.png")) },
			{ "wiz", Texture::Create(VFS::GetFile("Textures/Emojis/wiz.png")) },
			{ "wtf", Texture::Create(VFS::GetFile("Textures/Emojis/wtf.png")) },
			{ "yawn", Texture::Create(VFS::GetFile("Textures/Emojis/yawn.png")) },
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

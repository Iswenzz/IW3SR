#include "General.hpp"

#include <cctype>
#include <cstring>

namespace IW3SR::Addons
{
	constexpr int KeyCatchMessage = 0x20;
	constexpr int KeyTab = 9;
	constexpr int KeyUpArrow = 0x9A;
	constexpr int KeyDownArrow = 0x9B;
	constexpr size_t MaxSuggestions = 8;

	General::General() : Module("sr.graphics.general", "Graphics", "General")
	{
		UseEmojis = true;

		for (const auto& [path, info] : VFS::SearchFiles("Textures/Emojis/"))
			EmojiMap.emplace(std::filesystem::path(path).stem().string(), EmojiAsset{ path });
	}

	void General::Menu()
	{
		if (!ImGui::BeginSection("General"))
			return;

		ImGui::Property("Show Emojis");
		ImGui::Switch("##emojis", &UseEmojis);

		// Bound straight to the dvar rather than mirrored into a member, so the console and the
		// switch stay in step and the setting keeps saving through the dvar's own DVAR_SAVED.
		static const auto sr_portal_view = Dvar::Find("sr_portal_view");
		if (sr_portal_view)
		{
			ImGui::Property("Portal View");
			if (ImGui::Switch("##portal", &sr_portal_view->current.enabled))
				Dvar::SetBool(sr_portal_view, sr_portal_view->current.enabled);
			ImGui::Tooltip(
				"Draw the far side of a linked portal pair into their surfaces.\n"
				"Each portal on screen costs a second scene render.");
		}
		ImGui::EndSection();
	}

	void General::ProcessText(std::string& text, Font_s* font, vec2 position, vec2 scale, const vec4& color)
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
				cmd.Emoji = &it->second;
				cmd.Size = vec2(textSize.y * 1.1);
				cmd.Position = { position.x + textSize.x, position.y - cmd.Size.y };
				cmd.Color = vec4(1, 1, 1, color.w);
				{
					std::scoped_lock lock(EmojiMutex);
					EmojiCommands.push_back(cmd);
				}

				i += 8;
			}
			else
			{
				i++;
			}
		}
	}

	void General::OnDrawText(EventRendererText& event)
	{
		if (UseEmojis)
			ProcessText(event.text, event.font, event.position, event.size, event.color);
	}

	// Tab completes the highlighted suggestion and the arrows move through the list. Every other key,
	// and these ones while nothing is suggested, still reach the chat field.
	void General::OnChatKey(EventClientChatKey& event)
	{
		if (!UseEmojis || (event.key != KeyTab && event.key != KeyUpArrow && event.key != KeyDownArrow))
			return;

		field_t& field = player_keys->chatField;
		const auto query = FindQuery(field);
		if (!query)
			return;

		const auto suggestions = FindSuggestions(query->Prefix);
		if (suggestions.empty())
			return;

		event.PreventDefault = true;
		const int count = static_cast<int>(suggestions.size());

		std::scoped_lock lock(SuggestionMutex);
		if (SuggestionPrefix != query->Prefix)
		{
			SuggestionPrefix = query->Prefix;
			Suggestion = 0;
		}
		if (event.key == KeyUpArrow)
		{
			Suggestion = (Suggestion + count - 1) % count;
			return;
		}
		if (event.key == KeyDownArrow)
		{
			Suggestion = (Suggestion + 1) % count;
			return;
		}

		std::string text(field.buffer, strnlen(field.buffer, sizeof(field.buffer)));
		std::string emoji = std::format(":{}:", suggestions[Suggestion]->first);
		if (query->End == text.size())
			emoji += ' ';

		text.replace(query->Start, query->End - query->Start, emoji);
		if (text.size() < sizeof(field.buffer))
		{
			std::memcpy(field.buffer, text.c_str(), text.size() + 1);
			field.cursor = static_cast<int>(query->Start + emoji.size());
		}
		SuggestionPrefix.clear();
		Suggestion = 0;
	}

	// Text is queued from the main thread's draw calls while this runs on the render thread, so the
	// list is taken whole under the lock rather than walked in place.
	void General::OnRender()
	{
		std::vector<EmojiCommand> commands;
		{
			std::scoped_lock lock(EmojiMutex);
			commands.swap(EmojiCommands);
		}
		if (UseEmojis)
		{
			for (const auto& command : commands)
				Draw2D::DrawQuad(vec3(command.Position, 0), command.Size, 0, GetImage(*command.Emoji), command.Color);

			DrawSuggestions();
		}
	}

	// Loaded on first draw since there are close to two thousand of them; only the render thread
	// touches Image.
	const Ref<Texture>& General::GetImage(EmojiAsset& emoji)
	{
		if (!emoji.Image)
			emoji.Image = Texture::Load(emoji.Path);
		return emoji.Image;
	}

	// The emoji name being typed right before the cursor. Its colon has to start a word, so a time like
	// 12:30 never asks for one, and two letters are needed so :D and :P don't either.
	std::optional<EmojiQuery> General::FindQuery(const field_t& field)
	{
		const std::string_view text(field.buffer, strnlen(field.buffer, sizeof(field.buffer)));
		const size_t end = static_cast<size_t>(std::clamp(field.cursor, 0, static_cast<int>(text.size())));

		size_t start = end;
		while (start > 0 && std::isalnum(static_cast<unsigned char>(text[start - 1])))
			start--;

		if (end - start < 2 || start == 0 || text[start - 1] != ':')
			return std::nullopt;
		if (start > 1 && std::isalnum(static_cast<unsigned char>(text[start - 2])))
			return std::nullopt;

		std::string prefix(text.substr(start, end - start));
		std::ranges::transform(prefix, prefix.begin(), [](unsigned char c) { return std::tolower(c); });
		return EmojiQuery{ start - 1, end, prefix };
	}

	std::vector<General::EmojiEntry*> General::FindSuggestions(const std::string& prefix)
	{
		std::vector<EmojiEntry*> suggestions;
		for (auto it = EmojiMap.lower_bound(prefix); it != EmojiMap.end() && it->first.starts_with(prefix); ++it)
		{
			if (suggestions.size() == MaxSuggestions)
				return suggestions;
			suggestions.push_back(&*it);
		}
		for (auto& entry : EmojiMap)
		{
			if (suggestions.size() == MaxSuggestions)
				break;
			if (!entry.first.starts_with(prefix) && entry.first.contains(prefix))
				suggestions.push_back(&entry);
		}
		return suggestions;
	}

	// Listed under the say line, which Con_DrawSay draws 16 units tall and 24 below cg_hudSayPosition.
	void General::DrawSuggestions()
	{
		static const auto sayPosition = Dvar::Find("cg_hudSayPosition");
		if (!sayPosition || !(client_ui->keyCatchers & KeyCatchMessage))
			return;

		const auto query = FindQuery(player_keys->chatField);
		if (!query)
			return;

		const auto suggestions = FindSuggestions(query->Prefix);
		if (suggestions.empty())
			return;

		size_t selected = 0;
		{
			std::scoped_lock lock(SuggestionMutex);
			if (SuggestionPrefix == query->Prefix)
				selected = static_cast<size_t>(Suggestion);
		}

		const float icon = 16 * UI::Screen.VirtualToReal.y;
		const float padding = icon / 4;
		const float rowHeight = icon + padding;
		const Ref<Font> font = Font::Create({ .ID = FONT_OPENSANS, .Height = static_cast<int>(icon * 0.7f) });

		std::vector<std::string> labels;
		vec2 textSize = { 0, 0 };
		for (const auto entry : suggestions)
		{
			labels.push_back(std::format(":{}:", entry->first));
			textSize = glm::max(textSize, Draw2D::GetTextSize(labels.back(), font));
		}

		vec2 position = { sayPosition->current.vector.x, sayPosition->current.vector.y + 24 + 16 + 4 };
		UI::Screen.Apply(position, Horizontal::Left, Vertical::Top);

		const vec2 size = { icon + textSize.x + padding * 3, rowHeight * suggestions.size() + padding };
		Draw2D::DrawQuad(vec3(position, 0), size, { 0, 0, 0, 0.75f });

		for (size_t i = 0; i < suggestions.size(); i++)
		{
			const vec2 row = { position.x, position.y + padding / 2 + rowHeight * i };
			if (i == selected)
				Draw2D::DrawQuad(vec3(row, 0), { size.x, rowHeight }, { 1, 1, 1, 0.15f });

			const vec3 image = { row.x + padding, row.y + padding / 2, 0 };
			Draw2D::DrawQuad(image, vec2(icon), 0, GetImage(suggestions[i]->second), vec4(1));

			const vec3 text = { image.x + icon + padding, row.y + (rowHeight - textSize.y) / 2, 0 };
			Draw2D::DrawText(labels[i], font, text, 0, vec4(1), vec2(0));
		}
	}
}

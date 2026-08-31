#include "Autocomplete.hpp"
#include "Dvar.hpp"
#include "Patch.hpp"

#include <cctype>
#include <cstring>

namespace IW3SR
{
	constexpr uintptr_t CompleteArgumentCall = 0x467073;

	static std::vector<std::string> Tokenize(const char* line)
	{
		std::istringstream stream(line);
		std::vector<std::string> tokens;
		std::string token;

		while (stream >> token)
			tokens.push_back(token);
		return tokens;
	}

	static bool HasPrefix(const char* string, const std::string& prefix)
	{
		for (size_t i = 0; i < prefix.size(); i++)
		{
			const int left = std::tolower(static_cast<unsigned char>(string[i]));
			const int right = std::tolower(static_cast<unsigned char>(prefix[i]));

			if (!string[i] || left != right)
				return false;
		}
		return true;
	}

	void Autocomplete::Initialize()
	{
		if (Patch::UseCoD4X)
			return;

		// Retargeting anything but the call itself would leave the console executing rubble.
		if (Memory::Get<uint8_t>(CompleteArgumentCall) != 0xE8)
		{
			Log::WriteLine(Channel::Error, "No call to redirect at {:#x}, dvar value completion is off.",
				CompleteArgumentCall);
			return;
		}
		Memory::CALL(CompleteArgumentCall, reinterpret_cast<uintptr_t>(&Autocomplete::CompleteDvarValue));
	}

	void Autocomplete::CompleteDvarValue()
	{
		if (!g_consoleField)
			return;

		const std::vector<std::string> tokens = Tokenize(g_consoleField->buffer);
		if (tokens.size() != 2 || tokens[1].empty())
			return;

		std::string name = tokens[0];
		if (name.starts_with('/') || name.starts_with('\\'))
			name.erase(0, 1);

		const dvar_s* dvar = Dvar::Find(name);
		if (!dvar || dvar->type != DvarType::ENUMERATION)
			return;

		const std::string& prefix = tokens[1];
		Replace(prefix.size(), Match(dvar->domain.enumeration.strings, dvar->domain.enumeration.stringCount, prefix));
	}

	// An ambiguous prefix fills in only the part every candidate agrees on.
	std::string Autocomplete::Match(const char** strings, int count, const std::string& prefix)
	{
		std::string completed;
		bool found = false;

		if (!strings)
			return completed;

		for (int i = 0; i < count; i++)
		{
			const char* string = strings[i];
			if (!string || !HasPrefix(string, prefix))
				continue;

			if (!found)
			{
				completed = string;
				found = true;
				continue;
			}

			size_t shared = prefix.size();
			while (shared < completed.size() && string[shared] && completed[shared] == string[shared])
				shared++;
			completed.resize(shared);
		}
		return completed;
	}

	void Autocomplete::Replace(size_t replaceCount, const std::string& replacement)
	{
		if (replacement.empty())
			return;

		char* buffer = g_consoleField->buffer;
		size_t length = std::strlen(buffer);

		while (length && std::isspace(static_cast<unsigned char>(buffer[length - 1])))
			length--;

		if (replaceCount > length)
			return;

		const size_t start = length - replaceCount;
		const size_t room = sizeof(g_consoleField->buffer) - start - 1;
		const size_t count = std::min(replacement.size(), room);

		std::memcpy(buffer + start, replacement.data(), count);
		buffer[start + count] = '\0';

		g_consoleField->cursor = static_cast<int>(start + count);
	}
}

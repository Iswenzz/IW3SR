#include "ServerFilter.hpp"

#include "Game/System/Dvar.hpp"
#include "Game/System/Patch.hpp"

#include <charconv>

namespace IW3SR
{
	constexpr const char* FilterCache = "serverfilter.txt";
	constexpr const char* FilterMagic = "\\addr\\";

	constexpr size_t FilterMaxEntries = 1024;
	constexpr size_t FilterMaxSize = 8 * 1024;

	void ServerFilter::Initialize()
	{
		Url = Dvar::RegisterString("sr_serverfilter_url", DVAR_SAVED,
			"URL of the server filter list, empty to never fetch one", "");
		Enabled = Dvar::RegisterBool("sr_serverfilter", DVAR_SAVED,
			"Hide, refuse and redirect the servers the filter list names", true);

		Refresh();
	}

	void ServerFilter::Refresh()
	{
		if (Busy || Patch::UseCoD4X || !Url || !Url->current.string || !*Url->current.string)
			return;
		Busy = true;

		RemoteFile file;
		file.Url = Url->current.string;
		file.Cache = FilterCache;
		file.Magic = FilterMagic;
		file.MaxSize = FilterMaxSize;

		Remote::Fetch(file, Apply);
	}

	bool ServerFilter::Check(netadr_t& address, int severity)
	{
		if (Patch::UseCoD4X || !Enabled || !Enabled->current.enabled)
			return false;

		std::lock_guard lock(Guard);

		for (const FilterEntry& entry : Entries)
		{
			// An entry written without a port covers every server on the host.
			if (!Remote::CompareAddress(address, entry.Address, entry.Address.port != 0))
				continue;

			if (entry.Severity == FilterRedirect && entry.Redirect.type == NA_IP)
			{
				address = entry.Redirect;
				return false;
			}
			return entry.Severity >= severity;
		}
		return false;
	}

	bool ServerFilter::Command(const std::string& command)
	{
		std::istringstream stream(command);
		std::string name;
		stream >> name;

		if (name == "sr_serverfilter_refresh")
		{
			Refresh();
			return true;
		}
		if (name == "sr_serverfilter_list")
		{
			Com_PrintMessage(CON_CHANNEL_CONSOLEONLY,
				std::format("{} filter entries loaded.\n", Entries.size()).c_str(), 0);

			for (const FilterEntry& entry : Entries)
			{
				const std::string line = entry.Severity == FilterRedirect
					? std::format("  {} -> {}\n", Remote::FormatAddress(entry.Address),
						  Remote::FormatAddress(entry.Redirect))
					: std::format("  {} severity {}\n", Remote::FormatAddress(entry.Address), entry.Severity);

				Com_PrintMessage(CON_CHANNEL_CONSOLEONLY, line.c_str(), 0);
			}
			return true;
		}
		return false;
	}

	void ServerFilter::Apply(const RemoteResult& result)
	{
		Busy = false;

		if (!result.Error.empty())
			Com_PrintMessage(CON_CHANNEL_CONSOLEONLY,
				std::format("^3Server filter list download failed: {}.\n", result.Error).c_str(), 0);

		if (result.Source == RemoteSource::Missing)
			return;

		// Parsed first and swapped in whole, so a half read list never filters anything.
		std::vector<FilterEntry> parsed = Parse(result.Body);
		{
			std::lock_guard lock(Guard);
			Entries = std::move(parsed);
		}

		Com_PrintMessage(CON_CHANNEL_CONSOLEONLY,
			std::format("Server filter list read from {}, {} entries.\n",
				result.Source == RemoteSource::Network ? "the network" : "the cache", Entries.size())
				.c_str(),
			0);
	}

	// One info string per line: \addr\1.2.3.4:28960\type\8\destaddr\5.6.7.8:28960.
	std::vector<FilterEntry> ServerFilter::Parse(const std::string& body)
	{
		std::vector<FilterEntry> entries;
		std::istringstream stream(body);
		std::string line;

		while (std::getline(stream, line) && entries.size() < FilterMaxEntries)
		{
			const std::string address = Remote::InfoValueForKey(line, "addr");
			if (address.empty())
				continue;

			FilterEntry entry;
			if (!Remote::ParseAddress(address, entry.Address))
			{
				Log::WriteLine(Channel::Warning, "Server filter list: cannot read the address '{}'.", address);
				continue;
			}
			Remote::ParseAddress(Remote::InfoValueForKey(line, "destaddr"), entry.Redirect);

			const std::string severity = Remote::InfoValueForKey(line, "type");
			std::from_chars(severity.data(), severity.data() + severity.size(), entry.Severity);

			entries.push_back(entry);
		}
		return entries;
	}
}

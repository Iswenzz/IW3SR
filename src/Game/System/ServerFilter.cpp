#include "ServerFilter.hpp"

#include "Game/System/Dvar.hpp"
#include "Game/System/Net.hpp"
#include "Game/System/Patch.hpp"
#include "Game/System/System.hpp"

#include "Engine/Core/Network/HTTP.hpp"

#include <charconv>

namespace IW3SR
{
	constexpr const char* FilterMagic = "\\addr\\";

	constexpr size_t FilterMaxEntries = 1024;
	constexpr size_t FilterMaxSize = 8 * 1024;
	constexpr long FilterTimeout = 10;

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

		HTTPRequest request = HTTP::Get(Url->current.string, nullptr);
		request.TimeoutSeconds = FilterTimeout;
		request.ConnectTimeoutSeconds = FilterTimeout;
		request.Callback = [](const HTTPResponse& response)
		{
			std::string error;

			if (!response.Success)
				error = response.Error;
			else if (response.Code != 200)
				error = std::format("HTTP {}", response.Code);
			else if (response.Body.size() > FilterMaxSize)
				error = std::format("{} bytes, past the {} byte limit", response.Body.size(), FilterMaxSize);
			else if (!HasHeader(response.Body))
				error = std::format("no '{}' header", FilterMagic);

			// This runs on a pool thread; the list and the console belong to the game one.
			GSystem::Tasks.Add([body = error.empty() ? response.Body : std::string(), error]
				{ Apply(body, error); });
		};
		request.Send();
	}

	bool ServerFilter::Check(netadr_t& address, int severity)
	{
		if (Patch::UseCoD4X || !Enabled || !Enabled->current.enabled)
			return false;

		std::lock_guard lock(Guard);

		for (const FilterEntry& entry : Entries)
		{
			// An entry written without a port covers every server on the host.
			if (!Net::Equal(address, entry.Address, entry.Address.port != 0))
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
					? std::format("  {} -> {}\n", Net::ToString(entry.Address), Net::ToString(entry.Redirect))
					: std::format("  {} severity {}\n", Net::ToString(entry.Address), entry.Severity);

				Com_PrintMessage(CON_CHANNEL_CONSOLEONLY, line.c_str(), 0);
			}
			return true;
		}
		return false;
	}

	void ServerFilter::Apply(const std::string& body, const std::string& error)
	{
		Busy = false;

		if (!error.empty())
		{
			Com_PrintMessage(CON_CHANNEL_CONSOLEONLY,
				std::format("^3Server filter list download failed: {}. No servers are filtered.\n", error).c_str(), 0);
			return;
		}

		// Parsed first and swapped in whole, so a half read list never filters anything.
		std::vector<FilterEntry> parsed = Parse(body);
		{
			std::lock_guard lock(Guard);
			Entries = std::move(parsed);
		}

		Com_PrintMessage(CON_CHANNEL_CONSOLEONLY,
			std::format("Server filter list loaded, {} entries.\n", Entries.size()).c_str(), 0);
	}

	// One info string per line: \addr\1.2.3.4:28960\type\8\destaddr\5.6.7.8:28960.
	std::vector<FilterEntry> ServerFilter::Parse(const std::string& body)
	{
		std::vector<FilterEntry> entries;
		std::istringstream stream(body);
		std::string line;

		while (std::getline(stream, line) && entries.size() < FilterMaxEntries)
		{
			const std::string address = ValueForKey(line, "addr");
			if (address.empty())
				continue;

			FilterEntry entry;
			if (!Net::ParseAddress(address, entry.Address))
			{
				Log::WriteLine(Channel::Warning, "Server filter list: cannot read the address '{}'.", address);
				continue;
			}
			Net::ParseAddress(ValueForKey(line, "destaddr"), entry.Redirect);

			const std::string severity = ValueForKey(line, "type");
			std::from_chars(severity.data(), severity.data() + severity.size(), entry.Severity);

			entries.push_back(entry);
		}
		return entries;
	}

	std::string ServerFilter::ValueForKey(std::string_view info, std::string_view key)
	{
		while (!info.empty())
		{
			if (info.front() == '\\')
				info.remove_prefix(1);

			const size_t keyEnd = info.find('\\');
			if (keyEnd == std::string_view::npos)
				break;

			const std::string_view name = info.substr(0, keyEnd);
			info.remove_prefix(keyEnd + 1);

			const size_t valueEnd = info.find('\\');
			if (name == key)
				return std::string(info.substr(0, valueEnd));

			if (valueEnd == std::string_view::npos)
				break;
			info.remove_prefix(valueEnd);
		}
		return {};
	}

	// First line only, so a page that merely quotes the header cannot pass for the list.
	bool ServerFilter::HasHeader(const std::string& body)
	{
		const std::string_view first = std::string_view(body).substr(0, body.find('\n'));
		return first.find(FilterMagic) != std::string_view::npos;
	}
}

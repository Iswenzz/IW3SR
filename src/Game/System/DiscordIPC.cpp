#include "DiscordIPC.hpp"

#include <cstring>

namespace IW3SR
{
	// Discord opens one pipe per running client, and the first one that answers is ours.
	constexpr int PipeCount = 10;

	// A larger length means the stream desynchronised, so the connection is dropped.
	constexpr uint32_t MaxPayload = 64 * 1024;

	static bool WriteRegistry(const std::wstring& path, const wchar_t* name, const std::wstring& value)
	{
		HKEY key = nullptr;
		if (RegCreateKeyExW(HKEY_CURRENT_USER, path.c_str(), 0, nullptr, 0, KEY_WRITE, nullptr, &key, nullptr)
			!= ERROR_SUCCESS)
			return false;

		const DWORD size = static_cast<DWORD>((value.size() + 1) * sizeof(wchar_t));
		const LSTATUS result =
			RegSetValueExW(key, name, 0, REG_SZ, reinterpret_cast<const BYTE*>(value.c_str()), size);

		RegCloseKey(key);
		return result == ERROR_SUCCESS;
	}

	// Only ever fed an application id, which is digits, so widening a byte at a time is exact.
	static std::wstring Widen(const std::string& text)
	{
		std::wstring wide;
		wide.reserve(text.size());

		for (char c : text)
			wide += static_cast<wchar_t>(static_cast<unsigned char>(c));
		return wide;
	}

	bool DiscordIPC::Connect(const std::string& applicationId)
	{
		if (Pipe)
			return true;

		for (int i = 0; i < PipeCount && !Pipe; i++)
			Open(i);

		if (!Pipe)
			return false;

		Framed = false;
		return Send(DiscordOp::Handshake, { { "v", 1 }, { "client_id", applicationId } });
	}

	void DiscordIPC::Disconnect()
	{
		if (Pipe)
			CloseHandle(Pipe);

		Pipe = nullptr;
		Framed = false;
		Opcode = 0;
		Length = 0;
	}

	bool DiscordIPC::IsOpen()
	{
		return Pipe != nullptr;
	}

	bool DiscordIPC::Send(DiscordOp opcode, const nlohmann::json& payload)
	{
		if (!Pipe)
			return false;

		// Names arrive as raw bytes; the strict serialiser would throw out of the game loop on the
		// first non-UTF-8 one.
		const std::string body = payload.dump(-1, ' ', false, nlohmann::json::error_handler_t::replace);
		const uint32_t header[2] = { static_cast<uint32_t>(opcode), static_cast<uint32_t>(body.size()) };

		// Host order, which the protocol defines as little endian and the game only ever runs on.
		std::vector<uint8_t> frame(sizeof(header) + body.size());
		std::memcpy(frame.data(), header, sizeof(header));
		std::memcpy(frame.data() + sizeof(header), body.data(), body.size());

		DWORD written = 0;
		if (!WriteFile(Pipe, frame.data(), static_cast<DWORD>(frame.size()), &written, nullptr)
			|| written != frame.size())
		{
			Disconnect();
			return false;
		}
		return true;
	}

	bool DiscordIPC::Command(const std::string& command, const nlohmann::json& args)
	{
		nlohmann::json payload = { { "cmd", command }, { "nonce", std::to_string(++Nonce) } };

		if (!args.is_null())
			payload["args"] = args;

		return Send(DiscordOp::Frame, payload);
	}

	bool DiscordIPC::Subscribe(const std::string& event)
	{
		return Send(DiscordOp::Frame,
			{ { "cmd", "SUBSCRIBE" }, { "evt", event }, { "nonce", std::to_string(++Nonce) } });
	}

	// Never blocks: a half arrived frame is left for the next call, which is why the header stays
	// in Opcode and Length between calls.
	std::optional<DiscordMessage> DiscordIPC::Poll()
	{
		if (!Pipe)
			return std::nullopt;

		if (!Framed)
		{
			constexpr uint32_t headerSize = sizeof(uint32_t) * 2;
			uint32_t header[2] = {};

			if (Available() < headerSize || !Read(header, headerSize))
				return std::nullopt;

			Opcode = header[0];
			Length = header[1];
			Framed = true;

			if (Length > MaxPayload)
			{
				Log::WriteLine(Channel::Warning, "Discord sent a {} byte frame, dropping the connection.", Length);
				Disconnect();
				return std::nullopt;
			}
		}
		if (Available() < Length)
			return std::nullopt;

		std::string body(Length, '\0');
		if (Length && !Read(body.data(), Length))
			return std::nullopt;

		Framed = false;

		DiscordMessage message;
		message.Opcode = static_cast<DiscordOp>(Opcode);
		message.Payload = nlohmann::json::parse(body, nullptr, false);

		if (message.Payload.is_discarded())
			message.Payload = nlohmann::json::object();

		if (message.Opcode == DiscordOp::Ping)
			Send(DiscordOp::Pong, message.Payload);
		if (message.Opcode == DiscordOp::Close)
			Disconnect();

		return message;
	}

	// Discord launches the game through a discord-<appid> URL, which only resolves once it is
	// registered for the current user.
	bool DiscordIPC::Register(const std::string& applicationId)
	{
		wchar_t executable[MAX_PATH] = {};
		if (!GetModuleFileNameW(nullptr, executable, MAX_PATH))
			return false;

		const std::wstring root = L"Software\\Classes\\discord-" + Widen(applicationId);
		const std::wstring command = L"\"" + std::wstring(executable) + L"\"";

		return WriteRegistry(root, nullptr, L"URL:Run game " + Widen(applicationId))
			&& WriteRegistry(root, L"URL Protocol", L"")
			&& WriteRegistry(root + L"\\DefaultIcon", nullptr, executable)
			&& WriteRegistry(root + L"\\shell\\open\\command", nullptr, command);
	}

	bool DiscordIPC::Open(int index)
	{
		const std::string path = std::format("\\\\?\\pipe\\discord-ipc-{}", index);

		HANDLE pipe = CreateFileA(path.c_str(), GENERIC_READ | GENERIC_WRITE, 0, nullptr, OPEN_EXISTING, 0, nullptr);
		if (pipe == INVALID_HANDLE_VALUE)
			return false;

		DWORD mode = PIPE_READMODE_BYTE;
		SetNamedPipeHandleState(pipe, &mode, nullptr, nullptr);

		Pipe = pipe;
		return true;
	}

	uint32_t DiscordIPC::Available()
	{
		DWORD available = 0;
		if (!PeekNamedPipe(Pipe, nullptr, 0, nullptr, &available, nullptr))
		{
			Disconnect();
			return 0;
		}
		return available;
	}

	// Only called once a peek has said this many bytes are buffered, so it cannot stall the frame.
	bool DiscordIPC::Read(void* data, uint32_t size)
	{
		DWORD read = 0;
		if (!ReadFile(Pipe, data, size, &read, nullptr) || read != size)
		{
			Disconnect();
			return false;
		}
		return true;
	}
}

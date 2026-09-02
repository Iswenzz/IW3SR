#include "Download.hpp"

#include "Engine/Core/Network/HTTP.hpp"
#include "Game/System/Dvar.hpp"
#include "Game/System/Patch.hpp"

namespace IW3SR
{
	// Netchan command the reliable stream expects in front of a download subcommand.
	constexpr int32_t ClcDownload = 5;

	// The download progress menu reads these globals and not the matching fields in cls. Retail's
	// CL_BeginDownload (0x46AB00) is what fills them, and the takeover below is what skips it.
	constexpr uintptr_t DownloadNameAddress = 0x14B8C3C;
	constexpr size_t DownloadNameSize = 64;
	constexpr uintptr_t DownloadSizeAddress = 0x14B8C30;
	constexpr uintptr_t DownloadCountAddress = 0x14B8C34;
	constexpr uintptr_t DownloadTimeAddress = 0x14B8C38;

	// Cursor over one server message. Reads past the end fail instead of running off the buffer.
	struct DownloadReader
	{
		const uint8_t* Data = nullptr;
		int Size = 0;
		int Read = 0;
		bool Failed = false;

		uint8_t Byte();
		uint16_t Short();
		int32_t Long();
		std::string String();
		bool Block(void* out, int size);
		int Remaining() const;
	};

	struct DownloadWriter
	{
		std::vector<uint8_t> Data;

		void Byte(uint8_t value);
		void Long(int32_t value);
		void String(const std::string& value);
	};

	int DownloadReader::Remaining() const
	{
		return Size - Read;
	}

	uint8_t DownloadReader::Byte()
	{
		if (Remaining() < 1)
		{
			Failed = true;
			return 0;
		}
		return Data[Read++];
	}

	uint16_t DownloadReader::Short()
	{
		const uint16_t low = Byte();
		const uint16_t high = Byte();
		return static_cast<uint16_t>(low | (high << 8));
	}

	int32_t DownloadReader::Long()
	{
		const uint32_t low = Short();
		const uint32_t high = Short();
		return static_cast<int32_t>(low | (high << 16));
	}

	std::string DownloadReader::String()
	{
		std::string value;
		for (uint8_t c = Byte(); c && !Failed; c = Byte())
			value.push_back(static_cast<char>(c));
		return value;
	}

	bool DownloadReader::Block(void* out, int size)
	{
		if (size < 0 || Remaining() < size)
		{
			Failed = true;
			return false;
		}
		std::copy_n(Data + Read, size, static_cast<uint8_t*>(out));
		Read += size;
		return true;
	}

	void DownloadWriter::Byte(uint8_t value)
	{
		Data.push_back(value);
	}

	void DownloadWriter::Long(int32_t value)
	{
		const uint32_t bits = static_cast<uint32_t>(value);
		for (int shift = 0; shift < 32; shift += 8)
			Data.push_back(static_cast<uint8_t>((bits >> shift) & 0xFF));
	}

	void DownloadWriter::String(const std::string& value)
	{
		Data.insert(Data.end(), value.begin(), value.end());
		Data.push_back(0);
	}

	// No length here: ReliableMessages::Send prepends it. CoD4X writes a placeholder at this point and
	// backfills it in CL_SendReliableClientCommand instead, which would leave a second one on the wire.
	static DownloadWriter Message(DownloadRequest request)
	{
		DownloadWriter writer;
		writer.Long(ClcDownload);
		writer.Byte(static_cast<uint8_t>(request));
		return writer;
	}

	// Standard CRC-32, chained: several reads must come out the same as one pass over the whole file.
	static uint32_t Crc32(const void* data, size_t length, uint32_t previous)
	{
		static const std::array<uint32_t, 256> table = []
		{
			std::array<uint32_t, 256> result = {};
			for (uint32_t i = 0; i < 256; i++)
			{
				uint32_t crc = i;
				for (int bit = 0; bit < 8; bit++)
					crc = (crc >> 1) ^ ((crc & 1) * 0xEDB88320u);
				result[i] = crc;
			}
			return result;
		}();

		const uint8_t* bytes = static_cast<const uint8_t*>(data);
		uint32_t crc = ~previous;

		for (size_t i = 0; i < length; i++)
			crc = (crc >> 8) ^ table[(crc & 0xFF) ^ bytes[i]];

		return ~crc;
	}

	static void CopyString(char* destination, size_t size, const std::string& value)
	{
		if (!destination || size < 1)
			return;

		const size_t length = std::min(value.size(), size - 1);
		std::copy_n(value.data(), length, destination);
		destination[length] = '\0';
	}

	void GDownload::Initialize()
	{
		Enabled = Dvar::RegisterBool("sr_download", DVAR_SAVED,
			"Use the segmented resumable download protocol when the server offers it", true);
		UseCache = Dvar::RegisterBool("sr_download_cache", DVAR_SAVED,
			"Keep an interrupted download and reuse the segments of it that still verify", true);
	}

	void GDownload::Shutdown()
	{
		Reset();
		DL_BeginDownload_h.Remove();
	}

	void GDownload::Frame()
	{
		// Queued rather than run here. Frame is driven from GSystem::MainLoop, which hangs off the
		// PunkBuster call in WinMain's loop - past the point where Com_Frame returned, so the jmp_buf
		// the engine's Com_Error longjmps to belongs to a frame that is already gone. Cbuf_AddText
		// hands the command to the engine to run inside its own frame, where that target is live.
		if (Dropped)
		{
			Dropped = false;
			Cbuf_AddText(0, "disconnect\n");
			return;
		}

		// The transfer thread only publishes bytes and a verdict; every decision stays on this thread.
		if (State == DownloadState::Web)
		{
			if (!Web)
			{
				Abort("The web download was dropped from under the client.");
				return;
			}
			Count = static_cast<int>(std::min<int64_t>(Web->Count.load(), Size));
			Mirror();

			if (Web->Result.load() != 0)
				FinishWeb();
			return;
		}

		if (State == DownloadState::Idle || !Transport.Receive)
			return;

		if (Packet.size() != static_cast<size_t>(DownloadPacketMax))
			Packet.resize(DownloadPacketMax);

		// Bounded so a transport that never runs dry cannot hold the frame.
		for (int i = 0; i < DownloadMessagesPerFrame && State != DownloadState::Idle; i++)
		{
			const int size = Transport.Receive(Packet.data(), static_cast<int>(Packet.size()));
			if (size <= 0)
				break;

			Parse(Packet.data(), size);
		}
	}

	// A reconnect tears the channel down under whatever was in flight. Without this the transfer keeps
	// running against the old connection and its answers land on the new one's channel.
	void GDownload::Disconnected()
	{
		Reset();
		Dropped = false;
	}

	void GDownload::SetTransport(const DownloadTransport& transport)
	{
		Transport = transport;
	}

	static std::string NormalisePath(std::string_view path)
	{
		std::string name;
		name.reserve(path.size());

		for (char c : path)
		{
			if (c >= 'A' && c <= 'Z')
				c = static_cast<char>(c + ('a' - 'A'));
			else if (c == '\\')
				c = '/';

			name.push_back(c);
		}
		return name;
	}

	// Bytes per second for the download menu, which otherwise divides by the elapsed time truncated to
	// whole seconds. Zero rather than a guess while there is nothing to divide: the caller reads it as
	// "no rate yet" and leaves the estimated time left off the screen, which is what it is for.
	int GDownload::Rate(int count, int elapsed)
	{
		if (count < 1 || elapsed < 1)
			return 0;

		const int64_t rate = static_cast<int64_t>(count) * 1000 / elapsed;

		return rate > INT32_MAX ? INT32_MAX : static_cast<int>(rate);
	}

	// A server that can write into the update directories can replace the client binary.
	bool GDownload::IsPathAllowed(std::string_view path)
	{
		if (path.empty())
			return false;

		const std::string name = NormalisePath(path);

		// Both names legitimately contain ':' and a leading separator - one is an absolute OS path,
		// the other a URL - so only traversal is worth refusing here.
		return !name.contains("updates") && !name.contains("cod4update") && !name.contains("..");
	}

	// The svc_download path, which the redirect hook above never sees. Retail's CL_BeginDownload is
	// the only caller and it discards the return value, so refusing is simply not running the original.
	//
	// It is also where the extended protocol has to part company. An extended server ignores the
	// "download" command retail would put on the netchan and waits for a clc_download on the reliable
	// channel instead, so the request has to start here or nothing ever asks for the file.
	int GDownload::AllowBegin(const char* localName, const char* remoteName)
	{
		if (!localName || !remoteName)
			return 0;

		if (!IsPathAllowed(localName) || !IsPathAllowed(remoteName))
		{
			Com_PrintMessage(CON_CHANNEL_ERROR,
				std::format("^1Refused '{}': a server may not write into the update directories.\n", localName).c_str(),
				0);
			return 0;
		}

		if (!Takeover())
			return 1;

		Restarts = 0;
		Start(localName, remoteName);
		return 0;
	}

	int GDownload::BeginDownload(const char* localName, const char* remoteName)
	{
		if (!localName || !remoteName)
			return 0;

		Restarts = 0;

		if (!Accept(localName, remoteName))
			return 0;
		if (Takeover())
			return Start(localName, remoteName) ? 1 : 0;

		return DL_BeginDownload_h(localName, remoteName);
	}

	void GDownload::Parse(const uint8_t* data, int size)
	{
		if (!data || size < 1)
			return;

		switch (static_cast<DownloadCommand>(data[0]))
		{
		case DownloadCommand::FileInit:
			ParseFileInit(data + 1, size - 1);
			break;

		case DownloadCommand::Segment:
			ParseSegment(data + 1, size - 1);
			break;

		case DownloadCommand::WebDownload:
			ParseWeb(data + 1, size - 1);
			break;

		case DownloadCommand::Failed:
		{
			DownloadReader reader{ data + 1, size - 1 };
			Abort(reader.String());
			break;
		}

		default:
			Abort(std::format("Unknown download subcommand {}.", data[0]));
			break;
		}
	}

	bool GDownload::Command(const std::string& command)
	{
		if (command == "sr_download_status")
		{
			Status();
			return true;
		}
		if (command == "sr_download_cancel")
		{
			Cancel();
			return true;
		}
		return false;
	}

	// Handing the answer back and going idle is the whole refusal: WebFailed and WebChecksumFailed both
	// make the server clear its web flags and resend the gamestate, and the round after that serves the
	// file over this channel instead. So a refusal costs a restart, which is why it is the last resort
	// rather than the way the redirect is normally handled.
	void GDownload::RefuseWeb(DownloadRequest answer, const std::string& reason)
	{
		Com_PrintMessage(CON_CHANNEL_CLIENT, std::format("^3{} Asking the server to send it instead.\n", reason).c_str(),
			0);

		if (!Transmit(Message(answer).Data))
		{
			Abort("The download transport refused the web download answer.");
			return;
		}
		Reset();
	}

	// The redirect carries the URL, the size the server has, and its sv_wwwDlDisconnected setting.
	void GDownload::ParseWeb(const uint8_t* data, int size)
	{
		if (State == DownloadState::Idle)
		{
			Abort("The server redirected a download that was never started.");
			return;
		}

		DownloadReader reader{ data, size };
		const std::string url = reader.String();
		const int32_t length = reader.Long();
		const int32_t flags = reader.Long();

		if (reader.Failed || url.empty())
		{
			Abort("Truncated web download redirect.");
			return;
		}
		// Bit 2 asks the client to hand the URL to the shell instead of fetching it. That is the stock
		// autoupdate trick wearing a different hat, so it is refused the same way the paths are.
		if (flags & 2)
		{
			Abort(std::format("The server tried to make the client open '{}'.", url));
			return;
		}
		if (!url.starts_with("http://") && !url.starts_with("https://"))
		{
			RefuseWeb(DownloadRequest::WebFailed, std::format("'{}' is not an HTTP redirect.", url));
			return;
		}
		// FileInit normally arrives first and settles this; the redirect's own figure is the fallback.
		if (Size < 1)
			Size = length;

		StartWeb(url);
	}

	void GDownload::StartWeb(const std::string& url)
	{
		// The transfer owns the temp file from here, so the segment writer has to let go of it first.
		if (Output.is_open())
			Output.close();

		const auto transfer = std::make_shared<WebTransfer>();

		// Both buffers exist to keep the per chunk costs off a file this size: without them curl hands
		// over 16K at a time and every one of those becomes its own write.
		transfer->Buffer.resize(DownloadWebBufferSize);
		transfer->File.rdbuf()->pubsetbuf(transfer->Buffer.data(), static_cast<std::streamsize>(transfer->Buffer.size()));
		transfer->File.open(Resolve(TempName), std::ios::binary | std::ios::trunc);

		if (!transfer->File)
		{
			RefuseWeb(DownloadRequest::WebFailed, std::format("Could not create {}.", TempName));
			return;
		}
		Web = transfer;
		WebStart = std::chrono::steady_clock::now();

		// The menu divides the byte count by the time since this stamp, so it has to be the moment the
		// bytes start rather than the one Start set, back when the request went out. Retail never has
		// to restamp it: its DL_BeginDownload follows CL_BeginDownload directly, while everything the
		// extended protocol does in between would otherwise be charged to the transfer.
		if (cls)
			Memory::Set<int>(DownloadTimeAddress, cls->realtime);

		HTTPRequest request = HTTP::Get(url, nullptr);

		// No deadline: the only honest bound on a file this size is that it keeps arriving.
		request.TimeoutSeconds = 0;
		request.LowSpeedLimitBytes = 512;
		request.LowSpeedTimeSeconds = 30;
		request.BufferSizeBytes = DownloadWebBufferSize;

		request.OnData = [transfer](const char* data, size_t size)
		{
			if (transfer->Cancel.load())
				return false;

			transfer->File.write(data, static_cast<std::streamsize>(size));
			if (!transfer->File)
				return false;

			transfer->Count.fetch_add(static_cast<int64_t>(size));
			return true;
		};
		// Error is written before Result and read after it, which is the whole handshake with Frame.
		request.Callback = [transfer](const HTTPResponse& response)
		{
			transfer->File.close();

			if (transfer->Cancel.load())
				transfer->Error = "it was cancelled";
			else if (!response.Success)
				transfer->Error = response.Error;
			else if (response.Code != 200 && response.Code != 206)
				transfer->Error = std::format("the server answered HTTP {}", response.Code);

			transfer->Result = transfer->Error.empty() ? 1 : -1;
		};
		request.Send();

		// Start put the remote name in the menu's buffer; while the bytes are coming from a redirect
		// host, the address they are coming from is the thing worth showing. CoD4X shows it by moving
		// the URL into cls.downloadName, which is the same field one indirection further along.
		CopyString(reinterpret_cast<char*>(DownloadNameAddress), DownloadNameSize, url);

		State = DownloadState::Web;
		Com_PrintMessage(CON_CHANNEL_CLIENT, std::format("Downloading {} from {}.\n", LocalName, url).c_str(), 0);
	}

	// Reached once the transfer thread has stopped, so the temp file is closed and complete.
	void GDownload::FinishWeb()
	{
		const std::string localName = LocalName;
		const std::string reason = Web->Error;

		if (Web->Result.load() < 0)
		{
			RefuseWeb(DownloadRequest::WebFailed, std::format("The HTTP download of {} failed: {}.", localName,
				reason.empty() ? "unknown error" : reason));
			return;
		}
		// A redirect host serving a file that no longer matches the server's is the case this catches,
		// Stopped here rather than at the end: everything below reads the file back off the disk, and
		// counting that as transfer time is what made the first rate this printed too low to believe.
		const auto arrived = std::chrono::steady_clock::now();

		// and the answer for it is its own subcommand so the server can warn its owner about the two.
		if (Checksums.length > 0 && !VerifyFinal())
		{
			RefuseWeb(DownloadRequest::WebChecksumFailed,
				std::format("The HTTP copy of {} has the wrong checksum.", localName));
			return;
		}

		std::error_code error;
		std::filesystem::remove(Resolve(localName), error);
		std::filesystem::rename(Resolve(TempName), Resolve(localName), error);

		if (error)
		{
			RefuseWeb(DownloadRequest::WebFailed,
				std::format("Could not move {} into place: {}.", localName, error.message()));
			return;
		}
		Transmit(Message(DownloadRequest::WebDone).Data);

		// The two halves are reported apart because they are bound by different things: the transfer by
		// the redirect host, the checksum by how fast the file reads back.
		const auto done = std::chrono::steady_clock::now();
		const double seconds = std::max(std::chrono::duration<double>(arrived - WebStart).count(), 0.001);
		const double checked = std::chrono::duration<double>(done - arrived).count();

		Com_PrintMessage(CON_CHANNEL_CLIENT,
			std::format("Download of {} finished, {} bytes in {:.2f}s ({:.1f} MB/s), checksum {:.2f}s.\n", localName,
				Size, seconds, Size / seconds / (1024.0 * 1024.0), checked)
				.c_str(),
			0);

		Reset();
		NextDownload();
	}

	void GDownload::Cancel()
	{
		if (State == DownloadState::Idle)
			return;

		Transmit(Message(DownloadRequest::Stop).Data);
		Reset();
	}

	bool GDownload::Active()
	{
		return State != DownloadState::Idle;
	}

	int GDownload::Received()
	{
		return Count;
	}

	int GDownload::Total()
	{
		return Size;
	}

	// A refused name drops the connection rather than skipping the file: a server that tried it is
	// not one to keep talking to.
	bool GDownload::Accept(const std::string& localName, const std::string& remoteName)
	{
		if (IsPathAllowed(localName) && IsPathAllowed(remoteName))
			return true;

		Com_PrintMessage(CON_CHANNEL_ERROR,
			std::format("^1The server tried to push '{}' as an autoupdate file. Refusing.\n", localName).c_str(), 0);

		Reset();

		if (cls)
			cls->downloadList[0] = '\0';

		Dropped = true;
		return false;
	}

	bool GDownload::Takeover()
	{
		if (Patch::UseCoD4X)
			return false;
		if (!Enabled || !Enabled->current.enabled)
			return false;
		if (!Transport.Send)
			return false;

		return Transport.IsSegmented && Transport.IsSegmented();
	}

	bool GDownload::Start(const std::string& localName, const std::string& remoteName)
	{
		Reset();

		LocalName = localName;
		RemoteName = remoteName;
		TempName = localName + ".tmp";

		if (cls)
		{
			CopyString(cls->downloadName, sizeof(cls->downloadName), LocalName);
			CopyString(cls->downloadTempName, sizeof(cls->downloadTempName), TempName);
			cls->downloadBlock = 0;

			CopyString(reinterpret_cast<char*>(DownloadNameAddress), DownloadNameSize, RemoteName);
			Memory::Set<int>(DownloadTimeAddress, cls->realtime);
		}
		Mirror();

		DownloadWriter writer = Message(DownloadRequest::Begin);
		writer.String(RemoteName);

		if (!Transmit(writer.Data))
		{
			Abort("The download transport refused the begin request.");
			return false;
		}
		State = DownloadState::Waiting;
		return true;
	}

	// The cache is removed first, so a restart never reuses the bytes that failed to verify.
	void GDownload::Restart()
	{
		if (TempName.empty())
			return;

		const std::string localName = LocalName;
		const std::string remoteName = RemoteName;
		const std::filesystem::path temp = Resolve(TempName);
		const std::filesystem::path cache = CachePath();

		if (Output.is_open())
			Output.close();

		std::error_code error;
		std::filesystem::remove(cache, error);
		std::filesystem::remove(temp, error);

		if (++Restarts > DownloadRestartMax)
		{
			Abort(std::format("Download of {} stayed corrupted after {} attempts.", localName, DownloadRestartMax));
			return;
		}
		Com_PrintMessage(CON_CHANNEL_CLIENT, std::format("Restarting the download of {}.\n", localName).c_str(), 0);
		Start(localName, remoteName);
	}

	// Walks the @remote@local@remote@local string the server sent, consumed in place.
	void GDownload::NextDownload()
	{
		if (!cls || !cls->downloadList[0])
		{
			Complete();
			return;
		}

		std::string_view list = cls->downloadList;
		if (list.starts_with('@'))
			list.remove_prefix(1);

		const size_t remoteEnd = list.find('@');
		if (remoteEnd == std::string_view::npos)
		{
			Complete();
			return;
		}
		const std::string remoteName(list.substr(0, remoteEnd));
		list.remove_prefix(remoteEnd + 1);

		const size_t localEnd = list.find('@');
		const std::string localName(list.substr(0, localEnd == std::string_view::npos ? list.size() : localEnd));
		const std::string remainder(
			localEnd == std::string_view::npos ? std::string_view() : list.substr(localEnd + 1));

		CopyString(cls->downloadList, sizeof(cls->downloadList), remainder);
		cls->downloadRestart = 1;

		BeginDownload(localName.c_str(), remoteName.c_str());
	}

	void GDownload::Complete()
	{
		Reset();

		if (Transport.Complete)
			Transport.Complete();
		else
			Log::WriteLine(Channel::Warning, "Downloads finished with no way to hand the client back to the engine.");
	}

	void GDownload::Finish()
	{
		if (Output.is_open())
			Output.close();

		const std::filesystem::path temp = Resolve(TempName);
		const std::filesystem::path target = Resolve(LocalName);

		std::error_code error;
		std::filesystem::remove(CachePath(), error);

		if (!VerifyFinal())
		{
			Com_PrintMessage(CON_CHANNEL_CLIENT,
				std::format("Downloaded {} has a checksum error.\n", LocalName).c_str(), 0);
			Restart();
			return;
		}

		std::filesystem::remove(target, error);
		std::filesystem::rename(temp, target, error);

		if (error)
		{
			Abort(std::format("Could not move {} into place: {}", LocalName, error.message()));
			return;
		}
		Com_PrintMessage(CON_CHANNEL_CLIENT, std::format("Download of {} finished.\n", LocalName).c_str(), 0);

		Reset();
		NextDownload();
	}

	// The engine drops the connection on a broken download and so do we, but the disconnect waits
	// for the next frame rather than unwinding the message parser.
	void GDownload::Abort(const std::string& reason)
	{
		Com_PrintMessage(CON_CHANNEL_ERROR, std::format("^1{}\n", reason).c_str(), 0);

		if (State != DownloadState::Idle)
			Transmit(Message(DownloadRequest::Stop).Data);

		Reset();
		Dropped = true;
	}

	void GDownload::Reset()
	{
		// Letting go is the cancel: the thread still holds the object it is writing into, notices the
		// flag at its next chunk, and reports to nobody. Nothing here waits on it.
		if (Web)
		{
			Web->Cancel = true;
			Web.reset();
		}

		if (Output.is_open())
			Output.close();
		Output.clear();

		State = DownloadState::Idle;
		Checksums = {};
		LocalName.clear();
		RemoteName.clear();
		TempName.clear();
		Size = 0;
		Count = 0;
		CacheEof = false;

		if (cls)
		{
			cls->downloadName[0] = '\0';
			cls->downloadTempName[0] = '\0';
			cls->downloadCount = 0;
			cls->downloadSize = 0;
		}

		Memory::Set<char>(DownloadNameAddress, '\0');
		Memory::Set<int>(DownloadSizeAddress, 0);
		Memory::Set<int>(DownloadCountAddress, 0);
	}

	void GDownload::Status()
	{
		if (State == DownloadState::Idle)
		{
			Com_PrintMessage(CON_CHANNEL_CONSOLEONLY, "No download in progress.\n", 0);
			return;
		}
		if (State == DownloadState::Waiting)
		{
			Com_PrintMessage(CON_CHANNEL_CONSOLEONLY,
				std::format("Waiting for the server to describe {}.\n", LocalName).c_str(), 0);
			return;
		}
		Com_PrintMessage(CON_CHANNEL_CONSOLEONLY,
			std::format("{}: {} of {} bytes, checksum {:08x}, cache {}.\n", LocalName, Count, Size,
				static_cast<uint32_t>(Checksums.sum), CacheEof ? "spent" : "in use")
				.c_str(),
			0);
	}

	void GDownload::ParseFileInit(const uint8_t* data, int size)
	{
		if (State != DownloadState::Waiting)
		{
			Abort("The server began a download while one was already in progress.");
			return;
		}

		DownloadReader reader{ data, size };
		const int32_t length = reader.Long();
		reader.Long(); // the server writes a second long here that nothing reads

		constexpr int block = static_cast<int>(sizeof(DownloadChecksums));

		if (reader.Remaining() != block || !reader.Block(&Checksums, block))
		{
			Abort("Invalid checksum block received from the server.");
			return;
		}
		Checksums.qpath[sizeof(Checksums.qpath) - 1] = '\0';
		const std::string qpath = Checksums.qpath;

		if (length != Checksums.length)
		{
			Abort(std::format("The server disagrees with itself about the size of {}.", qpath));
			return;
		}
		if (LocalName != qpath)
		{
			Abort(std::format("Requested {} but the server offered {}.", LocalName, qpath));
			return;
		}
		if (!IsPathAllowed(qpath))
		{
			Abort(std::format("The server tried to push '{}' as an autoupdate file.", qpath));
			return;
		}
		// One CRC per segment and no more: a longer file would run the per segment lookup off the end.
		if (Checksums.length < 1 || Checksums.length > DownloadSizeMax)
		{
			Abort(std::format("The server sent an invalid download size of {}.", Checksums.length));
			return;
		}

		const std::filesystem::path temp = Resolve(TempName);
		const std::filesystem::path cache = CachePath();

		std::error_code error;
		std::filesystem::remove(cache, error);

		// Whatever an interrupted attempt left behind becomes the cache the resume reads from.
		if (Caching())
			std::filesystem::rename(temp, cache, error);
		else
			std::filesystem::remove(temp, error);

		Output.open(temp, std::ios::binary | std::ios::trunc);
		if (!Output)
		{
			Abort(std::format("Could not create {}.", TempName));
			return;
		}

		Size = Checksums.length;
		Count = 0;
		CacheEof = !Caching();
		State = DownloadState::Running;

		// Same reason as the web path: the rate on screen starts counting from here, not from the
		// request Start sent to get this reply.
		if (cls)
			Memory::Set<int>(DownloadTimeAddress, cls->realtime);

		Mirror();

		Com_PrintMessage(CON_CHANNEL_CLIENT,
			std::format("{} is {} bytes long, checksum {:08x}.\n", qpath, Checksums.length,
				static_cast<uint32_t>(Checksums.sum))
				.c_str(),
			0);

		Advance();
	}

	void GDownload::ParseSegment(const uint8_t* data, int size)
	{
		if (State != DownloadState::Running)
		{
			Abort("The server sent a download segment with no download in progress.");
			return;
		}

		DownloadReader reader{ data, size };
		const int32_t offset = reader.Long();
		const int blockSize = reader.Short();

		if (reader.Failed)
		{
			Abort("Truncated download segment header.");
			return;
		}
		if (offset != Count)
		{
			Abort(std::format("Broken download, expected offset {} and got {}.", Count, offset));
			return;
		}

		// A zero length block ends the range that was asked for, not the file.
		if (blockSize < 1)
		{
			Advance();
			return;
		}

		if (Count + blockSize > Size)
		{
			Abort("The server sent a download segment past the end of the file.");
			return;
		}
		if (reader.Remaining() < blockSize)
		{
			Abort("Truncated download segment.");
			return;
		}

		Output.write(reinterpret_cast<const char*>(data + reader.Read), blockSize);
		if (!Output)
		{
			Abort("Could not write the whole download segment.");
			return;
		}
		Count += blockSize;
		Mirror();
	}

	void GDownload::Advance()
	{
		switch (RequestAndRead())
		{
		case 1:
			Finish();
			break;

		case -1:
			Restart();
			break;

		default:
			break;
		}
	}

	// 1 when the file is complete, 0 when a request went out and the answer is still to come, -1
	// when the cache claimed to have filled the file but did not.
	int GDownload::RequestAndRead()
	{
		if (!Output.is_open())
		{
			Abort("The download file is not open.");
			return 0;
		}

		const bool done = NextSegment();
		if (Count != Size)
			return 0;

		return done ? 1 : -1;
	}

	bool GDownload::NextSegment()
	{
		if (Count >= Size)
			return true;

		if (Buffer.size() != static_cast<size_t>(DownloadSegmentSize))
			Buffer.resize(DownloadSegmentSize);

		while (!CacheEof)
		{
			// The tail never fills a whole segment, so it has no CRC and always comes from the server.
			if (Count + DownloadSegmentSize > Size)
			{
				CacheEof = true;
				break;
			}

			const int segment = SegmentFromCache();
			if (segment < 0)
			{
				CacheEof = true;
				break;
			}
			if (segment == 0)
			{
				RequestSegments(Count, std::min(DownloadSegmentSize, Size - Count));
				return false;
			}

			Output.write(reinterpret_cast<const char*>(Buffer.data()), DownloadSegmentSize);
			if (!Output)
			{
				Abort("Could not write a cached download segment.");
				return false;
			}
			Count += DownloadSegmentSize;
			Mirror();
		}

		const int remaining = Size - Count;
		if (remaining < 1)
			return true;

		RequestSegments(Count, remaining);
		return false;
	}

	// 1 when the cache holds this segment and its CRC matches, 0 when it holds it and does not,
	// -1 when the cache does not reach this far.
	int GDownload::SegmentFromCache()
	{
		const int index = Count / DownloadSegmentSize;
		if (index >= DownloadSegmentCount)
			return -1;

		std::ifstream cache(CachePath(), std::ios::binary | std::ios::ate);
		if (!cache)
			return -1;

		const std::streamoff length = cache.tellg();
		if (length < static_cast<std::streamoff>(Count) + DownloadSegmentSize)
			return -1;

		cache.seekg(Count, std::ios::beg);
		cache.read(reinterpret_cast<char*>(Buffer.data()), DownloadSegmentSize);

		if (cache.gcount() != DownloadSegmentSize)
			return 0;

		return Crc32(Buffer.data(), DownloadSegmentSize, 0) == static_cast<uint32_t>(Checksums.sums[index]) ? 1 : 0;
	}

	void GDownload::RequestSegments(int offset, int size)
	{
		DownloadWriter writer = Message(DownloadRequest::Segments);
		writer.Long(offset);
		writer.Long(size);

		if (!Transmit(writer.Data))
			Abort("The download transport refused a segment request.");
	}

	bool GDownload::VerifyFinal()
	{
		std::ifstream file(Resolve(TempName), std::ios::binary);
		if (!file)
			return false;

		std::vector<char> buffer(DownloadSegmentSize);
		uint32_t crc = 0;

		while (file.read(buffer.data(), static_cast<std::streamsize>(buffer.size())) || file.gcount() > 0)
		{
			crc = Crc32(buffer.data(), static_cast<size_t>(file.gcount()), crc);
			if (file.gcount() != static_cast<std::streamsize>(buffer.size()))
				break;
		}
		return crc == static_cast<uint32_t>(Checksums.sum);
	}

	bool GDownload::Transmit(const std::vector<uint8_t>& message)
	{
		if (!Transport.Send)
			return false;

		return Transport.Send(message.data(), static_cast<int>(message.size()));
	}

	bool GDownload::Caching()
	{
		return UseCache && UseCache->current.enabled;
	}

	// The engine's own download progress menu reads these.
	void GDownload::Mirror()
	{
		if (!cls)
			return;

		cls->downloadCount = Count;
		cls->downloadSize = Size;

		Memory::Set<int>(DownloadSizeAddress, Size);
		Memory::Set<int>(DownloadCountAddress, Count);
	}

	// Download names are relative to the write path, where the engine's own downloads land.
	std::filesystem::path GDownload::Resolve(const std::string& name)
	{
		std::filesystem::path root;

		if (const auto homepath = Dvar::Find("fs_homepath"); homepath && homepath->current.string)
			root = homepath->current.string;
		if (root.empty())
		{
			if (const auto basepath = Dvar::Find("fs_basepath"); basepath && basepath->current.string)
				root = basepath->current.string;
		}
		if (root.empty())
			root = Environment::Path(Directory::Base);

		const std::filesystem::path path = root / name;

		std::error_code error;
		std::filesystem::create_directories(path.parent_path(), error);

		return path;
	}

	std::filesystem::path GDownload::CachePath()
	{
		return Resolve(TempName + ".cache");
	}
}

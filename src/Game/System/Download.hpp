#pragma once
#include "Game/Base.hpp"

namespace IW3SR
{
	// One CRC per segment, so a checksum block cannot describe anything larger than these multiplied.
	constexpr int DownloadSegmentSize = 2 * 1024 * 1024;
	constexpr int DownloadSegmentCount = 256;
	constexpr int64_t DownloadSizeMax = static_cast<int64_t>(DownloadSegmentSize) * DownloadSegmentCount;

	// Past this the file itself is the problem, not the resume.
	constexpr int DownloadRestartMax = 2;

	constexpr int DownloadPacketMax = 0x10000;
	constexpr int DownloadMessagesPerFrame = 64;

	// Subcommand byte the client writes after the clc_download header.
	enum class DownloadRequest : uint8_t
	{
		Begin,
		Done,
		Stop,
		Segments,
		WebFailed,
		WebDone,
		WebLater,
		WebChecksumFailed
	};

	// Subcommand byte the server writes at the front of an svc_download payload.
	enum class DownloadCommand : uint8_t
	{
		Segment,
		FileInit,
		WebDownload,
		Failed
	};

	enum class DownloadState
	{
		Idle,
		Waiting,
		Running
	};

	// Copied straight off the wire, so the layout has to stay byte for byte what the server writes.
	struct DownloadChecksums
	{
		char qpath[64];
		int32_t length;
		int32_t sums[DownloadSegmentCount];
		int32_t sum;
	};
	static_assert(sizeof(DownloadChecksums) == 1096, "The checksum block is a wire layout.");

	// The seam with the network transport, which owns the socket and the reliable command stream.
	struct DownloadTransport
	{
		std::function<bool(const uint8_t* data, int size)> Send;
		std::function<int(uint8_t* data, int size)> Receive;
		std::function<bool()> IsSegmented;
		std::function<void()> Complete;
	};

	// Segmented, resumable, CRC verified file transfer.
	class GDownload
	{
	public:
		static void Initialize();
		static void Shutdown();
		static void Frame();

		static void SetTransport(const DownloadTransport& transport);

		static bool IsPathAllowed(std::string_view path);
		static int BeginDownload(const char* localName, const char* remoteName);
		static int AllowBegin(const char* localName, const char* remoteName);

		static void Parse(const uint8_t* data, int size);
		static bool Command(const std::string& command);
		static void Cancel();

		static bool Active();
		static int Received();
		static int Total();

	private:
		static bool Accept(const std::string& localName, const std::string& remoteName);
		static bool Takeover();
		static bool Start(const std::string& localName, const std::string& remoteName);
		static void Restart();
		static void NextDownload();
		static void Complete();
		static void Finish();
		static void Abort(const std::string& reason);
		static void Reset();
		static void Status();

		static void ParseFileInit(const uint8_t* data, int size);
		static void ParseSegment(const uint8_t* data, int size);

		static void Advance();
		static int RequestAndRead();
		static bool NextSegment();
		static int SegmentFromCache();
		static void RequestSegments(int offset, int size);
		static bool VerifyFinal();
		static bool Transmit(const std::vector<uint8_t>& message);

		static bool Caching();
		static void Mirror();
		static std::filesystem::path Resolve(const std::string& name);
		static std::filesystem::path CachePath();

		static inline dvar_s* Enabled = nullptr;
		static inline dvar_s* UseCache = nullptr;

		static inline DownloadTransport Transport;
		static inline DownloadState State = DownloadState::Idle;
		static inline DownloadChecksums Checksums = {};

		static inline std::string LocalName;
		static inline std::string RemoteName;
		static inline std::string TempName;

		static inline std::ofstream Output;
		static inline std::vector<uint8_t> Buffer;
		static inline std::vector<uint8_t> Packet;

		static inline int Size = 0;
		static inline int Count = 0;
		static inline int Restarts = 0;
		static inline bool CacheEof = false;
		static inline bool Dropped = false;
	};
}

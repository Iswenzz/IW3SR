#pragma once
#include "Game/Base.hpp"

namespace IW3SR
{
	// Wire format: both ends have to agree on the fragment size or a peer rejects the size field.
	constexpr int MaxReliablePacketSize = 1400;
	constexpr int MaxFragmentSize = MaxReliablePacketSize - 200;

	// First long of every reliable datagram. The netchan reads the same field as a sequence number.
	constexpr int32_t ReliableMarker = static_cast<int32_t>(0xFFFFFFF0u);

	// Little endian cursor over an outgoing datagram.
	struct NetWriter
	{
		uint8_t* Data = nullptr;
		int MaxSize = 0;
		int CurSize = 0;
		bool Overflowed = false;

		NetWriter(uint8_t* data, int size);

		void WriteByte(int value);
		void WriteShort(int value);
		void WriteLong(int32_t value);
		void WriteData(const void* data, int length);
	};

	// Little endian cursor over a received datagram. A read past the end sets Overflowed and yields -1,
	// so a caller can check once after a group of reads.
	struct NetReader
	{
		const uint8_t* Data = nullptr;
		int CurSize = 0;
		int ReadCount = 0;
		bool Overflowed = false;

		NetReader(const uint8_t* data, int size, int offset = 0);

		int ReadByte();
		int ReadShort();
		int ReadLong();
		bool ReadData(void* out, int length);
	};

	// One numbered slice of the stream. Ack holds the sequence the peer confirmed for this slot, or
	// -1 while in flight, so a reused slot never reads as acknowledged.
	struct Fragment
	{
		std::array<uint8_t, MaxFragmentSize> Data = {};
		int Length = 0;
		int Ack = -1;
		int PacketNum = 0;
		int SentTime = 0;
	};

	// Bytes is payload only; BytesTotal also carries headers and every retransmission.
	struct RateTracker
	{
		int NextSampleTime = 0;
		int Bytes = 0;
		int LastBytes = 0;
		int BytesPerSecond = 0;
		int BytesTotal = 0;
		int LastBytesTotal = 0;
		int BytesPerSecondTotal = 0;
	};

	// One direction of the sliding window. Sequence is the next fragment number to write on the send
	// side, and the window base still owed to the reader on the receive side.
	struct FrameWindow
	{
		int Sequence = 0;
		int Acknowledge = 0;
		int SelAckOffset = 0;
		int Frame = 0;
		int WindowSize = 0;
		int Packets = 0;
		int UnsentMilliPackets = 0;
		int PeerWindowSize = 0;

		std::vector<Fragment> Fragments;
		RateTracker Rate;

		// Tail of a fragment that did not fit the caller's buffer on the previous Receive.
		std::array<uint8_t, MaxFragmentSize> Pending = {};
		int PendingSize = 0;
		int PendingRead = 0;
	};

	// Shaped like NET_SendPacket, except the address comes by pointer.
	using PacketSender = std::function<void(netsrc_t sock, int length, const void* data, const netadr_t* to)>;

	// A TCP-like reliable byte stream over the game's own UDP socket
	// Numbered fragments, a cumulative acknowledge plus up to three selective ranges per packet.
	class API ReliableTransport
	{
	public:
		static void SetSender(PacketSender sender);
		static bool Available();

		bool Setup(netsrc_t sock, int qport, const netadr_t& remote);
		void Disconnect();
		bool IsActive() const;

		void SetSendWindow(int fragments);
		int SendWindow() const;

		int Send(const uint8_t* data, int length);
		int Receive(uint8_t* out, int length);
		int ReceiveSingleFragment(uint8_t* out, int length);
		bool HasData() const;

		void ReceivePacket(NetReader& reader);
		void ReceivePacket(const uint8_t* data, int length, int offset);

		void Frame(int time);

		int UsedFragmentCount() const;
		int UsedSendBufferSize() const;
		int PeerWindow() const;
		const RateTracker& SendRate() const;
		const RateTracker& ReceiveRate() const;

	private:
		int ChangeSendBufferSize(int fragmentCount);
		void TransmitNextFragment();
		void TrackRate();

		static int ContiguousAcknowledge(const FrameWindow& window);
		static void WriteSelectiveAckList(FrameWindow& window, NetWriter& writer);

		FrameWindow Tx;
		FrameWindow Rx;

		netadr_t Remote = {};
		netsrc_t Sock = NS_CLIENT1;
		int QPort = 0;
		int Time = 0;
		int NextAckTime = 0;
		bool Active = false;

		static inline PacketSender Sender = nullptr;
		static inline bool WarnedUnbound = false;
		static inline bool WarnedCoD4X = false;
	};

	// A peer claiming more than this is broken or hostile; the channel drops rather than allocate.
	constexpr int MaxReliableMessageSize = 1024 * 1024;

	// Ceiling on what may sit staged waiting for room in the window before Send starts refusing.
	constexpr size_t MaxReliableStageSize = 8 * 1024 * 1024;

	// Length prefixed messages over the byte stream: a long holding the size, then the body. What the
	// window has no room for stays staged, so a message is never truncated onto the wire.
	class API ReliableMessages
	{
	public:
		bool Setup(netsrc_t sock, int qport, const netadr_t& remote);
		void Disconnect();
		bool IsActive() const;

		bool Send(const uint8_t* body, int length);
		bool Receive(std::vector<uint8_t>& out);
		void Frame(int time);

		ReliableTransport& Transport();
		const ReliableTransport& Transport() const;

	private:
		void Flush();

		ReliableTransport Stream;
		std::vector<uint8_t> Outgoing;
		std::vector<uint8_t> Incoming;
	};
}

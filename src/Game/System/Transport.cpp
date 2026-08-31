#include "Transport.hpp"
#include "Patch.hpp"

#include <cstring>

namespace IW3SR
{
	// The peer refuses more than three selective ranges in a packet, so the writer stops there.
	constexpr int MaxSelectiveAcks = 3;

	// A fragment is about 1.2 KB, so the ceiling bounds the send buffer at roughly 2.5 MB.
	constexpr int DefaultFragmentBuffer = 32;
	constexpr int MaxFragmentBuffer = 2048;

	// Must stay under the buffer size, or an in-window fragment could land on a slot the window still
	// owes the reader.
	constexpr int SendWindowSize = 4;
	constexpr int ReceiveWindowSize = 20;

	constexpr int AckIntervalMs = 350;

	// A frame this far behind is a hitch, not real elapsed time; sending for all of it would burst.
	constexpr int MaxFrameElapsedMs = 250;

	constexpr int RateSampleIntervalMs = 1000;
	constexpr int ForceAckFlag = 1;

	NetWriter::NetWriter(uint8_t* data, int size) : Data(data), MaxSize(size > 0 ? size : 0) { }

	void NetWriter::WriteByte(int value)
	{
		if (MaxSize - CurSize < 1)
		{
			Overflowed = true;
			return;
		}
		Data[CurSize] = static_cast<uint8_t>(value);
		CurSize += 1;
	}

	void NetWriter::WriteShort(int value)
	{
		if (MaxSize - CurSize < 2)
		{
			Overflowed = true;
			return;
		}
		const int16_t narrowed = static_cast<int16_t>(value);
		std::memcpy(Data + CurSize, &narrowed, sizeof(narrowed));
		CurSize += 2;
	}

	void NetWriter::WriteLong(int32_t value)
	{
		if (MaxSize - CurSize < 4)
		{
			Overflowed = true;
			return;
		}
		std::memcpy(Data + CurSize, &value, sizeof(value));
		CurSize += 4;
	}

	void NetWriter::WriteData(const void* data, int length)
	{
		if (length <= 0)
			return;

		if (MaxSize - CurSize < length)
		{
			Overflowed = true;
			return;
		}
		std::memcpy(Data + CurSize, data, static_cast<size_t>(length));
		CurSize += length;
	}

	NetReader::NetReader(const uint8_t* data, int size, int offset)
		: Data(data), CurSize(size > 0 ? size : 0), ReadCount(offset > 0 ? offset : 0)
	{
		if (ReadCount > CurSize)
		{
			ReadCount = CurSize;
			Overflowed = true;
		}
	}

	int NetReader::ReadByte()
	{
		if (ReadCount + 1 > CurSize)
		{
			Overflowed = true;
			return -1;
		}
		const uint8_t value = Data[ReadCount];
		ReadCount += 1;
		return value;
	}

	// Sign extends, matching MSG_ReadShort. Every short here is a small positive count, so a negative
	// result is a malformed packet.
	int NetReader::ReadShort()
	{
		if (ReadCount + 2 > CurSize)
		{
			Overflowed = true;
			return -1;
		}
		int16_t value = 0;
		std::memcpy(&value, Data + ReadCount, sizeof(value));
		ReadCount += 2;
		return value;
	}

	int NetReader::ReadLong()
	{
		if (ReadCount + 4 > CurSize)
		{
			Overflowed = true;
			return -1;
		}
		int32_t value = 0;
		std::memcpy(&value, Data + ReadCount, sizeof(value));
		ReadCount += 4;
		return value;
	}

	bool NetReader::ReadData(void* out, int length)
	{
		if (length < 0 || ReadCount + length > CurSize)
		{
			Overflowed = true;
			return false;
		}
		if (length > 0)
			std::memcpy(out, Data + ReadCount, static_cast<size_t>(length));
		ReadCount += length;
		return true;
	}

	// An empty buffer is possible on a channel that was never set up.
	static const Fragment* At(const FrameWindow& window, int sequence)
	{
		const int length = static_cast<int>(window.Fragments.size());
		if (length <= 0 || sequence < 0)
			return nullptr;
		return &window.Fragments[sequence % length];
	}

	static Fragment* At(FrameWindow& window, int sequence)
	{
		return const_cast<Fragment*>(At(static_cast<const FrameWindow&>(window), sequence));
	}

	static void PrimeRate(RateTracker& rate, int time)
	{
		rate.NextSampleTime = time + RateSampleIntervalMs;
		rate.LastBytes = rate.Bytes;
		rate.LastBytesTotal = rate.BytesTotal;
	}

	static void SampleRate(RateTracker& rate, int time)
	{
		if (time <= rate.NextSampleTime)
			return;

		rate.BytesPerSecond = rate.Bytes - rate.LastBytes;
		rate.LastBytes = rate.Bytes;
		rate.BytesPerSecondTotal = rate.BytesTotal - rate.LastBytesTotal;
		rate.LastBytesTotal = rate.BytesTotal;
		rate.NextSampleTime = time + RateSampleIntervalMs;
	}

	void ReliableTransport::SetSender(PacketSender sender)
	{
		Sender = std::move(sender);
		WarnedUnbound = false;
	}

	// CoD4X owns the same marker on the same qport, so two channels would acknowledge over each other.
	bool ReliableTransport::Available()
	{
		return !Patch::UseCoD4X;
	}

	bool ReliableTransport::Setup(netsrc_t sock, int qport, const netadr_t& remote)
	{
		Disconnect();

		if (!Available())
		{
			if (!WarnedCoD4X)
			{
				WarnedCoD4X = true;
				Log::WriteLine(Channel::System, "CoD4x already owns the reliable channel, leaving it alone.");
			}
			return false;
		}

		try
		{
			Tx.Fragments.assign(DefaultFragmentBuffer, Fragment{});
			Rx.Fragments.assign(DefaultFragmentBuffer, Fragment{});
		}
		catch (const std::bad_alloc&)
		{
			Log::WriteLine(Channel::Error, "Out of memory setting up the reliable channel.");
			Disconnect();
			return false;
		}

		Tx.WindowSize = SendWindowSize;
		Rx.WindowSize = ReceiveWindowSize;

		Remote = remote;
		Sock = sock;
		QPort = qport;
		Time = 0;
		NextAckTime = 0;
		Active = true;
		return true;
	}

	void ReliableTransport::Disconnect()
	{
		Active = false;
		Tx = FrameWindow{};
		Rx = FrameWindow{};
		Remote = {};
		QPort = 0;
		Time = 0;
		NextAckTime = 0;
	}

	bool ReliableTransport::IsActive() const
	{
		return Active;
	}

	// A peer drops anything past its own receive window, so sending wider only burns bandwidth.
	void ReliableTransport::SetSendWindow(int fragments)
	{
		Tx.WindowSize = std::clamp(fragments, 1, ReceiveWindowSize);

		if (Tx.Frame >= Tx.Acknowledge + Tx.WindowSize)
			Tx.Frame = Tx.Acknowledge;
	}

	int ReliableTransport::SendWindow() const
	{
		return Tx.WindowSize;
	}

	// A full buffer is not an error: the count actually taken comes back and the caller offers the rest
	// again later.
	int ReliableTransport::Send(const uint8_t* data, int length)
	{
		if (!Active || !data || length <= 0)
			return 0;

		const int used = UsedFragmentCount();
		int available = static_cast<int>(Tx.Fragments.size()) - used;

		if (available < (length / MaxFragmentSize) + 1)
		{
			// 1.5 times what this message needs, so a stream of them stops reallocating.
			int wanted = length / MaxFragmentSize + used + 1;
			wanted += wanted / 2;

			const int resized = ChangeSendBufferSize(wanted);
			if (resized > used)
				available = resized - used;
		}

		int sent = 0;
		for (int i = 0; i < available; ++i)
		{
			const int remaining = length - sent;
			if (remaining <= 0)
				break;

			const int chunk = remaining >= MaxFragmentSize ? MaxFragmentSize : remaining;

			Fragment* fragment = At(Tx, Tx.Sequence);
			if (!fragment)
				break;

			std::memcpy(fragment->Data.data(), data + sent, static_cast<size_t>(chunk));
			fragment->Length = chunk;
			fragment->Ack = -1;
			fragment->SentTime = 0;
			fragment->PacketNum = 0;

			sent += chunk;
			Tx.Sequence++;
			Tx.Rate.Bytes += chunk;
		}
		return sent;
	}

	// The tail of a fragment that does not fit is held back and comes out first on the next call.
	int ReliableTransport::Receive(uint8_t* out, int length)
	{
		if (!Active || !out || length <= 0)
			return 0;

		const int available = ContiguousAcknowledge(Rx) - Rx.Sequence;
		int written = 0;

		const int pending = Rx.PendingSize - Rx.PendingRead;
		if (pending > 0)
		{
			const int chunk = std::min(pending, length);
			std::memcpy(out, Rx.Pending.data() + Rx.PendingRead, static_cast<size_t>(chunk));
			Rx.PendingRead += chunk;
			written += chunk;
		}
		if (Rx.PendingRead >= Rx.PendingSize)
		{
			Rx.PendingSize = 0;
			Rx.PendingRead = 0;
		}

		int consumed = 0;
		for (; consumed < available && written < length; ++consumed)
		{
			const Fragment* fragment = At(Rx, Rx.Sequence + consumed);
			if (!fragment)
				break;

			int chunk = fragment->Length;
			if (chunk > length - written)
			{
				chunk = length - written;

				Rx.PendingSize = fragment->Length - chunk;
				Rx.PendingRead = 0;
				std::memcpy(Rx.Pending.data(), fragment->Data.data() + chunk, static_cast<size_t>(Rx.PendingSize));
			}
			std::memcpy(out + written, fragment->Data.data(), static_cast<size_t>(chunk));
			written += chunk;
		}

		Rx.Sequence += consumed;

		// The saved scan position is an offset from the window base, meaningless once the base moves.
		if (consumed > 1)
			Rx.SelAckOffset = 1;

		return written;
	}

	// Bypasses the held back tail, so a channel must not mix this with Receive.
	int ReliableTransport::ReceiveSingleFragment(uint8_t* out, int length)
	{
		if (!Active || !out || length < MaxFragmentSize)
			return 0;

		if (ContiguousAcknowledge(Rx) - Rx.Sequence < 1)
			return 0;

		const Fragment* fragment = At(Rx, Rx.Sequence);
		if (!fragment)
			return 0;

		const int chunk = std::min(fragment->Length, length);
		std::memcpy(out, fragment->Data.data(), static_cast<size_t>(chunk));
		Rx.Sequence++;
		return chunk;
	}

	bool ReliableTransport::HasData() const
	{
		if (!Active)
			return false;
		if (Rx.PendingSize - Rx.PendingRead > 0)
			return true;
		return ContiguousAcknowledge(Rx) > Rx.Sequence;
	}

	void ReliableTransport::ReceivePacket(const uint8_t* data, int length, int offset)
	{
		if (!data)
			return;

		NetReader reader(data, length, offset);
		ReceivePacket(reader);
	}

	// The caller has already eaten the marker long and the qport short.
	void ReliableTransport::ReceivePacket(NetReader& reader)
	{
		if (!Active)
			return;

		const int sequence = reader.ReadLong();
		const int acknowledge = reader.ReadLong();
		const int flags = reader.ReadByte();
		if (reader.Overflowed)
			return;

		if (flags & ForceAckFlag)
			NextAckTime = 0;

		Rx.Packets++;
		Rx.Rate.BytesTotal += reader.CurSize;

		if (sequence >= Rx.Sequence + Rx.WindowSize)
			return;

		if (acknowledge > Tx.Acknowledge + Tx.WindowSize)
		{
			Log::WriteLine(Channel::Warning, "Illegible reliable acknowledge, got {} at {}.", acknowledge,
				Tx.Acknowledge);
			return;
		}
		if (acknowledge < Tx.Acknowledge)
			return;
		if (acknowledge > Tx.Sequence)
		{
			Log::WriteLine(Channel::Warning, "Reliable acknowledge {} is ahead of sequence {}.", acknowledge,
				Tx.Sequence);
			return;
		}

		const int selectiveAcks = reader.ReadByte();
		if (selectiveAcks < 0 || selectiveAcks > MaxSelectiveAcks)
		{
			Log::WriteLine(Channel::Warning, "Bad selective acknowledge count {}.", selectiveAcks);
			return;
		}

		for (int i = 0; i < selectiveAcks; ++i)
		{
			const int start = reader.ReadShort() + acknowledge;
			const int count = reader.ReadShort();
			if (reader.Overflowed)
				return;

			if (count < 0 || start < acknowledge || start + count > acknowledge + Tx.WindowSize)
			{
				Log::WriteLine(Channel::Warning, "Selective acknowledge {} is outside the window at {}.", start + count,
					acknowledge);
				return;
			}

			for (int j = 0; j < count; ++j)
			{
				// Confirming what was not sent yet would mark a slot Send is about to reuse.
				if (start + j >= Tx.Sequence)
					break;

				Fragment* fragment = At(Tx, start + j);
				if (fragment)
					fragment->Ack = start + j;
			}
		}

		Rx.PeerWindowSize = reader.ReadShort();

		const int fragmentSize = reader.ReadShort();
		if (reader.Overflowed)
			return;

		if (fragmentSize < 0 || fragmentSize > MaxFragmentSize)
		{
			Log::WriteLine(Channel::Warning, "Invalid reliable fragment size {}.", fragmentSize);
			return;
		}

		if (Tx.Acknowledge < acknowledge)
		{
			Tx.Acknowledge = acknowledge;

			if (UsedFragmentCount() < DefaultFragmentBuffer
				&& static_cast<int>(Tx.Fragments.size()) > DefaultFragmentBuffer)
				ChangeSendBufferSize(DefaultFragmentBuffer);
		}

		// An acknowledge only packet carries no fragment.
		if (sequence < 0 || sequence < Rx.Sequence)
			return;

		Fragment* fragment = At(Rx, sequence);
		if (!fragment)
			return;

		// Leave the slot unacknowledged on a truncated packet so the peer sends it again.
		if (!reader.ReadData(fragment->Data.data(), fragmentSize))
			return;

		fragment->Length = fragmentSize;
		fragment->Ack = sequence;
		Rx.Rate.Bytes += fragmentSize;
	}

	// Once per client frame, on the absolute millisecond clock CoD4X feeds it from cls.realtime.
	void ReliableTransport::Frame(int time)
	{
		if (!Active)
			return;

		int elapsed = time - Time;
		Time = time;

		if (elapsed < 0)
			elapsed = 0;
		else if (elapsed > MaxFrameElapsedMs)
			elapsed = MaxFrameElapsedMs;

		// The acknowledge timer outranks the rate: a stalled peer is waiting on this packet.
		if (NextAckTime < Time)
			TransmitNextFragment();

		// One window per second is the pacing, kept in thousandths so a short frame still counts.
		const int milliPackets = elapsed * Tx.WindowSize + Tx.UnsentMilliPackets;
		const int packets = milliPackets / 1000;
		Tx.UnsentMilliPackets = milliPackets % 1000;

		for (int i = 0; i < packets; ++i)
			TransmitNextFragment();

		TrackRate();
	}

	int ReliableTransport::UsedFragmentCount() const
	{
		if (!Active)
			return 0;
		return Tx.Sequence - Tx.Acknowledge;
	}

	int ReliableTransport::UsedSendBufferSize() const
	{
		return UsedFragmentCount() * MaxFragmentSize;
	}

	int ReliableTransport::PeerWindow() const
	{
		return Rx.PeerWindowSize;
	}

	const RateTracker& ReliableTransport::SendRate() const
	{
		return Tx.Rate;
	}

	const RateTracker& ReliableTransport::ReceiveRate() const
	{
		return Rx.Rate;
	}

	// Returns the resulting size, or -1 when the backlog would not fit and the old buffer was kept.
	int ReliableTransport::ChangeSendBufferSize(int fragmentCount)
	{
		const int used = UsedFragmentCount();
		const int current = static_cast<int>(Tx.Fragments.size());

		if (fragmentCount <= DefaultFragmentBuffer)
		{
			fragmentCount = DefaultFragmentBuffer;
			if (current == DefaultFragmentBuffer)
				return DefaultFragmentBuffer;
		}
		if (fragmentCount > MaxFragmentBuffer)
			fragmentCount = MaxFragmentBuffer;

		if (used >= fragmentCount || current <= 0)
			return -1;

		std::vector<Fragment> resized;
		try
		{
			// Every slot starts unacknowledged, so one the copy below misses is never mistaken for a
			// fragment the peer already took.
			resized.assign(fragmentCount, Fragment{});
		}
		catch (const std::bad_alloc&)
		{
			return -1;
		}

		for (int i = Tx.Acknowledge; i < Tx.Sequence; ++i)
			resized[i % fragmentCount] = Tx.Fragments[i % current];

		Tx.Fragments = std::move(resized);
		return fragmentCount;
	}

	void ReliableTransport::TransmitNextFragment()
	{
		if (!Sender)
		{
			if (!WarnedUnbound)
			{
				WarnedUnbound = true;
				Log::WriteLine(Channel::Error, "The reliable channel has no packet sender bound.");
			}
			return;
		}

		uint8_t data[MaxReliablePacketSize];
		NetWriter writer(data, static_cast<int>(sizeof(data)));

		if (Tx.Acknowledge == Tx.Sequence)
		{
			if (NextAckTime >= Time)
				return;

			writer.WriteLong(ReliableMarker);
			writer.WriteShort(QPort);
			// A sequence of -1 says the packet carries an acknowledge and nothing else.
			writer.WriteLong(-1);
			writer.WriteLong(Rx.Sequence);
			writer.WriteByte(0);

			WriteSelectiveAckList(Rx, writer);
			writer.WriteShort(Tx.WindowSize);
			writer.WriteShort(0);

			if (writer.Overflowed)
				return;

			Sender(Sock, writer.CurSize, writer.Data, &Remote);
			Tx.Packets++;
			Tx.Rate.BytesTotal += writer.CurSize;
			NextAckTime = Time + AckIntervalMs;
			return;
		}

		if (Tx.Frame < Tx.Acknowledge)
			Tx.Frame = Tx.Acknowledge;

		const int sequence = Tx.Frame;
		Fragment* fragment = sequence < Tx.Sequence ? At(Tx, sequence) : nullptr;

		// Skipping what the peer confirmed is what retransmits the holes, not the whole window.
		if (fragment && fragment->Ack != sequence)
		{
			writer.WriteLong(ReliableMarker);
			writer.WriteShort(QPort);
			writer.WriteLong(sequence);
			writer.WriteLong(Rx.Sequence);
			writer.WriteByte(0);

			WriteSelectiveAckList(Rx, writer);
			writer.WriteShort(Tx.WindowSize);
			writer.WriteShort(fragment->Length);
			writer.WriteData(fragment->Data.data(), fragment->Length);

			// Sized for the worst case header and fragment, so an overflow means the fragment is corrupt.
			if (!writer.Overflowed)
			{
				Sender(Sock, writer.CurSize, writer.Data, &Remote);
				Tx.Packets++;
				Tx.Rate.BytesTotal += writer.CurSize;
				NextAckTime = Time + AckIntervalMs;

				fragment->SentTime = Time;
				fragment->PacketNum++;
			}
		}

		++Tx.Frame;
		if (Tx.Frame >= Tx.Acknowledge + Tx.WindowSize)
			Tx.Frame = Tx.Acknowledge;
	}

	void ReliableTransport::TrackRate()
	{
		if (Rx.Rate.NextSampleTime == 0)
		{
			PrimeRate(Rx.Rate, Time);
			PrimeRate(Tx.Rate, Time);
			return;
		}
		SampleRate(Rx.Rate, Time);
		SampleRate(Tx.Rate, Time);
	}

	int ReliableTransport::ContiguousAcknowledge(const FrameWindow& window)
	{
		int i = 0;
		for (; i < window.WindowSize; ++i)
		{
			const Fragment* fragment = At(window, i + window.Sequence);
			if (!fragment || fragment->Ack != i + window.Sequence)
				break;
		}
		return i + window.Sequence;
	}

	// The scan resumes where the previous packet stopped, so more holes than fit in one packet still
	// all get reported over time.
	void ReliableTransport::WriteSelectiveAckList(FrameWindow& window, NetWriter& writer)
	{
		const int countPosition = writer.CurSize;
		writer.WriteByte(0);

		int count = 0;
		int rangeLength = 0;
		bool inRange = false;

		int i = window.SelAckOffset;
		for (; i < window.WindowSize; ++i)
		{
			const int sequence = i + window.Sequence;
			const Fragment* fragment = At(window, sequence);

			if (fragment && fragment->Ack == sequence)
			{
				if (!inRange)
				{
					writer.WriteShort(i);
					count++;
					rangeLength = 0;
				}
				inRange = true;
				rangeLength++;
			}
			else if (inRange)
			{
				writer.WriteShort(rangeLength);
				// Must clear before the break, or the trailing write emits a fourth length the peer never
				// reads and the packet desyncs.
				inRange = false;

				if (count >= MaxSelectiveAcks)
					break;
			}
		}
		if (inRange)
			writer.WriteShort(rangeLength);

		window.SelAckOffset = i < window.WindowSize ? i : 1;

		if (countPosition < writer.CurSize)
			writer.Data[countPosition] = static_cast<uint8_t>(count);
	}

	// The stream hands over whatever it has, so a short read is the normal case and not a failure.
	static int Pull(ReliableTransport& stream, std::vector<uint8_t>& buffer, int want)
	{
		const int have = static_cast<int>(buffer.size());
		if (want <= have)
			return 0;

		buffer.resize(static_cast<size_t>(want));
		const int received = stream.Receive(buffer.data() + have, want - have);
		buffer.resize(static_cast<size_t>(have + received));
		return received;
	}

	bool ReliableMessages::Setup(netsrc_t sock, int qport, const netadr_t& remote)
	{
		Outgoing.clear();
		Incoming.clear();
		return Stream.Setup(sock, qport, remote);
	}

	void ReliableMessages::Disconnect()
	{
		Stream.Disconnect();
		Outgoing.clear();
		Outgoing.shrink_to_fit();
		Incoming.clear();
		Incoming.shrink_to_fit();
	}

	bool ReliableMessages::IsActive() const
	{
		return Stream.IsActive();
	}

	bool ReliableMessages::Send(const uint8_t* body, int length)
	{
		if (!Stream.IsActive() || !body || length < 0 || length > MaxReliableMessageSize)
			return false;

		// Refusing lets a caller feel the channel back up, rather than growing until memory runs out.
		if (Outgoing.size() + 4 + static_cast<size_t>(length) > MaxReliableStageSize)
			return false;

		const int32_t header = length;
		const size_t at = Outgoing.size();
		Outgoing.resize(at + sizeof(header) + static_cast<size_t>(length));
		std::memcpy(Outgoing.data() + at, &header, sizeof(header));
		if (length > 0)
			std::memcpy(Outgoing.data() + at + sizeof(header), body, static_cast<size_t>(length));

		Flush();
		return true;
	}

	// A partial message is kept, so a caller may simply try again on the following frame.
	bool ReliableMessages::Receive(std::vector<uint8_t>& out)
	{
		if (!Stream.IsActive())
			return false;

		Pull(Stream, Incoming, 4);
		if (Incoming.size() < 4)
			return false;

		int32_t size = 0;
		std::memcpy(&size, Incoming.data(), sizeof(size));

		if (size < 0 || size > MaxReliableMessageSize)
		{
			Log::WriteLine(Channel::Error, "Oversize reliable message of {} bytes, dropping the channel.", size);
			Disconnect();
			return false;
		}

		Pull(Stream, Incoming, size + 4);
		if (static_cast<int>(Incoming.size()) < size + 4)
			return false;

		out.assign(Incoming.begin() + 4, Incoming.begin() + 4 + size);
		Incoming.erase(Incoming.begin(), Incoming.begin() + 4 + size);
		return true;
	}

	void ReliableMessages::Frame(int time)
	{
		Flush();
		Stream.Frame(time);
	}

	ReliableTransport& ReliableMessages::Transport()
	{
		return Stream;
	}

	const ReliableTransport& ReliableMessages::Transport() const
	{
		return Stream;
	}

	void ReliableMessages::Flush()
	{
		if (Outgoing.empty())
			return;

		const int size = static_cast<int>(Outgoing.size());
		const int taken = Stream.Send(Outgoing.data(), size);

		if (taken >= size)
			Outgoing.clear();
		else if (taken > 0)
			Outgoing.erase(Outgoing.begin(), Outgoing.begin() + taken);
	}
}

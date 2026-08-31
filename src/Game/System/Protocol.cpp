#include "Protocol.hpp"
#include "Dvar.hpp"
#include "Patch.hpp"

#include <charconv>
#include <cstdlib>
#include <cstring>
#include <fstream>

// CoD4X's extended protocol against stock protocol 6. Only the gamestate block differs: the extended
// one carries a second configstring table plus long names and clantags. Snapshots are protocol
// agnostic. A server offers it by tagging its challengeResponse with "xproto" (cl_main.c:3644).

namespace IW3SR
{
	// "cod" on a stock server, "cod<protocol> " and the asset counts on an extended one
	// (cl_main.c:8052 CL_CheckGameVersion).
	constexpr int VersionConfigString = 2;

	// Configstring 12 holds "x y z", the point the compass rotates around.
	constexpr int MapCenterConfigString = 12;

	// Connectionless packets are unauthenticated, so the scan is bounded on both ends.
	constexpr size_t MaxPacketLength = 1024;
	constexpr size_t MaxPacketTokens = 16;

	// BIG_INFO_STRING, the width systeminfo is written at, and what MAX_OSPATH leaves for a game dir.
	constexpr size_t MaxInfoString = 8192;
	constexpr size_t MaxGameDir = 64;

	// svc_ops_e (qcommon.h:293). A gamestate carries only the first four; svc_configclient exists in
	// the extended encoding alone.
	enum ServerCommand
	{
		ServerGamestate = 1,
		ServerConfigString = 2,
		ServerBaseline = 3,
		ServerEndOfFile = 7,
		ServerConfigClient = 11
	};

	constexpr int MaxConfigStrings = static_cast<int>(sizeof(gameState_t::stringOffsets) / sizeof(int));
	constexpr int MaxGameStateChars = static_cast<int>(sizeof(gameState_t::stringData));
	constexpr int MaxEntities = static_cast<int>(sizeof(clientActive_t::entityBaselines) / sizeof(entityState_s));

	// Retail memsets exactly this much of cl at 0x473D0E.
	constexpr size_t ClientActiveSize = 0x1B1BDC;

	// Retail's own baseline write is `imul eax,esi,0xf4; add eax,0xd28400` (0x47410A). A struct that
	// disagrees would send every baseline to the wrong slot, silently and only on this path.
	static_assert(sizeof(entityState_s) == 0xF4);
	static_assert(offsetof(clientActive_t, entityBaselines) == 0xD28400 - 0xC5F930);

	// The constants the HUD walks these two tables with, from 0x438CA1 (clientinfo) and 0x44D9FC (the
	// snapshot's own client states, where the blank on an extended server comes from).
	static_assert(offsetof(snapshot_s, clients) + offsetof(clientState_s, team) == 0x2177C);
	static_assert(offsetof(clientState_s, name) - offsetof(clientState_s, team) == 0x38);
	static_assert(sizeof(clientInfo_t) == 0x4CC);
	static_assert(
		0x74E338 + offsetof(cg_s, bgs) + offsetof(bgs_t, clientinfo) + offsetof(clientInfo_t, name) == 0x83927C);

	constexpr int MaxClients = 64;
	constexpr int BigInfoString = 8192;

	constexpr int MaxExtendedConfigStrings = 2 * MaxConfigStrings;

	// A `push imm8` inside CL_CheckForResend with no dvar behind it, so 21 goes in by rewriting the
	// one operand byte.
	//   0046b075: 6a 06        push 0x6
	//   0046b077: 68 0c 7c 6c  push 0x6c7c0c ("%i")
	constexpr uintptr_t AdvertiseSite = 0x46B075;
	constexpr uintptr_t AdvertiseFormat = 0x6C7C0C;

	// Retail's connectResponse forces CA_SENDINGSTATS, and CL_WritePacket sends nothing in that state
	// (cl_main.c:4260) - so against an extended server, whose stats handler is compiled out, the
	// connection would never ask for a gamestate. CoD4X stops at CA_CONNECTED (cl_main.c:3798).
	//   0046b98a: c7 05 00 f9 c5 00 06 00 00 00   mov DWORD PTR ds:0xc5f900,0x6
	constexpr uintptr_t ConnectedSite = 0x46B98A;
	constexpr uintptr_t ConnectionStateAddress = 0xC5F900;

	//   00463ebb: 3d e8 03 00 00   cmp eax,0x3e8
	constexpr uintptr_t ThrottleSite = 0x463EBB;
	constexpr int ConnectedSendInterval = 1000;
	constexpr int ExtendedSendInterval = 50;

	static bool ArmWarned = false;

	// The configstrings an extended server may elide (cl_main.c:47). 16 byte rows, first entry
	// { 0x14, 0x6C0A84 }, terminator at 0x723DB0.
	static const constConfigString_t* const ConstantConfigStrings =
		reinterpret_cast<const constConfigString_t*>(0x7209B0);

	// 832 rows plus the terminator. A larger cap would walk past the end of .data before tripping.
	constexpr int MaxConstantConfigStrings = 833;

	// The parallel configstring table an extended gamestate fills for indices at or above
	// MAX_CONFIGSTRINGS - extGameState in CoD4X (cl_main.c:6877). Nothing in retail owns it.
	static gameState_t ExtendedGameState = {};
	static ExtendedClient ExtendedClients[MaxClients] = {};
	int ExtendedConfigDataSequence = 0;

	// Waits out the fastfile loader thread (inlined at 0x47418F-0x4741A8): FS_Restart below rebuilds
	// the search path that thread is still reading images out of.
	static Function<void()> DB_SyncXAssets = 0x48A290;

	// Retail reloads the iwd set when the game directory or checksum feed changed (0x4741C7-0x4741E7).
	// Skipping it leaves the client unpure and every sv_pure server drops it.
	static Function<void(int localClientNum, int checksumFeed)> FS_Restart = 0x55ED10;

	// The feed FS_Restart last ran with, and fs_gameDirVar; both are what retail compares against.
	static const int* const LoadedChecksumFeed = reinterpret_cast<const int*>(0xCB199A0);
	static dvar_s** const GameDirVar = reinterpret_cast<dvar_s**>(0xCB199A4);
	static dvar_s** const ServerRunning = reinterpret_cast<dvar_s**>(0x1435D60);

	// CL_DownloadsComplete reloads the zone set only when this is set, and clears it on the way
	// through (read 0x46AA56, cleared 0x46A1A1). Retail latches fs_gameDirVar->modified into it at
	// the gamestate tail (0x4741C1).
	static int* const GameDirChanged = reinterpret_cast<int*>(0xC5B69C);

	// Retail dispatches svc_ opcodes through a seven entry table and rejects anything above six. The
	// extended protocol adds svc_configclient (cl_main.c:7450). Only twelve bytes of int3 sit behind
	// the table, not the twenty an extension needs, so the bound and displacement point at ours.
	//   004747ba: 83 ff 06                 cmp edi,0x6
	//   004747c3: ff 24 bd a8 48 47 00     jmp DWORD PTR [edi*4+0x4748a8]
	constexpr uintptr_t DispatchBoundSite = 0x4747BA;
	constexpr uintptr_t DispatchTableSite = 0x4747C3;
	constexpr uintptr_t DispatchTable = 0x4748A8;

	// Retail's entry for an unknown opcode.
	constexpr uintptr_t DispatchIllegible = 0x474890;

	// CoD4X sends entity origins as bare floats (CoD4x-Server/src/msg.c). Retail's readers still decode
	// the stock delta scheme, so every origin leaves the reader 24 bits behind and the gamestate dies at
	// the first baseline carrying one. CoD4X patches the same two sites
	// (CoD4x_Client_pub/src/sys_patch.c:1072).
	//
	// Both take the message in eax, return in st0 and leave their stack arguments to the caller, so one
	// thunk serves the call site and the whole function.
	//   00506c04: e8 e7 f9 ff ff   call 0x5065f0
	//   00506680: 51 56 8b f0 8b   push ecx; push esi; mov esi,eax; (mov edx,esi)
	constexpr uintptr_t OriginFloatSite = 0x506C04;
	constexpr uintptr_t OriginFloatTarget = 0x5065F0;
	constexpr uintptr_t OriginZFloatSite = 0x506680;

	static const std::vector<uint8_t> StockOriginZFloat = { 0x51, 0x56, 0x8B, 0xF0, 0x8B };

	static uintptr_t OriginThunk = 0;
	static bool OriginArmed = false;

	// An extended server leaves clientState_s::name empty and sends names as svc_configclient records
	// instead (sv_snapshot.c:339). CG_SetNextSnap announces a rename whenever that field disagrees with
	// clientinfo, so a blank every snapshot means a flicker and a repeated rename line. Filling it in as
	// the snapshot is assembled fixes every stock reader downstream at once.
	//
	// CG_ReadNextSnapshot calls CL_GetSnapshot exactly once, so this rides the call site.
	//   0044e317: e8 74 c8 00 00   call 0x45ab90
	constexpr uintptr_t SnapshotSite = 0x44E317;
	constexpr uintptr_t SnapshotTarget = 0x45AB90;

	static uintptr_t SnapshotThunk = 0;
	static bool SnapshotArmed = false;

	// A stock server announces a map change out of band, which retail acts on at once (0x46BF2F). An
	// extended server sends the reliable command 'l' instead (sv_main.c:4999), and retail only files
	// reliable commands for cgame to run at the next snapshot transition - which on a map change never
	// comes, leaving the old map drawn under "connection interrupted".
	//
	// CL_ParseCommandString is inlined into CL_ParseServerMessage, so the hook goes on the terminator
	// it writes over the stored command, with the string still in esi.
	//   00474807: c6 86 ff 03 00 00 00   mov BYTE PTR [esi+0x3ff],0x0
	constexpr uintptr_t ServerCommandSite = 0x474807;
	static const std::vector<uint8_t> StockServerCommand = { 0xC6, 0x86, 0xFF, 0x03, 0x00, 0x00, 0x00 };

	static uintptr_t ServerCommandThunk = 0;
	static bool ServerCommandArmed = false;

	// CL_InitCGame compares four bytes of configstring 2 - "cod" and its terminator - and drops with
	// "Client/Server game mismatch". An extended server writes "cod<protocol> " there (sv_main.c:3559),
	// so the terminator never matches and the map load dies after the gamestate is read. Three bytes
	// makes it CoD4X's prefix test (cl_main.c:8044); Confirm checks the version itself.
	//   0044002a: b9 04 00 00 00   mov ecx,0x4
	//   0044003b: f3 a6            repz cmpsb
	constexpr uintptr_t GameVersionSite = 0x44002A;

	constexpr int StockDispatchCount = 7;
	constexpr int ExtendedDispatchCount = ServerConfigClient + 1;

	static uintptr_t ExtendedDispatch[ExtendedDispatchCount] = {};

	// The client packet header. Retail writes nine bytes - a byte of cl.serverId then two longs - where
	// an extended server reads sixteen: four longs, cl.serverId widened and clc.serverConfigDataSequence
	// added (cl_main.c:4271-4290 against sv_main.c:1998-2022).
	//
	// The nine is encoded in six places that all have to move together: the header writes, the inlined
	// copy into the output arena, and four operands carrying the length into the compressor and encoder.
	// CL_Netchan_Encode is left alone - its key comes from cl.serverId in memory, not the wire.
	constexpr uintptr_t HeaderWriteSite = 0x463FFA;
	constexpr uintptr_t HeaderCopySite = 0x4642A9;
	constexpr uintptr_t CompressDestSite = 0x4642DF;
	constexpr uintptr_t CompressLengthSite = 0x4642E2;
	constexpr uintptr_t CompressSourceSite = 0x4642E6;
	constexpr uintptr_t OutLengthSite = 0x46432D;
	constexpr uintptr_t EncodeLengthSite = 0x464386;

	constexpr int LegacyHeaderSize = 9;
	constexpr int ExtendedHeaderSize = 16;

	constexpr uintptr_t ServerIdAddress = 0xC84FE4;

	// 0x463FFA: the two MSG_Init arguments are still on the stack, so the three that the original
	// add esp,0xc dropped become two once MSG_WriteByte is gone.
	static const std::vector<uint8_t> StockHeaderWrite = { 0x8B, 0x15, 0xE4, 0x4F, 0xC8, 0x00, 0x52, 0x8B, 0xC6, 0xE8,
		0x18, 0x14, 0x0A, 0x00, 0x8B, 0x3D, 0x1C, 0x4E, 0x91, 0x00, 0x83, 0xC4, 0x0C, 0xE8, 0x8A, 0x14, 0x0A, 0x00,
		0x8B, 0x3D, 0x20, 0x4E, 0x91, 0x00, 0xE8, 0x7F, 0x14, 0x0A, 0x00 };

	// 0x4642A9: the header copy, spread over six instructions as 4 + 4 + 1. Four dword pairs do not
	// fit, so the block becomes a rep movsd and the three instructions that have to keep their
	// addresses - add esp, cmp ebp and the jbe whose rel8 reaches 0x4642DD - stay where they were.
	static const std::vector<uint8_t> StockHeaderCopy = { 0x8B, 0x16, 0x8D, 0xBD, 0xF0, 0x24, 0xB2, 0x0C, 0x8B, 0x6C,
		0x24, 0x30, 0x89, 0x17, 0x8B, 0x46, 0x04, 0x89, 0x47, 0x04, 0x8A, 0x4E, 0x08, 0x83, 0xC4, 0x08, 0x81, 0xFD,
		0x00, 0x08, 0x00, 0x00, 0x88, 0x4F, 0x08, 0x76, 0x0F };

	static const std::vector<uint8_t> ExtendedHeaderCopy = {
		0x8D, 0xBD, 0xF0, 0x24, 0xB2, 0x0C, // lea edi,[ebp+0xCB224F0]   destination, before ebp moves
		0x8B, 0x6C, 0x24, 0x30,				// mov ebp,[esp+0x30]        buf.cursize
		0xB9, 0x04, 0x00, 0x00, 0x00,		// mov ecx,0x4
		0xF3, 0xA5,							// rep movsd
		0x83, 0xEE, 0x10,					// sub esi,0x10              both stay live past here
		0x83, 0xEF, 0x10,					// sub edi,0x10
		0x83, 0xC4, 0x08,					// add esp,0x8
		0x81, 0xFD, 0x00, 0x08, 0x00, 0x00, // cmp ebp,0x800
		0x90, 0x90, 0x90,					// was mov [edi+0x8],cl
		0x76, 0x0F							// jbe 0x4642DD
	};

	static uintptr_t ExtendedHeaderThunk = 0;
	static bool HeaderArmed = false;

	// Ports rather than calls: an extended gamestate is untrusted, and a port can be read next to the
	// bounds it feeds. Checked against MSG_ReadBit 0x505300, MSG_ReadBits 0x505270 and MSG_ReadLong
	// 0x5056F0, so the bit cursor they leave is the one MSG_ReadDeltaEntity expects.
	static int ReadBit(msg_t* msg)
	{
		const int offset = msg->bit & 7;
		if (!offset)
		{
			if (msg->readcount >= msg->cursize + msg->splitSize)
			{
				msg->overflowed = 1;
				return -1;
			}
			msg->bit = 8 * msg->readcount;
			msg->readcount++;
		}

		const int index = msg->bit / 8;
		msg->bit++;

		if (index < msg->cursize)
			return (msg->data[index] >> offset) & 1;
		if (!msg->splitData)
		{
			msg->overflowed = 1;
			return -1;
		}

		return (msg->splitData[index - msg->cursize] >> offset) & 1;
	}

	static int ReadBits(msg_t* msg, int bits)
	{
		int value = 0;

		for (int i = 0; i < bits; i++)
		{
			if (!(msg->bit & 7))
			{
				if (msg->readcount >= msg->cursize + msg->splitSize)
				{
					msg->overflowed = 1;
					return -1;
				}
				msg->bit = 8 * msg->readcount;
				msg->readcount++;
			}

			const int index = msg->bit / 8;
			int source = 0;

			if (index < msg->cursize)
				source = msg->data[index];
			else if (msg->splitData)
				source = msg->splitData[index - msg->cursize];
			else
			{
				msg->overflowed = 1;
				return -1;
			}

			value |= ((source >> (msg->bit & 7)) & 1) << i;
			msg->bit++;
		}
		return value;
	}

	// A message may be split across two buffers, and past cursize the bytes live in splitData.
	// Retail's MSG_ReadLong (0x5056F0) bounds against cursize + splitSize and reads through both.
	static int ReadRaw(const msg_t* msg, int index)
	{
		if (index < msg->cursize)
			return msg->data[index];
		if (!msg->splitData)
			return -1;

		return msg->splitData[index - msg->cursize];
	}

	static int ReadByte(msg_t* msg)
	{
		if (msg->readcount + 1 > msg->cursize + msg->splitSize)
		{
			msg->overflowed = 1;
			return -1;
		}

		const int value = ReadRaw(msg, msg->readcount);
		if (value < 0)
		{
			msg->overflowed = 1;
			return -1;
		}

		msg->readcount++;
		return value;
	}

	static int ReadLong(msg_t* msg)
	{
		if (msg->readcount + static_cast<int>(sizeof(int)) > msg->cursize + msg->splitSize)
		{
			msg->overflowed = 1;
			return -1;
		}

		uint32_t value = 0;
		for (int i = 0; i < static_cast<int>(sizeof(value)); i++)
		{
			const int byte = ReadRaw(msg, msg->readcount + i);
			if (byte < 0)
			{
				msg->overflowed = 1;
				return -1;
			}
			value |= static_cast<uint32_t>(byte) << (8 * i);
		}

		msg->readcount += static_cast<int>(sizeof(value));
		return static_cast<int>(value);
	}

	// Retail drains a string that outran its buffer and carries on. A configstring fails the parse
	// instead, since the room left is what stops the write running off the table. Names and clantags
	// truncate: the server sends up to 35 characters (server.h:173) into CoD4X's 33 byte buffer.
	static int ReadString(msg_t* msg, char* out, int length, bool truncate = false)
	{
		if (!out || length <= 0)
			return -1;

		int written = 0;
		while (true)
		{
			const int c = ReadByte(msg);
			if (c < 0)
				return -1;
			if (c == 0)
				break;

			if (written < length - 1)
			{
				// A format specifier surviving to a printf further down is a crash bug.
				out[written++] = c == '%' ? '.' : static_cast<char>(c);
			}
			else if (!truncate)
				return -1;
		}

		out[written] = '\0';
		return written;
	}

	static int ReadEntityIndex(msg_t* msg, int bits)
	{
		if (ReadBit(msg))
			msg->lastRefEntity++;
		else if (bits != 10 || ReadBit(msg))
			msg->lastRefEntity = ReadBits(msg, bits);
		else
			msg->lastRefEntity += ReadBits(msg, 4);

		return msg->lastRefEntity;
	}

	static gameState_t* TableFor(int index, int& slot)
	{
		if (index >= MaxConfigStrings)
		{
			slot = index - MaxConfigStrings;
			return &ExtendedGameState;
		}

		slot = index;
		return &clients->gameState;
	}

	// Callers have already range checked the index; the room left in the table is checked here.
	static bool StoreConfigString(int index, const char* text, int length)
	{
		int slot = 0;
		gameState_t* table = TableFor(index, slot);

		if (length < 0 || length >= BigInfoString)
			return false;
		if (table->dataCount < 1 || length + 1 > MaxGameStateChars - table->dataCount)
			return false;

		std::memcpy(&table->stringData[table->dataCount], text, static_cast<size_t>(length) + 1);
		table->stringOffsets[slot] = table->dataCount;
		table->dataCount += length + 1;
		return true;
	}

	// The table at 0x7209B0 is read through a pointer, so its strings are measured with a cap rather
	// than trusted to terminate.
	static int BoundedLength(const char* text, int limit)
	{
		int length = 0;
		while (length < limit && text[length])
			length++;

		return length;
	}

	// Refills the elided entries below the index the wire is about to name. The cursor only moves
	// forward, so a whole gamestate costs one pass over the retail table.
	static bool FillConstantConfigStrings(int& cursor, int limit)
	{
		while (cursor < MaxConstantConfigStrings)
		{
			const int index = ConstantConfigStrings[cursor].index;
			if (!index || index >= limit)
				return true;

			const char* text = ConstantConfigStrings[cursor].string;
			if (!text || index < 0 || index >= MaxExtendedConfigStrings)
				return false;

			if (!StoreConfigString(index, text, BoundedLength(text, BigInfoString)))
				return false;

			cursor++;
		}
		return false;
	}

	// A gamestate that does not parse cannot be diagnosed from the log: the bytes are the evidence and
	// the process is about to drop them. One file, overwritten each time, beside the client log.
	static void DumpGamestate(const msg_t* msg, int at)
	{
		constexpr const char* path = "iw3sr-gamestate.bin";

		if (std::ofstream file(path, std::ios::binary | std::ios::trunc); file)
			file.write(reinterpret_cast<const char*>(msg->data), msg->cursize);

		const int from = std::max(0, at - 32);
		const int to = std::min(msg->cursize, at + 32);

		std::string hex;
		for (int i = from; i < to; i++)
			hex += std::format("{:02X} ", msg->data[i]);

		Log::WriteLine(Channel::Error, "Gamestate bytes {}..{}: {}", from, to, hex);
		Log::WriteLine(Channel::Error, "Wrote the {} byte gamestate to {}.", msg->cursize, path);
	}

	// Refused unless the site still decodes as stock 1.7, so another mod on the same instruction is
	// left alone rather than corrupted.
	static bool WriteAdvertise(int version)
	{
		// The operand is sign extended, so anything above 0x7F would go out as a negative number.
		if (version < 0 || version > 0x7F)
			return false;
		if (Memory::Get<uint8_t>(AdvertiseSite) != 0x6A || Memory::Get<uint8_t>(AdvertiseSite + 2) != 0x68
			|| Memory::Get<uint32_t>(AdvertiseSite + 3) != AdvertiseFormat)
			return false;

		Memory::Set<uint8_t>(AdvertiseSite + 1, static_cast<uint8_t>(version));
		return true;
	}

	// Past CA_CONNECTED but not yet primed, CL_WritePacket sends at most one packet a second
	// (0x463EBB). The server only sends a gamestate on a netchan message whose serverId does not match
	// (sv_client.c:1893-1903), so that throttle paces the whole extended handshake. 50 is what the same
	// function uses while downloading (0x463EA0).
	static bool WriteSendInterval(int milliseconds)
	{
		// The compare is signed and followed by jl, so a negative would invert the gate entirely.
		if (milliseconds < 0 || milliseconds > ConnectedSendInterval)
			return false;

		//   00463eb7: 8b c6            mov eax,esi
		//   00463eb9: 2b c2            sub eax,edx
		//   00463ebb: 3d e8 03 00 00   cmp eax,0x3e8
		//   00463ec0: 7c 7a            jl
		if (Memory::Get<uint8_t>(ThrottleSite) != 0x3D || Memory::Get<uint16_t>(ThrottleSite - 4) != 0xC68B
			|| Memory::Get<uint16_t>(ThrottleSite - 2) != 0xC22B || Memory::Get<uint16_t>(ThrottleSite + 5) != 0x7A7C)
			return false;

		Memory::Set<uint32_t>(ThrottleSite + 1, static_cast<uint32_t>(milliseconds));
		return true;
	}

	static bool WriteConnectedState(int state)
	{
		if (Memory::Get<uint16_t>(ConnectedSite) != 0x05C7
			|| Memory::Get<uint32_t>(ConnectedSite + 2) != ConnectionStateAddress)
			return false;

		Memory::Set<uint32_t>(ConnectedSite + 6, static_cast<uint32_t>(state));
		return true;
	}

	static bool Matches(uintptr_t address, const std::vector<uint8_t>& bytes)
	{
		const auto* at = reinterpret_cast<const uint8_t*>(address);
		return std::equal(bytes.begin(), bytes.end(), at);
	}

	// Only the count moves, so a server running some other game still fails the comparison.
	static bool WriteGameVersion(bool extended)
	{
		const uint32_t want = extended ? 3 : 4;
		const uint32_t was = extended ? 4 : 3;

		if (Memory::Get<uint32_t>(GameVersionSite + 1) == want)
			return true;
		if (Memory::Get<uint8_t>(GameVersionSite) != 0xB9 || Memory::Get<uint32_t>(GameVersionSite + 1) != was)
			return false;

		Memory::Set<uint32_t>(GameVersionSite + 1, want);
		return true;
	}

	// Not every extended protocol sends them. A protocol 17 demo's first baseline is twelve bytes
	// shorter than a 21 one for the same three origin components, so with the raw reads armed the
	// gamestate drifts inside that baseline. Every version from 19 up reads correctly with them on.
	static bool SendsRawOrigins()
	{
		return GProtocol::Version() >= GProtocol::OldestRawOriginVersion;
	}

	// Armed off the reader rather than the wire: an extended demo carries the same raw floats, and
	// snapshots encode entities the way baselines do, so this stays on for the whole session.
	static bool WriteOriginReads(bool extended)
	{
		if (OriginArmed == extended)
			return true;
		if (!OriginThunk)
			return false;

		if (extended)
		{
			// A binary another mod already sits on is left alone rather than half rewritten.
			if (Memory::Get<uint8_t>(OriginFloatSite) != 0xE8
				|| OriginFloatSite + 5 + Memory::Get<int32_t>(OriginFloatSite + 1) != OriginFloatTarget
				|| !Matches(OriginZFloatSite, StockOriginZFloat))
				return false;

			Memory::CALL(OriginFloatSite, OriginThunk);
			Memory::JMP(OriginZFloatSite, OriginThunk);
		}
		else
		{
			Memory::CALL(OriginFloatSite, OriginFloatTarget);
			Memory::Write(OriginZFloatSite, StockOriginZFloat);
		}

		OriginArmed = extended;
		return true;
	}

	// All seven pieces move as one unit or none do: half applied puts a nine byte header behind a
	// sixteen byte length and corrupts every packet. Both blobs are shape checked before anything is
	// written, and what was done is tracked rather than inferred.
	static bool WriteHeaderSize(int size)
	{
		const bool extended = size == ExtendedHeaderSize;
		if (HeaderArmed == extended)
			return true;
		if (!ExtendedHeaderThunk)
			return false;

		// A replacement even one byte short would leave the tail of the block it overwrites in place.
		if (ExtendedHeaderCopy.size() != StockHeaderCopy.size() || StockHeaderWrite.size() < 8)
			return false;

		const uint8_t offset = static_cast<uint8_t>(extended ? ExtendedHeaderSize : LegacyHeaderSize);
		const uint8_t negated = static_cast<uint8_t>(-static_cast<int8_t>(offset));
		const uint8_t was = static_cast<uint8_t>(extended ? LegacyHeaderSize : ExtendedHeaderSize);
		const uint8_t wasNegated = static_cast<uint8_t>(-static_cast<int8_t>(was));

		if (Memory::Get<uint8_t>(CompressDestSite) != was || Memory::Get<uint8_t>(CompressSourceSite) != was
			|| Memory::Get<uint8_t>(OutLengthSite) != was || Memory::Get<uint8_t>(CompressLengthSite) != wasNegated
			|| Memory::Get<uint8_t>(EncodeLengthSite) != wasNegated)
			return false;

		if (extended)
		{
			if (!Matches(HeaderWriteSite, StockHeaderWrite) || !Matches(HeaderCopySite, StockHeaderCopy))
				return false;

			// The two MSG_Init arguments still have to go; the third belonged to the MSG_WriteByte this
			// replaces.
			Memory::Write(HeaderWriteSite, std::vector<uint8_t>{ 0x83, 0xC4, 0x08 });
			Memory::CALL(HeaderWriteSite + 3, ExtendedHeaderThunk);
			Memory::NOP(HeaderWriteSite + 8, static_cast<int>(StockHeaderWrite.size()) - 8);

			Memory::Write(HeaderCopySite, ExtendedHeaderCopy);
		}
		else
		{
			if (!Matches(HeaderCopySite, ExtendedHeaderCopy))
				return false;

			Memory::Write(HeaderWriteSite, StockHeaderWrite);
			Memory::Write(HeaderCopySite, StockHeaderCopy);
		}

		Memory::Set<uint8_t>(CompressDestSite, offset);
		Memory::Set<uint8_t>(CompressLengthSite, negated);
		Memory::Set<uint8_t>(CompressSourceSite, offset);
		Memory::Set<uint8_t>(OutLengthSite, offset);
		Memory::Set<uint8_t>(EncodeLengthSite, negated);

		HeaderArmed = extended;

		Log::WriteLine(Channel::Game, "Client header now {} bytes.", size);
		return true;
	}

	// Armed off the reader axis: an extended demo carries the same empty names, and the records that
	// fill them in are in band there too.
	static bool WriteSnapshotNames(bool extended)
	{
		if (SnapshotArmed == extended)
			return true;
		if (!SnapshotThunk)
			return false;

		if (extended)
		{
			if (Memory::Get<uint8_t>(SnapshotSite) != 0xE8
				|| SnapshotSite + 5 + Memory::Get<int32_t>(SnapshotSite + 1) != SnapshotTarget)
				return false;

			Memory::CALL(SnapshotSite, SnapshotThunk);
		}
		else
			Memory::CALL(SnapshotSite, SnapshotTarget);

		SnapshotArmed = extended;
		return true;
	}

	// Two bytes of the seven this replaces have nothing to hold, so they are left as nops rather than
	// folding the store into the thunk twice.
	static bool WriteServerCommand(bool extended)
	{
		if (ServerCommandArmed == extended)
			return true;
		if (!ServerCommandThunk)
			return false;

		if (extended)
		{
			if (!Matches(ServerCommandSite, StockServerCommand))
				return false;

			Memory::CALL(ServerCommandSite, ServerCommandThunk);
			Memory::NOP(ServerCommandSite + 5, 2);
		}
		else
			Memory::Write(ServerCommandSite, StockServerCommand);

		ServerCommandArmed = extended;
		return true;
	}

	// Driven off Current rather than what the connect packet says: an extended demo carries
	// svc_configclient in band too, and nothing about a demo goes into a connect packet.
	static bool WriteDispatch(bool extended)
	{
		if (Memory::Get<uint8_t>(DispatchBoundSite) != 0x83 || Memory::Get<uint8_t>(DispatchBoundSite + 1) != 0xFF
			|| Memory::Get<uint16_t>(DispatchTableSite) != 0x24FF
			|| Memory::Get<uint8_t>(DispatchTableSite + 2) != 0xBD)
			return false;

		if (extended && !ExtendedDispatch[0])
		{
			const auto* stock = reinterpret_cast<const uintptr_t*>(DispatchTable);
			for (int i = 0; i < ExtendedDispatchCount; i++)
				ExtendedDispatch[i] = i < StockDispatchCount ? stock[i] : DispatchIllegible;

			ExtendedDispatch[ServerConfigClient] = ASM_LOAD(ParseConfigClient_h);

			// The engine jumps through this table, so a zero in it is a jump to address zero the first
			// time that opcode arrives - checked here rather than found as a null eip in a minidump.
			for (int i = 0; i < ExtendedDispatchCount; i++)
			{
				if (ExtendedDispatch[i])
					continue;

				Log::WriteLine(Channel::Error, "svc dispatch entry {} is null; leaving the table stock.", i);
				ExtendedDispatch[0] = 0;
				return false;
			}
		}

		Memory::Set<uint8_t>(DispatchBoundSite + 2,
			static_cast<uint8_t>(extended ? ExtendedDispatchCount - 1 : StockDispatchCount - 1));
		Memory::Set<uint32_t>(DispatchTableSite + 3,
			static_cast<uint32_t>(extended ? reinterpret_cast<uintptr_t>(ExtendedDispatch) : DispatchTable));
		return true;
	}

	void GProtocol::Initialize()
	{
		// The one switch: everything else is negotiated per server. This is the way back out if a server
		// speaks something the extended reader cannot follow.
		AllowDvar = Dvar::RegisterBool("sr_extendedProtocol", DvarFlags(DVAR_SAVED | DVAR_NORESTART),
			"Speak the CoD4X extended protocol when a server offers it", true);

		// 0 until a session settles, so an idle client does not read as though it were speaking 6.
		ProtocolDvar = Dvar::RegisterInt("sr_protocol", DvarFlags(DVAR_READONLY | DVAR_NORESTART),
			"Protocol this session negotiated: 6 for stock, 21 for CoD4X, 0 for none", 0, 0, 255);

		CoD4XDvar = Dvar::Find("legacyProtocol");

		ExtendedHeaderThunk = ASM_LOAD(ExtendedHeader_h);
		OriginThunk = ASM_LOAD(ReadOriginFloat_h);
		SnapshotThunk = ASM_LOAD(CL_GetSnapshot_h);
		ServerCommandThunk = ASM_LOAD(CL_ServerCommand_h);

		AllowedLast = UsingExtended();
		Publish();
	}

	// The dispatch table and thunks live in asmjit memory that goes away with the module, so the engine
	// must not be left pointing into it. What is left behind is a stock client.
	void GProtocol::Shutdown()
	{
		if (Patch::UseCoD4X)
			return;

		Current = Protocol::Legacy;

		WriteDispatch(false);
		WriteOriginReads(false);
		WriteSnapshotNames(false);
		WriteServerCommand(false);
		WriteGameVersion(false);
		WriteHeaderSize(LegacyHeaderSize);
		WriteAdvertise(LegacyVersion);
		WriteConnectedState(CA_SENDINGSTATS);
		WriteSendInterval(ConnectedSendInterval);
	}

	void GProtocol::Frame()
	{
		if (!ProtocolDvar)
			return;

		// ParseGamestateHook cannot disconnect where it fails: it is called from inside
		// CL_ParseServerMessage, whose loop still holds pointers into the state a disconnect frees.
		if (DisconnectPending)
		{
			DisconnectPending = false;
			Com_PrintMessage(CON_CHANNEL_ERROR, "^1The extended gamestate could not be read.\n", 0);
			Cmd_ExecuteSingleCommand(0, 0, "disconnect\n");
			return;
		}
		if (Mirror())
			return;

		// Only takes effect on the next connect: rewriting the header under a server already reading
		// sixteen bytes off every packet desynchronises the acks instead of cleanly leaving.
		const bool allowed = UsingExtended();
		if (allowed != AllowedLast)
		{
			AllowedLast = allowed;

			const connstate_t now = client_ui ? client_ui->connectionState : CA_DISCONNECTED;
			if (!allowed && Current == Protocol::Extended && now < CA_CONNECTED)
			{
				Result.version = LegacyVersion;
				Apply(Protocol::Legacy);
			}
			else if (!allowed && Current == Protocol::Extended)
				Com_PrintMessage(CON_CHANNEL_CONSOLEONLY, "sr_extendedProtocol will apply on the next connect.\n", 0);
		}

		const connstate_t state = client_ui ? client_ui->connectionState : CA_DISCONNECTED;
		if (state < CA_CONNECTING)
		{
			Reset();
			return;
		}
		// Configstring 2 names the server the recording was made on, not the format the file is
		// written in, so confirming against it would undo Demo()'s answer every frame.
		if (!DemoSession && !clc.demoplaying && Confirm())
			return;
		if (Current != Protocol::Unknown)
			return;

		// A local server only speaks the stock encoding, as does a demo whose header was never read.
		if (DemoSession || clc.demoplaying || clc.serverAddress.type == NA_LOOPBACK)
			Apply(Protocol::Legacy);
	}

	void GProtocol::Connect()
	{
		DemoSession = false;
		Reset();
	}

	// A demo is not negotiated: the recorded protocol is the answer (cl_main.c:7833).
	void GProtocol::Demo(int protocol)
	{
		Result = {};
		DemoSession = true;
		Result.extended = protocol > LegacyVersion;
		Result.version = protocol > 0 ? protocol : LegacyVersion;

		Apply(Result.extended ? Protocol::Extended : Protocol::Legacy);
	}

	// Never consumes the packet: the engine still runs its own challengeResponse path, this only
	// listens in on the protocol tag.
	bool GProtocol::Inspect(const netadr_t* from, const char* packet)
	{
		// CoD4X runs the same handshake off the same packet, and two readers would only race.
		if (!packet || Mirror())
			return false;

		const std::vector<std::string> args = Tokenize(packet);
		if (args.empty() || !Equals(args[0], "challengeResponse"))
			return false;

		return ChallengeResponse(from, args);
	}

	Protocol GProtocol::Negotiated()
	{
		return Current;
	}

	bool GProtocol::IsLegacy()
	{
		return Current != Protocol::Extended;
	}

	int GProtocol::Version()
	{
		if (Current != Protocol::Extended)
			return LegacyVersion;
		return Result.version > LegacyVersion ? Result.version : ExtendedVersion;
	}

	// What the connect packet's "protocol" key must carry. Asking for the extended one without a
	// decoder in the process would earn a gamestate we cannot read.
	int GProtocol::Advertise()
	{
		return Current == Protocol::Extended && UsingExtended() ? ExtendedVersion : LegacyVersion;
	}

	// ParseGamestate below is what makes this answer safe to give without CoD4X in the process.
	bool GProtocol::UsingExtended()
	{
		if (Patch::UseCoD4X)
			return true;

		return !AllowDvar || AllowDvar->current.enabled;
	}

	const ProtocolHandshake& GProtocol::Handshake()
	{
		return Result;
	}

	// Retail's CL_ParseGamestate prologue (0x473CFA-0x473D3C). The clear of cl is what stops a
	// configstring the wire does not resend from pointing into the previous map's string data.
	void GProtocol::PrepareGamestate(int localClientNum)
	{
		if (CL_ClearState)
			CL_ClearState(localClientNum);

		clc.connectPacketCount = 0;

		if (localClientNum < 1 && clients)
			std::memset(clients, 0, ClientActiveSize);

		if (Com_ClientDObjClearAllSkel)
			Com_ClientDObjClearAllSkel();

		if (cls)
		{
			cls->mapCenter[0] = 0.0f;
			cls->mapCenter[1] = 0.0f;
			cls->mapCenter[2] = 0.0f;
		}
	}

	// Replaces retail's CL_ParseGamestate (0x473CE0), ported from CL_ParseGamestateX (cl_main.c:8256).
	// The demo path: a live server sends its gamestate down the reliable channel instead.
	void GProtocol::ParseGamestateHook(int localClientNum, msg_t* msg)
	{
		if (IsLegacy() || !msg)
		{
			CL_ParseGamestate_h(localClientNum, msg);
			return;
		}
		PrepareGamestate(localClientNum);

		if (ParseGamestate(msg))
			return;

		// A half read gamestate leaves cl unusable, and the overflow flag is what stops
		// CL_ParseServerMessage's loop (0x47483C). The disconnect waits a frame; that loop is on the stack.
		msg->overflowed = 1;
		DisconnectPending = true;
	}

	// The reliable channel's entry to the same reader. The body is the svc_gamestate byte that
	// CL_ExecuteReliableMessage pops (cl_main.c:3237-3240), then the block itself.
	void GProtocol::ReliableGamestate(uint8_t* body, int length)
	{
		if (!body || length < 1 || body[0] != ServerGamestate)
			return;

		PrepareGamestate(0);

		msg_t msg = {};
		msg.data = body;
		msg.maxsize = length;
		msg.cursize = length;
		msg.readcount = 1;
		msg.bit = 8;
		msg.lastRefEntity = -1;

		if (ParseGamestate(&msg))
			return;

		DisconnectPending = true;
	}

	// An extended server sends the value whole, so there is no delta to apply and the old value the
	// stock readers took is not needed.
	float GProtocol::ReadOriginFloat(msg_t* msg)
	{
		const int bits = ReadLong(msg);

		float value = 0.0f;
		std::memcpy(&value, &bits, sizeof(value));
		return value;
	}

	// An extended server leaves clientState_s::name empty, so every stock reader of it draws a blank.
	// Filling it in here fixes all of them at once, where chasing each would be one detour per caller.
	//
	// It must be the block CL_GetSnapshot just assembled: CG_SetNextSnap announces a rename on every
	// disagreement with clientinfo, so one that reaches it blank is a rename line and a flicker.
	//
	// The field is sixteen bytes, the engine's own limit; the untruncated name stays in ExtendedClients.
	void GProtocol::ApplySnapshotNames(void* snapshot)
	{
		if (IsLegacy() || !snapshot)
			return;

		snapshot_s* snap = static_cast<snapshot_s*>(snapshot);
		const int count = std::min(snap->numClients, MaxClients);

		for (int i = 0; i < count; i++)
		{
			clientState_s& state = snap->clients[i];
			if (state.clientIndex < 0 || state.clientIndex >= MaxClients)
				continue;

			const char* name = ExtendedClients[state.clientIndex].Name;
			if (!name[0])
				continue;

			const size_t length = std::min(std::strlen(name), sizeof(state.name) - 1);

			std::memcpy(state.name, name, length);
			state.name[length] = '\0';
		}
	}

	// Seen as they are stored rather than when cgame gets to them. Only the commands CoD4X runs early
	// are handled (CL_ExecuteServerCommand, cl_main.c:7198).
	//
	// The string is left exactly as it arrived. Blanking it would spare cgame an "Unknown client game
	// command", the line CoD4X leaves commented out beside its own switch, but the stored command also
	// feeds the usercmd key (cl_main.c:4352):
	//
	//   key ^= Com_HashKey(clc.serverCommands[clc.serverCommandSequence & 0x7F], 32)
	//
	// and the server hashes its own copy (SV_UserMove). Blanking one side leaves the keys disagreeing
	// for every usercmd until a newer command lands - the seconds after a map change or fast restart.
	void GProtocol::ServerCommand(char* command)
	{
		if (!command || IsLegacy() || clc.demoplaying)
			return;

		switch (command[0])
		{
		case 'l':
			LoadingNewMap(command);
			break;

		// The in band replacement for the out of band "fastrestart" packet, and the same test retail
		// makes on that one (0x46C011): a restart announced before the map is live is not one.
		case 'm':
			clc.isServerRestarting = client_ui && client_ui->connectionState == CA_ACTIVE;
			break;

		default:
			break;
		}
	}

	// Retail's own loadingnewmap handler (0x46BF2F), off the reliable command sent in its place.
	// Dropping to CA_CONNECTED is what stops the old map being drawn.
	void GProtocol::LoadingNewMap(const char* command)
	{
		// A client still fetching files is not going anywhere; retail ignores it for the same reason.
		if (!cls || cls->downloadName[0])
			return;

		const std::vector<std::string> args = Tokenize(command);
		if (args.size() < 3 || args[1].empty() || args[2].empty())
			return;

		Log::WriteLine(Channel::Game, "Server is changing map to {} ({}).", args[1], args[2]);

		if (UI_CloseAllMenus)
			UI_CloseAllMenus(0);

		if (Cbuf_AddText)
			Cbuf_AddText(0, "uploadStats\n");

		if (client_ui)
			client_ui->connectionState = CA_CONNECTED;

		if (CL_SetupForNewServerMap)
			CL_SetupForNewServerMap(args[1].c_str(), args[2].c_str());
	}

	bool GProtocol::ParseGamestate(void* message)
	{
		msg_t* msg = static_cast<msg_t*>(message);

		if (!msg || !msg->data || !clients || !MSG_ReadDeltaEntity)
			return false;
		if (msg->readcount < 0 || msg->cursize < 0 || msg->splitSize < 0)
			return false;

		// Read back by index rather than walked, so a stale offset from the last map would survive.
		std::memset(&ExtendedGameState, 0, sizeof(ExtendedGameState));
		std::memset(ExtendedClients, 0, sizeof(ExtendedClients));
		ExtendedConfigDataSequence = 0;

		// MSG_ClearLastReferencedEntity: baselines delta against the entity the message last named,
		// and a gamestate starts having named none.
		msg->lastRefEntity = -1;

		// Offset 0 is left empty so an unset configstring reads back as the empty string.
		clients->gameState.dataCount = 1;
		ExtendedGameState.dataCount = 1;

		clc.serverCommandSequence = ReadLong(msg);

		// One huge configstring command then a short tail of baselines, which is where a reader that
		// drifted a byte first shows it. The cap keeps a healthy gamestate to a handful of lines.
		constexpr int MaxTracedCommands = 24;
		int traced = 0;

		while (true)
		{
			const int began = msg->readcount;
			const int command = ReadByte(msg);
			if (command == ServerEndOfFile)
				break;

			bool parsed = false;
			switch (command)
			{
			case ServerConfigString:
				parsed = ReadConfigStrings(msg);
				break;
			case ServerBaseline:
				parsed = ReadBaseline(msg);
				break;
			case ServerConfigClient:
				parsed = ReadConfigClient(msg);
				break;
			default:
				parsed = false;
				break;
			}

			if (!parsed || msg->overflowed)
			{
				Log::WriteLine(Channel::Error,
					"Extended gamestate: command byte {} at {} did not parse, stopped at {}/{}.", command, began,
					msg->readcount, msg->cursize);
				DumpGamestate(msg, began);
				return false;
			}

			if (traced++ < MaxTracedCommands)
				Log::WriteLine(Channel::Game, "Gamestate command {} at {}, ended {}, entity {}, bit {}.", command,
					began, msg->readcount, msg->lastRefEntity, msg->bit);
		}

		// Retail's clientConnection_t has no field for this, so it is kept here rather than written over
		// the field that follows.
		ExtendedConfigDataSequence = ReadLong(msg);

		const int clientNum = ReadLong(msg);
		const int checksumFeed = ReadLong(msg);

		// The public CoD4X build leaves FALLBACK_SIGNALING off, so no pure checksum follows the feed.
		if (msg->overflowed || clientNum < 0 || clientNum >= MaxClients)
		{
			Log::WriteLine(Channel::Error, "Extended gamestate: bad client number {}.", clientNum);
			return false;
		}

		clc.clientNum = clientNum;
		clc.checksumFeed = checksumFeed;

		Log::WriteLine(Channel::Game, "Extended gamestate read: client {}, configdata {}.", clientNum,
			ExtendedConfigDataSequence);

		// Retail's own tail, 0x47418F to 0x474261. CL_SystemInfoChanged fills cl.serverId and
		// CL_DownloadsComplete (0x46AC60) carries the connection past the gamestate. Not reproduced: the
		// download pass at 0x474234, which retail skips for an unpure server on this machine or the LAN.

		// A demo named on the command line gets here a frame after Com_Init, with the boot fastfiles
		// still streaming, so the wait is not optional: FS_Restart would pull the search path out from
		// under the loader thread and the first unreopenable image takes the process down.
		if (const dvar_s* fastFiles = Dvar::Find("com_useFastFiles"); !fastFiles || fastFiles->current.enabled)
			DB_SyncXAssets();

		SystemInfoChanged();

		// The first long of every client packet header. SV_ExecuteClientMessage silently stops sending
		// snapshots when its top 24 bits do not match sv.start_frameTime (sv_client.c:1893).
		Log::WriteLine(Channel::Game, "cl.serverId {:#x}, systeminfo \"{}\".", Memory::Get<uint32_t>(ServerIdAddress),
			ConfigString(1));

		// Retail's order: the pure reload first, then the downloads that may need the new set.
		const bool hosting = ServerRunning && *ServerRunning && (*ServerRunning)->current.enabled;
		const bool moved = GameDirVar && *GameDirVar && (*GameDirVar)->modified;

		// Latched before FS_Restart, which clears the modified flag this reads. Without it a connect to a
		// mod server keeps the zones it booted with and the mod fastfile never loads.
		if (moved)
			*GameDirChanged = 1;

		if (!hosting && FS_Restart && (moved || !LoadedChecksumFeed || *LoadedChecksumFeed != checksumFeed))
			FS_Restart(0, checksumFeed);

		if (CL_InitDownloads)
			CL_InitDownloads(0);

		// Retail clears this last, after CL_InitDownloads has possibly started the map load (0x474261).
		if (dvar_s* paused = Dvar::Find("cl_paused"))
			paused->current.integer = 0;

		return true;
	}

	bool GProtocol::ReadConfigStrings(msg_t* msg)
	{
		const int count = ReadLong(msg);
		if (count <= 0 || count > MaxExtendedConfigStrings)
			return false;

		const int began = msg->readcount;

		int index = -1;
		int cursor = 0;

		for (int i = 0; i < count; i++)
		{
			// Strictly increasing is what lets one forward pass over the constant table cover the block,
			// and the only thing stopping a crafted gamestate rewriting an earlier entry.
			const int next = ReadLong(msg);
			if (next <= index || next >= MaxExtendedConfigStrings)
				return false;

			index = next;

			if (!FillConstantConfigStrings(cursor, index))
				return false;

			// The wire gives this index a value of its own, so the constant is dropped.
			if (cursor < MaxConstantConfigStrings && ConstantConfigStrings[cursor].index == index)
				cursor++;

			int slot = 0;
			gameState_t* table = TableFor(index, slot);

			if (table->dataCount < 1 || table->dataCount >= MaxGameStateChars)
				return false;

			// Read straight into the table, which is what caps the string at the room left in it.
			char* text = &table->stringData[table->dataCount];
			const int room = std::min(BigInfoString, MaxGameStateChars - table->dataCount);

			const int length = ReadString(msg, text, room);
			if (length < 0)
				return false;

			table->stringOffsets[slot] = table->dataCount;
			table->dataCount += length + 1;
		}

		if (!FillConstantConfigStrings(cursor, MaxExtendedConfigStrings))
			return false;

		Log::WriteLine(Channel::Game, "Configstrings: {} entries, last index {}, {} bytes consumed, at {}/{}.", count,
			index, msg->readcount - began, msg->readcount, msg->cursize);

		ReadMapCenter();
		return true;
	}

	bool GProtocol::ReadBaseline(msg_t* msg)
	{
		const int number = ReadEntityIndex(msg, 10);
		if (msg->overflowed || number < 0 || number >= MaxEntities)
			return false;

		const entityState_s nullstate = {};
		MSG_ReadDeltaEntity(msg, &clients->entityBaselines[number], &nullstate, number);

		return !msg->overflowed;
	}

	bool GProtocol::ReadConfigClient(msg_t* msg)
	{
		const int clientNum = ReadByte(msg);
		if (clientNum < 0 || clientNum >= MaxClients)
			return false;

		ExtendedClient& client = ExtendedClients[clientNum];

		if (ReadString(msg, client.Name, static_cast<int>(sizeof(client.Name)), true) < 0)
			return false;

		if (ReadString(msg, client.Clantag, static_cast<int>(sizeof(client.Clantag)), true) < 0)
			return false;

		Log::WriteLine(Channel::Game, "Configclient {}: name \"{}\", clantag \"{}\".", clientNum, client.Name,
			client.Clantag);
		return true;
	}

	// In band svc_configclient, sent whenever a name or clantag changes (sv_snapshot.c:339). The
	// gamestate carries the same rows without the message number, which is why ReadConfigClient does
	// not read one.
	//
	// The message is retail's, so leaving readcount where the read ended is what lets
	// CL_ParseServerMessage carry on, and overflowed is what stops it.
	void GProtocol::ParseConfigClient(msg_t* msg)
	{
		if (!msg)
			return;

		const int messageNum = ReadLong(msg);
		const int clientNum = ReadByte(msg);

		// Strictly in order; anything else drains without advancing (cl_main.c:7261-7273). Acknowledging
		// a number never received makes the server's resend loop skip those rows for good
		// (sv_snapshot.c:375).
		const bool keep = messageNum == ExtendedConfigDataSequence + 1 && clientNum >= 0 && clientNum < MaxClients;

		// Consumed either way: the dispatch loop reads the next opcode at msg->readcount, so a skipped
		// record still has to be walked past.
		ExtendedClient discard = {};
		ExtendedClient& target = keep ? ExtendedClients[clientNum] : discard;

		if (ReadString(msg, target.Name, static_cast<int>(sizeof(target.Name)), true) < 0
			|| ReadString(msg, target.Clantag, static_cast<int>(sizeof(target.Clantag)), true) < 0)
		{
			msg->overflowed = 1;
			return;
		}

		if (keep)
			ExtendedConfigDataSequence = messageNum;
	}

	// The compass origin is three floats in a configstring, not in the block, so it is only known
	// once the configstrings are in (cl_main.c:8416).
	void GProtocol::ReadMapCenter()
	{
		if (!cls)
			return;

		const char* text = ConfigString(MapCenterConfigString);
		char* end = nullptr;

		for (float& axis : cls->mapCenter)
		{
			const float value = std::strtof(text, &end);
			if (end == text)
				return;

			axis = value;
			text = end;
		}
	}

	// The protocol number is the CoD4X release number, not a format revision: PROTOCOL_VERSION is
	// (unsigned)atof(UPDATE_VERSION_NUM) (q_shared.h:61), so every release bumps it whether or not
	// anything moved. Across 19.3, 20.5 and 21.3 CL_ParseGamestateX reads the same opcodes in the same
	// order and msg.c's delta tables are unchanged, so one reader covers all three. 18 comes in on
	// CoD4X's own pairing, which plays an 18 demo on the 19 client (demoprotocolinfo, cl_main.c:7740).
	//
	// 17 is in on the same footing, once the raw origin reads are held back from it - see
	// SendsRawOrigins. Its delta encoding is otherwise the same: bin/cod4x_017/cod4x_017.dll carries
	// its netField_t tables with the field names in them, and all 59 entityState and 141 playerState
	// entries match 21.3 exactly, names, order and bit counts.
	//
	// Anything older still takes CoD4X's relaunch path into bin/cod4x_0NN.
	bool GProtocol::CanParse(int protocol)
	{
		return (protocol > 0 && protocol <= LegacyVersion)
			|| (protocol >= OldestExtendedVersion && protocol <= ExtendedVersion);
	}

	// CL_GetConfigString over both tables (cl_main.c:6900).
	const char* GProtocol::ExtendedConfigString(int index)
	{
		if (index < MaxConfigStrings)
			return ConfigString(index);
		if (index >= MaxExtendedConfigStrings)
			return "";

		const int offset = ExtendedGameState.stringOffsets[index - MaxConfigStrings];
		if (offset <= 0 || offset >= MaxGameStateChars)
			return "";

		return &ExtendedGameState.stringData[offset];
	}

	const char* GProtocol::ClientName(int clientNum)
	{
		if (clientNum < 0 || clientNum >= MaxClients)
			return "";

		return ExtendedClients[clientNum].Name;
	}

	const char* GProtocol::ClientClantag(int clientNum)
	{
		if (clientNum < 0 || clientNum >= MaxClients)
			return "";

		return ExtendedClients[clientNum].Clantag;
	}

	int GProtocol::ConfigDataSequence()
	{
		return ExtendedConfigDataSequence;
	}

	void GProtocol::Apply(Protocol protocol)
	{
		if (Current == protocol)
			return;

		Current = protocol;
		Publish();

		if (protocol == Protocol::Unknown)
			return;

		// The one line that says whether the negotiation engaged, and if not, which half declined.
		Log::WriteLine(Channel::Game, "Negotiated protocol {} ({}). Server offered {}, extended protocol {}.",
			Version(), protocol == Protocol::Legacy ? "legacy" : "extended", Result.extended ? "extended" : "legacy",
			UsingExtended() ? "allowed" : "disabled");
	}

	// Armed or restored as one unit. Half armed is worse than legacy either way: a 21 behind a nine
	// byte header makes the server read four longs out of nine and start its decode window seven bytes
	// early, and the mirror image is the same. A failure anywhere puts all of it back.
	//
	// The reader and the wire are separate axes: a demo needs the extended dispatch table but must
	// never advertise anything, being decoded and never dialled.
	bool GProtocol::Arm(bool reader, bool live)
	{
		if (!WriteDispatch(reader))
			return false;

		if (!WriteOriginReads(reader && SendsRawOrigins()) || !WriteSnapshotNames(reader) || !WriteServerCommand(reader)
			|| !WriteGameVersion(reader) || !WriteHeaderSize(live ? ExtendedHeaderSize : LegacyHeaderSize)
			|| !WriteAdvertise(live ? ExtendedVersion : LegacyVersion))
		{
			WriteDispatch(false);
			WriteOriginReads(false);
			WriteSnapshotNames(false);
			WriteServerCommand(false);
			WriteGameVersion(false);
			WriteHeaderSize(LegacyHeaderSize);
			WriteAdvertise(LegacyVersion);
			WriteConnectedState(CA_SENDINGSTATS);
			WriteSendInterval(ConnectedSendInterval);
			return false;
		}

		// Skipping the stats phase without the connect packet going out as 21 would strand the
		// connection, so that half only arms behind a successful write.
		WriteConnectedState(live ? CA_CONNECTED : CA_SENDINGSTATS);
		WriteSendInterval(live ? ExtendedSendInterval : ConnectedSendInterval);
		return true;
	}

	void GProtocol::Publish()
	{
		// The bytes settle first: the dvars report what the engine managed, not what was asked for.
		if (!Patch::UseCoD4X)
		{
			const bool reader = Current == Protocol::Extended;
			const bool wire = reader && !DemoSession && !clc.demoplaying && UsingExtended();

			if (!Arm(reader, wire))
			{
				if (!ArmWarned)
				{
					ArmWarned = true;
					Log::WriteLine(Channel::Error, "The engine does not decode as stock 1.7; staying on protocol {}.",
						LegacyVersion);
				}
				Current = Protocol::Legacy;
				Result.extended = false;
				Result.version = LegacyVersion;
			}
		}
		const int version = Current == Protocol::Unknown ? 0 : Version();

		if (ProtocolDvar)
		{
			ProtocolDvar->current.integer = version;
			ProtocolDvar->latched.integer = version;
		}
	}

	void GProtocol::Reset()
	{
		if (Current == Protocol::Unknown && !Result.extended && !Result.version)
			return;

		Current = Protocol::Unknown;
		Result = {};
		DemoSession = false;

		// The fourth long of every extended client packet header, armed from the challenge response. A
		// second connect would otherwise acknowledge the previous session's configdata and the server
		// would stop resending (sv_snapshot.c:363).
		ExtendedConfigDataSequence = 0;

		Publish();
	}

	bool GProtocol::Mirror()
	{
		if (!Patch::UseCoD4X)
			return false;

		if (!CoD4XDvar)
			CoD4XDvar = Dvar::Find("legacyProtocol");

		if (!CoD4XDvar)
			return false;

		const bool legacy = CoD4XDvar->current.enabled;
		Result.extended = !legacy;
		if (!legacy && Result.version <= LegacyVersion)
			Result.version = ExtendedVersion;

		Apply(legacy ? Protocol::Legacy : Protocol::Extended);
		return true;
	}

	// Second opinion, from the version tag in the gamestate. It says what the server is, not what it
	// sent us: one configstring serves every client, so a session that asked for 6 still reads "cod21"
	// off an extended server and has to stay legacy.
	bool GProtocol::Confirm()
	{
		const char* tag = ConfigString(VersionConfigString);
		if (!Equals(std::string_view(tag).substr(0, 3), "cod"))
			return false;

		int version = 0;
		const std::string_view digits = std::string_view(tag).substr(3);
		std::from_chars(digits.data(), digits.data() + digits.size(), version);

		Result.extended = version > LegacyVersion;

		// Only the demotion is acted on: the connect packet's protocol was settled in ChallengeResponse,
		// so promoting here would aim the extended reader at a gamestate encoded for protocol 6.
		const bool extended = Current == Protocol::Extended && Result.extended && UsingExtended();
		Result.version = extended ? version : LegacyVersion;

		Apply(extended ? Protocol::Extended : Protocol::Legacy);
		return true;
	}

	bool GProtocol::ChallengeResponse(const netadr_t* from, const std::vector<std::string>& args)
	{
		// Retail only accepts a challengeResponse while CA_CONNECTING (0x46B66E), moving to
		// CA_CHALLENGING at 0x46B6D2. This hook sits on CL_ConnectionlessPacket's entry, before that move,
		// so without the same test any datagram from the server's host could flip a settled session.
		if (!client_ui || client_ui->connectionState != CA_CONNECTING)
			return false;
		if (from && (!SameBaseAddress(*from, clc.serverAddress) || from->port != clc.serverAddress.port))
			return false;

		Result = {};
		if (args.size() > 1)
			std::from_chars(args[1].data(), args[1].data() + args[1].size(), Result.challenge);

		// CoD4X answers "challengeResponse <challenge> <clientChallenge> 0 xproto <branch>" where a stock
		// server stops after the challenge. The trailing token is the branch, not a protocol number: the
		// extended wire version is fixed (sv_client.c:124).
		Result.extended = args.size() > 4 && Equals(args[4], "xproto");
		Result.version = Result.extended ? ExtendedVersion : LegacyVersion;

		// A listen server only ever speaks the stock encoding (cl_main.c:2856).
		const bool loopback = clc.serverAddress.type == NA_LOOPBACK;
		const bool extended = Result.extended && !loopback && UsingExtended();

		Apply(extended ? Protocol::Extended : Protocol::Legacy);

		if (Result.extended && !extended && !loopback)
			Log::WriteLine(Channel::Game, "Server offers protocol {}; connecting with {} instead.", Result.version,
				LegacyVersion);
		return true;
	}

	// The engine builds paths out of this and the file was written elsewhere, so only the characters a
	// mod folder is actually spelled with go through.
	static bool IsGameDirSafe(std::string_view value)
	{
		if (value.empty() || value.size() >= MaxGameDir || value.contains(".."))
			return false;

		return std::ranges::all_of(value,
			[](char c)
			{
				return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '_'
					|| c == '-' || c == '/';
			});
	}

	// An info string is \key\value pairs and a value may hold spaces, so Tokenize is no help here.
	static std::string InfoValue(const char* info, std::string_view key)
	{
		if (!info)
			return {};

		const std::string_view text(info, strnlen(info, MaxInfoString));
		size_t i = 0;

		while (i < text.size())
		{
			const size_t name = text.find_first_not_of('\\', i);
			if (name == std::string_view::npos)
				break;

			const size_t split = text.find('\\', name);
			if (split == std::string_view::npos)
				break;

			const size_t end = text.find('\\', split + 1);
			if (text.substr(name, split - name) == key)
				return std::string(text.substr(split + 1, end - split - 1));

			i = end;
		}
		return {};
	}

	// Retail's CL_SystemInfoChanged applies no systeminfo dvar at all while a demo plays (0x473B07),
	// the Q3 rule it inherited. fs_game has to land anyway: without it the mod fastfile never loads,
	// the mod's iwds stay off the search path, and DB_TryLoadXFile skips the usermaps branch it gates
	// on fs_game (0x48A9CE).
	//
	// Hooked rather than called from the readers so the stock gamestate tail (0x4741AD) is covered too;
	// a legacy demo never reaches the extended reader. Retail latches fs_gameDirVar->modified straight
	// after this returns (0x4741B2), which is what turns the switch into a zone reload.
	void GProtocol::SystemInfoChanged()
	{
		CL_SystemInfoChanged_h();

		if (DemoSession || clc.demoplaying)
			DemoGameDir(ConfigString(1));
	}

	// Retail uses the same setter (0x473C54), source 0 for a value that did not come from the console.
	void GProtocol::DemoGameDir(const char* systeminfo)
	{
		const std::string game = InfoValue(systeminfo, "fs_game");
		if (game.empty())
			return;

		if (!IsGameDirSafe(game))
		{
			Log::WriteLine(Channel::Warning, "Demo names an unusable fs_game \"{}\"; leaving it alone.", game);
			return;
		}

		if (const dvar_s* current = Dvar::Find("fs_game");
			current && current->current.string && game == current->current.string)
			return;

		Log::WriteLine(Channel::Game, "Demo was recorded under fs_game \"{}\"; switching to it.", game);
		Dvar_SetFromStringByNameFromSource("fs_game", game.c_str(), 0);
	}

	const char* GProtocol::ConfigString(int index)
	{
		if (!clients || index < 0 || index >= static_cast<int>(std::size(clients->gameState.stringOffsets)))
			return "";

		const int offset = clients->gameState.stringOffsets[index];
		if (offset <= 0 || offset >= static_cast<int>(std::size(clients->gameState.stringData)))
			return "";

		return &clients->gameState.stringData[offset];
	}

	std::vector<std::string> GProtocol::Tokenize(const char* packet)
	{
		size_t length = 0;
		while (length < MaxPacketLength && packet[length])
			length++;

		const std::string_view text(packet, length);
		const auto space = [](char c) { return c == ' ' || c == '\t' || c == '\n' || c == '\r'; };

		std::vector<std::string> args;
		size_t i = 0;

		while (i < text.size() && args.size() < MaxPacketTokens)
		{
			while (i < text.size() && space(text[i]))
				i++;
			if (i >= text.size())
				break;

			if (text[i] == '"')
			{
				const size_t start = ++i;
				while (i < text.size() && text[i] != '"')
					i++;

				args.emplace_back(text.substr(start, i - start));
				if (i < text.size())
					i++;
				continue;
			}

			const size_t start = i;
			while (i < text.size() && !space(text[i]))
				i++;

			args.emplace_back(text.substr(start, i - start));
		}
		return args;
	}

	// Protocol tokens are ASCII, so the fold does not need a locale.
	bool GProtocol::Equals(std::string_view a, std::string_view b)
	{
		const auto lower = [](char c) { return c >= 'A' && c <= 'Z' ? static_cast<char>(c + 32) : c; };

		return a.size() == b.size()
			&& std::equal(a.begin(), a.end(), b.begin(), [&](char x, char y) { return lower(x) == lower(y); });
	}

	// Base address only, so a server that answers from another port still counts as itself.
	bool GProtocol::SameBaseAddress(const netadr_t& a, const netadr_t& b)
	{
		if (a.type != b.type)
			return false;
		if (a.type == NA_LOOPBACK || a.type == NA_BOT)
			return true;

		return std::equal(std::begin(a.ip), std::end(a.ip), std::begin(b.ip));
	}
}

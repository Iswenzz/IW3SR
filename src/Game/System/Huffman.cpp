#include "Huffman.hpp"
#include "Patch.hpp"

namespace IW3SR
{
	constexpr uintptr_t DecompressCall = 0x52D12D;
	constexpr uintptr_t MSG_ReadBitsCompressAddress = 0x5053C0;

	constexpr int OutputSize = 0x800;
	constexpr int MaxCompressedSize = 0x20000;
	constexpr int MaxExpansion = 8;

	void GHuffman::Initialize()
	{
		if (Patch::UseCoD4X || Installed)
			return;

		if (Memory::Get<uint8_t>(DecompressCall) != 0xE8)
		{
			Log::WriteLine(Channel::Error, "No call to redirect at {:#x}, the Huffman bounds check is off.",
				DecompressCall);
			return;
		}
		// Anything but the stock decoder is someone else's fix for the same hole; stacking two would
		// leave the second decoding out of the first one's staging buffer.
		const int32_t relative = Memory::Get<int32_t>(DecompressCall + 1);
		const uintptr_t target = static_cast<uintptr_t>(static_cast<intptr_t>(DecompressCall) + 5 + relative);

		if (target != MSG_ReadBitsCompressAddress)
		{
			Log::WriteLine(Channel::Error,
				"The call at {:#x} already points at {:#x}, the Huffman bounds check is off.", DecompressCall, target);
			return;
		}
		// Worst case once, so the decode path never allocates and cannot throw bad_alloc back across
		// the thunk, which has no unwind data.
		Staging.resize(static_cast<size_t>(MaxCompressedSize) * MaxExpansion);

		Memory::CALL(DecompressCall, ASM_LOAD(MSG_ReadBitsCompress_h));
		Installed = true;
	}

	// The original stops on the input, never on the output, so it is only safe to call with a buffer
	// it cannot fill. Nothing may throw: the thunk has no unwind data and the retail caller no
	// handler, so an escaping exception would terminate instead of dropping the packet.
	int GHuffman::Decompress(const uint8_t* input, uint8_t* output, int readsize)
	{
		try
		{
			if (!input || !output || readsize <= 0 || readsize > MaxCompressedSize || !MSG_ReadBitsCompress)
				return 0;

			int decoded = MSG_ReadBitsCompress(input, Staging.data(), readsize);
			if (decoded <= 0)
				return 0;

			if (decoded > OutputSize)
			{
				if (!Reported)
				{
					Log::WriteLine(Channel::Warning, "A client message decoded to {} bytes, truncated to {}.",
						decoded, OutputSize);
					Reported = true;
				}
				decoded = OutputSize;
			}
			std::memcpy(output, Staging.data(), static_cast<size_t>(decoded));
			return decoded;
		}
		catch (...)
		{
			return 0;
		}
	}
}


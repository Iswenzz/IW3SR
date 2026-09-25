#pragma once
#include "Game/Base.hpp"

struct OpusDecoder;
struct OpusEncoder;
struct SpeexBits;
struct SRC_STATE_tag;

namespace IW3SR
{
	// The low bits of the first byte of every voice packet on a relay server, whichever way it travels.
	enum class VoiceCodec : uint8_t
	{
		Speex,
		Opus
	};

	// One ear of the binaural panner.
	struct VoiceEar
	{
		float Delay = 0.0f;
		float Gain = 1.0f;
		float Coefficient = 1.0f;
		float Filtered = 0.0f;
	};

	// One talker's decoders, both ending in 48 kHz stereo, and their panner. Both codecs predict each frame
	// from the one before, so two talkers through one decoder garble each other, which is what retail's
	// single decoder does.
	class VoiceStream
	{
	public:
		VoiceStream();
		~VoiceStream();

		VoiceStream(const VoiceStream&) = delete;
		VoiceStream& operator=(const VoiceStream&) = delete;

		std::vector<int16_t>& Decode(VoiceCodec codec, const uint8_t* data, int size);
		void Spatialize(std::vector<int16_t>& pcm, std::optional<float> azimuth);

	private:
		std::array<float, 64> History = {};
		int HistoryAt = 0;
		std::array<VoiceEar, 2> Ears = {};

		float Delayed(float delay) const;

		void* Speex = nullptr;
		SpeexBits* Bits = nullptr;
		int SpeexFrameSize = 0;
		SRC_STATE_tag* Upsampler = nullptr;

		OpusDecoder* Opus = nullptr;

		std::vector<int16_t> Narrow;
		std::vector<float> NarrowFloat;
		std::vector<float> Resampled;
		std::vector<int16_t> Output;
	};

	// Voice captured at 48 kHz and played at 48 kHz in stereo. On a server that relays it, IW3SR clients
	// talk Opus to each other and the server hands stock clients Speex; anywhere else they send Speex
	// ultra-wideband, which every client reads.
	class GVoice
	{
	public:
		static constexpr int Rate = 48000;
		static constexpr int CaptureBytesPerSecond = Rate * 2;
		static constexpr int PlaybackChannels = 2;
		static constexpr int PlaybackBytesPerSecond = Rate * PlaybackChannels * 2;

		static void Initialize();

		static int QueueAudioData(audioSample_t* sample);
		static void IncomingVoiceData(uint8_t talker, uint8_t* data, int size);

	private:
		static inline bool Installed = false;
		static inline bool Relay = false;

		static inline OpusEncoder* Encoder = nullptr;
		static inline void* SpeexEncoder = nullptr;
		static inline SpeexBits* EncoderBits = nullptr;
		static inline int SpeexFrameSize = 0;
		static inline SRC_STATE_tag* Downsampler = nullptr;
		static inline std::vector<float> Captured;
		static inline std::vector<float> Narrow;
		static inline std::vector<int16_t> Frame;
		static inline std::array<std::unique_ptr<VoiceStream>, 64> Streams;
		static inline std::array<void*, 64> StereoBuffers = {};

		static void RefreshRelay();
		static bool UseStereoBuffer(int talker, dsound_sample_t* sample);
		static bool CreateEncoder();
		static bool CreateSpeexEncoder();
		static void ResetCapture();
		static int EncodeOpus();
		static int EncodeSpeex();
	};
}

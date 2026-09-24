#include "Voice.hpp"

#include <mmsystem.h>

#include <dsound.h>
#include <mmdeviceapi.h>
#include <opus/opus.h>
#include <samplerate.h>
#include <speex/speex.h>

namespace IW3SR
{
	// Speex ultra-wideband runs at four times the retail 8192 Hz, so the narrowband layer a stock client
	// plays keeps its pitch.
	constexpr int SpeexRate = 8192 * 4;
	constexpr int UltraWideband = 2;
	constexpr int Quality = 10;
	constexpr int Complexity = 10;

	constexpr int OpusFrameSize = GVoice::Rate / 50;
	constexpr int OpusMaxFrameSize = GVoice::Rate * 120 / 1000;
	constexpr int OpusBitrate = 64000;

	// Codec, then the gain the server applies for proximity, where 64 is unity. MSG_WriteByte carries
	// the size, so a packet tops out at 255 bytes.
	constexpr int HeaderSize = 2;
	constexpr int UnityGain = 64;
	constexpr int MaxPacketSize = 255;

	constexpr int MaxTalkers = 64;
	constexpr int PlaybackBufferBytes = GVoice::PlaybackBytesPerSecond;
	constexpr uintptr_t SpeexModeEncoderCtl = 0x2C;

	// Encode_Init reads the bandwidth setting Voice_Init just zeroed, and stores it back for Decode_Init.
	constexpr uintptr_t EncodeInitBandwidth = 0x4ECA63;
	constexpr uintptr_t EncodeSetOptionsCall = 0x4ECAC5;
	// Encode_Sample's jump over re-applying sv_voiceQuality whenever it differs from the encoder's.
	constexpr uintptr_t EncodeQualityJump = 0x4ECB0E;

	constexpr uintptr_t CaptureByteRateSite = 0x4ED639;
	constexpr uintptr_t CaptureRate = 0x4ED654;

	using SpeexCtl = int(__cdecl*)(void* state, int request, void* value);
	using EncodeSample_t = int(__cdecl*)(const int16_t* input, uint8_t* output, int size);
	using SendVoiceData_t = int(__cdecl*)(const uint8_t* data, int bytes);
	using UpdateSample_t = void(__cdecl*)(dsound_sample_t* sample, const void* data, int length);

	static void** EncoderState = Signature(0xD5EC440);
	static int* EncoderFrameSize = Signature(0xD5EC444);
	static int* EncoderQuality = Signature(0x724ED0);
	static int* EncoderSampleRate = Signature(0x724ED4);
	static dsound_sample_t** ClientSamples = Signature(0xCC1B4D0);
	static IDirectSound** DirectSound = Signature(0xD5EC44C);
	static float* MicScaler = Signature(0x71FB08);
	static float* VoiceLevel = Signature(0xCC1B4CC);
	static uint8_t* TalkKeyHeld = Signature(0x8F176C);

	static Function<bool()> Voice_SendVoiceData = 0x46C850;
	static Function<void()> CL_VoiceTransmit = 0x46C780;

	static EncodeSample_t EncodeSample = nullptr;
	static SendVoiceData_t SendVoiceData = nullptr;
	static UpdateSample_t UpdateSample = nullptr;

	// Both called as retail calls them, so they cover CoD4X's Voice_Init as well as the stock one.
	static Hook<int(audioSample_t* sample)> Record_QueueAudioDataForEncoding_h(0x4ED320, GVoice::QueueAudioData);
	static Hook<void(uint8_t talker, uint8_t* data, int size)>
		Voice_IncomingVoiceData_h(0x57AF60, GVoice::IncomingVoiceData);

	static HRESULT STDCALL CreateCapture(LPCGUID device, LPDIRECTSOUNDCAPTURE* capture, LPUNKNOWN outer);

	// dsound.dll's own export, so it catches CoD4X's SND_InitDSCaptureDevice as well as retail's.
	static Hook<HRESULT STDCALL(LPCGUID device, LPDIRECTSOUNDCAPTURE* capture, LPUNKNOWN outer)>
		DirectSoundCaptureCreate_h(DirectSoundCaptureCreate, CreateCapture);

	// PKEY_AudioEndpoint_GUID and PKEY_Device_FriendlyName, which the SDK only defines under INITGUID.
	constexpr PROPERTYKEY EndpointGuidKey = {
		{ 0x1DA5D803, 0xD492, 0x4EDD, { 0x8C, 0x23, 0xE0, 0xC0, 0xFF, 0xEE, 0x7F, 0x0E } }, 4
	};
	constexpr PROPERTYKEY FriendlyNameKey = {
		{ 0xA45C254E, 0xDF1C, 0x4EFD, { 0x80, 0x20, 0x67, 0xD1, 0x46, 0xA8, 0x50, 0xE0 } }, 14
	};

	static std::string ToUtf8(const wchar_t* text)
	{
		const int size = WideCharToMultiByte(CP_UTF8, 0, text, -1, nullptr, 0, nullptr, nullptr);
		if (size <= 1)
			return {};

		std::string result(size - 1, '\0');
		WideCharToMultiByte(CP_UTF8, 0, text, -1, result.data(), size, nullptr, nullptr);
		return result;
	}

	// Windows' default recording device, as DirectSound identifies it.
	static bool DefaultCaptureDevice(GUID& device, std::string& name)
	{
		const HRESULT apartment = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);

		IMMDeviceEnumerator* enumerator = nullptr;
		IMMDevice* endpoint = nullptr;
		IPropertyStore* properties = nullptr;
		bool found = false;

		if (SUCCEEDED(CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_ALL, __uuidof(IMMDeviceEnumerator),
				reinterpret_cast<void**>(&enumerator)))
			&& SUCCEEDED(enumerator->GetDefaultAudioEndpoint(eCapture, eConsole, &endpoint))
			&& SUCCEEDED(endpoint->OpenPropertyStore(STGM_READ, &properties)))
		{
			PROPVARIANT value;
			PropVariantInit(&value);
			if (SUCCEEDED(properties->GetValue(EndpointGuidKey, &value)) && value.vt == VT_LPWSTR)
				found = SUCCEEDED(CLSIDFromString(value.pwszVal, &device));
			PropVariantClear(&value);

			if (SUCCEEDED(properties->GetValue(FriendlyNameKey, &value)) && value.vt == VT_LPWSTR)
				name = ToUtf8(value.pwszVal);
			PropVariantClear(&value);
		}
		if (properties)
			properties->Release();
		if (endpoint)
			endpoint->Release();
		if (enumerator)
			enumerator->Release();
		if (SUCCEEDED(apartment))
			CoUninitialize();

		return found;
	}

	// Retail and CoD4X both ask for the default device with NULL. Named explicitly instead, so it is the
	// device Windows lists as default, and the log says which one it is.
	static HRESULT STDCALL CreateCapture(LPCGUID device, LPDIRECTSOUNDCAPTURE* capture, LPUNKNOWN outer)
	{
		GUID fallback = {};
		std::string name;

		if (!device && DefaultCaptureDevice(fallback, name))
		{
			Log::WriteLine(Channel::Game, "Voice captures from \"{}\".", name);
			device = &fallback;
		}
		return DirectSoundCaptureCreate_h(device, capture, outer);
	}

	// speex_encoder_ctl, which retail inlines: every encoder state opens with its SpeexMode.
	static int EncoderCtl(void* state, int request, void* value)
	{
		const uintptr_t mode = *static_cast<uintptr_t*>(state);
		const SpeexCtl ctl = *reinterpret_cast<SpeexCtl*>(mode + SpeexModeEncoderCtl);
		return ctl(state, request, value);
	}

	VoiceStream::VoiceStream()
	{
		int enhancement = 1;
		int error = 0;

		Bits = new SpeexBits();
		speex_bits_init(Bits);
		Speex = speex_decoder_init(speex_lib_get_mode(SPEEX_MODEID_UWB));
		speex_decoder_ctl(Speex, SPEEX_SET_ENH, &enhancement);
		speex_decoder_ctl(Speex, SPEEX_GET_FRAME_SIZE, &SpeexFrameSize);
		Upsampler = src_new(SRC_SINC_MEDIUM_QUALITY, 1, &error);

		Opus = opus_decoder_create(GVoice::Rate, GVoice::PlaybackChannels, &error);
	}

	VoiceStream::~VoiceStream()
	{
		if (Opus)
			opus_decoder_destroy(Opus);
		if (Upsampler)
			src_delete(Upsampler);
		if (Speex)
			speex_decoder_destroy(Speex);

		speex_bits_destroy(Bits);
		delete Bits;
	}

	// Ultra-wideband reads narrowband and wideband packets too, leaving the bands they lack empty. A stereo
	// Opus decoder plays mono packets in both ears, so voice and the server's stereo radio share it.
	std::vector<int16_t>& VoiceStream::Decode(VoiceCodec codec, const uint8_t* data, int size)
	{
		constexpr int channels = GVoice::PlaybackChannels;
		Output.clear();

		if (codec == VoiceCodec::Opus)
		{
			if (!Opus)
				return Output;

			Output.resize(OpusMaxFrameSize * channels);
			const int samples = opus_decode(Opus, data, size, Output.data(), OpusMaxFrameSize, 0);
			Output.resize(std::max(samples, 0) * channels);
			return Output;
		}
		if (!Speex || !Upsampler)
			return Output;

		Narrow.resize(SpeexFrameSize);
		speex_bits_read_from(Bits, reinterpret_cast<const char*>(data), size);
		if (speex_decode_int(Speex, Bits, Narrow.data()) != 0)
			return Output;

		NarrowFloat.resize(SpeexFrameSize);
		src_short_to_float_array(Narrow.data(), NarrowFloat.data(), SpeexFrameSize);

		const int capacity = SpeexFrameSize * 2;
		Resampled.resize(capacity);

		SRC_DATA resample = {};
		resample.data_in = NarrowFloat.data();
		resample.input_frames = SpeexFrameSize;
		resample.data_out = Resampled.data();
		resample.output_frames = capacity;
		resample.src_ratio = static_cast<double>(GVoice::Rate) / SpeexRate;

		if (src_process(Upsampler, &resample) != 0)
			return Output;

		const int frames = static_cast<int>(resample.output_frames_gen);
		Output.resize(frames * channels);

		for (int i = 0; i < frames; i++)
		{
			const auto value = static_cast<int16_t>(std::clamp(static_cast<int>(Resampled[i] * 32768.0f), -32768, 32767));
			for (int channel = 0; channel < channels; channel++)
				Output[i * channels + channel] = value;
		}
		return Output;
	}

	void GVoice::Initialize()
	{
		if (Installed)
			return;
		Installed = true;

		// mov edi, UltraWideband; nop
		Memory::Write(EncodeInitBandwidth, std::vector<uint8_t>{ 0xBF, UltraWideband, 0x00, 0x00, 0x00, 0x90 });
		Memory::CALL(EncodeSetOptionsCall, reinterpret_cast<uintptr_t>(&GVoice::SetEncoderOptions));
		Memory::Set<uint8_t>(EncodeQualityJump, 0xEB);

		Memory::Set<uint32_t>(CaptureRate, Rate);
		Memory::JMP(CaptureByteRateSite, ASM_LOAD(CaptureByteRate_h));

		// Retail's jitter buffer counts bytes, so its thresholds are set in 48 kHz stereo: slow down below
		// 20 ms, start and settle at 250 ms, speed up above 450 ms.
		const std::pair<uintptr_t, int32_t> thresholds[] = {
			{ 0x4ECDF5, PlaybackBytesPerSecond * 20 / 1000 },
			{ 0x4ECE03, PlaybackBytesPerSecond * 450 / 1000 },
			{ 0x4ECE18, PlaybackBytesPerSecond * 250 / 1000 },
			{ 0x4ECE25, PlaybackBytesPerSecond * 250 / 1000 },
			{ 0x4ECF32, PlaybackBytesPerSecond * 250 / 1000 },
		};
		for (const auto& [address, bytes] : thresholds)
			Memory::Set<int32_t>(address, bytes);

		EncodeSample = reinterpret_cast<EncodeSample_t>(ASM_LOAD(Encode_Sample_h));
		SendVoiceData = reinterpret_cast<SendVoiceData_t>(ASM_LOAD(Client_SendVoiceData_h));
		UpdateSample = reinterpret_cast<UpdateSample_t>(ASM_LOAD(DSound_UpdateSample_h));

		DirectSoundCaptureCreate_h.Install();
		Record_QueueAudioDataForEncoding_h.Install();
		Voice_IncomingVoiceData_h.Install();
	}

	// Read from each gamestate's systeminfo. The encoder starts over so a stream never spans two servers.
	void GVoice::SetRelay(bool relay)
	{
		if (relay == Relay)
			return;
		Relay = relay;
		ResetCapture();

		if (Encoder)
			opus_encoder_ctl(Encoder, OPUS_RESET_STATE);
		if (Relay && !Encoder)
			Relay = CreateEncoder();
	}

	// AUDIO over VOIP: VOIP shapes everything toward intelligible speech, which thins out music and
	// anything else a player puts through the mic.
	bool GVoice::CreateEncoder()
	{
		int error = 0;

		Encoder = opus_encoder_create(Rate, 1, OPUS_APPLICATION_AUDIO, &error);
		if (!Encoder)
		{
			Log::WriteLine(Channel::Error, "Could not create the Opus voice encoder, staying on Speex.");
			return false;
		}
		opus_encoder_ctl(Encoder, OPUS_SET_BITRATE(OpusBitrate));
		opus_encoder_ctl(Encoder, OPUS_SET_COMPLEXITY(10));
		return true;
	}

	void GVoice::ResetCapture()
	{
		Captured.clear();
		Narrow.clear();

		if (Downsampler)
			src_reset(Downsampler);
	}

	// Voice activity detection and DTX drop frames they judge to be noise to the 2 kbps comfort-noise
	// mode, which clips quiet speech whatever the quality.
	void GVoice::SetEncoderOptions()
	{
		void* encoder = *EncoderState;
		if (!encoder)
			return;

		int rate = SpeexRate;
		int quality = Quality;
		int complexity = Complexity;
		int off = 0;

		EncoderCtl(encoder, SPEEX_SET_SAMPLING_RATE, &rate);
		EncoderCtl(encoder, SPEEX_SET_QUALITY, &quality);
		EncoderCtl(encoder, SPEEX_SET_COMPLEXITY, &complexity);
		EncoderCtl(encoder, SPEEX_SET_VAD, &off);
		EncoderCtl(encoder, SPEEX_SET_DTX, &off);
		EncoderCtl(encoder, SPEEX_GET_FRAME_SIZE, EncoderFrameSize);

		*EncoderQuality = quality;
		*EncoderSampleRate = rate;
	}

	// Replaces Record_QueueAudioDataForEncoding, whose partial buffer holds 640 samples and so cannot frame
	// 48 kHz audio for either codec. The mic scaler and level meter are retail's.
	int GVoice::QueueAudioData(audioSample_t* sample)
	{
		auto* pcm = reinterpret_cast<int16_t*>(sample->buffer);
		const int length = sample->lengthInSamples;
		*VoiceLevel = 0.0f;

		if (sample->bytesPerSample != 2 || length <= 0)
			return 0;

		float level = 0.0f;
		for (int i = 0; i < length; i++)
		{
			pcm[i] = static_cast<int16_t>(std::clamp(static_cast<int>(pcm[i] * *MicScaler), -32768, 32767));
			level += static_cast<float>(std::abs(pcm[i]));
		}
		*VoiceLevel = level / length;

		if (!Voice_SendVoiceData())
			return 0;

		if (!*TalkKeyHeld)
		{
			CL_VoiceTransmit();
			ResetCapture();
			return 0;
		}
		const size_t start = Captured.size();
		Captured.resize(start + length);
		src_short_to_float_array(pcm, Captured.data() + start, length);

		return Relay ? EncodeOpus() : EncodeSpeex();
	}

	int GVoice::EncodeOpus()
	{
		uint8_t packet[MaxPacketSize];
		int sent = 0;

		while (Captured.size() >= OpusFrameSize)
		{
			const int bytes =
				opus_encode_float(Encoder, Captured.data(), OpusFrameSize, packet + HeaderSize, MaxPacketSize - HeaderSize);
			Captured.erase(Captured.begin(), Captured.begin() + OpusFrameSize);

			if (bytes <= 0)
				continue;

			packet[0] = static_cast<uint8_t>(VoiceCodec::Opus);
			packet[1] = UnityGain;
			sent += SendVoiceData(packet, bytes + HeaderSize);
		}
		return sent;
	}

	// Down to Speex's rate, then through retail's encoder a frame at a time.
	int GVoice::EncodeSpeex()
	{
		const int frame = *EncoderFrameSize;
		if (frame <= 0)
			return 0;

		if (!Downsampler)
		{
			int error = 0;
			Downsampler = src_new(SRC_SINC_MEDIUM_QUALITY, 1, &error);
			if (!Downsampler)
				return 0;
		}
		const size_t start = Narrow.size();
		Narrow.resize(start + Captured.size());

		SRC_DATA resample = {};
		resample.data_in = Captured.data();
		resample.input_frames = static_cast<long>(Captured.size());
		resample.data_out = Narrow.data() + start;
		resample.output_frames = static_cast<long>(Captured.size());
		resample.src_ratio = static_cast<double>(SpeexRate) / Rate;

		const bool resampled = src_process(Downsampler, &resample) == 0;
		Narrow.resize(start + (resampled ? resample.output_frames_gen : 0));
		Captured.clear();

		uint8_t packet[MaxPacketSize];
		int sent = 0;
		Frame.resize(frame);

		while (Narrow.size() >= static_cast<size_t>(frame))
		{
			src_float_to_short_array(Narrow.data(), Frame.data(), frame);
			Narrow.erase(Narrow.begin(), Narrow.begin() + frame);

			const int bytes = EncodeSample(Frame.data(), packet, MaxPacketSize);
			if (bytes > 0)
				sent += SendVoiceData(packet, bytes);
		}
		return sent;
	}

	// Retail and CoD4X both create each talker's buffer mono, and a DirectSound buffer keeps the format it
	// was made with. Swapped for a stereo one the first time the talker is heard, and again whenever a
	// Voice_Init has made a new one. The sample is left as DSound_NewSample leaves a fresh one.
	bool GVoice::UseStereoBuffer(int talker, dsound_sample_t* sample)
	{
		if (sample->DSB && sample->DSB == StereoBuffers[talker])
			return true;

		IDirectSound* device = *DirectSound;
		if (!device)
			return false;

		WAVEFORMATEX format = {};
		format.wFormatTag = WAVE_FORMAT_PCM;
		format.nChannels = PlaybackChannels;
		format.nSamplesPerSec = Rate;
		format.wBitsPerSample = 16;
		format.nBlockAlign = PlaybackChannels * 2;
		format.nAvgBytesPerSec = PlaybackBytesPerSecond;

		// Retail's flags: global focus, volume, pan, frequency, software.
		DSBUFFERDESC description = {};
		description.dwSize = sizeof(description);
		description.dwFlags = 0x80E8;
		description.dwBufferBytes = PlaybackBufferBytes;
		description.lpwfxFormat = &format;

		IDirectSoundBuffer* buffer = nullptr;
		if (FAILED(device->CreateSoundBuffer(&description, &buffer, nullptr)) || !buffer)
			return false;

		if (auto* previous = static_cast<IDirectSoundBuffer*>(sample->DSB))
		{
			previous->Stop();
			previous->Release();
		}
		sample->DSB = buffer;
		sample->dwBufferSize = PlaybackBufferBytes;
		sample->currentOffset = 0;
		sample->lastOffset = 0;
		sample->currentBufferLength = 0;
		sample->stopPosition = -1;
		sample->lastPlayPos = 0;
		sample->bytesBuffered = 0;
		sample->playing = false;
		sample->playMode = 2;
		sample->channels = PlaybackChannels;

		StereoBuffers[talker] = buffer;
		return true;
	}

	// Decoded here per talker rather than through retail's single decoder, then handed to the talker's
	// DirectSound buffer. DSound_AdjustSamplePlayback sets the buffer's rate from the sample every frame,
	// so the frequency field is what makes it play at 48 kHz.
	void GVoice::IncomingVoiceData(uint8_t talker, uint8_t* data, int size)
	{
		dsound_sample_t* sample = talker < MaxTalkers ? ClientSamples[talker] : nullptr;
		if (!sample || !UpdateSample)
		{
			Voice_IncomingVoiceData_h(talker, data, size);
			return;
		}
		sample->frequency = Rate;
		const bool stereo = UseStereoBuffer(talker, sample);

		// With nothing to decode it still stamps the talker for the speaking icon.
		Voice_IncomingVoiceData_h(talker, data, 0);

		VoiceCodec codec = VoiceCodec::Speex;
		float gain = 1.0f;

		if (Relay)
		{
			if (size <= HeaderSize || data[0] > static_cast<uint8_t>(VoiceCodec::Opus))
				return;

			codec = static_cast<VoiceCodec>(data[0]);
			gain = static_cast<float>(data[1]) / UnityGain;
			data += HeaderSize;
			size -= HeaderSize;
		}
		auto& stream = Streams[talker];
		if (!stream)
			stream = std::make_unique<VoiceStream>();

		std::vector<int16_t>& pcm = stream->Decode(codec, data, size);
		if (pcm.empty())
			return;

		if (gain != 1.0f)
		{
			for (int16_t& value : pcm)
				value = static_cast<int16_t>(std::clamp(static_cast<int>(value * gain), -32768, 32767));
		}
		// Still the mono buffer if the stereo one could not be made.
		if (!stereo)
		{
			for (size_t i = 0; i < pcm.size() / PlaybackChannels; i++)
				pcm[i] = static_cast<int16_t>((pcm[i * PlaybackChannels] + pcm[i * PlaybackChannels + 1]) / 2);
			pcm.resize(pcm.size() / PlaybackChannels);
		}
		UpdateSample(sample, pcm.data(), static_cast<int>(pcm.size() * sizeof(int16_t)));
	}
}

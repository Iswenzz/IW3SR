#pragma once
#include "Game/Base.hpp"

#include <audioclient.h>
#include <mmdeviceapi.h>

namespace IW3SR
{
	// Loopback capture of the default render endpoint, written straight to a WAV.
	class API Audio
	{
	public:
		static inline std::atomic<bool> Recording = false;

		static bool Start(const std::filesystem::path& output);
		static void Stop();

		static uint32_t Rate();
		static uint32_t Channels();

	private:
		static inline IMMDeviceEnumerator* Enumerator = nullptr;
		static inline IMMDevice* Device = nullptr;
		static inline IAudioClient* Client = nullptr;
		static inline IAudioCaptureClient* Capture = nullptr;
		static inline WAVEFORMATEX* Format = nullptr;

		static inline std::ofstream File;
		static inline std::thread Reader;
		static inline uint64_t Written = 0;
		static inline bool Apartment = false;

		static bool Open();
		static void Close();
		static void Read();

		static void WriteHeader();
		static void PatchHeader();
		static void WriteSilence(uint32_t frames);
	};
}

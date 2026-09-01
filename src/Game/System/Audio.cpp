#include "Audio.hpp"

#include <functiondiscoverykeys_devpkey.h>

namespace IW3SR
{
	// Half the endpoint period is plenty to keep up with a shared-mode loopback stream, and short
	// enough that Stop does not sit waiting on the reader.
	constexpr DWORD PollInterval = 5;

	constexpr uint32_t HeaderSize = 44;

	template <class T>
	void Release(T*& value)
	{
		if (!value)
			return;

		value->Release();
		value = nullptr;
	}

	// The WAVE fields are each written at the width of the type they are held in, so the header
	// stays correct without a cast at every call. DWORD and uint32_t are distinct types on Win32,
	// which is what made a pair of overloads ambiguous here.
	template <class T>
	void Write(std::ofstream& file, T value)
	{
		file.write(reinterpret_cast<const char*>(&value), sizeof(value));
	}

	bool Audio::Start(const std::filesystem::path& output)
	{
		if (Recording)
			return false;

		// The game never initializes COM on the thread a console command runs on, and a capture can
		// start from either. Whoever already owns the apartment keeps it.
		const HRESULT initialized = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
		Apartment = SUCCEEDED(initialized);

		if (!Open())
		{
			Close();
			return false;
		}
		File.open(output, std::ios::binary | std::ios::trunc);
		if (!File.is_open())
		{
			Log::WriteLine(Channel::Error, "Cannot open the capture audio file: {}", output.string());
			Close();
			return false;
		}
		WriteHeader();

		Written = 0;
		Recording = true;

		if (FAILED(Client->Start()))
		{
			Recording = false;
			File.close();
			Close();
			return false;
		}
		Reader = std::thread(Read);
		return true;
	}

	void Audio::Stop()
	{
		if (!Recording)
			return;
		Recording = false;

		if (Reader.joinable())
			Reader.join();

		if (Client)
			Client->Stop();

		PatchHeader();
		File.close();
		Close();
	}

	uint32_t Audio::Rate()
	{
		return Format ? Format->nSamplesPerSec : 0;
	}

	uint32_t Audio::Channels()
	{
		return Format ? Format->nChannels : 0;
	}

	bool Audio::Open()
	{
		if (FAILED(CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_ALL,
				__uuidof(IMMDeviceEnumerator), reinterpret_cast<void**>(&Enumerator))))
		{
			Log::WriteLine(Channel::Error, "No audio endpoint enumerator, the capture will be silent.");
			return false;
		}
		if (FAILED(Enumerator->GetDefaultAudioEndpoint(eRender, eConsole, &Device)))
		{
			Log::WriteLine(Channel::Error, "No default playback device, the capture will be silent.");
			return false;
		}
		if (FAILED(Device->Activate(__uuidof(IAudioClient), CLSCTX_ALL, nullptr,
				reinterpret_cast<void**>(&Client))))
			return false;

		if (FAILED(Client->GetMixFormat(&Format)) || !Format)
			return false;

		// A loopback client is opened against the render endpoint in shared mode; the buffer
		// duration is a hint the mixer is free to round up.
		constexpr REFERENCE_TIME duration = 10000000;
		if (FAILED(Client->Initialize(AUDCLNT_SHAREMODE_SHARED, AUDCLNT_STREAMFLAGS_LOOPBACK, duration, 0, Format,
				nullptr)))
		{
			Log::WriteLine(Channel::Error, "The audio endpoint refused a loopback client.");
			return false;
		}
		return SUCCEEDED(Client->GetService(__uuidof(IAudioCaptureClient), reinterpret_cast<void**>(&Capture)));
	}

	void Audio::Close()
	{
		Release(Capture);
		Release(Client);
		Release(Device);
		Release(Enumerator);

		if (Format)
		{
			CoTaskMemFree(Format);
			Format = nullptr;
		}
		if (Apartment)
		{
			CoUninitialize();
			Apartment = false;
		}
	}

	// Loopback goes quiet rather than producing packets when nothing is playing, so a silent stretch
	// has to be written out by hand or the audio ends up shorter than the video it belongs to.
	void Audio::Read()
	{
		CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);

		const uint32_t frameSize = Format->nBlockAlign;
		const auto start = std::chrono::steady_clock::now();

		while (Recording)
		{
			uint32_t packet = 0;
			if (FAILED(Capture->GetNextPacketSize(&packet)))
				break;

			while (packet)
			{
				uint8_t* data = nullptr;
				uint32_t frames = 0;
				DWORD flags = 0;

				if (FAILED(Capture->GetBuffer(&data, &frames, &flags, nullptr, nullptr)))
					break;

				if (flags & AUDCLNT_BUFFERFLAGS_SILENT)
					WriteSilence(frames);
				else
					File.write(reinterpret_cast<const char*>(data), static_cast<std::streamsize>(frames) * frameSize);

				Written += frames;
				Capture->ReleaseBuffer(frames);

				if (FAILED(Capture->GetNextPacketSize(&packet)))
					break;
			}
			const auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
				std::chrono::steady_clock::now() - start);

			// The endpoint stops handing out packets entirely while the system is silent, which
			// leaves a hole the mux would swallow. Pad up to where the clock says we should be.
			const uint64_t expected = elapsed.count() * Format->nSamplesPerSec / 1000;
			if (expected > Written)
			{
				const auto padding = static_cast<uint32_t>(expected - Written);
				WriteSilence(padding);
				Written += padding;
			}

			Sleep(PollInterval);
		}
		CoUninitialize();
	}

	void Audio::WriteHeader()
	{
		const uint32_t byteRate = Format->nSamplesPerSec * Format->nBlockAlign;

		File.write("RIFF", 4);
		Write(File, static_cast<uint32_t>(0));
		File.write("WAVEfmt ", 8);
		Write(File, static_cast<uint32_t>(16));

		// The mix format is float in practice, but the endpoint owns that choice; the tag is copied
		// through so the muxer reads whatever the device actually handed us.
		Write(File, static_cast<uint16_t>(Format->wFormatTag == WAVE_FORMAT_EXTENSIBLE ? WAVE_FORMAT_IEEE_FLOAT
																					   : Format->wFormatTag));
		Write(File, Format->nChannels);
		Write(File, Format->nSamplesPerSec);
		Write(File, byteRate);
		Write(File, Format->nBlockAlign);
		Write(File, Format->wBitsPerSample);

		File.write("data", 4);
		Write(File, static_cast<uint32_t>(0));
	}

	void Audio::PatchHeader()
	{
		if (!File.is_open())
			return;

		const auto data = static_cast<uint32_t>(Written * Format->nBlockAlign);

		File.seekp(4);
		Write(File, data + HeaderSize - 8);
		File.seekp(40);
		Write(File, data);
	}

	void Audio::WriteSilence(uint32_t frames)
	{
		static const std::vector<uint8_t> zeros(4096, 0);
		size_t left = static_cast<size_t>(frames) * Format->nBlockAlign;

		while (left)
		{
			const size_t chunk = std::min(left, zeros.size());
			File.write(reinterpret_cast<const char*>(zeros.data()), static_cast<std::streamsize>(chunk));
			left -= chunk;
		}
	}
}

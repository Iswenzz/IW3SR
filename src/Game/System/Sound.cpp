#include "Sound.hpp"
#include "Patch.hpp"

namespace IW3SR
{
	constexpr uintptr_t SND_PauseSoundsAddress = 0x5C4210;
	constexpr uintptr_t SND_UnpauseSoundsAddress = 0x5C4370;

	constexpr unsigned int AmbientTrackFirst = 1;
	constexpr unsigned int AmbientTrackLast = 4;

	static const uint8_t* SoundInitialized2d = Signature(0xCC8E250);

	static Function<void()> SND_PauseSounds = SND_PauseSoundsAddress;
	static Function<void()> SND_UnpauseSounds = SND_UnpauseSoundsAddress;

	bool GSound::Command(const std::string& command)
	{
		if (Patch::UseCoD4X)
			return false;

		std::istringstream stream(command);
		std::string name;
		stream >> name;

		if (name == "snd_pause")
			Pause();
		else if (name == "snd_unpause")
			Unpause();
		else if (name == "snd_stopambient")
			StopAmbient(0);
		else
			return false;

		return true;
	}

	void GSound::Pause()
	{
		SND_PauseSounds();
	}

	void GSound::Unpause()
	{
		SND_UnpauseSounds();
	}

	void GSound::StopAmbient(int fadeTime)
	{
		if (!*SoundInitialized2d || fadeTime < 0)
			return;

		for (unsigned int track = AmbientTrackFirst; track <= AmbientTrackLast; track++)
			SND_StopBackground(track, fadeTime);
	}
}


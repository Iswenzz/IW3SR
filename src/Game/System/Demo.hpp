#pragma once
#include "Game/Base.hpp"

namespace IW3SR
{
	struct DemoHeader
	{
		bool Valid = false;
		bool Legacy = true;
		uint32_t Protocol = 0;
	};

	class API Demo
	{
	public:
		static void Initialize();
		static void Tick();

		static void ReadMessage(int localClientNum);
		static bool Command(const std::string& command);
		static bool Replay();

		static DemoHeader Inspect(const std::filesystem::path& path);
		static std::filesystem::path Resolve(const std::string& name, bool fullpath);
		static const std::string& LastPlayed();

		static bool IsPlaying();
		static std::string PlayerName();
		static int FPS();
		static int RunTime();
		static bool HasKeys();
		static usercmd_s Keys();

		static void SentPlayerState(playerState_s* to, const playerState_s* from);

	private:
		static inline dvar_s* LastDemo = nullptr;

		static inline std::string Name;
		static inline bool Playing = false;
		static inline int StartTime = 0;

		static void Unrestrict();
		static void Remember(const std::string& value);
		static bool Playable(const std::string& name, bool fullpath);

		static std::vector<std::filesystem::path> Bases();
	};
}

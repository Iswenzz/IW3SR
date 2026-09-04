#pragma once
#include "Game/Base.hpp"

namespace IW3SR
{
	enum class RenderState
	{
		Idle,
		Requested,
		Waiting,
		Recording,
		Finished
	};

	// One automated demo render, driven from the console or the command line.
	struct RenderRequest
	{
		std::string demo;
		std::string output;
		bool quitWhenDone;
	};

	// A frame handed over to the encoder thread.
	struct CaptureFrame
	{
		std::vector<uint8_t> pixels;
	};

	// Surfaces the readback cycles through. Locking the one the GPU was asked to fill this
	// frame stalls the pipeline, so we always read the oldest instead.
	constexpr size_t StagingCount = 3;

	class API Capture
	{
	public:
		static inline bool Recording = false;

		static void Initialize();
		static void Shutdown();

		static bool Start(const std::string& output);
		static void Stop();

		static void Frame(IDirect3DDevice9* device);
		static bool Command(const std::string& command);
		static bool DrawOverlay();
		static void Tick();

		static int RestartForDemo(int protocol);
		static void Disconnected();

	private:
		static inline dvar_s* Fps = nullptr;
		static inline dvar_s* Quality = nullptr;
		static inline dvar_s* Bitrate = nullptr;
		static inline dvar_s* Encoder = nullptr;
		static inline dvar_s* Preset = nullptr;
		static inline dvar_s* Binary = nullptr;
		static inline dvar_s* Overlay = nullptr;
		static inline dvar_s* Hidden = nullptr;
		static inline dvar_s* Sound = nullptr;

		static inline RenderState State = RenderState::Idle;
		static inline RenderRequest Request;
		static inline uint64_t Deadline = 0;

		static inline IDirect3DSurface9* Resolve = nullptr;
		static inline std::array<IDirect3DSurface9*, StagingCount> Staging = {};
		static inline size_t Slot = 0;
		static inline size_t Queued = 0;
		static inline int Width = 0;
		static inline int Height = 0;

		static inline HANDLE Process = nullptr;
		static inline HANDLE Pipe = nullptr;
		static inline HANDLE Report = nullptr;

		static inline std::thread Writer;
		static inline std::deque<CaptureFrame> Pending;
		static inline std::vector<CaptureFrame> Recycled;
		static inline std::mutex Mutex;
		static inline std::condition_variable Signal;
		static inline bool Draining = false;
		static inline std::atomic<bool> Aborted = false;
		static inline int Frames = 0;

		static inline bool Paced = false;
		static inline uint64_t Started = 0;
		static inline std::filesystem::path Target;
		static inline std::filesystem::path Track;

		static bool CreateSurfaces(IDirect3DDevice9* device);
		static void ReleaseSurfaces();
		static bool Copy(IDirect3DDevice9* device, size_t slot);
		static bool Read(size_t slot, int repeat = 1);
		static void Flush();
		static void Submit(CaptureFrame&& frame);
		static void Encode();

		static int Due();
		static void Mux();
		static std::filesystem::path Intermediate();

		static void Hide(bool state);
		static bool Spawn(const std::string& output);
		static void Terminate();
		static std::string Executable();
		static std::string Arguments(const std::string& output);
		static std::string Resolved(const std::string& output);
	};
}

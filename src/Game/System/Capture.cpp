#include "Capture.hpp"

#include "Game/System/Dvar.hpp"
#include "Game/System/System.hpp"

namespace IW3SR
{
	// Frames buffered ahead of the encoder. Deep enough to ride out an ffmpeg hiccup,
	// shallow enough that a 4K capture does not eat hundreds of megabytes.
	constexpr size_t MaxPendingFrames = 4;

	constexpr DWORD EncoderTimeout = 30000;

	constexpr std::array<std::string_view, 5> Containers = { ".mp4", ".mkv", ".mov", ".avi", ".webm" };

	void Capture::Initialize()
	{
		Fps = Dvar::RegisterInt("sr_capture_fps", DVAR_SAVED, "Frame rate of the recorded video", 60, 1, 1000);
		Quality = Dvar::RegisterInt("sr_capture_quality", DVAR_SAVED,
			"Constant rate factor handed to the encoder, lower is better", 18, 0, 51);
		Encoder = Dvar::RegisterString("sr_capture_encoder", DVAR_SAVED, "Video encoder used by ffmpeg", "libx264");
		Preset = Dvar::RegisterString("sr_capture_preset", DVAR_SAVED, "Encoder preset used by ffmpeg", "veryfast");
		Binary = Dvar::RegisterString("sr_capture_ffmpeg", DVAR_SAVED, "Path to ffmpeg, empty to search for it", "");
		Overlay = Dvar::RegisterBool("sr_capture_overlay", DVAR_SAVED,
			"Record the client overlay, huds included. The menu itself is only drawn while it is open", true);
	}

	void Capture::Shutdown()
	{
		Stop();
	}

	bool Capture::Start(const std::string& output)
	{
		// The dvars only exist once the renderer is up, which is also when there is a device.
		if (Recording || !Fps)
			return false;

		const auto device = dx ? dx->device : nullptr;
		if (!device)
		{
			Com_PrintMessage(CON_CHANNEL_ERROR, "^1No render device, cannot start a capture.\n", 0);
			return false;
		}
		if (!CreateSurfaces(device))
		{
			Com_PrintMessage(CON_CHANNEL_ERROR, "^1Failed to create the capture surfaces.\n", 0);
			return false;
		}
		const std::string path = Resolved(output);
		if (!Spawn(path))
		{
			ReleaseSurfaces();
			return false;
		}
		Frames = 0;
		Draining = false;
		Aborted = false;
		Recording = true;
		Writer = std::thread(Encode);

		// Fixes the client timestep so playback speed no longer depends on how fast we encode.
		Dvar::Set<int>("cl_avidemo", Fps->current.integer);

		Com_PrintMessage(CON_CHANNEL_CONSOLEONLY,
			std::format("Recording {}x{} at {} fps to {}\n", Width, Height, Fps->current.integer, path).c_str(), 0);
		return true;
	}

	void Capture::Stop()
	{
		if (!Recording)
			return;
		Recording = false;

		Dvar::Set<int>("cl_avidemo", 0);

		{
			std::lock_guard lock(Mutex);
			Draining = true;
		}
		Signal.notify_all();

		if (Writer.joinable())
			Writer.join();

		Terminate();
		ReleaseSurfaces();

		Com_PrintMessage(CON_CHANNEL_CONSOLEONLY, std::format("Recorded {} frames.\n", Frames).c_str(), 0);
	}

	void Capture::Frame(IDirect3DDevice9* device)
	{
		if (Recording && Aborted)
		{
			Stop();
			return;
		}
		if (!Recording || !device || !Resolve || !Staging)
			return;

		CaptureFrame frame;
		{
			std::lock_guard lock(Mutex);
			if (!Recycled.empty())
			{
				frame = std::move(Recycled.back());
				Recycled.pop_back();
			}
		}
		if (!Read(device, frame))
			return;

		Frames++;
		Submit(std::move(frame));
	}

	bool Capture::Command(const std::string& command)
	{
		std::istringstream stream(command);
		std::string name;
		stream >> name;

		if (name == "sr_capture")
		{
			std::string argument;
			stream >> std::quoted(argument);

			if (argument == "stop")
			{
				State = RenderState::Idle;
				Stop();
				return true;
			}
			if (argument.empty())
				argument = clc.demoplaying ? clc.demoName : "capture";

			Start(argument);
			return true;
		}

		if (name == "sr_render")
		{
			std::string demo, output;
			stream >> std::quoted(demo) >> std::quoted(output);

			if (demo.empty())
			{
				Com_PrintMessage(CON_CHANNEL_CONSOLEONLY, "Usage: sr_render <demo> [output]\n", 0);
				return true;
			}
			if (output.empty())
				output = std::filesystem::path(demo).stem().string();

			Request = { demo, output, true };
			State = RenderState::Requested;
			return true;
		}

		// cl_avidemo asks for a screenshot every frame, we take the backbuffer instead.
		return Recording && name.starts_with("screenshot");
	}

	bool Capture::DrawOverlay()
	{
		return Overlay && Overlay->current.enabled;
	}

	void Capture::Disconnected()
	{
		if (State == RenderState::Recording)
			State = RenderState::Finished;

		Stop();
	}

	bool Capture::CreateSurfaces(IDirect3DDevice9* device)
	{
		IDirect3DSurface9* back = nullptr;
		if (FAILED(device->GetBackBuffer(0, 0, D3DBACKBUFFER_TYPE_MONO, &back)) || !back)
			return false;

		D3DSURFACE_DESC desc = {};
		const HRESULT hr = back->GetDesc(&desc);
		back->Release();

		if (FAILED(hr))
			return false;

		if (FAILED(device->CreateRenderTarget(desc.Width, desc.Height, desc.Format, D3DMULTISAMPLE_NONE, 0, FALSE,
				&Resolve, nullptr)))
			return false;

		if (FAILED(device->CreateOffscreenPlainSurface(desc.Width, desc.Height, desc.Format, D3DPOOL_SYSTEMMEM,
				&Staging, nullptr)))
		{
			ReleaseSurfaces();
			return false;
		}
		Width = static_cast<int>(desc.Width);
		Height = static_cast<int>(desc.Height);
		return true;
	}

	void Capture::ReleaseSurfaces()
	{
		if (Resolve)
		{
			Resolve->Release();
			Resolve = nullptr;
		}
		if (Staging)
		{
			Staging->Release();
			Staging = nullptr;
		}
	}

	bool Capture::Read(IDirect3DDevice9* device, CaptureFrame& frame)
	{
		IDirect3DSurface9* back = nullptr;
		if (FAILED(device->GetBackBuffer(0, 0, D3DBACKBUFFER_TYPE_MONO, &back)) || !back)
			return false;

		// StretchRect resolves multisampling, GetRenderTargetData cannot.
		const HRESULT resolved = device->StretchRect(back, nullptr, Resolve, nullptr, D3DTEXF_NONE);
		back->Release();

		if (FAILED(resolved) || FAILED(device->GetRenderTargetData(Resolve, Staging)))
			return false;

		D3DLOCKED_RECT locked = {};
		if (FAILED(Staging->LockRect(&locked, nullptr, D3DLOCK_READONLY)))
			return false;

		const size_t stride = static_cast<size_t>(Width) * 4;
		frame.pixels.resize(stride * Height);

		const auto* source = static_cast<const uint8_t*>(locked.pBits);
		for (int y = 0; y < Height; y++)
			std::memcpy(frame.pixels.data() + stride * y, source + static_cast<size_t>(locked.Pitch) * y, stride);

		Staging->UnlockRect();
		return true;
	}

	void Capture::Submit(CaptureFrame&& frame)
	{
		std::unique_lock lock(Mutex);
		Signal.wait(lock, [] { return Pending.size() < MaxPendingFrames || Draining; });

		Pending.push_back(std::move(frame));
		lock.unlock();

		Signal.notify_all();
	}

	void Capture::Encode()
	{
		while (true)
		{
			CaptureFrame frame;
			{
				std::unique_lock lock(Mutex);
				Signal.wait(lock, [] { return !Pending.empty() || Draining; });

				if (Pending.empty())
					break;

				frame = std::move(Pending.front());
				Pending.pop_front();
			}
			Signal.notify_all();

			const uint8_t* data = frame.pixels.data();
			size_t left = frame.pixels.size();

			while (left)
			{
				DWORD written = 0;
				if (!WriteFile(Pipe, data, static_cast<DWORD>(left), &written, nullptr) || !written)
				{
					Log::WriteLine(Channel::Error, "The video encoder closed the pipe, aborting the capture.");

					// Release whoever is waiting on a slot, the render thread stops us next frame.
					std::lock_guard lock(Mutex);
					Draining = true;
					Aborted = true;
					Signal.notify_all();
					return;
				}
				data += written;
				left -= written;
			}
			std::lock_guard lock(Mutex);
			Recycled.push_back(std::move(frame));
		}
	}

	bool Capture::Spawn(const std::string& output)
	{
		const std::string executable = Executable();
		if (executable.empty())
		{
			Com_PrintMessage(CON_CHANNEL_ERROR,
				"^1ffmpeg was not found. Put ffmpeg.exe next to the game or set sr_capture_ffmpeg.\n", 0);
			return false;
		}
		SECURITY_ATTRIBUTES attributes = { sizeof(SECURITY_ATTRIBUTES), nullptr, TRUE };
		HANDLE read = nullptr;

		if (!CreatePipe(&read, &Pipe, &attributes, 1 << 20))
			return false;

		SetHandleInformation(Pipe, HANDLE_FLAG_INHERIT, 0);

		const auto log = Environment::Path(Directory::Reports) / "ffmpeg.log";
		Report = CreateFileA(log.string().c_str(), GENERIC_WRITE, FILE_SHARE_READ, &attributes, CREATE_ALWAYS,
			FILE_ATTRIBUTE_NORMAL, nullptr);

		if (Report == INVALID_HANDLE_VALUE)
			Report = CreateFileA("NUL", GENERIC_WRITE, 0, &attributes, OPEN_EXISTING, 0, nullptr);

		STARTUPINFOA startup = {};
		startup.cb = sizeof(startup);
		startup.dwFlags = STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;
		startup.wShowWindow = SW_HIDE;
		startup.hStdInput = read;
		startup.hStdOutput = Report;
		startup.hStdError = Report;

		PROCESS_INFORMATION process = {};
		std::string command = std::format("\"{}\" {}", executable, Arguments(output));

		const bool created = CreateProcessA(nullptr, command.data(), nullptr, nullptr, TRUE, CREATE_NO_WINDOW, nullptr,
			nullptr, &startup, &process);

		CloseHandle(read);

		if (!created)
		{
			Com_PrintMessage(CON_CHANNEL_ERROR, "^1Failed to launch the video encoder.\n", 0);
			Terminate();
			return false;
		}
		CloseHandle(process.hThread);
		Process = process.hProcess;
		return true;
	}

	void Capture::Terminate()
	{
		if (Pipe)
		{
			// Closing our end tells ffmpeg to finalize the container.
			CloseHandle(Pipe);
			Pipe = nullptr;
		}
		if (Process)
		{
			if (WaitForSingleObject(Process, EncoderTimeout) == WAIT_TIMEOUT)
				TerminateProcess(Process, 1);

			CloseHandle(Process);
			Process = nullptr;
		}
		if (Report && Report != INVALID_HANDLE_VALUE)
		{
			CloseHandle(Report);
			Report = nullptr;
		}
		std::lock_guard lock(Mutex);
		Pending.clear();
		Recycled.clear();
	}

	std::string Capture::Executable()
	{
		if (Binary && Binary->current.string && Binary->current.string[0])
			return Binary->current.string;

		const std::array<std::filesystem::path, 2> candidates = {
			Environment::Path(Directory::Bin) / "ffmpeg.exe",
			Environment::Path(Directory::Base) / "ffmpeg.exe",
		};
		for (const auto& candidate : candidates)
		{
			if (std::filesystem::exists(candidate))
				return candidate.string();
		}
		char found[MAX_PATH] = {};
		if (SearchPathA(nullptr, "ffmpeg.exe", nullptr, MAX_PATH, found, nullptr))
			return found;

		return {};
	}

	std::string Capture::Arguments(const std::string& output)
	{
		return std::format(
			"-hide_banner -loglevel error -y -f rawvideo -pixel_format bgra -video_size {}x{} -framerate {} -i - "
			"-an -vf \"scale=trunc(iw/2)*2:trunc(ih/2)*2\" -c:v {} -preset {} -crf {} -pix_fmt yuv420p \"{}\"",
			Width, Height, Fps->current.integer, Encoder->current.string, Preset->current.string,
			Quality->current.integer, output);
	}

	std::string Capture::Resolved(const std::string& output)
	{
		std::filesystem::path path = output;

		// Demo names carry a .dm_1 extension, which is not something ffmpeg can mux into.
		if (std::ranges::find(Containers, path.extension().string()) == Containers.end())
			path.replace_extension(".mp4");

		if (path.is_relative())
			path = Environment::Path(Directory::App) / "Videos" / path;

		std::error_code ec;
		std::filesystem::create_directories(path.parent_path(), ec);

		const std::filesystem::path stem = path.parent_path() / path.stem();
		const std::string extension = path.extension().string();

		for (int index = 1; std::filesystem::exists(path); index++)
			path = std::format("{}_{}{}", stem.string(), index, extension);

		return path.string();
	}

	void Capture::Tick()
	{
		switch (State)
		{
		case RenderState::Requested:
			State = RenderState::Waiting;

			// Already watching the demo the user asked for, just start recording it.
			if (clc.demoplaying)
				break;

			{
				// CoD4X needs the fullpath keyword for demos that live outside the demo folder.
				const bool fullpath = std::filesystem::exists(Request.demo);
				const std::string command = fullpath ? std::format("demo \"{}\" fullpath\n", Request.demo)
													 : std::format("demo \"{}\"\n", Request.demo);

				Cmd_ExecuteSingleCommand(0, 0, command.c_str());
			}
			break;

		case RenderState::Waiting:
			if (!clc.demoplaying || client_ui->connectionState != CA_ACTIVE)
				break;

			State = Start(Request.output) ? RenderState::Recording : RenderState::Finished;
			break;

		case RenderState::Finished:
			State = RenderState::Idle;
			if (Request.quitWhenDone)
				GSystem::ExitRequested = true;
			break;

		default:
			break;
		}
	}
}

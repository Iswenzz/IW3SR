#include "About.hpp"

#include "Engine/Core/IO/Zip.hpp"
#include "Engine/Core/Network/HTTP.hpp"
#include "Engine/Core/System/Environment.hpp"
#include "Engine/Core/System/ThreadPool.hpp"

#include "Game/System/System.hpp"

#include <shellapi.h>

namespace IW3SR::UC
{
	static std::optional<std::array<int, 3>> ParseVersion(std::string_view text)
	{
		std::array<int, 3> version = {};
		const char* cursor = text.data();
		const char* const end = text.data() + text.size();

		for (size_t i = 0; i < version.size(); i++)
		{
			if (i && (cursor == end || *cursor++ != '.'))
				return std::nullopt;

			const auto [next, error] = std::from_chars(cursor, end, version[i]);
			if (error != std::errc{})
				return std::nullopt;
			cursor = next;
		}
		if (cursor != end)
			return std::nullopt;
		return version;
	}

	void About::Initialize()
	{
		CheckUpdate();
	}

	void About::CheckUpdate()
	{
		static bool notified = false;
		Checking = true;
		StatusMessage = "Checking for updates...";

		auto req = HTTP::Get("https://iswenzz.com/static/updates/iw3sr/version.txt",
			[](const HTTPResponse& response)
			{
				std::string latest = response.Body;
				const auto space = [](unsigned char c) { return std::isspace(c); };
				latest.erase(latest.begin(), std::ranges::find_if_not(latest, space));
				latest.erase(std::find_if_not(latest.rbegin(), latest.rend(), space).base(), latest.end());

				const auto remote = ParseVersion(latest);
				const bool ok = response.Code == 200 && remote;
				const bool newer = ok && *remote > *ParseVersion(APPLICATION_VERSION);

				GRenderer::Tasks.Add(
					[ok, newer, latest]()
					{
						Checking = false;
						if (!ok)
						{
							StatusMessage = "Failed to check for updates.";
							return;
						}
						if (!newer)
						{
							StatusMessage = "You are up to date.";
							return;
						}
						UpdateAvailable = true;
						LatestVersion = latest;
						StatusMessage = "Update available: " + LatestVersion;

						if (!notified)
						{
							notified = true;
							Notifications::Push(StatusMessage, NotificationLevel::Info, 6.0f, 2.0f);
						}
					});
			});

		req.Send();
	}

	void About::StartUpdate()
	{
		if (!UpdateAvailable || Downloading || Extracting)
			return;

		std::filesystem::path tempDir = std::filesystem::temp_directory_path() / "IW3SR";
		std::string zipPath = (tempDir / "IW3SR.zip").string();
		std::string filesDir = (tempDir / "files").string();
		std::string scriptPath = (tempDir / "updater.bat").string();
		std::string gameDir = Environment::Path(Directory::Base).string();

		std::error_code ec;
		std::filesystem::remove_all(filesDir, ec);
		std::filesystem::create_directories(tempDir, ec);
		std::filesystem::create_directories(filesDir, ec);

		Downloading = true;
		Progress = 0.0f;
		StatusMessage = "Downloading...";

		auto req = HTTP::Get("https://iswenzz.com/static/updates/iw3sr/IW3SR.zip",
			[zipPath, filesDir, scriptPath, gameDir](const HTTPResponse& response)
			{
				const auto fail = [](std::string message)
				{
					GRenderer::Tasks.Add(
						[message = std::move(message)]()
						{
							Downloading = false;
							Extracting = false;
							StatusMessage = message;
						});
				};

				// The code is the one in the headers, which a transfer cut off halfway still carries.
				if (!response.Success || response.Code != 200)
					return fail("Download failed.");

				{
					std::ofstream file(zipPath, std::ios::binary);
					if (!file)
						return fail("Could not write the downloaded archive.");

					file.write(response.Body.data(), response.Body.size());
					if (!file)
						return fail("Could not write the downloaded archive.");
				}

				GRenderer::Tasks.Add(
					[]()
					{
						Downloading = false;
						Extracting = true;
						StatusMessage = "Extracting...";
					});

				if (!Zip::Extract(zipPath, filesDir))
					return fail("Extraction failed.");

				std::error_code ec;
				std::filesystem::remove(zipPath, ec);

				std::string script =
					"@echo off\n"
					"title IW3SR\n"
					"echo Waiting for game to close...\n"
					"timeout /t 2 /nobreak > nul\n"
					":waitloop\n"
					"tasklist | find /i \"iw3mp.exe\" > nul\n"
					"if not errorlevel 1 (\n"
					"    timeout /t 1 /nobreak > nul\n"
					"    goto waitloop\n"
					")\n"
					"echo Copying files...\n"
					"xcopy /E /Y /I \"" + filesDir + "\" \"" + gameDir + "\"\n"
					"timeout /t 1 /nobreak > nul\n"
					"echo Cleaning up...\n"
					"rmdir /S /Q \"" + filesDir + "\"\n"
					"timeout /t 1 /nobreak > nul\n"
					"echo Launching game...\n"
					"timeout /t 2 /nobreak > nul\n"
					"start \"\" \"" + gameDir + "\\iw3mp.exe\"\n"
					"exit\n";

				{
					std::ofstream file(scriptPath);
					if (!file)
						return fail("Could not write the updater script.");

					file << script;
				}

				GRenderer::Tasks.Add(
					[scriptPath]()
					{
						Extracting = false;
						StatusMessage = "Launching updater...";

						ShellExecute(nullptr, "open", scriptPath.c_str(), nullptr, nullptr, SW_SHOW);
						GSystem::ExitRequested = true;
					});
			});

		req.TimeoutSeconds = 0;
		req.LowSpeedLimitBytes = 512;
		req.LowSpeedTimeSeconds = 30;
		req.OnProgress = [](float progress) { Progress = progress; };
		req.Send();
	}

	void About::Render()
	{
		constexpr auto description =
			"A client modification for Call of Duty 4, powered by IzEngine. "
			"It improves performance and gameplay with an in-game GUI, a runtime plugin system, new movement physics, "
			"and more.";

		constexpr float padding = 16;

		const bool showButton = (!Downloading && !Extracting) && (UpdateAvailable || !Checking);
		const bool showProgress = Downloading || Extracting;
		const bool showStatus = !StatusMessage.empty();

		const float width = ImGui::GetContentRegionAvail().x;
		const float logoSize = ImClamp(width * 0.35f, 72.0f, 160.0f);

		const Ref<Texture> logo = Texture::Load("Textures/Logo/sr.jpg");

		if (logo)
		{
			ImGui::Image(std::static_pointer_cast<DX9Texture>(logo)->Data, ImVec2(logoSize, logoSize));
			ImGui::SameLine(0.0f, padding);
		}
		ImGui::BeginGroup();
		ImGui::PushTextWrapPos(ImGui::GetCursorPosX() + ImGui::GetContentRegionAvail().x);
		ImGui::TextWrapped("%s", description);
		ImGui::PopTextWrapPos();

		ImGui::Spacing();
		ImGui::TextDisabled("Version " APPLICATION_VERSION);
		ImGui::EndGroup();

		ImGui::Spacing();
		ImGui::Separator();
		ImGui::Spacing();

		if (showStatus)
			ImGui::TextDisabled("%s", StatusMessage.c_str());

		if (showProgress)
		{
			ImGui::Spacing();
			ImGui::ProgressBar(Progress.load(), ImVec2(-1, 0));
		}
		if (showButton)
		{
			ImGui::Spacing();
			if (UpdateAvailable)
			{
				ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0.4, 1, 1));
				ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0, 0.4, 1, 0.7));
				if (ImGui::Button("Update Now", ImVec2(-1, 0)))
					StartUpdate();
				ImGui::PopStyleColor(2);
			}
			else if (ImGui::Button("Check for Updates", ImVec2(-1, 0)))
			{
				CheckUpdate();
			}
		}
	}
}

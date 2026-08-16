#include "About.hpp"

#include "Engine/Core/IO/Zip.hpp"
#include "Engine/Core/Network/HTTP.hpp"
#include "Engine/Core/System/Environment.hpp"
#include "Engine/Core/System/ThreadPool.hpp"

#include "Game/System/System.hpp"

#include <shellapi.h>

namespace IW3SR::UC
{
	About::About() : Frame("About")
	{
		SetRectAlignment(Horizontal::Center, Vertical::Center);
		SetFlags(ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar);

		CheckUpdate();
	}

	void About::CheckUpdate()
	{
		Checking = true;
		StatusMessage = "Checking for updates...";

		auto req = HTTP::Get("https://iswenzz.com/static/updates/iw3sr/version.txt",
			[](const HTTPResponse& response)
			{
				const bool ok = response.Code == 200;
				std::string latest = response.Body;
				latest.erase(std::remove(latest.begin(), latest.end(), '\n'), latest.end());
				latest.erase(std::remove(latest.begin(), latest.end(), '\r'), latest.end());

				Actions::Add(
					[ok, latest]()
					{
						Checking = false;
						if (!ok)
						{
							StatusMessage = "Failed to check for updates.";
							return;
						}
						if (latest == APPLICATION_VERSION)
						{
							StatusMessage = "You are up to date.";
							return;
						}
						UpdateAvailable = true;
						LatestVersion = latest;
						StatusMessage = "Update available: " + LatestVersion;
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
					Actions::Add(
						[message = std::move(message)]()
						{
							Downloading = false;
							Extracting = false;
							StatusMessage = message;
						});
				};

				if (response.Code != 200)
					return fail("Download failed.");

				{
					std::ofstream file(zipPath, std::ios::binary);
					if (!file)
						return fail("Could not write the downloaded archive.");

					file.write(response.Body.data(), response.Body.size());
					if (!file)
						return fail("Could not write the downloaded archive.");
				}

				Actions::Add(
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

				Actions::Add(
					[scriptPath]()
					{
						Extracting = false;
						StatusMessage = "Launching updater...";

						ShellExecute(nullptr, "open", scriptPath.c_str(), nullptr, nullptr, SW_SHOW);
						GSystem::ExitRequested = true;
					});
			});

		req.OnProgress = [](float progress) { Progress = progress; };
		req.Send();
	}

	void About::OnRender()
	{
		constexpr auto title = "IW3SR";
		constexpr auto description =
			"A client modification for Call of Duty 4, powered by IzEngine."
			"It improves performance and gameplay with an in-game GUI, a runtime plugin system, new movement physics, "
			"and more.";

		constexpr int logoSize = 250;
		constexpr int padding = 30;

		bool showButton = (!Downloading && !Extracting) && (UpdateAvailable || !Checking);
		bool showProgress = Downloading || Extracting;
		bool showStatus = !StatusMessage.empty();

		SetRect(-200, -150, 400, 0);
		Logo = Texture::Load("Textures/Logo/sr.jpg");

		Begin();
		ImGui::Spacing();
		ImGui::Spacing();
		if (Logo)
		{
			ImGui::Image(std::static_pointer_cast<DX9Texture>(Logo)->Data, ImVec2(logoSize, logoSize));
			ImGui::SameLine(0.0f, padding);
		}
		float rightWidth = ImGui::GetWindowSize().x - logoSize - padding * 3;
		ImGui::BeginGroup();
		ImGui::PushFont(ImGui::H2);
		ImGui::TextUnformatted(title);
		ImGui::PopFont();

		ImGui::Spacing();

		ImGui::PushTextWrapPos(ImGui::GetCursorPosX() + rightWidth);
		ImGui::TextWrapped("%s", description);
		ImGui::PopTextWrapPos();

		ImGui::Spacing();

		ImGui::TextUnformatted("Version " APPLICATION_VERSION);

		ImGui::EndGroup();

		if (showStatus)
		{
			ImGui::Spacing();
			ImGui::TextDisabled("%s", StatusMessage.c_str());
		}
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
			else
			{
				if (ImGui::Button("Check for Updates", ImVec2(-1, 0)))
					CheckUpdate();
			}
		}
		End();
	}
}

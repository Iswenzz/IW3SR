#include "About.hpp"

#include "Game/System/System.hpp"

#include "Core/IO/Zip.hpp"
#include "Core/Network/HTTP.hpp"
#include "Core/System/Environment.hpp"
#include "Core/System/ThreadPool.hpp"

#include <shellapi.h>

namespace IW3SR::UC
{
	About::About() : Frame("About")
	{
		SetRectAlignment(Horizontal::Center, Vertical::Center);
		SetFlags(ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar);
		Logo = Texture::Create(VFS::GetFile("Textures/Logo/sr.jpg"));

		CheckUpdate();
	}

	void About::CheckUpdate()
	{
		Checking = true;
		StatusMessage = "Checking for updates...";

		auto* req = HTTP::Get("https://iswenzz.com/static/updates/iw3sr/version.txt",
			[this](const HTTPResponse& response)
			{
				Checking = false;
				if (response.Code != 200)
				{
					StatusMessage = "Failed to check for updates.";
					return;
				}
				std::string latest = response.Body;
				latest.erase(std::remove(latest.begin(), latest.end(), '\n'), latest.end());
				latest.erase(std::remove(latest.begin(), latest.end(), '\r'), latest.end());

				if (latest == APPLICATION_VERSION)
				{
					StatusMessage = "You are up to date.";
					return;
				}
				UpdateAvailable = true;
				LatestVersion = latest;
				StatusMessage = "Update available: " + LatestVersion;
			});

		req->Send();
	}

	void About::StartUpdate()
	{
		if (!UpdateAvailable || Downloading || Extracting)
			return;

		std::filesystem::path tempDir = std::filesystem::temp_directory_path() / "IW3SR_Update";
		std::string zipPath = (tempDir / "IW3SR.zip").string();
		std::string updateDir = (tempDir / "extracted").string();
		std::string updaterDest = (tempDir / "Updater.exe").string();
		std::string gameDir = Environment::Path(Directory::Base).string();

		std::filesystem::create_directories(tempDir);
		std::filesystem::create_directories(updateDir);
		std::filesystem::copy_file(Environment::Path(Directory::Bin) / "Updater.exe", updaterDest,
			std::filesystem::copy_options::overwrite_existing);

		Downloading = true;
		Progress = 0.0f;
		StatusMessage = "Downloading...";

		auto* req = HTTP::Get("https://iswenzz.com/static/updates/iw3sr/IW3SR.zip",
			[=](const HTTPResponse& response)
			{
				if (response.Code != 200)
				{
					Downloading = false;
					StatusMessage = "Download failed.";
					return;
				}
				std::ofstream file(zipPath, std::ios::binary);
				file.write(response.Body.data(), response.Body.size());
				file.close();

				Downloading = false;
				Extracting = true;
				StatusMessage = "Extracting...";

				Zip::Extract(zipPath, updateDir);
				std::filesystem::remove(zipPath);

				Extracting = false;
				StatusMessage = "Launching updater...";
				std::string args = "\"" + gameDir + "\"";
				ShellExecute(nullptr, "open", updaterDest.c_str(), args.c_str(), nullptr, SW_SHOW);
				GSystem::ExitRequested = true;
			});

		req->OnProgress = [this](float progress) { Progress = progress; };
		req->Send();
	}

	void About::OnRender()
	{
		constexpr auto title = "IW3SR";
		constexpr auto description =
			"Powered by IzEngine — featuring an in-game GUI, plugin system, "
			"runtime module reloading, rotating platform interpolation, Q3/CPM/CS movements, "
			"CGAZ HUD, offline shaders, velocity meter and more.";

		constexpr int logoSize = 250;
		constexpr int padding = 20;

		bool showButton = (!Downloading && !Extracting) && (UpdateAvailable || !Checking);
		bool showProgress = Downloading || Extracting;
		bool showStatus = !StatusMessage.empty();

		SetRect(-200, -150, 400, 0);

		Begin();
		ImGui::Spacing();
		if (Logo && Logo->Data)
		{
			ImGui::Image(Logo->Data, ImVec2(logoSize, logoSize));
			ImGui::SameLine(0.0f, padding);
		}
		float rightWidth = ImGui::GetWindowSize().x - logoSize - padding * 3;
		ImGui::BeginGroup();
		ImGui::PushFont(ImGui::H2);
		ImGui::Text(title);
		ImGui::PopFont();

		ImGui::Spacing();

		ImGui::PushTextWrapPos(ImGui::GetCursorPosX() + rightWidth);
		ImGui::TextWrapped(description);
		ImGui::PopTextWrapPos();

		ImGui::Spacing();

		ImGui::Text("Version " APPLICATION_VERSION);

		ImGui::EndGroup();

		if (showStatus)
		{
			ImGui::Spacing();
			ImGui::TextDisabled("%s", StatusMessage.c_str());
		}
		if (showProgress)
		{
			ImGui::Spacing();
			ImGui::ProgressBar(Progress, ImVec2(-1, 0));
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

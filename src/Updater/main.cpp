#define WIN32_LEAN_AND_MEAN
#define UUID_DEFINED

#include <Windows.h>

#include <TlHelp32.h>
#include <shellapi.h>
#include <chrono>
#include <filesystem>
#include <iostream>
#include <string>
#include <thread>

static bool WaitForProcessExit(const std::string& processName, int timeoutMs = 30000)
{
	auto start = std::chrono::steady_clock::now();
	while (true)
	{
		HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
		PROCESSENTRY32 entry = { sizeof(PROCESSENTRY32) };
		bool found = false;

		if (Process32First(snapshot, &entry))
		{
			do
			{
				if (processName == entry.szExeFile)
				{
					found = true;
					break;
				}
			} while (Process32Next(snapshot, &entry));
		}
		CloseHandle(snapshot);

		if (!found)
			return true;

		auto elapsed = std::chrono::steady_clock::now() - start;
		if (elapsed > std::chrono::milliseconds(timeoutMs))
			return false;

		std::this_thread::sleep_for(std::chrono::milliseconds(500));
	}
}

int main(int argc, char* argv[])
{
	SetConsoleTitle("IW3SR");

	if (argc < 2)
		return 1;

	std::string gameDir = argv[1];
	std::filesystem::path tempDir = std::filesystem::temp_directory_path() / "IW3SR_Update";
	std::string updateDir = (tempDir / "extracted").string();

	std::cout << "Waiting for game to close..." << std::endl;
	WaitForProcessExit("iw3mp.exe");
	std::this_thread::sleep_for(std::chrono::milliseconds(500));

	std::cout << "Copying files..." << std::endl;
	for (const auto& entry : std::filesystem::recursive_directory_iterator(updateDir))
	{
		if (!entry.is_regular_file())
			continue;

		std::filesystem::path dest =
			std::filesystem::path(gameDir) / std::filesystem::relative(entry.path(), updateDir);
		std::filesystem::create_directories(dest.parent_path());
		std::filesystem::copy_file(entry.path(), dest, std::filesystem::copy_options::overwrite_existing);
	}
	std::this_thread::sleep_for(std::chrono::milliseconds(500));

	std::cout << "Cleaning up..." << std::endl;
	std::filesystem::remove_all(updateDir);
	std::this_thread::sleep_for(std::chrono::milliseconds(500));

	std::cout << "Launching game..." << std::endl;
	std::this_thread::sleep_for(std::chrono::milliseconds(2000));
	ShellExecute(nullptr, "open", (std::filesystem::path(gameDir) / "iw3mp.exe").string().c_str(), nullptr,
		gameDir.c_str(), SW_SHOW);
	return 0;
}

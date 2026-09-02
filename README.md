# IW3SR

![](https://i.imgur.com/O6goVqC.jpeg)

A client modification for Call of Duty 4, powered by [IzEngine](https://github.com/Iswenzz/IzEngine). It improves performance and gameplay with an in-game GUI, a runtime plugin system, new movement physics, and more.

## Features

### Interface
- In-game GUI, with a settings panel for every module.
- Plugin system to rebuild and swap modules in without restarting the game.
- Console autocompletion, recolored output and widened color escape characters.
- Emoji support.
- Discord Rich Presence, with join invites.
- Built-in browser for video playback.
- Shell integration for `cod4://` links and `.dm_1` demo files.

### Movement
- Movement rate decoupled from the frame rate: `com_maxfps` drives the physics, `sr_maxfps` the renderer.
- Alternative physics modes, listed below.
- Bunny hop script.
- CGAZ HUD, velocity meter, snap zones, pmove HUD, lagometer, FPS counter and key display.
- Raw input mouse, free of pointer acceleration and steady at high polling rates.

### Rendering
- Interpolation of rotating platforms.
- Portal view rendering, drawing the far side into the portal surface.
- Offline shader playback.
- Post-processing tweaks: brightness, contrast, desaturation, glow and sun.
- Frame limiter that waits on a high resolution timer instead of spinning the core.

### Demos
- Demo playback with missing fastfiles.
- Demo rendering to MP4 with audio, unattended through `sr_render`.

### Assets
- Extra zone loading and usermap search.
- Default assets in place of missing viewmodels, weapons and effects.
- Asset dumping, and a report of what each pool is using.
- Enlarged asset pools and hunk.

### Network
- Segmented resumable downloads, reusing the segments of an interrupted transfer that still verify.
- Server browser filtering: hide, redirect or block a list of servers.
- Master server override.
- Standalone Valve A2S_INFO query client.
- Artificial latency and packet loss on the game's UDP sockets.
- qWAVE QoS, prioritising the game socket over other traffic on the machine.
- Player profile in `LOCALAPPDATA`, with decoded stats and a backup.
- CD key protection, and an rcon password that never reaches `config_mp.cfg`.
- Patches for known client remote code execution bugs.

## Physics
- **Q3** — Quake 3 original movements.
- **Q3CPM** — Quake 3 CPM movements.
- **CS** — Counter-Strike bhop and surf physics.

## CoD4X
The client carries the major CoD4X client patches on its own, and negotiates both stock protocol 6 and
extended protocol 21.
When cod4x dll is present, IW3SR loads alongside it and hands those features back to it rather than
running its own. It can be kept out entirely from the settings panel.

## Redistributables
The client requires the following runtimes to be installed.
- [Microsoft Visual C++ Redistributable (v14, x86)](https://aka.ms/vc14/vc_redist.x86.exe)
- [DirectX End-User Runtime (June 2010)](https://www.microsoft.com/en-us/download/details.aspx?id=8109)

## Instructions
In order to use this client, download the archived file down below, and extract it to your cod4 directory.
To remove the client you can delete ddraw.dll and optionally every other files extracted from the archive.

## Building

_Pre-Requisites:_

1. [Visual Studio](https://visualstudio.microsoft.com/)
2. [CMake](https://cmake.org/) and [vcpkg](https://vcpkg.io/en/).

_Build Command:_

    ./build.bat

### [Download](https://github.com/Iswenzz/IW3SR/releases)

## Contributors

**_Note:_** If you would like to contribute to this repository, feel free to send a pull request, and I will review your code.
Also feel free to post about any problems that may arise in the issues section of the repository.

<a href="https://github.com/Dualiteee"><img src="https://avatars.githubusercontent.com/u/134146664?v=4" height=64 style="border-radius: 50%"></a>
<a href="https://github.com/xoxor4d"><img src="https://avatars.githubusercontent.com/u/45299104?v=4" height=64 style="border-radius: 50%"></a>


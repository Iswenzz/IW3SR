#include "VR.hpp"
#include "Body.hpp"

#include "Game/Renderer/Portal/Portal.hpp"
#include "Game/System/Dvar.hpp"
#include "Game/System/Timestep.hpp"

#include <glm/gtc/quaternion.hpp>

namespace IW3SR
{
	namespace
	{
		// D3D9 caps render targets here on any card that can drive a headset, and the game sizes every
		// buffer it draws through to the display.
		constexpr int MaxRenderSize = 8192;

		constexpr float HudDistance = 1.5f;

		// The game reaches Direct3DCreate9 through its import thunk, which other mods may have redirected.
		constexpr uintptr_t Direct3DCreate9Thunk = 0x670284;

		constexpr float StickDeadzone = 0.15f;
		constexpr float TriggerThreshold = 0.5f;

		// The gun keeps the aim this long past the trigger, for shots that leave a few commands later.
		constexpr std::chrono::milliseconds GunAimHold{ 300 };

		constexpr float CullMargin = 1.02f;

		// How far the body turns from where the head looks, at most, in degrees.
		constexpr float MaxTwist = 60.0f;

		// How far the view may follow the headset sideways, the reach of a lean. Further, the eye would
		// leave the player's hitbox and look through walls it cannot pass. Height is never followed, since
		// the hitbox cannot follow it either.
		constexpr float MaxLean = 0.35f;

		// A headset this far from where it was centred, for this long, was put on after the centre was
		// taken, the usual case for a game started with the headset still on the desk.
		constexpr float StaleDistance = 0.5f;
		constexpr float StaleSeconds = 1.5f;

		constexpr uintptr_t DrawScene = 0xD584070;
		constexpr int DrawSceneStandard = 3;

		// The declared GfxViewInfo is not the size the engine uses; its stride and the offset of the view's
		// own 2D command list are taken from RB_StandardDrawCommandsCommon.
		constexpr size_t ViewInfoStride = 0x67B0;
		constexpr size_t ViewInfoCommands = 0x5688;

		// Projection matrices are scaled just under one, as InfinitePerspectiveMatrix builds them.
		constexpr float ProjectionScale = 0.99951172f;

		// One pose action per tracker role, in VRTracker's order.
		struct TrackerRole
		{
			const char* Action;
			const char* Name;
			const char* Path;
		};
		constexpr TrackerRole TrackerRoles[VRTrackerCount] = {
			{ "tracker_waist", "Waist tracker", "/user/vive_tracker_htcx/role/waist/input/grip/pose" },
			{ "tracker_chest", "Chest tracker", "/user/vive_tracker_htcx/role/chest/input/grip/pose" },
			{ "tracker_left_foot", "Left foot tracker", "/user/vive_tracker_htcx/role/left_foot/input/grip/pose" },
			{ "tracker_right_foot", "Right foot tracker", "/user/vive_tracker_htcx/role/right_foot/input/grip/pose" },
			{ "tracker_left_knee", "Left knee tracker", "/user/vive_tracker_htcx/role/left_knee/input/grip/pose" },
			{ "tracker_right_knee", "Right knee tracker", "/user/vive_tracker_htcx/role/right_knee/input/grip/pose" },
			{ "tracker_left_elbow", "Left elbow tracker", "/user/vive_tracker_htcx/role/left_elbow/input/grip/pose" },
			{ "tracker_right_elbow", "Right elbow tracker", "/user/vive_tracker_htcx/role/right_elbow/input/grip/pose" },
		};

		constexpr int KeyCatchConsole = 0x1;
		constexpr int KeyCatchUi = 0x10;
		constexpr int KeyCatchMessage = 0x20;

		constexpr D3DRENDERSTATETYPE HudStates[] = { D3DRS_SEPARATEALPHABLENDENABLE, D3DRS_SRCBLENDALPHA,
			D3DRS_DESTBLENDALPHA, D3DRS_BLENDOPALPHA, D3DRS_COLORWRITEENABLE };

		// A pose in the game's frame, x forward, y left and z up, in meters from the recentred origin.
		struct GamePose
		{
			vec3 Position{};
			mat3 Axis{ 1.0f };
		};

		vec3 Rotate(const XrQuaternionf& q, const vec3& v)
		{
			const vec3 u(q.x, q.y, q.z);
			return 2.0f * glm::dot(u, v) * u + (q.w * q.w - glm::dot(u, u)) * v + 2.0f * q.w * glm::cross(u, v);
		}

		// The runtime's space has x right, y up and z behind.
		vec3 ToGame(const vec3& v)
		{
			return { -v.z, -v.x, v.y };
		}

		vec3 Unyaw(const vec3& v, float yaw)
		{
			const float c = std::cos(yaw);
			const float s = std::sin(yaw);
			return { c * v.x - s * v.z, v.y, s * v.x + c * v.z };
		}

		GamePose ToGamePose(const XrPosef& pose, float yaw, const vec3& origin)
		{
			const vec3 position = vec3(pose.position.x, pose.position.y, pose.position.z) - origin;
			const vec3 forward = Unyaw(Rotate(pose.orientation, { 0, 0, -1 }), yaw);
			const vec3 left = Unyaw(Rotate(pose.orientation, { -1, 0, 0 }), yaw);
			const vec3 up = Unyaw(Rotate(pose.orientation, { 0, 1, 0 }), yaw);

			return { ToGame(Unyaw(position, yaw)), mat3(ToGame(forward), ToGame(left), ToGame(up)) };
		}

		void Multiply44(const float a[4][4], const float b[4][4], float out[4][4])
		{
			for (int i = 0; i < 4; i++)
			{
				for (int j = 0; j < 4; j++)
					out[i][j] = a[i][0] * b[0][j] + a[i][1] * b[1][j] + a[i][2] * b[2][j] + a[i][3] * b[3][j];
			}
		}

		// Row major in, row major out: loading it into glm as is hands glm the transpose, and the inverse of
		// a transpose is the transpose of the inverse, so storing the result back as is undoes it.
		void Inverse44(const float m[4][4], float out[4][4])
		{
			glm::mat4 g;
			for (int i = 0; i < 4; i++)
			{
				for (int j = 0; j < 4; j++)
					g[i][j] = m[i][j];
			}
			const glm::mat4 inverse = glm::inverse(g);
			for (int i = 0; i < 4; i++)
			{
				for (int j = 0; j < 4; j++)
					out[i][j] = inverse[i][j];
			}
		}

		// Pitch, yaw and roll, in degrees, of an axis in the game's frame: x forward, y left, z up.
		vec3 AxisAngles(const mat3& axis)
		{
			const vec3 forward = axis[0];
			const float yaw = Math::RadToDeg(std::atan2(forward.y, forward.x));
			const float pitch = Math::RadToDeg(-std::asin(std::clamp(forward.z, -1.0f, 1.0f)));
			const float roll = Math::RadToDeg(std::atan2(axis[1].z, axis[2].z));
			return { pitch, yaw, roll };
		}

		// Heading and pitch, in degrees, of a direction in the game's frame.
		vec2 Heading(const vec3& forward)
		{
			return { Math::RadToDeg(std::atan2(forward.y, forward.x)),
				Math::RadToDeg(-std::asin(std::clamp(forward.z, -1.0f, 1.0f))) };
		}

		mat3 YawAxis(float yaw)
		{
			vec3 forward, right, up;
			Math::AngleVectors({ 0.0f, yaw, 0.0f }, forward, right, up);
			return mat3(forward, -right, up);
		}

		mat3 ToAxis(const float (&axis)[3][3])
		{
			return { vec3(axis[0][0], axis[0][1], axis[0][2]), vec3(axis[1][0], axis[1][1], axis[1][2]),
				vec3(axis[2][0], axis[2][1], axis[2][2]) };
		}

		char ToMove(float value)
		{
			return static_cast<char>(std::clamp(std::round(value), -127.0f, 127.0f));
		}

		// Turns a move made facing one way into the same move for a command facing another. The game scales
		// a move by its largest component, so the turned one is scaled back to it, or moving at an angle to
		// the aim would be slower.
		void Steer(usercmd_s* cmd, float degrees)
		{
			const float forward = cmd->forwardmove;
			const float right = cmd->rightmove;
			const float largest = std::max(std::abs(forward), std::abs(right));
			if (largest <= 0.0f)
				return;

			const float c = std::cos(Math::DegToRad(degrees));
			const float s = std::sin(Math::DegToRad(degrees));
			const float turnedForward = forward * c + right * s;
			const float turnedRight = right * c - forward * s;
			const float scale = largest / std::max(std::abs(turnedForward), std::abs(turnedRight));

			cmd->forwardmove = ToMove(turnedForward * scale);
			cmd->rightmove = ToMove(turnedRight * scale);
		}

		// Turns a boolean dvar off for the frame; Show gives the player's value back.
		void Hide(dvar_s*& var, const char* name, bool& hidden)
		{
			if (!var)
				var = Dvar::Find(name);
			if (var && var->current.enabled)
			{
				var->current.enabled = false;
				hidden = true;
			}
		}

		void Show(dvar_s* var, bool& hidden)
		{
			if (var && hidden)
				var->current.enabled = true;
			hidden = false;
		}

		// Escape as a key, so the game opens or closes its menu the way it would for the keyboard.
		void PressEscape()
		{
			if (!g_wv || !g_wv->hWnd)
				return;

			constexpr LPARAM EscapeScanCode = 0x01 << 16;
			PostMessage(g_wv->hWnd, WM_KEYDOWN, VK_ESCAPE, EscapeScanCode | 1);
			PostMessage(g_wv->hWnd, WM_KEYUP, VK_ESCAPE, EscapeScanCode | 0xC0000001);
		}
	}

	void GVR::RegisterDvars()
	{
		EnabledVar = Dvar::RegisterBool("sr_vr", DvarFlags(DVAR_SAVED | DVAR_LATCHED),
			"Play in an OpenXR headset. Applies on vid_restart", false);
		ScaleVar = Dvar::RegisterFloat("sr_vr_scale", DvarFlags(DVAR_SAVED | DVAR_LATCHED),
			"VR resolution, relative to what the headset asks for. Applies on vid_restart", 1.0f, 0.5f, 2.0f);
		HudFovVar = Dvar::RegisterFloat("sr_vr_hud_fov", DVAR_SAVED, "Width of the VR HUD panel, in degrees", 60.0f,
			20.0f, 100.0f);
		GunVar = Dvar::RegisterBool("sr_vr_gun", DVAR_SAVED, "Draw the weapon in VR", true);
		TurnSpeedVar = Dvar::RegisterFloat("sr_vr_turn_speed", DVAR_SAVED,
			"VR controller turning speed, in degrees a second", 180.0f, 30.0f, 720.0f);

		// Registering again does not move a latched value into place, and these are only read here.
		for (dvar_s* var : { EnabledVar, ScaleVar })
		{
			if (var)
				var->current = var->latched;
		}
	}

	// Made before the session, which takes the action set as it is then.
	void GVR::CreateActions()
	{
		Actions.Move = OpenXR::CreateAction("move", "Move", XR_ACTION_TYPE_VECTOR2F_INPUT);
		Actions.Turn = OpenXR::CreateAction("turn", "Turn", XR_ACTION_TYPE_VECTOR2F_INPUT);
		Actions.Jump = OpenXR::CreateAction("jump", "Jump", XR_ACTION_TYPE_BOOLEAN_INPUT);
		Actions.Crouch = OpenXR::CreateAction("crouch", "Crouch", XR_ACTION_TYPE_BOOLEAN_INPUT);
		Actions.Attack = OpenXR::CreateAction("attack", "Attack", XR_ACTION_TYPE_FLOAT_INPUT);
		Actions.Aim = OpenXR::CreateAction("aim", "Aim down sights", XR_ACTION_TYPE_FLOAT_INPUT);
		Actions.Use = OpenXR::CreateAction("use", "Use / reload", XR_ACTION_TYPE_BOOLEAN_INPUT);
		Actions.Melee = OpenXR::CreateAction("melee", "Melee", XR_ACTION_TYPE_FLOAT_INPUT);
		Actions.Sprint = OpenXR::CreateAction("sprint", "Sprint", XR_ACTION_TYPE_BOOLEAN_INPUT);
		Actions.Weapon = OpenXR::CreateAction("weapon", "Next weapon", XR_ACTION_TYPE_BOOLEAN_INPUT);
		Actions.Menu = OpenXR::CreateAction("menu", "Menu", XR_ACTION_TYPE_BOOLEAN_INPUT);
		Actions.Recenter = OpenXR::CreateAction("recenter", "Recenter", XR_ACTION_TYPE_BOOLEAN_INPUT);
		Actions.HandGrip = OpenXR::CreateAction("hand_grip", "Weapon hand", XR_ACTION_TYPE_POSE_INPUT);
		Actions.HandAim = OpenXR::CreateAction("hand_aim", "Weapon aim", XR_ACTION_TYPE_POSE_INPUT);
		Actions.LeftGrip = OpenXR::CreateAction("left_grip", "Free hand", XR_ACTION_TYPE_POSE_INPUT);

		OpenXR::SuggestBindings("/interaction_profiles/oculus/touch_controller",
			{
				{ Actions.HandGrip, "/user/hand/right/input/grip/pose" },
				{ Actions.HandAim, "/user/hand/right/input/aim/pose" },
				{ Actions.LeftGrip, "/user/hand/left/input/grip/pose" },
				{ Actions.Move, "/user/hand/left/input/thumbstick" },
				{ Actions.Turn, "/user/hand/right/input/thumbstick" },
				{ Actions.Jump, "/user/hand/right/input/a/click" },
				{ Actions.Crouch, "/user/hand/right/input/b/click" },
				{ Actions.Attack, "/user/hand/right/input/trigger/value" },
				{ Actions.Aim, "/user/hand/left/input/trigger/value" },
				{ Actions.Use, "/user/hand/left/input/x/click" },
				{ Actions.Weapon, "/user/hand/left/input/y/click" },
				{ Actions.Melee, "/user/hand/right/input/squeeze/value" },
				{ Actions.Sprint, "/user/hand/left/input/thumbstick/click" },
				{ Actions.Recenter, "/user/hand/right/input/thumbstick/click" },
				{ Actions.Menu, "/user/hand/left/input/menu/click" },
			});

		OpenXR::SuggestBindings("/interaction_profiles/valve/index_controller",
			{
				{ Actions.HandGrip, "/user/hand/right/input/grip/pose" },
				{ Actions.HandAim, "/user/hand/right/input/aim/pose" },
				{ Actions.LeftGrip, "/user/hand/left/input/grip/pose" },
				{ Actions.Move, "/user/hand/left/input/thumbstick" },
				{ Actions.Turn, "/user/hand/right/input/thumbstick" },
				{ Actions.Jump, "/user/hand/right/input/a/click" },
				{ Actions.Crouch, "/user/hand/right/input/b/click" },
				{ Actions.Attack, "/user/hand/right/input/trigger/value" },
				{ Actions.Aim, "/user/hand/left/input/trigger/value" },
				{ Actions.Use, "/user/hand/left/input/a/click" },
				{ Actions.Menu, "/user/hand/left/input/b/click" },
				{ Actions.Melee, "/user/hand/right/input/squeeze/value" },
				{ Actions.Sprint, "/user/hand/left/input/thumbstick/click" },
				{ Actions.Recenter, "/user/hand/right/input/thumbstick/click" },
			});

		// Body trackers, when the runtime has them: SteamVR's own, SlimeVR, or a Quest's body forwarded by
		// Virtual Desktop as trackers.
		if (OpenXR::HasExtension(XR_HTCX_VIVE_TRACKER_INTERACTION_EXTENSION_NAME))
		{
			std::vector<XRBinding> trackers;
			for (int i = 0; i < VRTrackerCount; i++)
			{
				Actions.Trackers[i] = OpenXR::CreateAction(TrackerRoles[i].Action, TrackerRoles[i].Name,
					XR_ACTION_TYPE_POSE_INPUT);
				trackers.push_back({ Actions.Trackers[i], TrackerRoles[i].Path });
			}
			OpenXR::SuggestBindings("/interaction_profiles/htc/vive_tracker_htcx", trackers);
		}

		OpenXR::SuggestBindings("/interaction_profiles/khr/simple_controller",
			{
				{ Actions.HandGrip, "/user/hand/right/input/grip/pose" },
				{ Actions.HandAim, "/user/hand/right/input/aim/pose" },
				{ Actions.LeftGrip, "/user/hand/left/input/grip/pose" },
				{ Actions.Attack, "/user/hand/right/input/select/click" },
				{ Actions.Menu, "/user/hand/left/input/menu/click" },
			});
	}

	VRControls GVR::ReadControls()
	{
		VRControls input;
		input.Move = OpenXR::GetVector2(Actions.Move);
		input.Turn = OpenXR::GetVector2(Actions.Turn);
		input.Jump = OpenXR::GetBoolean(Actions.Jump);
		input.Crouch = OpenXR::GetBoolean(Actions.Crouch);
		input.Attack = OpenXR::GetFloat(Actions.Attack) > TriggerThreshold;
		input.Aim = OpenXR::GetFloat(Actions.Aim) > TriggerThreshold;
		input.Use = OpenXR::GetBoolean(Actions.Use);
		input.Melee = OpenXR::GetFloat(Actions.Melee) > TriggerThreshold;
		input.Sprint = OpenXR::GetBoolean(Actions.Sprint);
		input.Weapon = OpenXR::GetBoolean(Actions.Weapon);
		input.Menu = OpenXR::GetBoolean(Actions.Menu);
		input.Recenter = OpenXR::GetBoolean(Actions.Recenter);
		return input;
	}

	// Runs as the renderer makes its D3D9, before the window and the device exist, since a headset decides
	// what D3D9 runs on, their size, and whether the monitor's refresh may pace the game.
	void GVR::Startup()
	{
		if (Started)
			return;
		Started = true;
		MainThread = GetCurrentThreadId();

		RegisterDvars();
		Enabled = EnabledVar && EnabledVar->current.enabled;
		RenderSize = {};

		// The restart that followed a session that could not start runs flat once; sr_vr stays as the player
		// left it, so the next restart tries again.
		if (std::exchange(Failed, false))
			Enabled = false;
		if (!Enabled)
			return;

		// A Direct3D 12 session gets the frames without them leaving the GPU, once the game's D3D9 runs on its
		// device. Direct3D 11 takes them through system memory, and is what is left when that cannot be.
		std::string error;
		if ((TwelveFailed || !Connect(Direct3D12, error)) && !Connect(Direct3D11, error))
		{
			Com_PrintMessage(CON_CHANNEL_ERROR, std::format("^1VR: {}, staying on the monitor.\n", error).c_str(), 0);
			Enabled = false;
			return;
		}
		if (dx && dx->d3d9)
			IDirect3D9_CreateDevice_h.Update(VTABLE(dx->d3d9, 16));

		R_RenderScene_h.Install();
		CG_Draw2D_h.Install();
		RB_Draw3D_h.Install();
		RB_ViewCommands_h.Install();
		CG_UpdateViewModelPose_h.Install();
		CG_Player_h.Install();
		UI_MouseEvent_h.Install();
	}

	// Starts OpenXR on a graphics API, with the bridge that carries the frames to it and the actions.
	bool GVR::Connect(XRGraphics& graphics, std::string& error)
	{
		OpenXR::RequestExtension(XR_HTCX_VIVE_TRACKER_INTERACTION_EXTENSION_NAME);
		if (!OpenXR::Initialize(graphics, error))
			return false;

		Graphics = &graphics;
		if (Graphics == &Direct3D12)
			Bridge = CreateScope<DX9XRD3D12Bridge>(Direct3D12);
		else
			Bridge = CreateScope<DX9XRBridge>(Direct3D11);
		OpenXR::OnStateChanged = [](XrSessionState state)
		{ Com_PrintMessage(CON_CHANNEL_LOG, std::format("VR: session {}.\n", OpenXR::StateName(state)).c_str(), 0); };
		CreateActions();

		const float scale = ScaleVar ? ScaleVar->current.value : 1.0f;
		const glm::ivec2 limit = glm::min(OpenXR::MaximumSize(), glm::ivec2(MaxRenderSize));
		glm::ivec2 size = glm::ivec2(glm::round(vec2(OpenXR::RecommendedSize()) * scale));
		size = glm::clamp(size, glm::ivec2(256), glm::max(limit, glm::ivec2(256)));
		RenderSize = size & ~1;

		const char* path = Graphics == &Direct3D12 ? "Direct3D 12" : "Direct3D 11 through system memory";
		Com_PrintMessage(CON_CHANNEL_LOG,
			std::format("VR: {} at {}x{} per eye, {}.\n", OpenXR::RuntimeName(), RenderSize.x, RenderSize.y, path).c_str(),
			0);
		return true;
	}

	void GVR::Disconnect()
	{
		Bridge.reset();
		OpenXR::Shutdown();
		OpenXR::OnStateChanged = nullptr;
		Actions = {};
		Graphics = nullptr;
	}

	// In place of the renderer's Direct3DCreate9. With a Direct3D 12 session, the game's D3D9 is made by
	// D3D9On12 on its device; failing that, the session moves to Direct3D 11.
	IDirect3D9* STDCALL GVR::CreateDirect3D(UINT sdkVersion)
	{
		Startup();

		IDirect3D9* d3d = nullptr;
		if (Enabled && Graphics == &Direct3D12)
		{
			d3d = DX9XRD3D12Bridge::CreateDirect3D(Direct3D12, sdkVersion);
			if (!d3d)
			{
				Com_PrintMessage(CON_CHANNEL_ERROR, "^1VR: D3D9On12 is unavailable, frames go through system memory.\n", 0);
				TwelveFailed = true;
				Disconnect();

				std::string error;
				if (!Connect(Direct3D11, error))
				{
					Com_PrintMessage(CON_CHANNEL_ERROR, std::format("^1VR: {}, staying on the monitor.\n", error).c_str(), 0);
					Enabled = false;
					RenderSize = {};
				}
			}
		}
		if (!d3d)
			d3d = reinterpret_cast<IDirect3D9*(WINAPI*)(UINT)>(Direct3DCreate9Thunk)(sdkVersion);
		if (d3d && OwnsWindow())
			IDirect3D9_CreateDevice_h.Update(VTABLE(d3d, 16));
		return d3d;
	}

	// The menus take the pointer in the window's pixels and divide them by their own layout, which in VR is
	// the HUD band shown fitted to the window. The pointer is put in the band's pixels first.
	void GVR::MouseEvent(int x, int y)
	{
		RECT client = {};
		const glm::ivec2 area = Frame.Active ? HudArea : RenderSize;
		if (!OwnsWindow() || !g_wv || !GetClientRect(g_wv->hWnd, &client) || area.x <= 0 || area.y <= 0)
		{
			UI_MouseEvent_h(x, y);
			return;
		}

		const vec2 window(std::max(1L, client.right), std::max(1L, client.bottom));
		const float scale = std::min(window.x / area.x, window.y / area.y);
		const vec2 offset = (window - vec2(area) * scale) * 0.5f;
		UI_MouseEvent_h(static_cast<int>(std::lround((x - offset.x) / scale)),
			static_cast<int>(std::lround((y - offset.y) / scale)));
	}

	// The engine is told the display is the headset's eye, which sizes every buffer it draws through. The
	// window is kept as the player set it up; the device's back buffer follows the window, not this.
	void GVR::WindowParms(GfxWindowParms* parms)
	{
		if (!parms || !OwnsWindow())
			return;

		DesktopSize = { parms->displayWidth, parms->displayHeight };
		DesktopFullscreen = parms->fullscreen;

		parms->fullscreen = false;
		parms->hz = 60;
		parms->sceneWidth = parms->displayWidth = RenderSize.x;
		parms->sceneHeight = parms->displayHeight = RenderSize.y;

		// The HUD is laid out in a band at the top of the eye's buffer, shaped like the window, so the
		// monitor shows it whole and the panel in the headset matches.
		const glm::ivec2 window = WindowSize();
		const float aspect = window.x > 0 && window.y > 0 ? static_cast<float>(window.x) / window.y : 16.0f / 9.0f;
		// Never wider than the window either: the console and the menus draw in pixels, which a band at a
		// supersampled eye's width would shrink.
		const float width = std::min({ static_cast<float>(RenderSize.x), RenderSize.y * aspect,
			static_cast<float>(std::max(window.x, 1)) });
		HudArea = { static_cast<int>(width), static_cast<int>(std::lround(width / aspect)) };

		HudSize = HudArea;

		// The desktop copy must not wait on the monitor's refresh while the headset sets the pace. Read
		// once when the device is made, and put back as soon as it has been.
		Vsync = Dvar::Find("r_vsync");
		if (Vsync && Vsync->current.enabled)
		{
			Vsync->current.enabled = false;
			VsyncRestore = true;
		}
	}

	bool GVR::OwnsWindow()
	{
		return Enabled && RenderSize.x > 0;
	}

	// The client area the player's own settings would give the window. Fullscreen becomes a window the
	// size of the monitor, which the borderless patch then turns into one: an exclusive mode would lose the
	// device every time the headset's software takes focus.
	glm::ivec2 GVR::WindowSize()
	{
		if (!DesktopFullscreen)
			return DesktopSize;

		MONITORINFO info = { sizeof(info) };
		const HMONITOR monitor = MonitorFromPoint({ 0, 0 }, MONITOR_DEFAULTTOPRIMARY);
		if (!monitor || !GetMonitorInfoA(monitor, &info))
			return DesktopSize;

		return { info.rcMonitor.right - info.rcMonitor.left, info.rcMonitor.bottom - info.rcMonitor.top };
	}

	// The device's back buffer follows the window's client area, as it would without a headset. Only the
	// mirror is drawn into it, so it needs no samples; a multisampled one would refuse the stretched copy.
	void GVR::PresentSize(D3DPRESENT_PARAMETERS* parameters)
	{
		if (!parameters || !OwnsWindow())
			return;

		RECT client = {};
		const HWND window = parameters->hDeviceWindow ? parameters->hDeviceWindow : (g_wv ? g_wv->hWnd : nullptr);
		if (!window || !GetClientRect(window, &client))
			return;

		parameters->BackBufferWidth = static_cast<UINT>(std::max(1L, client.right - client.left));
		parameters->BackBufferHeight = static_cast<UINT>(std::max(1L, client.bottom - client.top));
		parameters->MultiSampleType = D3DMULTISAMPLE_NONE;
		parameters->MultiSampleQuality = 0;
	}

	HRESULT GVR::CreateDevice(IDirect3D9* d3d, UINT adapter, D3DDEVTYPE type, HWND window, DWORD flags,
		D3DPRESENT_PARAMETERS* parameters, IDirect3DDevice9** device)
	{
		PresentSize(parameters);

		const HRESULT hr = IDirect3D9_CreateDevice_h(d3d, adapter, type, window, flags, parameters, device);
		if (SUCCEEDED(hr) && device && *device && OwnsWindow())
			IDirect3DDevice9_GetBackBuffer_h.Update(VTABLE(*device, 18));
		return hr;
	}

	// The engine takes its frame buffer from here right after the device is made, and again after every
	// reset. Handing it a surface at the headset's size is what renders the game there.
	HRESULT GVR::GetBackBuffer(IDirect3DDevice9* device, UINT swapChain, UINT index, D3DBACKBUFFER_TYPE type,
		IDirect3DSurface9** surface)
	{
		if (!OwnsWindow() || swapChain != 0 || index != 0 || !surface || !dx)
			return IDirect3DDevice9_GetBackBuffer_h(device, swapChain, index, type, surface);

		*surface = Bridge ? Bridge->RenderTarget(device, RenderSize, static_cast<D3DMULTISAMPLE_TYPE>(dx->multiSampleType),
								dx->multiSampleQuality)
						  : nullptr;
		if (*surface)
			return D3D_OK;

		// The window's back buffer is the desktop's size, which the engine, told otherwise, cannot draw into.
		if (!Failed)
		{
			Com_PrintMessage(CON_CHANNEL_ERROR, "^1VR: the headset's frame buffer could not be made, restarting flat.\n", 0);
			Failed = true;
			Cbuf_AddText(0, "vid_restart\n");
		}
		return IDirect3DDevice9_GetBackBuffer_h(device, swapChain, index, type, surface);
	}

	void GVR::Initialize()
	{
		if (Vsync && VsyncRestore)
			Vsync->current.enabled = true;
		VsyncRestore = false;

		if (Ready || !Enabled || !dx || !dx->device)
			return;

		// The copies on the GPU need the device D3D9On12 made on the session's own; the restart that follows
		// one that is not goes through system memory.
		if (!Bridge->Attach(dx->device))
		{
			Com_PrintMessage(CON_CHANNEL_ERROR, "^1VR: the game's D3D9 is not on the headset's GPU, restarting.\n", 0);
			Shutdown();
			TwelveFailed = true;
			Cbuf_AddText(0, "vid_restart\n");
			return;
		}

		std::string error;
		if (!OpenXR::CreateSession(RenderSize, HudSize, error))
		{
			Com_PrintMessage(CON_CHANNEL_ERROR, std::format("^1VR: {}, staying on the monitor.\n", error).c_str(), 0);

			// The device was already made for the headset, drawing into a surface nothing will show.
			Shutdown();
			Failed = true;
			Cbuf_AddText(0, "vid_restart\n");
			return;
		}

		// Both eyes are drawn inside one backend frame, and the captures and the submit that follow it have
		// to run on the thread that owns the OpenXR session.
		SmpBackend = Dvar::Find("r_smp_backend");
		if (SmpBackend && SmpBackend->current.enabled)
		{
			SmpBackend->current.enabled = false;
			SmpRestore = true;
		}

		IDirect3DDevice9_SetRenderState_h.Update(VTABLE(dx->device, 57));

		Ready = true;
		RecenterPending = true;
		LastFrame = std::chrono::steady_clock::now();
		Com_PrintMessage(CON_CHANNEL_LOG, "VR: session started, sr_vr_recenter to recenter the view.\n", 0);
	}

	// The device is about to go, and everything of ours made on it goes first. A vid_restart runs
	// Startup again, which is where sr_vr and sr_vr_scale are read back.
	void GVR::Shutdown()
	{
		if (!Started)
			return;

		if (OpenXR::FrameOpen())
			OpenXR::EndFrame({});

		HudPass = false;
		RestoreView();
		Show(DrawGun, GunHidden);
		Show(DrawCrosshair, CrosshairHidden);

		IDirect3DDevice9_SetRenderState_h.Remove();
		IDirect3DDevice9_GetBackBuffer_h.Remove();
		IDirect3D9_CreateDevice_h.Remove();
		R_RenderScene_h.Remove();
		CG_Draw2D_h.Remove();
		RB_Draw3D_h.Remove();
		RB_ViewCommands_h.Remove();
		CG_UpdateViewModelPose_h.Remove();
		CG_Player_h.Remove();
		UI_MouseEvent_h.Remove();
		GVRBody::Reset();
		Disconnect();

		if (SmpBackend && SmpRestore)
			SmpBackend->current.enabled = true;
		SmpRestore = false;

		Frame = {};
		Actions = {};
		Stage = VRStage::None;
		Ready = false;
		Started = false;
		Enabled = false;
		RenderSize = {};
		HudArea = {};
		HudSize = {};
	}

	// Called as the engine opens a frame. Waits for the headset's next frame slot, then takes the poses
	// that frame is predicted to be seen at, so the commands built and the eyes drawn below share them.
	void GVR::BeginFrame()
	{
		if (!Ready || GetCurrentThreadId() != MainThread)
			return;

		if (HudPass)
			EndHud();
		RestoreView();
		Show(DrawGun, GunHidden);

		Frame = {};
		Stage = VRStage::None;
		GVRBody::Reset();

		const auto now = std::chrono::steady_clock::now();
		const float seconds = std::clamp(std::chrono::duration<float>(now - LastFrame).count(), 0.0f, 0.1f);
		LastFrame = now;

		if (SmpBackend && SmpBackend->current.enabled)
		{
			R_SyncRenderThread();
			SmpBackend->current.enabled = false;
			SmpRestore = true;
		}

		// The swapchains were made for the size asked of the window. A device the engine kept from before
		// VR was turned on is still the monitor's size and needs a vid_restart first.
		const glm::ivec2 size = FrameSize();
		if (size.x <= 0 || size.y <= 0)
			return;
		if (size != RenderSize)
		{
			static bool warned = false;
			if (!std::exchange(warned, true))
				Com_PrintMessage(CON_CHANNEL_ERROR, "^1VR: the game is not at the headset's size, vid_restart.\n", 0);
			return;
		}
		if (!Bridge->Prepare(dx->device, RenderSize, HudSize))
			return;

		OpenXR::PollEvents();
		if (OpenXR::Ended())
		{
			OpenXR::Shutdown();
			Com_PrintMessage(CON_CHANNEL_LOG,
				"VR: the headset closed the session, the game carries on in the window (sr_vr 0; vid_restart to "
				"leave VR).\n",
				0);
		}

		Frame.Active = OpenXR::BeginFrame();
		LayoutHud(Frame.Active);
		if (!Frame.Active)
			return;

		Frame.Tracked = OpenXR::Located();
		Frame.Eyes[0] = OpenXR::View(0);
		Frame.Eyes[1] = OpenXR::View(1);
		Frame.Head = OpenXR::Head();
		Frame.Input = ReadControls();
		Controls(seconds);
		if (!Frame.Tracked)
			return;

		const vec3 head(Frame.Head.position.x, Frame.Head.position.y, Frame.Head.position.z);
		Stale = glm::distance(head, RecenterPosition) > StaleDistance ? Stale + seconds : 0.0f;

		if (RecenterPending || Stale > StaleSeconds)
			Recenter();

		const vec2 looking = Heading(ToGamePose(Frame.Head, RecenterYaw, RecenterPosition).Axis[0]);
		Frame.Yaw = looking.x;
		Frame.Pitch = looking.y;

		// The hands are followed whenever tracked, and the gun held in the right one whenever it is drawn,
		// aiming from there.
		Frame.RightTracked =
			OpenXR::GetPose(Actions.HandGrip, Frame.Grip) && OpenXR::GetPose(Actions.HandAim, Frame.Pointer);
		Frame.LeftTracked = OpenXR::GetPose(Actions.LeftGrip, Frame.LeftGrip);
		Frame.Holding = Frame.RightTracked && GunVar && GunVar->current.enabled;
		if (Frame.Holding)
		{
			const vec2 hand = Heading(ToGamePose(Frame.Pointer, RecenterYaw, RecenterPosition).Axis[0]);
			Frame.HandYaw = hand.x;
			Frame.HandPitch = hand.y;
		}

		// A tracker sits on the body however it was strapped on, which is taken the first time it is seen
		// after a recenter, the player standing straight and facing the way it looks.
		for (int i = 0; i < VRTrackerCount; i++)
		{
			if (!Actions.Trackers[i] || !OpenXR::GetPose(Actions.Trackers[i], Frame.TrackerPoses[i]))
				continue;

			Frame.TrackerMask |= 1u << i;
			if (MountedTrackers & (1u << i))
				continue;
			const GamePose tracker = ToGamePose(Frame.TrackerPoses[i], RecenterYaw, RecenterPosition);
			TrackerMounts[i] = glm::transpose(tracker.Axis) * YawAxis(Frame.Yaw);
			MountedTrackers |= 1u << i;
		}
		float floor = 0.0f;
		if (Frame.TrackerMask && OpenXR::FloorHeight(floor))
			Frame.Floor = floor;
	}

	// The game lays out its 2D for the screen it believes it has, set once when the renderer starts.
	// With a headset frame the HUD goes in the band the panel shows; without one, the whole buffer the
	// window shows.
	void GVR::LayoutHud(bool headset)
	{
		const glm::ivec2 area = headset ? HudArea : RenderSize;
		for (ScreenPlacement* place : { scrPlaceView, scrPlaceFull, scrPlaceFullUnsafe })
			ScrPlace_SetupFloatViewport(place, 0.0f, 0.0f, static_cast<float>(area.x), static_cast<float>(area.y));
	}

	void GVR::Controls(float seconds)
	{
		const VRControls& input = Frame.Input;
		if (clients)
		{
			// Counted from the deadzone's edge, so the turn eases in instead of starting at 15% speed.
			const float turn = input.Turn.x;
			if (std::abs(turn) > StickDeadzone)
			{
				const float speed = TurnSpeedVar ? TurnSpeedVar->current.value : 180.0f;
				const float amount = (std::abs(turn) - StickDeadzone) / (1.0f - StickDeadzone);
				clients->viewangles[YAW] -= std::copysign(amount, turn) * speed * seconds;
			}

			if (input.Recenter && !Previous.Recenter)
				RecenterPending = true;
			if (input.Weapon && !Previous.Weapon)
				Cbuf_AddText(0, "weapnext\n");
			if (input.Menu && !Previous.Menu)
				PressEscape();
		}
		Previous = input;
	}

	// Takes the headset's current place and heading as the origin. Only the heading around the vertical
	// is taken, so looking down while recentring does not tilt the world.
	void GVR::Recenter()
	{
		if (Stale > StaleSeconds)
			Com_PrintMessage(CON_CHANNEL_LOG, "VR: view recentred on where the headset is now.\n", 0);

		RecenterPending = false;
		Stale = 0.0f;
		MountedTrackers = 0;

		const vec3 forward = Rotate(Frame.Head.orientation, { 0, 0, -1 });
		RecenterYaw = std::atan2(-forward.x, -forward.z);
		RecenterPosition = { Frame.Head.position.x, Frame.Head.position.y, Frame.Head.position.z };
	}

	// Rebuilds the view from the headset once the engine has built its own. The body keeps the yaw the
	// mouse and the controllers give it; everything else is the head. The refdef is left at the head
	// with a frustum wide enough for both eyes, which is what sound, effects and the HUD are drawn from.
	void GVR::CalcViewValues(int localClientNum)
	{
		Timestep::CalcViewValues(localClientNum);

		if (!Frame.Active)
			FollowView();
		if (!Frame.Active || !Frame.Tracked || GPortal::Rendering || !cgs || !clients)
			return;

		refdef_s& refdef = cgs->refdef;
		const playerState_s& ps = cgs->predictedPlayerState;

		// The command's yaw already carries the head, so the body is the mouse's part of it. Following
		// someone else, or a demo, the view is theirs and the head only looks around it.
		float yaw = cgs->refdefViewAngles[YAW];
		if (Controlled(ps))
			yaw = clients->viewangles[YAW] + ps.delta_angles[YAW];

		const mat3 body = YawAxis(yaw);
		const vec3 origin = refdef.vieworg;

		Place(refdef, body);

		// Without a hand to hold it, only carried along with the head, never turned with it:
		// CG_AddViewWeapon orients the gun from the player's view angles, which hold no roll, and only
		// places it from this.
		const vec3 moved = refdef.vieworg - origin;
		for (int i = 0; i < 3; i++)
			cgs->viewModelAxis[3][i] += moved[i];

		cgs->refdefViewAngles = Math::VectorToAngles(refdef.viewaxis[0]);

		if (GunVar && !GunVar->current.enabled)
			Hide(DrawGun, "cg_drawGun", GunHidden);
		Frame.Armed = true;

		ShareState(refdef, yaw);
	}

	// What others need to draw this player's body, and spectators to see through its eyes, relative to
	// where the player stands, and what the player sees of its own. The body faces the way the waist or
	// chest tracker does; without one, the way the player turned it, the play space's forward, kept within
	// a twist of where the head looks.
	void GVR::ShareState(const refdef_s& refdef, float body)
	{
		const bool gun = GunVar && GunVar->current.enabled;
		const playerState_s& ps = cgs->predictedPlayerState;
		if (!Controlled(ps) || ps.pm_type == PM_SPECTATOR || ps.pm_type >= PM_DEAD || cgs->renderingThirdPerson)
		{
			GVRBody::SetOwn(nullptr, gun);
			return;
		}

		const vec3 origin = ps.origin;
		const auto pose = [&](const vec3& at, const mat3& axis) { return VRNetPose{ at - origin, glm::quat_cast(axis) }; };

		VRNetState state;
		state.Parts = VRPartHead;
		state.HeadAngles = AxisAngles(refdef.viewaxis);
		const auto worn = [](VRTracker role) { return Frame.TrackerMask & (1u << static_cast<int>(role)); };
		if (worn(VRTracker::Waist) || worn(VRTracker::Chest))
		{
			const VRTracker hips = worn(VRTracker::Waist) ? VRTracker::Waist : VRTracker::Chest;
			state.BodyYaw = Heading(Frame.TrackerAxes[static_cast<int>(hips)][0]).x;
		}
		else
			state.BodyYaw = state.HeadAngles[YAW]
				+ std::clamp(Math::AngleDelta(body, state.HeadAngles[YAW]), -MaxTwist, MaxTwist);
		state.Head = pose(refdef.vieworg, refdef.viewaxis);
		if (Frame.RightTracked)
		{
			state.Parts |= VRPartRight;
			state.RightHand = pose(Frame.HandOrigin, Frame.HandAxis);
		}
		if (Frame.LeftTracked)
		{
			state.Parts |= VRPartLeft;
			state.LeftHand = pose(Frame.LeftOrigin, Frame.LeftAxis);
		}
		state.Trackers = Frame.TrackerMask;
		for (int i = 0; i < VRTrackerCount; i++)
		{
			if (state.Trackers & (1u << i))
				state.TrackerPoses[i] = pose(Frame.TrackerOrigins[i], Frame.TrackerAxes[i]);
		}
		GVRNetwork::Send(state);
		GVRBody::SetOwn(&state, gun);
	}

	// Following a VR player, the view is where that player's head is and looks, rather than where it
	// aims. Its head is relative to where it stands, which carries it however late the server shows it.
	void GVR::FollowView()
	{
		if (!cgs || GPortal::Rendering || clc.demoplaying)
			return;

		const playerState_s& ps = cgs->predictedPlayerState;
		const VRNetState* state = ps.clientNum != cgs->clientNum ? GVRNetwork::Get(ps.clientNum) : nullptr;
		if (!state || !(state->Parts & VRPartHead))
			return;

		cgs->refdef.vieworg = vec3(ps.origin) + state->Head.Offset;
		cgs->refdef.viewaxis = glm::mat3_cast(state->Head.Rotation);
		cgs->refdefViewAngles = state->HeadAngles;
	}

	// The eyes keep their spacing around the head, and the head follows the headset sideways as far as a
	// lean reaches, never in height.
	void GVR::Place(refdef_s& refdef, const mat3& body)
	{
		const GamePose head = ToGamePose(Frame.Head, RecenterYaw, RecenterPosition);
		const vec3 base = refdef.vieworg;

		vec2 lean(head.Position.x, head.Position.y);
		const float reach = glm::length(lean);
		if (reach > MaxLean)
			lean *= MaxLean / reach;
		const vec3 offset(lean, 0.0f);
		const auto place = [&](const vec3& p) { return base + body * ((p - head.Position + offset) * UnitsPerMeter); };

		for (int eye = 0; eye < 2; eye++)
		{
			const GamePose pose = ToGamePose(Frame.Eyes[eye].Pose, RecenterYaw, RecenterPosition);
			const XrFovf& fov = Frame.Eyes[eye].Fov;

			VRView& view = Frame.Views[eye];
			view.Origin = place(pose.Position);
			view.Axis = body * pose.Axis;
			view.Left = std::tan(fov.angleLeft);
			view.Right = std::tan(fov.angleRight);
			view.Down = std::tan(fov.angleDown);
			view.Up = std::tan(fov.angleUp);
		}

		if (Frame.RightTracked)
		{
			Frame.HandOrigin = place(ToGamePose(Frame.Grip, RecenterYaw, RecenterPosition).Position);
			Frame.HandAxis = body * ToGamePose(Frame.Pointer, RecenterYaw, RecenterPosition).Axis;
		}
		if (Frame.LeftTracked)
		{
			const GamePose left = ToGamePose(Frame.LeftGrip, RecenterYaw, RecenterPosition);
			Frame.LeftOrigin = place(left.Position);
			Frame.LeftAxis = body * left.Axis;
		}

		// Trackers stand on the floor, which the game keeps at the player's feet whatever height it gives
		// the view.
		const float feet = cgs->predictedPlayerState.origin[2];
		for (int i = 0; i < VRTrackerCount; i++)
		{
			if (!(Frame.TrackerMask & (1u << i)))
				continue;
			const GamePose tracker = ToGamePose(Frame.TrackerPoses[i], RecenterYaw, RecenterPosition);
			Frame.TrackerOrigins[i] = place(tracker.Position);
			if (Frame.Floor)
				Frame.TrackerOrigins[i].z = feet + (tracker.Position.z - (*Frame.Floor - RecenterPosition.y)) * UnitsPerMeter;
			Frame.TrackerAxes[i] = body * tracker.Axis * TrackerMounts[i];
		}

		const vec3 origin = place(head.Position);
		const mat3 axis = body * head.Axis;
		Frustum(origin, axis);

		refdef.vieworg = origin;
		refdef.viewaxis = axis;
		refdef.tanHalfFovX = Frame.CullTanX;
		refdef.tanHalfFovY = Frame.CullTanY;
	}

	// The one frustum the scene is culled with: from the head, turned the head's way, and pulled back
	// until both eyes' frustums fit inside it, eye spacing and canted displays included.
	void GVR::Frustum(const vec3& head, const mat3& axis)
	{
		float tanX = 0.1f;
		float tanY = 0.1f;

		for (const VRView& view : Frame.Views)
		{
			for (const float x : { view.Left, view.Right })
			{
				for (const float y : { view.Down, view.Up })
				{
					const vec3 ray = view.Axis[0] - x * view.Axis[1] + y * view.Axis[2];
					const float along = std::max(glm::dot(ray, axis[0]), 0.01f);

					tanX = std::max(tanX, std::abs(glm::dot(ray, axis[1])) / along);
					tanY = std::max(tanY, std::abs(glm::dot(ray, axis[2])) / along);
				}
			}
		}
		tanX *= CullMargin;
		tanY *= CullMargin;

		float back = 0.0f;
		for (const VRView& view : Frame.Views)
		{
			const vec3 offset = view.Origin - head;
			const float along = glm::dot(offset, axis[0]);

			back = std::max(back, std::abs(glm::dot(offset, axis[1])) / tanX - along);
			back = std::max(back, std::abs(glm::dot(offset, axis[2])) / tanY - along);
		}

		Frame.Cull = head - axis[0] * (back + 1.0f);
		Frame.CullTanX = tanX;
		Frame.CullTanY = tanY;
	}

	// The 2D the HUD draws from 3D positions, names and objective markers, is projected with the refdef.
	// Narrowed to the panel's own angle while it draws, those land where the panel shows them. The
	// crosshair marks where the head looks, which aims nothing while the gun is in the hand.
	void GVR::Draw2D(int localClientNum)
	{
		if (!Frame.Armed || !cgs)
		{
			CG_Draw2D_Original(localClientNum);
			return;
		}

		refdef_s& refdef = cgs->refdef;
		const float tanX = refdef.tanHalfFovX;
		const float tanY = refdef.tanHalfFovY;
		const vec2 hud = HudTangents();

		if (Frame.Holding)
			Hide(DrawCrosshair, "cg_drawCrosshair", CrosshairHidden);
		refdef.tanHalfFovX = hud.x;
		refdef.tanHalfFovY = hud.y;
		CG_Draw2D_Original(localClientNum);
		refdef.tanHalfFovX = tanX;
		refdef.tanHalfFovY = tanY;
		Show(DrawCrosshair, CrosshairHidden);
	}

	void GVR::RenderScene(const refdef_s* refdef)
	{
		if (!Frame.Armed || GPortal::Rendering || !cgs || refdef != &cgs->refdef)
		{
			R_RenderScene_Original(refdef);
			return;
		}
		Show(DrawGun, GunHidden);

		static refdef_s view;
		view = *refdef;
		view.vieworg = Frame.Cull;
		view.tanHalfFovX = Frame.CullTanX;
		view.tanHalfFovY = Frame.CullTanY;

		static const char* const names[] = { "r_lodScaleRigid", "r_lodBiasRigid", "r_lodScaleSkinned",
			"r_lodBiasSkinned" };
		static dvar_s* lods[std::size(names)] = {};
		float saved[std::size(names)] = {};

		// Level of detail is picked from the vertical field of view, and the headset's is about twice a
		// monitor's. The distances are scaled back to what cg_fov gives on the monitor, or models would
		// drop their detail at half the range.
		static dvar_s* const fov = Dvar::Find("cg_fov");
		static dvar_s* const fovScale = Dvar::Find("cg_fovScale");
		const float desktop = (fov ? fov->current.value : 65.0f) * (fovScale ? fovScale->current.value : 1.0f);
		const float tangent = std::tan(Math::DegToRad(std::clamp(desktop, 1.0f, 160.0f) * 0.5f)) * 0.75f;
		const float lod = std::min(1.0f, tangent / Frame.CullTanY);
		for (size_t i = 0; i < std::size(names); i++)
		{
			if (!lods[i])
				lods[i] = Dvar::Find(names[i]);
			if (!lods[i])
				continue;

			saved[i] = lods[i]->current.value;
			lods[i]->current.value *= lod;
		}

		R_RenderScene_Original(&view);

		for (size_t i = 0; i < std::size(names); i++)
		{
			if (lods[i])
				lods[i]->current.value = saved[i];
		}
	}

	// Stands in for RB_Draw3D. The scene was culled once, for both eyes; here it is drawn twice from the
	// same lists, each time from one eye. The left eye is finished off right here, post effects and all,
	// and captured before the view's 2D goes over it. The right eye is left for the engine to finish.
	void GVR::Draw3D()
	{
		GfxBackEndData* data = gfx_backEndData ? *gfx_backEndData : nullptr;

		if (!Frame.Armed || GPortal::Rendering || !data || data->viewInfoCount != 1 || !data->viewInfo
			|| *reinterpret_cast<const int*>(DrawScene) != DrawSceneStandard)
		{
			RB_Draw3D_h();
			return;
		}

		auto* view = reinterpret_cast<uint8_t*>(data->viewInfo) + data->viewInfoIndex * ViewInfoStride;
		ViewParms = reinterpret_cast<GfxViewParms*>(view);
		Commands = reinterpret_cast<const void**>(view + ViewInfoCommands);

		SavedParms = *ViewParms;
		SavedCommands = *Commands;

		Stage = VRStage::LeftEye;
		SetEye(*ViewParms, 0);
		RB_Draw3DInternal(reinterpret_cast<GfxViewInfo*>(view));
		RB_StandardDrawCommandsCommon();

		Stage = VRStage::RightEye;
		SetEye(*ViewParms, 1);
		RB_Draw3DInternal(reinterpret_cast<GfxViewInfo*>(view));
	}

	// Reached as a view's own 2D command list is read, with the eye finished in the frame buffer. Those
	// commands are never drawn over an eye, only into the HUD layer, so the left eye's pass reads none.
	void GVR::ViewCommands(GfxViewInfo* view)
	{
		if (!Commands || reinterpret_cast<const void**>(reinterpret_cast<uint8_t*>(view) + ViewInfoCommands) != Commands)
			return;

		if (Stage == VRStage::LeftEye)
		{
			Bridge->CaptureEye(0, FrameBuffer());
			*Commands = nullptr;
			Stage = VRStage::None;
		}
		else if (Stage == VRStage::RightEye)
		{
			Bridge->CaptureEye(1, FrameBuffer());
			*Commands = SavedCommands;
			Frame.Drawn = true;
			ClearFrameBuffer();
			BeginHud();
			Stage = VRStage::Hud;
		}
	}

	// Reached before every command list is walked; only the frame's shared one matters here, the menus
	// and the console. With no world drawn this frame, whatever the buffer still holds from the last one
	// is cleared out of the HUD layer before they go on.
	void GVR::SharedCommands(const void* cmds)
	{
		GfxBackEndData* data = gfx_backEndData ? *gfx_backEndData : nullptr;
		if (!Frame.Active || Stage != VRStage::None || GPortal::Rendering || !data || cmds != data->cmds)
			return;

		ClearFrameBuffer();
		BeginHud();
		Stage = VRStage::Hud;
	}

	// Reached once the engine is done with the frame, before the overlay. The HUD layer is taken now and
	// the desktop's own back buffer made the target, so the overlay lands on the monitor at the window's
	// size, where the mouse is, instead of in the headset's buffer.
	void GVR::BeforeOverlay()
	{
		if (!OwnsWindow() || GPortal::Rendering || !Ready || !dx || !dx->device)
			return;

		IDirect3DDevice9* device = dx->device;
		if (Frame.Active && Stage == VRStage::Hud)
		{
			Bridge->CapturePanel(FrameBuffer(), { 0, 0, HudArea.x, HudArea.y });
			EndHud();
			Frame.Hud = true;
		}

		IDirect3DSurface9* screen = nullptr;
		if (!IDirect3DDevice9_GetBackBuffer_h || FAILED(IDirect3DDevice9_GetBackBuffer_h(device, 0, 0,
													 D3DBACKBUFFER_TYPE_MONO, &screen))
			|| !screen)
			return;

		// D3D9On12 presents large back buffers black, so on that path the mirror and the overlay are drawn
		// into a canvas the bridge shows itself.
		IDirect3DSurface9* target = screen;
		RECT client = {};
		if (Graphics == &Direct3D12 && g_wv && GetClientRect(g_wv->hWnd, &client))
		{
			const glm::ivec2 window(client.right - client.left, client.bottom - client.top);
			if (window.x > 0 && window.y > 0)
				WindowCanvas = Bridge->Canvas(device, window);
			if (WindowCanvas)
				target = WindowCanvas;
		}
		WindowScreen = screen;

		// Without a headset frame, from a session that ended or before one started, the engine drew its
		// ordinary view into its buffer, and that is what the window gets.
		if (Frame.Active)
			Bridge->Mirror(target, Frame.Drawn, Frame.Hud, MirrorFocus());
		else
			Bridge->Mirror(target, FrameBuffer());

		device->GetRenderTarget(0, &EngineTarget);
		device->GetDepthStencilSurface(&EngineDepth);
		device->GetViewport(&EngineViewport);
		device->SetRenderTarget(0, target);
		device->SetDepthStencilSurface(nullptr);
	}

	void GVR::AfterOverlay()
	{
		if (GPortal::Rendering || !dx || !dx->device)
			return;

		if (EngineTarget)
		{
			IDirect3DDevice9* device = dx->device;
			device->SetRenderTarget(0, EngineTarget);
			device->SetDepthStencilSurface(EngineDepth);
			device->SetViewport(&EngineViewport);
			EngineTarget->Release();
			if (EngineDepth)
				EngineDepth->Release();
			EngineTarget = nullptr;
			EngineDepth = nullptr;
		}
		if (WindowScreen)
		{
			if (WindowCanvas && !(g_wv && Bridge->Present(g_wv->hWnd)))
				dx->device->StretchRect(WindowCanvas, nullptr, WindowScreen, nullptr, D3DTEXF_LINEAR);
			WindowScreen->Release();
			WindowScreen = nullptr;
			WindowCanvas = nullptr;
		}
		RestoreView();
		Stage = VRStage::None;
	}

	// This frame's pixels start their way back from the GPU, and the previous frame's, already there,
	// go to the runtime with the poses they were drawn from.
	void GVR::Submit()
	{
		if (!Frame.Active || GPortal::Rendering)
			return;
		Frame.Active = false;

		XRLayers layers;
		layers.Eyes = Frame.Drawn;
		layers.Views[0] = Frame.Eyes[0];
		layers.Views[1] = Frame.Eyes[1];
		layers.Panel = Frame.Hud;
		layers.PanelDistance = HudDistance;
		layers.PanelSize = HudTangents() * (2.0f * HudDistance);

		XRLayers ready;
		if (!Bridge->Deliver(layers, ready))
			ready = {};
		OpenXR::EndFrame(ready);
	}

	// The head aims, so whoever follows the player sees what the player sees: its yaw is added to the
	// body's and its pitch replaces the mouse's, which is dropped. The gun in the hand takes over while it
	// fires, moving still going the head's way. The controllers add their movement and buttons on top of
	// the keyboard's. The command is built before the next frame is drawn, from the last one's head and
	// controllers.
	void GVR::FinishMove(usercmd_s* cmd)
	{
		if (!cmd || !Frame.Tracked || !cgs || !clients)
			return;

		if (!client_ui || !(client_ui->keyCatchers & (KeyCatchConsole | KeyCatchUi | KeyCatchMessage)))
			ApplyControls(cmd);

		const auto now = std::chrono::steady_clock::now();
		const bool firing = Frame.Holding && (cmd->buttons & BUTTON_FIRE);
		if (firing)
			LastShot = now;
		const bool gun = Frame.Holding && now - LastShot < GunAimHold;

		// The server aims a shot with the angles of the command before it, so the gun takes the aim one
		// command before the trigger goes through.
		if (firing && !GunAimed)
			cmd->buttons &= ~BUTTON_FIRE;
		GunAimed = gun;

		const playerState_s& ps = cgs->predictedPlayerState;
		const float yaw = gun ? Frame.HandYaw : Frame.Yaw;
		const float pitch = gun ? Frame.HandPitch : Frame.Pitch;
		clients->viewangles[PITCH] = 0.0f;
		cmd->angles[PITCH] = ANGLE2SHORT(pitch - ps.delta_angles[PITCH]);
		cmd->angles[YAW] = ANGLE2SHORT(clients->viewangles[YAW] + yaw);
		if (gun)
			Steer(cmd, Frame.Yaw - yaw);
	}

	void GVR::ApplyControls(usercmd_s* cmd)
	{
		const VRControls& input = Frame.Input;
		if (glm::length(input.Move) > StickDeadzone)
		{
			cmd->forwardmove = ToMove(input.Move.y * 127.0f);
			cmd->rightmove = ToMove(input.Move.x * 127.0f);
		}

		int buttons = 0;
		if (input.Attack)
			buttons |= BUTTON_FIRE;
		if (input.Aim)
			buttons |= BUTTON_ADS;
		if (input.Jump)
			buttons |= BUTTON_JUMP;
		if (input.Crouch)
			buttons |= BUTTON_CROUCH;
		if (input.Use)
			buttons |= BUTTON_USE_RELOAD;
		if (input.Melee)
			buttons |= BUTTON_MELEE;
		if (input.Sprint)
			buttons |= BUTTON_SPRINT;
		cmd->buttons |= buttons;
	}

	// The view weapon is not drawn in VR, the player's body holds the gun, but its muzzle flash and laser
	// still come from it, so it is placed in the hand: the bone the gun hangs from lands in the controller,
	// turned the way it points. Where that bone sits in the model moves with every animation, so it is
	// measured each time, from the model first posed at the hand. The engine's posing is done here too,
	// and true skips the rest of it.
	bool GVR::ViewModelPose(DObj_s* obj)
	{
		if (Posing || !Frame.Armed || !Frame.Holding || !obj || !cgs || GPortal::Rendering)
			return false;

		const mat3& hand = Frame.HandAxis;
		PlaceViewModel(obj, hand, Frame.HandOrigin);

		float tag[3][3] = {};
		vec3 tagOrigin{};
		if (CG_DObjGetWorldTagMatrix(&cgs->viewModelPose, obj, scr_const_tag_weapon, tag, &tagOrigin[0]))
		{
			const mat3 local = glm::transpose(hand) * ToAxis(tag);
			const vec3 offset = glm::transpose(hand) * (tagOrigin - Frame.HandOrigin);
			const mat3 root = hand * glm::transpose(local);
			PlaceViewModel(obj, root, Frame.HandOrigin - root * offset);
		}
		return true;
	}

	// The engine poses the model from cg.viewModelAxis, forgetting every bone worked out so far.
	void GVR::PlaceViewModel(DObj_s* obj, const mat3& axis, const vec3& origin)
	{
		for (int i = 0; i < 3; i++)
		{
			for (int j = 0; j < 3; j++)
				cgs->viewModelAxis[i][j] = axis[i][j];
			cgs->viewModelAxis[3][i] = origin[i];
		}

		Posing = true;
		CG_UpdateViewModelPose(obj);
		Posing = false;
	}

	bool GVR::Command(const std::string& command)
	{
		if (command != "sr_vr_recenter")
			return false;

		RecenterPending = true;
		return true;
	}

	// While the HUD layer is drawn, alpha is accumulated as coverage whatever each material asks for, so
	// the layer carries its own transparency to the compositor. What the engine asked for is kept, to be
	// put back once the layer is done and its state cache is true again.
	HRESULT GVR::SetRenderState(IDirect3DDevice9* device, D3DRENDERSTATETYPE state, DWORD value)
	{
		if (HudPass)
		{
			switch (state)
			{
			case D3DRS_SEPARATEALPHABLENDENABLE:
				Requested[0] = value;
				value = TRUE;
				break;
			case D3DRS_SRCBLENDALPHA:
				Requested[1] = value;
				value = D3DBLEND_ONE;
				break;
			case D3DRS_DESTBLENDALPHA:
				Requested[2] = value;
				value = D3DBLEND_INVSRCALPHA;
				break;
			case D3DRS_BLENDOPALPHA:
				Requested[3] = value;
				value = D3DBLENDOP_ADD;
				break;
			case D3DRS_COLORWRITEENABLE:
				Requested[4] = value;
				if (value & (D3DCOLORWRITEENABLE_RED | D3DCOLORWRITEENABLE_GREEN | D3DCOLORWRITEENABLE_BLUE))
					value |= D3DCOLORWRITEENABLE_ALPHA;
				break;
			default:
				break;
			}
		}
		return IDirect3DDevice9_SetRenderState_h(device, state, value);
	}

	void GVR::BeginHud()
	{
		if (HudPass || !dx || !dx->device || !IDirect3DDevice9_SetRenderState_h)
			return;

		IDirect3DDevice9* device = dx->device;
		for (size_t i = 0; i < std::size(HudStates); i++)
			device->GetRenderState(HudStates[i], &Requested[i]);

		HudPass = true;
		for (size_t i = 0; i < std::size(HudStates); i++)
			SetRenderState(device, HudStates[i], Requested[i]);
	}

	void GVR::EndHud()
	{
		if (!HudPass)
			return;
		HudPass = false;

		if (!dx || !dx->device || !IDirect3DDevice9_SetRenderState_h)
			return;

		for (size_t i = 0; i < std::size(HudStates); i++)
			IDirect3DDevice9_SetRenderState_h(dx->device, HudStates[i], Requested[i]);
	}

	// ColorFill ignores every state but refuses a multisampled buffer, which only Clear takes, and Clear
	// goes through the viewport and the write mask.
	void GVR::ClearFrameBuffer()
	{
		IDirect3DDevice9* device = dx ? dx->device : nullptr;
		IDirect3DSurface9* target = FrameBuffer();
		if (!device || !target)
			return;

		D3DSURFACE_DESC desc = {};
		target->GetDesc(&desc);

		if (desc.MultiSampleType == D3DMULTISAMPLE_NONE)
		{
			device->ColorFill(target, nullptr, D3DCOLOR_ARGB(0, 0, 0, 0));
			return;
		}

		IDirect3DSurface9* current = nullptr;
		device->GetRenderTarget(0, &current);
		D3DVIEWPORT9 viewport = {};
		device->GetViewport(&viewport);
		DWORD scissor = FALSE;
		DWORD mask = 0xF;
		device->GetRenderState(D3DRS_SCISSORTESTENABLE, &scissor);
		device->GetRenderState(D3DRS_COLORWRITEENABLE, &mask);

		device->SetRenderTarget(0, target);
		IDirect3DDevice9_SetRenderState_h(device, D3DRS_SCISSORTESTENABLE, FALSE);
		IDirect3DDevice9_SetRenderState_h(device, D3DRS_COLORWRITEENABLE, 0xF);
		device->Clear(0, nullptr, D3DCLEAR_TARGET, D3DCOLOR_ARGB(0, 0, 0, 0), 1.0f, 0);
		IDirect3DDevice9_SetRenderState_h(device, D3DRS_COLORWRITEENABLE, mask);
		IDirect3DDevice9_SetRenderState_h(device, D3DRS_SCISSORTESTENABLE, scissor);

		if (current)
		{
			device->SetRenderTarget(0, current);
			current->Release();
		}
		device->SetViewport(&viewport);
	}

	void GVR::SetEye(GfxViewParms& parms, int eye)
	{
		const VRView& view = Frame.Views[eye];
		const vec3& o = view.Origin;
		const mat3& a = view.Axis;

		parms = SavedParms;

		// MatrixForViewer: the view space is x right, y up and z forward.
		float(&v)[4][4] = parms.viewMatrix.m;
		for (int i = 0; i < 3; i++)
		{
			v[i][0] = -a[1][i];
			v[i][1] = a[2][i];
			v[i][2] = a[0][i];
			v[i][3] = 0.0f;
		}
		v[3][0] = -(o.x * v[0][0] + o.y * v[1][0] + o.z * v[2][0]);
		v[3][1] = -(o.x * v[0][1] + o.y * v[1][1] + o.z * v[2][1]);
		v[3][2] = -(o.x * v[0][2] + o.y * v[1][2] + o.z * v[2][2]);
		v[3][3] = 1.0f;

		// InfinitePerspectiveMatrix, shifted off centre to the eye's own frustum.
		const float width = view.Right - view.Left;
		const float height = view.Up - view.Down;
		float(&p)[4][4] = parms.projectionMatrix.m;
		std::memset(p, 0, sizeof(parms.projectionMatrix.m));
		p[0][0] = 2.0f * ProjectionScale / width;
		p[1][1] = 2.0f * ProjectionScale / height;
		p[2][0] = -ProjectionScale * (view.Right + view.Left) / width;
		p[2][1] = -ProjectionScale * (view.Up + view.Down) / height;
		p[2][2] = ProjectionScale;
		p[2][3] = 1.0f;
		p[3][2] = -SavedParms.zNear * ProjectionScale;

		Multiply44(v, p, parms.viewProjectionMatrix.m);
		Inverse44(parms.viewProjectionMatrix.m, parms.inverseViewProjectionMatrix.m);

		parms.origin[0] = o.x;
		parms.origin[1] = o.y;
		parms.origin[2] = o.z;
		parms.origin[3] = 1.0f;
		for (int i = 0; i < 3; i++)
		{
			for (int j = 0; j < 3; j++)
				parms.axis[i][j] = a[i][j];
		}
	}

	void GVR::RestoreView()
	{
		if (ViewParms)
			*ViewParms = SavedParms;
		if (Commands)
			*Commands = SavedCommands;

		ViewParms = nullptr;
		Commands = nullptr;
	}

	bool GVR::Controlled(const playerState_s& ps)
	{
		return !clc.demoplaying && ps.clientNum == cgs->clientNum && ps.pm_type <= PM_SPECTATOR;
	}

	IDirect3DSurface9* GVR::FrameBuffer()
	{
		return gfx_renderTargets ? gfx_renderTargets[R_RENDERTARGET_FRAME_BUFFER].surface.color : nullptr;
	}

	glm::ivec2 GVR::FrameSize()
	{
		if (!gfx_renderTargets)
			return {};

		const GfxRenderTarget& target = gfx_renderTargets[R_RENDERTARGET_FRAME_BUFFER];
		return { static_cast<int>(target.width), static_cast<int>(target.height) };
	}

	// The panel's half angles, shaped like the band the HUD is laid out in.
	vec2 GVR::HudTangents()
	{
		const float fov = HudFovVar ? HudFovVar->current.value : 60.0f;
		const float tanX = std::tan(Math::DegToRad(fov * 0.5f));
		return { tanX, HudArea.x > 0 ? tanX * static_cast<float>(HudArea.y) / static_cast<float>(HudArea.x) : tanX };
	}

	// Where the left eye looks straight ahead, as a fraction of its image. A headset's eye is off centre,
	// so the window's crop centred on the image would not follow the head.
	vec2 GVR::MirrorFocus()
	{
		const XrFovf& fov = Frame.Eyes[0].Fov;
		const float left = std::tan(fov.angleLeft);
		const float right = std::tan(fov.angleRight);
		const float down = std::tan(fov.angleDown);
		const float up = std::tan(fov.angleUp);
		if (right <= left || up <= down)
			return vec2(0.5f);

		return { -left / (right - left), up / (up - down) };
	}
}

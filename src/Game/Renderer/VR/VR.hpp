#pragma once
#include "Game/Base.hpp"

#include "Engine/Backend/DX9/XRBridge.hpp"
#include "Engine/Backend/DX9/XRD3D12Bridge.hpp"

namespace IW3SR
{
	// The game's unit is the inch.
	constexpr float UnitsPerMeter = 39.3701f;

	// How close the free hand has to come to where the animation holds the weapon, in meters, to be left
	// there, and how far before it follows its controller alone.
	constexpr float VRGripNear = 0.09f;
	constexpr float VRGripFar = 0.16f;

	// Which pass the backend is in, so the captures know what the frame buffer holds.
	enum class VRStage
	{
		None,
		LeftEye,
		RightEye,
		Hud
	};

	// One eye placed in the world, with the edges of its frustum as tangents.
	struct VRView
	{
		vec3 Origin{};
		mat3 Axis{ 1.0f };
		float Left = -1.0f;
		float Right = 1.0f;
		float Down = -1.0f;
		float Up = 1.0f;
	};

	// The controllers for one frame, reduced to what the game acts on.
	struct VRControls
	{
		vec2 Move{};
		vec2 Turn{};
		bool Jump = false;
		bool Crouch = false;
		bool Attack = false;
		bool Aim = false;
		bool Use = false;
		bool Melee = false;
		bool Sprint = false;
		bool Weapon = false;
		bool Menu = false;
		bool Recenter = false;
	};

	// Body trackers, by the role the runtime gave each one.
	enum class VRTracker
	{
		Waist,
		Chest,
		LeftFoot,
		RightFoot,
		LeftKnee,
		RightKnee,
		LeftElbow,
		RightElbow,
		Count
	};
	constexpr int VRTrackerCount = static_cast<int>(VRTracker::Count);

	struct VRActions
	{
		XrAction Move = XR_NULL_HANDLE;
		XrAction Turn = XR_NULL_HANDLE;
		XrAction Jump = XR_NULL_HANDLE;
		XrAction Crouch = XR_NULL_HANDLE;
		XrAction Attack = XR_NULL_HANDLE;
		XrAction Aim = XR_NULL_HANDLE;
		XrAction Use = XR_NULL_HANDLE;
		XrAction Melee = XR_NULL_HANDLE;
		XrAction Sprint = XR_NULL_HANDLE;
		XrAction Weapon = XR_NULL_HANDLE;
		XrAction Menu = XR_NULL_HANDLE;
		XrAction Recenter = XR_NULL_HANDLE;
		XrAction HandGrip = XR_NULL_HANDLE;
		XrAction HandAim = XR_NULL_HANDLE;
		XrAction LeftGrip = XR_NULL_HANDLE;
		XrAction Trackers[VRTrackerCount] = {};
	};

	// Everything a frame is drawn from, gathered as it starts. It outlives the frame until the next one
	// starts, since the game builds its next command from it before drawing again.
	struct VRFrame
	{
		bool Active = false;
		bool Tracked = false;
		bool Armed = false;
		bool Drawn = false;
		bool Hud = false;
		bool Holding = false;
		bool RightTracked = false;
		bool LeftTracked = false;
		XRView Eyes[2];
		XrPosef Head{ { 0, 0, 0, 1 }, { 0, 0, 0 } };
		XrPosef Grip{ { 0, 0, 0, 1 }, { 0, 0, 0 } };
		XrPosef Pointer{ { 0, 0, 0, 1 }, { 0, 0, 0 } };
		XrPosef LeftGrip{ { 0, 0, 0, 1 }, { 0, 0, 0 } };
		VRControls Input;
		float Yaw = 0.0f;
		float Pitch = 0.0f;
		float HandYaw = 0.0f;
		float HandPitch = 0.0f;
		vec3 HandOrigin{};
		mat3 HandAxis{ 1.0f };
		vec3 LeftOrigin{};
		mat3 LeftAxis{ 1.0f };
		uint32_t TrackerMask = 0;
		XrPosef TrackerPoses[VRTrackerCount] = {};
		vec3 TrackerOrigins[VRTrackerCount] = {};
		mat3 TrackerAxes[VRTrackerCount] = {};
		std::optional<float> Floor;
		VRView Views[2];
		vec3 Cull{};
		float CullTanX = 1.0f;
		float CullTanY = 1.0f;
	};

	// Renders the game to an OpenXR headset: both eyes from one culled scene, the HUD and menus on a
	// panel held in front of the head, and the head aiming the player.
	class API GVR
	{
	public:
		static void Startup();
		static IDirect3D9* STDCALL CreateDirect3D(UINT sdkVersion);
		static void MouseEvent(int x, int y);
		static void Initialize();
		static void Shutdown();

		static void WindowParms(GfxWindowParms* parms);
		static bool OwnsWindow();
		static glm::ivec2 WindowSize();
		static void PresentSize(D3DPRESENT_PARAMETERS* parameters);

		static void BeginFrame();
		static void CalcViewValues(int localClientNum);
		static void Draw2D(int localClientNum);
		static void RenderScene(const refdef_s* refdef);
		static void Draw3D();
		static void ViewCommands(GfxViewInfo* view);
		static void SharedCommands(const void* cmds);
		static void BeforeOverlay();
		static void AfterOverlay();
		static void Submit();
		static void FinishMove(usercmd_s* cmd);
		static bool ViewModelPose(DObj_s* obj);
		static bool Command(const std::string& command);

		static HRESULT STDCALL SetRenderState(IDirect3DDevice9* device, D3DRENDERSTATETYPE state, DWORD value);
		static HRESULT STDCALL CreateDevice(IDirect3D9* d3d, UINT adapter, D3DDEVTYPE type, HWND window, DWORD flags,
			D3DPRESENT_PARAMETERS* parameters, IDirect3DDevice9** device);
		static HRESULT STDCALL GetBackBuffer(IDirect3DDevice9* device, UINT swapChain, UINT index, D3DBACKBUFFER_TYPE type,
			IDirect3DSurface9** surface);

	private:
		static inline XRD3D11 Direct3D11;
		static inline XRD3D12 Direct3D12;
		static inline XRGraphics* Graphics = nullptr;
		static inline Scope<DX9XRCapture> Bridge;
		static inline bool TwelveFailed = false;
		static inline bool Enabled = false;
		static inline bool Started = false;
		static inline bool Ready = false;
		static inline bool Failed = false;
		static inline DWORD MainThread = 0;
		static inline glm::ivec2 RenderSize{};
		static inline glm::ivec2 HudArea{};
		static inline glm::ivec2 HudSize{};
		static inline glm::ivec2 DesktopSize{};
		static inline bool DesktopFullscreen = false;
		static inline IDirect3DSurface9* EngineTarget = nullptr;
		static inline IDirect3DSurface9* WindowScreen = nullptr;
		static inline IDirect3DSurface9* WindowCanvas = nullptr;
		static inline IDirect3DSurface9* EngineDepth = nullptr;
		static inline D3DVIEWPORT9 EngineViewport = {};

		static inline VRFrame Frame;
		static inline VRStage Stage = VRStage::None;
		static inline GfxViewParms* ViewParms = nullptr;
		static inline GfxViewParms SavedParms = {};
		static inline const void** Commands = nullptr;
		static inline const void* SavedCommands = nullptr;

		static inline VRActions Actions;
		static inline VRControls Previous;
		static inline bool RecenterPending = true;
		static inline float RecenterYaw = 0.0f;
		static inline vec3 RecenterPosition{};
		static inline float Stale = 0.0f;
		static inline mat3 TrackerMounts[VRTrackerCount] = {};
		static inline uint32_t MountedTrackers = 0;
		static inline std::chrono::steady_clock::time_point LastFrame;
		static inline std::chrono::steady_clock::time_point LastShot;
		static inline bool GunAimed = false;

		static inline bool HudPass = false;
		static inline DWORD Requested[5] = {};
		static inline bool Posing = false;

		static inline dvar_s* SmpBackend = nullptr;
		static inline bool SmpRestore = false;
		static inline dvar_s* Vsync = nullptr;
		static inline bool VsyncRestore = false;
		static inline dvar_s* DrawGun = nullptr;
		static inline bool GunHidden = false;
		static inline dvar_s* DrawCrosshair = nullptr;
		static inline bool CrosshairHidden = false;

		static inline dvar_s* EnabledVar = nullptr;
		static inline dvar_s* ScaleVar = nullptr;
		static inline dvar_s* HudFovVar = nullptr;
		static inline dvar_s* GunVar = nullptr;
		static inline dvar_s* TurnSpeedVar = nullptr;

		static void RegisterDvars();
		static bool Connect(XRGraphics& graphics, std::string& error);
		static void Disconnect();
		static void CreateActions();
		static VRControls ReadControls();
		static void Controls(float seconds);
		static void ApplyControls(usercmd_s* cmd);
		static void Recenter();
		static void LayoutHud(bool headset);
		static void Place(refdef_s& refdef, const mat3& body);
		static void ShareState(const refdef_s& refdef, float body);
		static void FollowView();
		static void PlaceViewModel(DObj_s* obj, const mat3& axis, const vec3& origin);
		static void Frustum(const vec3& head, const mat3& axis);
		static void SetEye(GfxViewParms& parms, int eye);
		static void BeginHud();
		static void EndHud();
		static void ClearFrameBuffer();
		static void RestoreView();
		static bool Controlled(const playerState_s& ps);
		static IDirect3DSurface9* FrameBuffer();
		static glm::ivec2 FrameSize();
		static vec2 HudTangents();
		static vec2 MirrorFocus();
	};
}

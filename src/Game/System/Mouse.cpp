#include "Mouse.hpp"
#include "Dvar.hpp"

// The engine derives the view from the OS pointer, which loses counts to acceleration and to the
// edges of the screen, and CoD4X's raw path recenters the cursor from inside the WM_INPUT handler,
// which a high polling rate turns into thousands of cursor calls a second.
//
// Instead the pointer is pinned to a single pixel for as long as the engine is moving the view: its
// own per frame path keeps running and keeps owning menus, the cursor and the choice between the
// two, but always reads a null delta, while the raw counts go straight into the accumulator
// CL_MouseEvent would have written. Nothing but a read of the report happens per event.

namespace IW3SR
{
	// Mouse motion reported in absolute coordinates, kept to turn it back into a delta.
	static POINT absolute = { 0, 0 };
	static bool hasAbsolute = false;

	// Last rectangle handed to ClipCursor, so an unchanged window costs nothing.
	static RECT clipped = { 0, 0, 0, 0 };

	void GMouse::Initialize()
	{
		RawInputDvar = Dvar::RegisterBool("sr_rawinput", DVAR_SAVED,
			"Read the mouse straight from raw input instead of the OS pointer", true);

		Enabled = RawInputDvar && RawInputDvar->current.enabled;
	}

	void GMouse::Shutdown()
	{
		SetLooking(false);
	}

	// Drives the mouse ownership state machine, once per frame on the main thread.
	void GMouse::Frame()
	{
		if (!RawInputDvar)
			return;

		if (!LegacyRawInputDvar)
			LegacyRawInputDvar = Dvar::Find("raw_input");
		if (!MouseDvar)
			MouseDvar = Dvar::Find("in_mouse");

		const bool enabled = RawInputDvar->current.enabled && (!MouseDvar || MouseDvar->current.enabled);
		if (Enabled != enabled)
		{
			Enabled = enabled;
			if (!enabled)
				SetLooking(false);
		}
		if (!Enabled)
			return;

		// CoD4X's raw path makes its IN_Frame skip the per frame update we read our state from, so
		// it has to stay off for as long as we own the mouse.
		if (LegacyRawInputDvar && LegacyRawInputDvar->current.integer)
		{
			LegacyRawInputDvar->current.integer = 0;
			LegacyRawInputDvar->latched.integer = 0;

			if (!Overridden)
			{
				Overridden = true;
				Log::WriteLine(Channel::Warning, "raw_input is superseded by sr_rawinput and stays off.");
			}
		}

		const HWND hwnd = reinterpret_cast<HWND>(Window::Handle);
		SetLooking(hwnd && GetForegroundWindow() == hwnd && IsGameplay());

		if (Looking)
			Confine();
	}

	// Turns a WM_INPUT message into view movement. Runs at the polling rate of the device, so it
	// must stay free of window and cursor calls.
	RawInput GMouse::Process(uintptr_t rawInput)
	{
		RAWINPUT raw;
		UINT size = sizeof(raw);

		if (GetRawInputData(reinterpret_cast<HRAWINPUT>(rawInput), RID_INPUT, &raw, &size, sizeof(RAWINPUTHEADER))
			== static_cast<UINT>(-1))
			return RawInput::Ignored;

		if (raw.header.dwType == RIM_TYPEKEYBOARD)
			return RawInput::Keyboard;
		if (raw.header.dwType != RIM_TYPEMOUSE)
			return RawInput::Ignored;

		const RAWMOUSE& mouse = raw.data.mouse;

		// Buttons and the wheel are rare enough that reparsing them in the engine is free.
		if (!Enabled || mouse.usButtonFlags)
			Mouse::Process(WM_INPUT, rawInput);

		if (!Enabled)
			return RawInput::Handled;
		if (!Looking)
			return RawInput::Consumed;

		int dx = mouse.lLastX;
		int dy = mouse.lLastY;

		// Remote desktop, tablets and virtual machines report a position instead of a delta.
		if (mouse.usFlags & MOUSE_MOVE_ABSOLUTE)
		{
			const bool desktop = mouse.usFlags & MOUSE_VIRTUAL_DESKTOP;
			const int width = GetSystemMetrics(desktop ? SM_CXVIRTUALSCREEN : SM_CXSCREEN);
			const int height = GetSystemMetrics(desktop ? SM_CYVIRTUALSCREEN : SM_CYSCREEN);

			const POINT position = { MulDiv(mouse.lLastX, width, USHRT_MAX), MulDiv(mouse.lLastY, height, USHRT_MAX) };

			dx = hasAbsolute ? position.x - absolute.x : 0;
			dy = hasAbsolute ? position.y - absolute.y : 0;

			absolute = position;
			hasAbsolute = true;
		}
		else
		{
			hasAbsolute = false;
		}

		if (dx || dy)
		{
			clientActive_t& cl = clients[0];
			cl.mouseDx[cl.mouseIndex] += dx;
			cl.mouseDy[cl.mouseIndex] += dy;
		}
		return RawInput::Consumed;
	}

	bool GMouse::IsLooking()
	{
		return Looking;
	}

	// CL_MouseEvent already picks between view movement and a menu pointer every frame and leaves
	// the answer in g_wv.recenterMouse. Reading it back keeps the scoreboard, the quick message
	// menu and cl_bypassMouseInput behaving exactly as they do without us.
	bool GMouse::IsGameplay()
	{
		return !UI::Open && g_wv->recenterMouse;
	}

	void GMouse::SetLooking(bool state)
	{
		if (Looking == state)
			return;
		Looking = state;

		hasAbsolute = false;
		SetRectEmpty(&clipped);

		if (state)
		{
			SetCursorHidden(true);
			Confine();
		}
		else
		{
			ClipCursor(nullptr);
			SetCursorHidden(false);
		}

		// The engine remembers where the pointer was, and a stale position snaps the view.
		if (s_wmv)
			GetCursorPos(&s_wmv->oldPos);
	}

	// Pins the pointer to the center of the window. It cannot click through to another application,
	// it stops emitting WM_MOUSEMOVE, and the IN_MouseMove the engine still runs every frame reads
	// a null delta instead of adding the same motion a second time.
	void GMouse::Confine()
	{
		const HWND hwnd = reinterpret_cast<HWND>(Window::Handle);
		if (!hwnd)
			return;

		RECT client;
		GetClientRect(hwnd, &client);
		MapWindowPoints(hwnd, nullptr, reinterpret_cast<POINT*>(&client), 2);

		if (IsRectEmpty(&client))
			return;

		const POINT center = { (client.left + client.right) / 2, (client.top + client.bottom) / 2 };
		const RECT rect = { center.x, center.y, center.x + 1, center.y + 1 };

		if (EqualRect(&rect, &clipped))
			return;

		clipped = rect;
		ClipCursor(&rect);
	}

	// CL_MouseEvent owns the cursor counter, but it only runs while the engine drives the pointer.
	void GMouse::SetCursorHidden(bool state)
	{
		if (CursorHidden == state)
			return;
		CursorHidden = state;

		if (state)
			while (ShowCursor(false) >= 0)
				;
		else
			while (ShowCursor(true) < 0)
				;
	}
}

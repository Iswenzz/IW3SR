#pragma once
#include "DX9/Base.hpp"

namespace IW3SR
{
	/// <summary>
	/// DirectX3D 9EX.
	/// </summary>
	class D3D9EX : public IDirect3D9Ex
	{
	public:
		D3D9EX(IDirect3D9Ex* d3d9ex);
		virtual ~D3D9EX() = default;

		COM_DECLSPEC_NOTHROW HRESULT STDCALL QueryInterface(REFIID riid, void** ppvObj) override;
		COM_DECLSPEC_NOTHROW ULONG STDCALL AddRef() override;
		COM_DECLSPEC_NOTHROW ULONG STDCALL Release() override;
		COM_DECLSPEC_NOTHROW HRESULT STDCALL RegisterSoftwareDevice(void* pInitializeFunction) override;
		COM_DECLSPEC_NOTHROW UINT STDCALL GetAdapterCount() override;
		COM_DECLSPEC_NOTHROW HRESULT STDCALL GetAdapterIdentifier(UINT Adapter, DWORD Flags,
			D3DADAPTER_IDENTIFIER9* pIdentifier) override;
		COM_DECLSPEC_NOTHROW UINT STDCALL GetAdapterModeCount(UINT Adapter, D3DFORMAT Format) override;
		COM_DECLSPEC_NOTHROW HRESULT STDCALL EnumAdapterModes(UINT Adapter, D3DFORMAT Format, UINT Mode,
			D3DDISPLAYMODE* pMode) override;
		COM_DECLSPEC_NOTHROW HRESULT STDCALL GetAdapterDisplayMode(UINT Adapter, D3DDISPLAYMODE* pMode) override;
		COM_DECLSPEC_NOTHROW HRESULT STDCALL CheckDeviceType(UINT iAdapter, D3DDEVTYPE DevType, D3DFORMAT DisplayFormat,
			D3DFORMAT BackBufferFormat, BOOL bWindowed) override;
		COM_DECLSPEC_NOTHROW HRESULT STDCALL CheckDeviceFormat(UINT Adapter, D3DDEVTYPE DeviceType,
			D3DFORMAT AdapterFormat, DWORD Usage, D3DRESOURCETYPE RType, D3DFORMAT CheckFormat) override;
		COM_DECLSPEC_NOTHROW HRESULT STDCALL CheckDeviceMultiSampleType(UINT Adapter, D3DDEVTYPE DeviceType,
			D3DFORMAT SurfaceFormat, BOOL Windowed, D3DMULTISAMPLE_TYPE MultiSampleType,
			DWORD* pQualityLevels) override;
		COM_DECLSPEC_NOTHROW HRESULT STDCALL CheckDepthStencilMatch(UINT Adapter, D3DDEVTYPE DeviceType,
			D3DFORMAT AdapterFormat, D3DFORMAT RenderTargetFormat, D3DFORMAT DepthStencilFormat) override;
		COM_DECLSPEC_NOTHROW HRESULT STDCALL CheckDeviceFormatConversion(UINT Adapter, D3DDEVTYPE DeviceType,
			D3DFORMAT SourceFormat, D3DFORMAT TargetFormat) override;
		COM_DECLSPEC_NOTHROW HRESULT STDCALL GetDeviceCaps(UINT Adapter, D3DDEVTYPE DeviceType,
			D3DCAPS9* pCaps) override;
		COM_DECLSPEC_NOTHROW HMONITOR STDCALL GetAdapterMonitor(UINT Adapter) override;
		COM_DECLSPEC_NOTHROW HRESULT STDCALL CreateDevice(UINT Adapter, D3DDEVTYPE DeviceType, HWND hFocusWindow,
			DWORD BehaviorFlags, D3DPRESENT_PARAMETERS* pPresentationParameters,
			IDirect3DDevice9** ppReturnedDeviceInterface) override;
		COM_DECLSPEC_NOTHROW UINT STDCALL GetAdapterModeCountEx(UINT Adapter,
			const D3DDISPLAYMODEFILTER* pFilter) override;
		COM_DECLSPEC_NOTHROW HRESULT STDCALL EnumAdapterModesEx(UINT Adapter, const D3DDISPLAYMODEFILTER* pFilter,
			UINT Mode, D3DDISPLAYMODEEX* pMode) override;
		COM_DECLSPEC_NOTHROW HRESULT STDCALL GetAdapterDisplayModeEx(UINT Adapter, D3DDISPLAYMODEEX* pMode,
			D3DDISPLAYROTATION* pRotation) override;
		COM_DECLSPEC_NOTHROW HRESULT STDCALL CreateDeviceEx(UINT Adapter, D3DDEVTYPE DeviceType, HWND hFocusWindow,
			DWORD BehaviorFlags, D3DPRESENT_PARAMETERS* pPresentationParameters,
			D3DDISPLAYMODEEX* pFullscreenDisplayMode, IDirect3DDevice9Ex** ppReturnedDeviceInterface) override;
		COM_DECLSPEC_NOTHROW HRESULT STDCALL GetAdapterLUID(UINT Adapter, LUID* pLUID) override;

	private:
		IDirect3D9Ex* pIDirect3D9Ex;
	};
}

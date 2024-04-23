#include "D3D9EX.hpp"
#include "D3D9.hpp"

namespace IW3SR
{
	D3D9EX::D3D9EX(IDirect3D9Ex* d3d9ex) : pIDirect3D9Ex(d3d9ex) { }

	HRESULT D3D9EX::QueryInterface(REFIID riid, void** ppvObj)
	{
		*ppvObj = nullptr;
		HRESULT hRes = pIDirect3D9Ex->QueryInterface(riid, ppvObj);

		if (hRes == NOERROR)
			*ppvObj = this;
		return hRes;
	}

	ULONG D3D9EX::AddRef()
	{
		return pIDirect3D9Ex->AddRef();
	}

	ULONG D3D9EX::Release()
	{
		ULONG count = pIDirect3D9Ex->Release();
		if (!count)
			delete this;
		return count;
	}

	HRESULT D3D9EX::RegisterSoftwareDevice(void* pInitializeFunction)
	{
		return pIDirect3D9Ex->RegisterSoftwareDevice(pInitializeFunction);
	}

	UINT D3D9EX::GetAdapterCount()
	{
		return pIDirect3D9Ex->GetAdapterCount();
	}

	HRESULT D3D9EX::GetAdapterIdentifier(UINT Adapter, DWORD Flags, D3DADAPTER_IDENTIFIER9* pIdentifier)
	{
		return pIDirect3D9Ex->GetAdapterIdentifier(Adapter, Flags, pIdentifier);
	}

	UINT D3D9EX::GetAdapterModeCount(UINT Adapter, D3DFORMAT Format)
	{
		return pIDirect3D9Ex->GetAdapterModeCount(Adapter, Format);
	}

	HRESULT D3D9EX::EnumAdapterModes(UINT Adapter, D3DFORMAT Format, UINT Mode, D3DDISPLAYMODE* pMode)
	{
		return pIDirect3D9Ex->EnumAdapterModes(Adapter, Format, Mode, pMode);
	}

	HRESULT D3D9EX::GetAdapterDisplayMode(UINT Adapter, D3DDISPLAYMODE* pMode)
	{
		return pIDirect3D9Ex->GetAdapterDisplayMode(Adapter, pMode);
	}

	HRESULT D3D9EX::CheckDeviceType(UINT iAdapter, D3DDEVTYPE DevType, D3DFORMAT DisplayFormat,
		D3DFORMAT BackBufferFormat, BOOL bWindowed)
	{
		return pIDirect3D9Ex->CheckDeviceType(iAdapter, DevType, DisplayFormat, BackBufferFormat, bWindowed);
	}

	HRESULT D3D9EX::CheckDeviceFormat(UINT Adapter, D3DDEVTYPE DeviceType, D3DFORMAT AdapterFormat, DWORD Usage,
		D3DRESOURCETYPE RType, D3DFORMAT CheckFormat)
	{
		return pIDirect3D9Ex->CheckDeviceFormat(Adapter, DeviceType, AdapterFormat, Usage, RType, CheckFormat);
	}

	HRESULT D3D9EX::CheckDeviceMultiSampleType(UINT Adapter, D3DDEVTYPE DeviceType, D3DFORMAT SurfaceFormat,
		BOOL Windowed, D3DMULTISAMPLE_TYPE MultiSampleType, DWORD* pQualityLevels)
	{
		return pIDirect3D9Ex->CheckDeviceMultiSampleType(Adapter, DeviceType, SurfaceFormat, Windowed, MultiSampleType,
			pQualityLevels);
	}

	HRESULT D3D9EX::CheckDepthStencilMatch(UINT Adapter, D3DDEVTYPE DeviceType, D3DFORMAT AdapterFormat,
		D3DFORMAT RenderTargetFormat, D3DFORMAT DepthStencilFormat)
	{
		return pIDirect3D9Ex->CheckDepthStencilMatch(Adapter, DeviceType, AdapterFormat, RenderTargetFormat,
			DepthStencilFormat);
	}

	HRESULT D3D9EX::CheckDeviceFormatConversion(UINT Adapter, D3DDEVTYPE DeviceType, D3DFORMAT SourceFormat,
		D3DFORMAT TargetFormat)
	{
		return pIDirect3D9Ex->CheckDeviceFormatConversion(Adapter, DeviceType, SourceFormat, TargetFormat);
	}

	HRESULT D3D9EX::GetDeviceCaps(UINT Adapter, D3DDEVTYPE DeviceType, D3DCAPS9* pCaps)
	{
		return pIDirect3D9Ex->GetDeviceCaps(Adapter, DeviceType, pCaps);
	}

	HMONITOR D3D9EX::GetAdapterMonitor(UINT Adapter)
	{
		return pIDirect3D9Ex->GetAdapterMonitor(Adapter);
	}

	HRESULT D3D9EX::CreateDevice(UINT Adapter, D3DDEVTYPE DeviceType, HWND hFocusWindow, DWORD BehaviorFlags,
		D3DPRESENT_PARAMETERS* pPresentationParameters, IDirect3DDevice9** ppReturnedDeviceInterface)
	{
		HRESULT hres = pIDirect3D9Ex->CreateDevice(Adapter, DeviceType, hFocusWindow, BehaviorFlags,
			pPresentationParameters, ppReturnedDeviceInterface);

		*ppReturnedDeviceInterface = new D3D9Device(*ppReturnedDeviceInterface);
		return hres;
	}

	UINT D3D9EX::GetAdapterModeCountEx(UINT Adapter, const D3DDISPLAYMODEFILTER* pFilter)
	{
		return (pIDirect3D9Ex->GetAdapterModeCountEx(Adapter, pFilter));
	}

	HRESULT D3D9EX::EnumAdapterModesEx(UINT Adapter, const D3DDISPLAYMODEFILTER* pFilter, UINT Mode,
		D3DDISPLAYMODEEX* pMode)
	{
		return (pIDirect3D9Ex->EnumAdapterModesEx(Adapter, pFilter, Mode, pMode));
	}

	HRESULT D3D9EX::GetAdapterDisplayModeEx(UINT Adapter, D3DDISPLAYMODEEX* pMode, D3DDISPLAYROTATION* pRotation)
	{
		return (pIDirect3D9Ex->GetAdapterDisplayModeEx(Adapter, pMode, pRotation));
	}

	HRESULT D3D9EX::CreateDeviceEx(UINT Adapter, D3DDEVTYPE DeviceType, HWND hFocusWindow, DWORD BehaviorFlags,
		D3DPRESENT_PARAMETERS* pPresentationParameters, D3DDISPLAYMODEEX* pFullscreenDisplayMode,
		IDirect3DDevice9Ex** ppReturnedDeviceInterface)
	{
		return (pIDirect3D9Ex->CreateDeviceEx(Adapter, DeviceType, hFocusWindow, BehaviorFlags, pPresentationParameters,
			pFullscreenDisplayMode, ppReturnedDeviceInterface));
	}

	HRESULT D3D9EX::GetAdapterLUID(UINT Adapter, LUID* pLUID)
	{
		return (pIDirect3D9Ex->GetAdapterLUID(Adapter, pLUID));
	}
}

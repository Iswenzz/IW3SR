#pragma once
#include "DX9/Base.hpp"

namespace IW3SR
{
	class D3D9 : public IDirect3D9
	{
	public:
		D3D9(IDirect3D9* d3d9);
		virtual ~D3D9() = default;

		static IDirect3D9* STDCALL Direct3DCreate9(UINT sdk);

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

	private:
		IDirect3D9* pIDirect3D9;
	};

	class D3D9Device : public IDirect3DDevice9
	{
	public:
		D3D9Device(IDirect3DDevice9* d3d9_device);
		virtual ~D3D9Device() = default;

		COM_DECLSPEC_NOTHROW HRESULT STDCALL QueryInterface(REFIID riid, void** ppvObj) override;
		COM_DECLSPEC_NOTHROW ULONG STDCALL AddRef() override;
		COM_DECLSPEC_NOTHROW ULONG STDCALL Release() override;
		COM_DECLSPEC_NOTHROW HRESULT STDCALL TestCooperativeLevel() override;
		COM_DECLSPEC_NOTHROW UINT STDCALL GetAvailableTextureMem() override;
		COM_DECLSPEC_NOTHROW HRESULT STDCALL EvictManagedResources() override;
		COM_DECLSPEC_NOTHROW HRESULT STDCALL GetDirect3D(IDirect3D9** ppD3D9) override;
		COM_DECLSPEC_NOTHROW HRESULT STDCALL GetDeviceCaps(D3DCAPS9* pCaps) override;
		COM_DECLSPEC_NOTHROW HRESULT STDCALL GetDisplayMode(UINT iSwapChain, D3DDISPLAYMODE* pMode) override;
		COM_DECLSPEC_NOTHROW HRESULT STDCALL GetCreationParameters(D3DDEVICE_CREATION_PARAMETERS* pParameters) override;
		COM_DECLSPEC_NOTHROW HRESULT STDCALL SetCursorProperties(UINT XHotSpot, UINT YHotSpot,
			IDirect3DSurface9* pCursorBitmap) override;
		COM_DECLSPEC_NOTHROW void STDCALL SetCursorPosition(int X, int Y, DWORD Flags) override;
		COM_DECLSPEC_NOTHROW BOOL STDCALL ShowCursor(BOOL bShow) override;
		COM_DECLSPEC_NOTHROW HRESULT STDCALL CreateAdditionalSwapChain(D3DPRESENT_PARAMETERS* pPresentationParameters,
			IDirect3DSwapChain9** pSwapChain) override;
		COM_DECLSPEC_NOTHROW HRESULT STDCALL GetSwapChain(UINT iSwapChain, IDirect3DSwapChain9** pSwapChain) override;
		COM_DECLSPEC_NOTHROW UINT STDCALL GetNumberOfSwapChains() override;
		COM_DECLSPEC_NOTHROW HRESULT STDCALL Reset(D3DPRESENT_PARAMETERS* pPresentationParameters) override;
		COM_DECLSPEC_NOTHROW HRESULT STDCALL Present(CONST RECT* pSourceRect, CONST RECT* pDestRect,
			HWND hDestWindowOverride, CONST RGNDATA* pDirtyRegion) override;
		COM_DECLSPEC_NOTHROW HRESULT STDCALL GetBackBuffer(UINT iSwapChain, UINT iBackBuffer, D3DBACKBUFFER_TYPE Type,
			IDirect3DSurface9** ppBackBuffer) override;
		COM_DECLSPEC_NOTHROW HRESULT STDCALL GetRasterStatus(UINT iSwapChain, D3DRASTER_STATUS* pRasterStatus) override;
		COM_DECLSPEC_NOTHROW HRESULT STDCALL SetDialogBoxMode(BOOL bEnableDialogs) override;
		COM_DECLSPEC_NOTHROW void STDCALL SetGammaRamp(UINT iSwapChain, DWORD Flags,
			CONST D3DGAMMARAMP* pRamp) override;
		COM_DECLSPEC_NOTHROW void STDCALL GetGammaRamp(UINT iSwapChain, D3DGAMMARAMP* pRamp) override;
		COM_DECLSPEC_NOTHROW HRESULT STDCALL CreateTexture(UINT Width, UINT Height, UINT Levels, DWORD Usage,
			D3DFORMAT Format, D3DPOOL Pool, IDirect3DTexture9** ppTexture, HANDLE* pSharedHandle) override;
		COM_DECLSPEC_NOTHROW HRESULT STDCALL CreateVolumeTexture(UINT Width, UINT Height, UINT Depth, UINT Levels,
			DWORD Usage, D3DFORMAT Format, D3DPOOL Pool, IDirect3DVolumeTexture9** ppVolumeTexture,
			HANDLE* pSharedHandle) override;
		COM_DECLSPEC_NOTHROW HRESULT STDCALL CreateCubeTexture(UINT EdgeLength, UINT Levels, DWORD Usage,
			D3DFORMAT Format, D3DPOOL Pool, IDirect3DCubeTexture9** ppCubeTexture, HANDLE* pSharedHandle) override;
		COM_DECLSPEC_NOTHROW HRESULT STDCALL CreateVertexBuffer(UINT Length, DWORD Usage, DWORD FVF, D3DPOOL Pool,
			IDirect3DVertexBuffer9** ppVertexBuffer, HANDLE* pSharedHandle) override;
		COM_DECLSPEC_NOTHROW HRESULT STDCALL CreateIndexBuffer(UINT Length, DWORD Usage, D3DFORMAT Format, D3DPOOL Pool,
			IDirect3DIndexBuffer9** ppIndexBuffer, HANDLE* pSharedHandle) override;
		COM_DECLSPEC_NOTHROW HRESULT STDCALL CreateRenderTarget(UINT Width, UINT Height, D3DFORMAT Format,
			D3DMULTISAMPLE_TYPE MultiSample, DWORD MultisampleQuality, BOOL Lockable, IDirect3DSurface9** ppSurface,
			HANDLE* pSharedHandle) override;
		COM_DECLSPEC_NOTHROW HRESULT STDCALL CreateDepthStencilSurface(UINT Width, UINT Height, D3DFORMAT Format,
			D3DMULTISAMPLE_TYPE MultiSample, DWORD MultisampleQuality, BOOL Discard, IDirect3DSurface9** ppSurface,
			HANDLE* pSharedHandle) override;
		COM_DECLSPEC_NOTHROW HRESULT STDCALL UpdateSurface(IDirect3DSurface9* pSourceSurface, CONST RECT* pSourceRect,
			IDirect3DSurface9* pDestinationSurface, CONST POINT* pDestPoint) override;
		COM_DECLSPEC_NOTHROW HRESULT STDCALL UpdateTexture(IDirect3DBaseTexture9* pSourceTexture,
			IDirect3DBaseTexture9* pDestinationTexture) override;
		COM_DECLSPEC_NOTHROW HRESULT STDCALL GetRenderTargetData(IDirect3DSurface9* pRenderTarget,
			IDirect3DSurface9* pDestSurface) override;
		COM_DECLSPEC_NOTHROW HRESULT STDCALL GetFrontBufferData(UINT iSwapChain,
			IDirect3DSurface9* pDestSurface) override;
		COM_DECLSPEC_NOTHROW HRESULT STDCALL StretchRect(IDirect3DSurface9* pSourceSurface, CONST RECT* pSourceRect,
			IDirect3DSurface9* pDestSurface, CONST RECT* pDestRect, D3DTEXTUREFILTERTYPE Filter) override;
		COM_DECLSPEC_NOTHROW HRESULT STDCALL ColorFill(IDirect3DSurface9* pSurface, CONST RECT* pRect,
			D3DCOLOR color) override;
		COM_DECLSPEC_NOTHROW HRESULT STDCALL CreateOffscreenPlainSurface(UINT Width, UINT Height, D3DFORMAT Format,
			D3DPOOL Pool, IDirect3DSurface9** ppSurface, HANDLE* pSharedHandle) override;
		COM_DECLSPEC_NOTHROW HRESULT STDCALL SetRenderTarget(DWORD RenderTargetIndex,
			IDirect3DSurface9* pRenderTarget) override;
		COM_DECLSPEC_NOTHROW HRESULT STDCALL GetRenderTarget(DWORD RenderTargetIndex,
			IDirect3DSurface9** ppRenderTarget) override;
		COM_DECLSPEC_NOTHROW HRESULT STDCALL SetDepthStencilSurface(IDirect3DSurface9* pNewZStencil) override;
		COM_DECLSPEC_NOTHROW HRESULT STDCALL GetDepthStencilSurface(IDirect3DSurface9** ppZStencilSurface) override;
		COM_DECLSPEC_NOTHROW HRESULT STDCALL BeginScene() override;
		COM_DECLSPEC_NOTHROW HRESULT STDCALL EndScene() override;
		COM_DECLSPEC_NOTHROW HRESULT STDCALL Clear(DWORD Vertex, CONST D3DRECT* pRects, DWORD Flags, D3DCOLOR Color,
			float Z, DWORD Stencil) override;
		COM_DECLSPEC_NOTHROW HRESULT STDCALL SetTransform(D3DTRANSFORMSTATETYPE State,
			CONST D3DMATRIX* pMatrix) override;
		COM_DECLSPEC_NOTHROW HRESULT STDCALL GetTransform(D3DTRANSFORMSTATETYPE State, D3DMATRIX* pMatrix) override;
		COM_DECLSPEC_NOTHROW HRESULT STDCALL MultiplyTransform(D3DTRANSFORMSTATETYPE State,
			CONST D3DMATRIX* pMatrix) override;
		COM_DECLSPEC_NOTHROW HRESULT STDCALL SetViewport(CONST D3DVIEWPORT9* pViewport) override;
		COM_DECLSPEC_NOTHROW HRESULT STDCALL GetViewport(D3DVIEWPORT9* pViewport) override;
		COM_DECLSPEC_NOTHROW HRESULT STDCALL SetMaterial(CONST D3DMATERIAL9* pMaterial) override;
		COM_DECLSPEC_NOTHROW HRESULT STDCALL GetMaterial(D3DMATERIAL9* pMaterial) override;
		COM_DECLSPEC_NOTHROW HRESULT STDCALL SetLight(DWORD Index, CONST D3DLIGHT9* pLight) override;
		COM_DECLSPEC_NOTHROW HRESULT STDCALL GetLight(DWORD Index, D3DLIGHT9* pLight) override;
		COM_DECLSPEC_NOTHROW HRESULT STDCALL LightEnable(DWORD Index, BOOL Enable) override;
		COM_DECLSPEC_NOTHROW HRESULT STDCALL GetLightEnable(DWORD Index, BOOL* pEnable) override;
		COM_DECLSPEC_NOTHROW HRESULT STDCALL SetClipPlane(DWORD Index, CONST float* pPlane) override;
		COM_DECLSPEC_NOTHROW HRESULT STDCALL GetClipPlane(DWORD Index, float* pPlane) override;
		COM_DECLSPEC_NOTHROW HRESULT STDCALL SetRenderState(D3DRENDERSTATETYPE State, DWORD Value) override;
		COM_DECLSPEC_NOTHROW HRESULT STDCALL GetRenderState(D3DRENDERSTATETYPE State, DWORD* pValue) override;
		COM_DECLSPEC_NOTHROW HRESULT STDCALL CreateStateBlock(D3DSTATEBLOCKTYPE Type,
			IDirect3DStateBlock9** ppSB) override;
		COM_DECLSPEC_NOTHROW HRESULT STDCALL BeginStateBlock() override;
		COM_DECLSPEC_NOTHROW HRESULT STDCALL EndStateBlock(IDirect3DStateBlock9** ppSB) override;
		COM_DECLSPEC_NOTHROW HRESULT STDCALL SetClipStatus(CONST D3DCLIPSTATUS9* pClipStatus) override;
		COM_DECLSPEC_NOTHROW HRESULT STDCALL GetClipStatus(D3DCLIPSTATUS9* pClipStatus) override;
		COM_DECLSPEC_NOTHROW HRESULT STDCALL GetTexture(DWORD Stage, IDirect3DBaseTexture9** ppTexture) override;
		COM_DECLSPEC_NOTHROW HRESULT STDCALL SetTexture(DWORD Stage, IDirect3DBaseTexture9* pTexture) override;
		COM_DECLSPEC_NOTHROW HRESULT STDCALL GetTextureStageState(DWORD Stage, D3DTEXTURESTAGESTATETYPE Type,
			DWORD* pValue) override;
		COM_DECLSPEC_NOTHROW HRESULT STDCALL SetTextureStageState(DWORD Stage, D3DTEXTURESTAGESTATETYPE Type,
			DWORD Value) override;
		COM_DECLSPEC_NOTHROW HRESULT STDCALL GetSamplerState(DWORD Sampler, D3DSAMPLERSTATETYPE Type,
			DWORD* pValue) override;
		COM_DECLSPEC_NOTHROW HRESULT STDCALL SetSamplerState(DWORD Sampler, D3DSAMPLERSTATETYPE Type,
			DWORD Value) override;
		COM_DECLSPEC_NOTHROW HRESULT STDCALL ValidateDevice(DWORD* pNumPasses) override;
		COM_DECLSPEC_NOTHROW HRESULT STDCALL SetPaletteEntries(UINT PaletteNumber,
			CONST PALETTEENTRY* pEntries) override;
		COM_DECLSPEC_NOTHROW HRESULT STDCALL GetPaletteEntries(UINT PaletteNumber, PALETTEENTRY* pEntries) override;
		COM_DECLSPEC_NOTHROW HRESULT STDCALL SetCurrentTexturePalette(UINT PaletteNumber) override;
		COM_DECLSPEC_NOTHROW HRESULT STDCALL GetCurrentTexturePalette(UINT* PaletteNumber) override;
		COM_DECLSPEC_NOTHROW HRESULT STDCALL SetScissorRect(CONST RECT* pRect) override;
		COM_DECLSPEC_NOTHROW HRESULT STDCALL GetScissorRect(RECT* pRect) override;
		COM_DECLSPEC_NOTHROW HRESULT STDCALL SetSoftwareVertexProcessing(BOOL bSoftware) override;
		COM_DECLSPEC_NOTHROW BOOL STDCALL GetSoftwareVertexProcessing() override;
		COM_DECLSPEC_NOTHROW HRESULT STDCALL SetNPatchMode(float nSegments) override;
		COM_DECLSPEC_NOTHROW float STDCALL GetNPatchMode() override;
		COM_DECLSPEC_NOTHROW HRESULT STDCALL DrawPrimitive(D3DPRIMITIVETYPE PrimitiveType, UINT StartVertex,
			UINT PrimitiveCount) override;
		COM_DECLSPEC_NOTHROW HRESULT STDCALL DrawIndexedPrimitive(D3DPRIMITIVETYPE PrimitiveType, INT BaseVertexIndex,
			UINT MinVertexIndex, UINT NumVertices, UINT startIndex, UINT primCount) override;
		COM_DECLSPEC_NOTHROW HRESULT STDCALL DrawPrimitiveUP(D3DPRIMITIVETYPE PrimitiveType, UINT PrimitiveCount,
			CONST void* pVertexStreamZeroData, UINT VertexStreamZeroStride) override;
		COM_DECLSPEC_NOTHROW HRESULT STDCALL DrawIndexedPrimitiveUP(D3DPRIMITIVETYPE PrimitiveType, UINT MinVertexIndex,
			UINT NumVertices, UINT PrimitiveCount, CONST void* pIndexData, D3DFORMAT IndexDataFormat,
			CONST void* pVertexStreamZeroData, UINT VertexStreamZeroStride) override;
		COM_DECLSPEC_NOTHROW HRESULT STDCALL ProcessVertices(UINT SrcStartIndex, UINT DestIndex, UINT VertexCount,
			IDirect3DVertexBuffer9* pDestBuffer, IDirect3DVertexDeclaration9* pVertexDecl, DWORD Flags) override;
		COM_DECLSPEC_NOTHROW HRESULT STDCALL CreateVertexDeclaration(CONST D3DVERTEXELEMENT9* pVertexElements,
			IDirect3DVertexDeclaration9** ppDecl) override;
		COM_DECLSPEC_NOTHROW HRESULT STDCALL SetVertexDeclaration(IDirect3DVertexDeclaration9* pDecl) override;
		COM_DECLSPEC_NOTHROW HRESULT STDCALL GetVertexDeclaration(IDirect3DVertexDeclaration9** ppDecl) override;
		COM_DECLSPEC_NOTHROW HRESULT STDCALL SetFVF(DWORD FVF) override;
		COM_DECLSPEC_NOTHROW HRESULT STDCALL GetFVF(DWORD* pFVF) override;
		COM_DECLSPEC_NOTHROW HRESULT STDCALL CreateVertexShader(CONST DWORD* pFunction,
			IDirect3DVertexShader9** ppShader) override;
		COM_DECLSPEC_NOTHROW HRESULT STDCALL SetVertexShader(IDirect3DVertexShader9* pShader) override;
		COM_DECLSPEC_NOTHROW HRESULT STDCALL GetVertexShader(IDirect3DVertexShader9** ppShader) override;
		COM_DECLSPEC_NOTHROW HRESULT STDCALL SetVertexShaderConstantF(UINT StartRegister, CONST float* pConstantData,
			UINT Vector4fCount) override;
		COM_DECLSPEC_NOTHROW HRESULT STDCALL GetVertexShaderConstantF(UINT StartRegister, float* pConstantData,
			UINT Vector4fCount) override;
		COM_DECLSPEC_NOTHROW HRESULT STDCALL SetVertexShaderConstantI(UINT StartRegister, CONST int* pConstantData,
			UINT Vector4iCount) override;
		COM_DECLSPEC_NOTHROW HRESULT STDCALL GetVertexShaderConstantI(UINT StartRegister, int* pConstantData,
			UINT Vector4iCount) override;
		COM_DECLSPEC_NOTHROW HRESULT STDCALL SetVertexShaderConstantB(UINT StartRegister, CONST BOOL* pConstantData,
			UINT BoolCount) override;
		COM_DECLSPEC_NOTHROW HRESULT STDCALL GetVertexShaderConstantB(UINT StartRegister, BOOL* pConstantData,
			UINT BoolCount) override;
		COM_DECLSPEC_NOTHROW HRESULT STDCALL SetStreamSource(UINT StreamNumber, IDirect3DVertexBuffer9* pStreamData,
			UINT OffsetInBytes, UINT Stride) override;
		COM_DECLSPEC_NOTHROW HRESULT STDCALL GetStreamSource(UINT StreamNumber, IDirect3DVertexBuffer9** ppStreamData,
			UINT* OffsetInBytes, UINT* pStride) override;
		COM_DECLSPEC_NOTHROW HRESULT STDCALL SetStreamSourceFreq(UINT StreamNumber, UINT Divider) override;
		COM_DECLSPEC_NOTHROW HRESULT STDCALL GetStreamSourceFreq(UINT StreamNumber, UINT* Divider) override;
		COM_DECLSPEC_NOTHROW HRESULT STDCALL SetIndices(IDirect3DIndexBuffer9* pIndexData) override;
		COM_DECLSPEC_NOTHROW HRESULT STDCALL GetIndices(IDirect3DIndexBuffer9** ppIndexData) override;
		COM_DECLSPEC_NOTHROW HRESULT STDCALL CreatePixelShader(CONST DWORD* pFunction,
			IDirect3DPixelShader9** ppShader) override;
		COM_DECLSPEC_NOTHROW HRESULT STDCALL SetPixelShader(IDirect3DPixelShader9* pShader) override;
		COM_DECLSPEC_NOTHROW HRESULT STDCALL GetPixelShader(IDirect3DPixelShader9** ppShader) override;
		COM_DECLSPEC_NOTHROW HRESULT STDCALL SetPixelShaderConstantF(UINT StartRegister, CONST float* pConstantData,
			UINT Vector4fCount) override;
		COM_DECLSPEC_NOTHROW HRESULT STDCALL GetPixelShaderConstantF(UINT StartRegister, float* pConstantData,
			UINT Vector4fCount) override;
		COM_DECLSPEC_NOTHROW HRESULT STDCALL SetPixelShaderConstantI(UINT StartRegister, CONST int* pConstantData,
			UINT Vector4iCount) override;
		COM_DECLSPEC_NOTHROW HRESULT STDCALL GetPixelShaderConstantI(UINT StartRegister, int* pConstantData,
			UINT Vector4iCount) override;
		COM_DECLSPEC_NOTHROW HRESULT STDCALL SetPixelShaderConstantB(UINT StartRegister, CONST BOOL* pConstantData,
			UINT BoolCount) override;
		COM_DECLSPEC_NOTHROW HRESULT STDCALL GetPixelShaderConstantB(UINT StartRegister, BOOL* pConstantData,
			UINT BoolCount) override;
		COM_DECLSPEC_NOTHROW HRESULT STDCALL DrawRectPatch(UINT Handle, CONST float* pNumSegs,
			CONST D3DRECTPATCH_INFO* pRectPatchInfo) override;
		COM_DECLSPEC_NOTHROW HRESULT STDCALL DrawTriPatch(UINT Handle, CONST float* pNumSegs,
			CONST D3DTRIPATCH_INFO* pTriPatchInfo) override;
		COM_DECLSPEC_NOTHROW HRESULT STDCALL DeletePatch(UINT Handle) override;
		COM_DECLSPEC_NOTHROW HRESULT STDCALL CreateQuery(D3DQUERYTYPE Type, IDirect3DQuery9** ppQuery) override;

	private:
		IDirect3DDevice9* pIDirect3DDevice9;
	};
}

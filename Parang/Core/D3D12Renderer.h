#pragma once

namespace Parang {

	const UINT SWAP_CHAIN_FRAME_COUNT = 2; // Double buffering
	
	class D3D12Renderer
	{
	public:
		D3D12Renderer();
		~D3D12Renderer();

		BOOL Initialize(HWND hWnd, BOOL bEnableDebugLayer, BOOL bEnableGBV); // GBV: GPU Based Validation
		void BeginRender();
		void EndRender();
		void Present();
		BOOL UpdateWindowSize(DWORD dwBackBufferWidth, DWORD dwBackBufferHeight);

		void* CreateBasicMeshObject();
		void  DeleteBasicMeshObject(void* pMeshObjHandle);
		void  RenderMeshObject(void* pMeshObjHandle);

		// for internal
		ID3D12Device5* INL_GetD3DDevice() const { return m_pD3DDevice; }

	private:
		void CreateFence();
		void CleanupFence();
		void CreateCommandList();
		void CleanupCommandList();
		BOOL CreateDescriptorHeap();
		void CleanupDescriptorHeap();

		UINT64 Fence();
		void WaitForFenceValue();

		void Cleanup();

	private:
		HWND	m_hWnd = nullptr;
		ID3D12Device5* m_pD3DDevice = nullptr;
		ID3D12CommandQueue* m_pCommandQueue = nullptr;
		ID3D12CommandAllocator* m_pCommandAllocator = nullptr;
		ID3D12GraphicsCommandList* m_pCommandList = nullptr;
		UINT64	m_ui64FenceValue = 0;

		D3D_FEATURE_LEVEL	m_FeatureLevel = D3D_FEATURE_LEVEL_11_0;
		DXGI_ADAPTER_DESC1	m_AdapterDesc = {};
		IDXGISwapChain3*	m_pSwapChain = nullptr; // VSync등 기능지원
		D3D12_VIEWPORT  m_Viewport = {};
		D3D12_RECT		m_ScissorRect = {};
		DWORD			m_dwWidth = 0;
		DWORD			m_dwHeight = 0;

		ID3D12Resource*			m_pRenderTargets[SWAP_CHAIN_FRAME_COUNT] = {};
		ID3D12DescriptorHeap*	m_pRTVHeap = nullptr;
		ID3D12DescriptorHeap*	m_pDSVHeap = nullptr;
		ID3D12DescriptorHeap*	m_pSRVHeap = nullptr;
		UINT	m_rtvDescriptorSize = 0;
		UINT	m_dwSwapChainFlags = 0;
		UINT	m_uiRenderTargetIndex = 0;
		HANDLE	m_hFenceEvent = nullptr;
		ID3D12Fence* m_pFence = nullptr;

		DWORD	m_dwCurContextIndex = 0;
	};

}
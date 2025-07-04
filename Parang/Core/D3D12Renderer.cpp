#include "pch.h"
#include <dxgi.h>
#include <dxgidebug.h>
#include "Util/D3DUtils.h"
#include "D3D12Renderer.h"
#include "BasicMeshObject.h"

namespace Parang {
	D3D12Renderer::D3D12Renderer()
	{

	}
	D3D12Renderer::~D3D12Renderer()
	{
		Cleanup();
	}

	BOOL D3D12Renderer::Initialize(HWND hWnd, BOOL bEnableDebugLayer, BOOL bEnableGBV)
	{
	BOOL	bResult = FALSE;

	HRESULT hr = S_OK;
	ID3D12Debug*	pDebugController = nullptr;
	IDXGIFactory4*	pFactory = nullptr;
	IDXGIAdapter1*	pAdapter = nullptr;
	DXGI_ADAPTER_DESC1 AdapterDesc = {};

	DWORD dwCreateFlags = 0;
	DWORD dwCreateFactoryFlags = 0;

	// if use debug Layer...
	if (bEnableDebugLayer)
	{
		// Enable the D3D12 debug layer.
		if (SUCCEEDED(hr = D3D12GetDebugInterface(IID_PPV_ARGS(&pDebugController))))
		{
			pDebugController->EnableDebugLayer();
		}
		dwCreateFactoryFlags = DXGI_CREATE_FACTORY_DEBUG;
		if (bEnableGBV)
		{
			ID3D12Debug5*	pDebugController5 = nullptr;
			if (S_OK == pDebugController->QueryInterface(IID_PPV_ARGS(&pDebugController5)))
			{
				pDebugController5->SetEnableGPUBasedValidation(TRUE);
				pDebugController5->SetEnableAutoName(TRUE);
				pDebugController5->Release();
			}
		}
	}

	// Create DXGIFactory
	CreateDXGIFactory2(dwCreateFactoryFlags, IID_PPV_ARGS(&pFactory));

	D3D_FEATURE_LEVEL	featureLevels[] =
	{
		D3D_FEATURE_LEVEL_12_2,
		D3D_FEATURE_LEVEL_12_1,
		D3D_FEATURE_LEVEL_12_0,
		D3D_FEATURE_LEVEL_11_1,
		D3D_FEATURE_LEVEL_11_0
	};

	DWORD	FeatureLevelNum = _countof(featureLevels);

	for (DWORD featerLevelIndex = 0; featerLevelIndex < FeatureLevelNum; featerLevelIndex++)
	{
		UINT adapterIndex = 0;
		while (DXGI_ERROR_NOT_FOUND != pFactory->EnumAdapters1(adapterIndex, &pAdapter))
		{
			pAdapter->GetDesc1(&AdapterDesc);

			if (SUCCEEDED(D3D12CreateDevice(pAdapter, featureLevels[featerLevelIndex], IID_PPV_ARGS(&m_pD3DDevice))))
			{
				goto lb_exit;

			}
			pAdapter->Release();
			pAdapter = nullptr;
			adapterIndex++;
		}
	}
lb_exit:

	if (!m_pD3DDevice)
	{
		__debugbreak();
		goto lb_return;
	}

	m_AdapterDesc = AdapterDesc;
	m_hWnd = hWnd;

	if (pDebugController)
	{
		SetDebugLayerInfo(m_pD3DDevice);
	}

	// Describe and create the command queue.
	{
		D3D12_COMMAND_QUEUE_DESC queueDesc = {};
		queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
		queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;

		hr = m_pD3DDevice->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&m_pCommandQueue));
		if (FAILED(hr))
		{
			__debugbreak();
			goto lb_return;
		}
	}

	CreateDescriptorHeap();

	RECT	rect;
	::GetClientRect(hWnd, &rect);
	DWORD	dwWndWidth = rect.right - rect.left;
	DWORD	dwWndHeight = rect.bottom - rect.top;
	UINT	dwBackBufferWidth = rect.right - rect.left;
	UINT	dwBackBufferHeight = rect.bottom - rect.top;

	// Describe and create the swap chain.
	{
		DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
		swapChainDesc.Width = dwBackBufferWidth;
		swapChainDesc.Height = dwBackBufferHeight;
		swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		//swapChainDesc.BufferDesc.RefreshRate.Numerator = m_uiRefreshRate;
		//swapChainDesc.BufferDesc.RefreshRate.Denominator = 1;
		swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		swapChainDesc.BufferCount = SWAP_CHAIN_FRAME_COUNT;
		swapChainDesc.SampleDesc.Count = 1;
		swapChainDesc.SampleDesc.Quality = 0;
		swapChainDesc.Scaling = DXGI_SCALING_NONE;
		swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
		swapChainDesc.AlphaMode = DXGI_ALPHA_MODE_IGNORE;
		swapChainDesc.Flags |= DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING;
		m_dwSwapChainFlags = swapChainDesc.Flags;


		DXGI_SWAP_CHAIN_FULLSCREEN_DESC fsSwapChainDesc = {};
		fsSwapChainDesc.Windowed = TRUE;

		IDXGISwapChain1*	pSwapChain1 = nullptr;
		if (FAILED(pFactory->CreateSwapChainForHwnd(m_pCommandQueue, hWnd, &swapChainDesc, &fsSwapChainDesc, nullptr, &pSwapChain1)))
		{
			__debugbreak();
		}
		pSwapChain1->QueryInterface(IID_PPV_ARGS(&m_pSwapChain));
		pSwapChain1->Release();
		pSwapChain1 = nullptr;
		m_uiRenderTargetIndex = m_pSwapChain->GetCurrentBackBufferIndex(); // 0, 1
	}

	m_Viewport.Width = (float)dwWndWidth;
	m_Viewport.Height = (float)dwWndHeight;
	m_Viewport.MinDepth = 0.0f;
	m_Viewport.MaxDepth = 1.0f;

	m_ScissorRect.left = 0;
	m_ScissorRect.top = 0;
	m_ScissorRect.right = dwWndWidth;
	m_ScissorRect.bottom = dwWndHeight;

	m_dwWidth = dwWndWidth;
	m_dwHeight = dwWndHeight;

	// Create frame resources.
	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(m_pRTVHeap->GetCPUDescriptorHandleForHeapStart());

	// Create a RTV for each frame.
	// Descriptor Table
	// |        0        |        1	       |
	// | Render Target 0 | Render Target 1 |
	for (UINT n = 0; n < SWAP_CHAIN_FRAME_COUNT; n++)
	{
		m_pSwapChain->GetBuffer(n, IID_PPV_ARGS(&m_pRenderTargets[n]));
		m_pD3DDevice->CreateRenderTargetView(m_pRenderTargets[n], nullptr, rtvHandle);
		rtvHandle.Offset(1, m_rtvDescriptorSize);
	}

	CreateCommandList();
	
	// Create synchronization objects.
	CreateFence();

	bResult = TRUE;
lb_return:
	if (pDebugController)
	{
		pDebugController->Release();
		pDebugController = nullptr;
	}
	if (pAdapter)
	{
		pAdapter->Release();
		pAdapter = nullptr;
	}
	if (pFactory)
	{
		pFactory->Release();
		pFactory = nullptr;
	}
	return bResult;

	}

	BOOL D3D12Renderer::UpdateWindowSize(DWORD dwBackBufferWidth, DWORD dwBackBufferHeight)
	{
		BOOL	bResult = FALSE;

		if (!(dwBackBufferWidth * dwBackBufferHeight))
			return FALSE;

		if (m_dwWidth == dwBackBufferWidth && m_dwHeight == dwBackBufferHeight)
			return FALSE;
		//WaitForFenceValue();

		/*
		if (FAILED(m_pCommandAllocator->Reset()))
			__debugbreak();

		if (FAILED(m_pCommandList->Reset(m_pCommandAllocator,nullptr)))
			__debugbreak();

		m_pCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_pRenderTargets[m_uiRenderTargetIndex], D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));

		CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(m_pRTVHeap->GetCPUDescriptorHandleForHeapStart(), m_uiRenderTargetIndex, m_rtvDescriptorSize);
		CD3DX12_CPU_DESCRIPTOR_HANDLE dsvHandle(m_pDSVHeap->GetCPUDescriptorHandleForHeapStart());

		m_pCommandList->OMSetRenderTargets(1, &rtvHandle, FALSE, &dsvHandle);
		*/


		DXGI_SWAP_CHAIN_DESC1	desc;
		HRESULT	hr = m_pSwapChain->GetDesc1(&desc);
		if (FAILED(hr))
			__debugbreak();

		for (UINT n = 0; n < SWAP_CHAIN_FRAME_COUNT; n++)
		{
			m_pRenderTargets[n]->Release();
			m_pRenderTargets[n] = nullptr;
		}

		if (FAILED(m_pSwapChain->ResizeBuffers(SWAP_CHAIN_FRAME_COUNT, dwBackBufferWidth, dwBackBufferHeight, DXGI_FORMAT_R8G8B8A8_UNORM, m_dwSwapChainFlags)))
		{
			__debugbreak();
		}
		m_uiRenderTargetIndex = m_pSwapChain->GetCurrentBackBufferIndex();

		// Create frame resources.
		CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(m_pRTVHeap->GetCPUDescriptorHandleForHeapStart());

		// Create a RTV for each frame.
		for (UINT n = 0; n < SWAP_CHAIN_FRAME_COUNT; n++)
		{
			m_pSwapChain->GetBuffer(n, IID_PPV_ARGS(&m_pRenderTargets[n]));
			m_pD3DDevice->CreateRenderTargetView(m_pRenderTargets[n], nullptr, rtvHandle);
			rtvHandle.Offset(1, m_rtvDescriptorSize);
		}
		m_dwWidth = dwBackBufferWidth;
		m_dwHeight = dwBackBufferHeight;
		m_Viewport.Width = (float)m_dwWidth;
		m_Viewport.Height = (float)m_dwHeight;
		m_ScissorRect.left = 0;
		m_ScissorRect.top = 0;
		m_ScissorRect.right = m_dwWidth;
		m_ScissorRect.bottom = m_dwHeight;


		return TRUE;
	}

	void D3D12Renderer::BeginRender()
	{
		//
		// 화면 클리어 및 이번 프레임 렌더링을 위한 자료구조 초기화
		//
		if (FAILED(m_pCommandAllocator->Reset()))
			__debugbreak();

		if (FAILED(m_pCommandList->Reset(m_pCommandAllocator, nullptr)))
			__debugbreak();

		CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(m_pRTVHeap->GetCPUDescriptorHandleForHeapStart(), m_uiRenderTargetIndex, m_rtvDescriptorSize);

		// 리소스 동기화 우선 (Present -> RTV 트랜지션)
		m_pCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_pRenderTargets[m_uiRenderTargetIndex], D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));

		// Record commands.
		const float BackColor[] = { 0.0f, 0.0f, 1.0f, 1.0f };
		m_pCommandList->ClearRenderTargetView(rtvHandle, BackColor, 0, nullptr);

		m_pCommandList->RSSetViewports(1, &m_Viewport);
		m_pCommandList->RSSetScissorRects(1, &m_ScissorRect);
		m_pCommandList->OMSetRenderTargets(1, &rtvHandle, FALSE, nullptr);
	}

	void D3D12Renderer::RenderMeshObject(void* pMeshObjHandle)
	{
		BasicMeshObject* pMeshObj = (BasicMeshObject*)pMeshObjHandle;
		pMeshObj->Draw(m_pCommandList);
	}

	void D3D12Renderer::EndRender()
	{
		//
		// 지오메트리 렌더링
		//

		// 리소스 동기화 우선 (RTV -> Present 트랜지션)
		m_pCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_pRenderTargets[m_uiRenderTargetIndex], D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));
		m_pCommandList->Close();

		ID3D12CommandList* ppCommandLists[] = { m_pCommandList };
		m_pCommandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);
	}

	void D3D12Renderer::Present()
	{
		//
		// Back Buffer 화면을 Primary Buffer로 전송
		//
		UINT m_SyncInterval = 1;	// VSync On
		//UINT m_SyncInterval = 0;	// VSync Off

		UINT uiSyncInterval = m_SyncInterval;
		UINT uiPresentFlags = 0;

		if (!uiSyncInterval)
		{
			uiPresentFlags = DXGI_PRESENT_ALLOW_TEARING; // 티어링 무시 (VSync 완전 Off)
		}

		HRESULT hr = m_pSwapChain->Present(uiSyncInterval, uiPresentFlags);

		if (DXGI_ERROR_DEVICE_REMOVED == hr)
		{
			__debugbreak();
		}

		// for next frame
		m_uiRenderTargetIndex = m_pSwapChain->GetCurrentBackBufferIndex();


		// GPU에 시킨 일이 끝날때까지 기다림 (동기화)
		//TODO: 삭제 - 비동기적으로 처리하는게 좋음 (추후 업데이트)
		Fence();

		WaitForFenceValue();
	}

	void* D3D12Renderer::CreateBasicMeshObject()
	{
		BasicMeshObject* pMeshObj = new BasicMeshObject;
		pMeshObj->Initialize(this);
		pMeshObj->CreateMesh();
		return pMeshObj;
	}
	void D3D12Renderer::DeleteBasicMeshObject(void* pMeshObjHandle)
	{
		BasicMeshObject* pMeshObj = (BasicMeshObject*)pMeshObjHandle;
		delete pMeshObj;
	}

	UINT64 D3D12Renderer::Fence()
	{
		m_ui64FenceValue++; // CommandList 처리완료 마커
		m_pCommandQueue->Signal(m_pFence, m_ui64FenceValue);
		return m_ui64FenceValue;
	}

	void D3D12Renderer::WaitForFenceValue()
	{
		const UINT64 ExpectedFenceValue = m_ui64FenceValue;

		// Wait until the previous frame is finished.
		if (m_pFence->GetCompletedValue() < ExpectedFenceValue) // 마커 확인
		{
			m_pFence->SetEventOnCompletion(ExpectedFenceValue, m_hFenceEvent); // Event Set -> Event Signal(처리완료 시)
			WaitForSingleObject(m_hFenceEvent, INFINITE); // 메인스레드 blocking
		}
	}
	void D3D12Renderer::CreateCommandList()
	{
		// Command List와 항상 짝을 이루는 Command Allocator
		if (FAILED(m_pD3DDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&m_pCommandAllocator))))
		{
			__debugbreak();
		}

		// Create the command list.
		if (FAILED(m_pD3DDevice->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_pCommandAllocator, nullptr, IID_PPV_ARGS(&m_pCommandList))))
		{
			__debugbreak();
		}

		// Command lists are created in the recording state, but there is nothing
		// to record yet. The main loop expects it to be closed, so close it now. (초기화)
		m_pCommandList->Close();
	}
	void D3D12Renderer::CleanupCommandList()
	{
		if (m_pCommandList)
		{
			m_pCommandList->Release();
			m_pCommandList = nullptr;
		}
		if (m_pCommandAllocator)
		{
			m_pCommandAllocator->Release();
			m_pCommandAllocator = nullptr;
		}
	}
	void D3D12Renderer::CreateFence()
	{
		// Create synchronization objects and wait until assets have been uploaded to the GPU.
		if (FAILED(m_pD3DDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_pFence))))
		{
			__debugbreak();
		}

		m_ui64FenceValue = 0;

		// Create an event handle to use for frame synchronization.
		m_hFenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr); // 윈도우 이벤트 객체
	}
	void D3D12Renderer::CleanupFence()
	{
		if (m_hFenceEvent) // 끝났는지 확인
		{
			CloseHandle(m_hFenceEvent);
			m_hFenceEvent = nullptr;
		}
		if (m_pFence)
		{
			m_pFence->Release();
			m_pFence = nullptr;
		}
	}
	BOOL D3D12Renderer::CreateDescriptorHeap()
	{
		HRESULT hr = S_OK;

		// 렌더타겟용 디스크립터힙 (2칸 <- Double buffer)
		D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
		rtvHeapDesc.NumDescriptors = SWAP_CHAIN_FRAME_COUNT;	// SwapChain Buffer 0	| SwapChain Buffer 1
		rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
		rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
		if (FAILED(m_pD3DDevice->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&m_pRTVHeap))))
		{
			__debugbreak();
		}

		// GPU마다 사이즈 다름
		m_rtvDescriptorSize = m_pD3DDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

		return TRUE;
	}
	void D3D12Renderer::CleanupDescriptorHeap()
	{
		if (m_pRTVHeap)
		{
			m_pRTVHeap->Release();
			m_pRTVHeap = nullptr;
		}
	}
	void D3D12Renderer::Cleanup()
	{
		WaitForFenceValue(); // 종료 전 GPU 모든 작업이 끝날 때까지 기다림

		CleanupDescriptorHeap();

		for (DWORD i = 0; i < SWAP_CHAIN_FRAME_COUNT; i++)
		{
			if (m_pRenderTargets[i])
			{
				m_pRenderTargets[i]->Release(); // Release 호출 시 리턴 값 = 'rax 레지스터' (rax == 0, 객체 완전 파괴)
				m_pRenderTargets[i] = nullptr;
			}
		}
		if (m_pSwapChain)
		{
			m_pSwapChain->Release();
			m_pSwapChain = nullptr;
		}

		if (m_pCommandQueue)
		{
			m_pCommandQueue->Release();
			m_pCommandQueue = nullptr;
		}

		CleanupCommandList();

		CleanupFence();

		if (m_pD3DDevice)
		{
			ULONG ref_count = m_pD3DDevice->Release();
			if (ref_count)
			{
				//resource leak!!!
				IDXGIDebug1* pDebug = nullptr;
				if (SUCCEEDED(DXGIGetDebugInterface1(0, IID_PPV_ARGS(&pDebug))))
				{
					pDebug->ReportLiveObjects(DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_SUMMARY); // Report live objects (Output)
					pDebug->Release();
				}
				__debugbreak();
			}

			m_pD3DDevice = nullptr;

		}
	}
} // namespace Parang
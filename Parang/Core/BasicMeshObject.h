#pragma once

namespace Parang {

	class D3D12Renderer;
	class BasicMeshObject
	{
	public:
		BasicMeshObject();
		~BasicMeshObject();

		BOOL Initialize(D3D12Renderer* pRenderer);
		void Draw(ID3D12GraphicsCommandList* pCommandList);
		BOOL CreateMesh();

	private:
		BOOL InitCommonResources();
		void CleanupSharedResources();

		BOOL InitRootSinagture();
		BOOL InitPipelineState();

		void Cleanup();

	private:
		// shared by all BasicMeshObject instances.
		static ID3D12RootSignature* m_pRootSignature;
		static ID3D12PipelineState* m_pPipelineState;
		static DWORD	m_dwInitRefCount;

		D3D12Renderer* m_pRenderer = nullptr;

		// vertex data
		ID3D12Resource* m_pVertexBuffer = nullptr;
		D3D12_VERTEX_BUFFER_VIEW m_VertexBufferView = {};

	};
} // namespace Parang
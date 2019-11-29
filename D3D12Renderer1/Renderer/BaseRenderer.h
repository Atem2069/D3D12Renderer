#pragma once

#include<d3d12.h>
#include<dxgi1_6.h>
#include<d3dcompiler.h>
#include <directxmath.h>
#include"d3dx12.h"
#include<iostream>

using namespace DirectX;
struct Vertex
{
	XMFLOAT3 position;
	XMFLOAT3 normal;
	XMFLOAT2 texcoord;
};


struct CommandList
{
	ID3D12GraphicsCommandList* m_commandList;
	ID3D12CommandAllocator* m_commandAllocators[2];
};

class D3D
{
public:
	bool init(int width, int height, HWND hwnd);
	void destroy();

	bool submitCurrentlySetCommandList();
	//Execute a given command list and fully synchronize
	bool submitCommandList(CommandList& commandList);
	//Temporary cause this codebase literally sucks
	bool presentWithCommandLists(CommandList* commandLists, int numCommandLists);

	int getCurrentBuffer();

	//Begins renderpass with default RTV and DSV. Sets a resource barrier to set it to the 'render-target' state
	void beginRenderPass(float r, float g, float b, float a);

	//Ends renderpass with default RTV and DSV. Sets a resource barrier to set it to the 'present' state, suitable for DXGI to work with
	void endRenderPass();

	void bindAllResourceHeaps(ID3D12DescriptorHeap** descriptorHeaps, int numDescriptorHeaps);

	CommandList createCommandList();
	
	const CommandList & getCurrentCommandList();
	void setCurrentCommandList(CommandList& commandList);;

	ID3D12Device* getDevice();
	//ID3D12GraphicsCommandList* getCommandList();

	std::string graphicsAdapterName;
private:
	HANDLE fenceEvent;
	UINT64 m_fenceValues[2];
	ID3D12Fence* m_fences[2];
	IDXGISwapChain3* m_swapChain;
	ID3D12Device* m_device;
	//ID3D12CommandAllocator* m_commandAllocators[2];
	ID3D12CommandQueue* m_commandQueue;
	//ID3D12GraphicsCommandList* m_commandList;
	CommandList m_currentCommandList;
	ID3D12DescriptorHeap* m_renderTargetDescriptorHeap;
	ID3D12Resource* m_renderTargets[2];
	UINT m_renderTargetDescriptorSize;

	ID3D12DescriptorHeap* m_depthStencilDescriptorHeap;
	ID3D12Resource* m_depthStencilBuffer;

	D3D12_VIEWPORT m_viewport;
	D3D12_RECT m_scissorRect;

};

class D3DContext
{
public:
	static void Register(D3D& d3d);
	static D3D* getCurrent();

private:
	static D3D* m_current;
};
#pragma once

#include<d3d12.h>
#include<dxgi1_6.h>
#include"d3dx12.h"
#include<iostream>

class D3D
{
public:
	bool init(int width, int height, HWND hwnd);
	void destroy();

	//Closes command list and executes all changes, then synchronizes -- used for executing staging GPU changes from initializing
	bool executeAndSynchronize();
	//Force fence synchronization, and then reset the command list (only 1 as this is a singlethreaded renderer)
	bool synchronizeAndReset();
	//Close command list, execute it on command queue and present swapchain
	bool executeAndPresent();
	//Specifies the current buffer that is free to be worked on
	int getCurrentBuffer();

	ID3D12Device* getDevice();
	ID3D12GraphicsCommandList* getCommandList();
private:
	HANDLE fenceEvent;
	UINT64 m_fenceValues[2];
	ID3D12Fence* m_fences[2];
	IDXGISwapChain3* m_swapChain;
	ID3D12Device* m_device;
	ID3D12CommandAllocator* m_commandAllocators[2];
	ID3D12CommandQueue* m_commandQueue;
	ID3D12GraphicsCommandList* m_commandList;
};

class D3DContext
{
public:
	static void Register(D3D& d3d);
	static D3D* getCurrent();

private:
	static D3D* m_current;
};
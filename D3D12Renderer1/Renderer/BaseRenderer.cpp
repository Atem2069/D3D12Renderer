#include "BaseRenderer.h"

bool D3D::init(int width, int height, HWND hwnd)
{
	HRESULT result;
	result = D3D12CreateDevice(nullptr, D3D_FEATURE_LEVEL_12_0, __uuidof(ID3D12Device), (void**)&m_device);
	if (FAILED(result))
	{
		std::cout << "Creating D3D12 device failed.. GPU must support feature level 12.0!" << std::endl;
		return false;
	}

	//Create command queue
	D3D12_COMMAND_QUEUE_DESC cmdQueueDesc = {};
	cmdQueueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	cmdQueueDesc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
	cmdQueueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
	result = m_device->CreateCommandQueue(&cmdQueueDesc, __uuidof(ID3D12CommandQueue), (void**)&m_commandQueue);
	if (FAILED(result))
	{
		std::cout << "Failed to create command queue.. " << std::endl;
		return false;
	}

	//Create swapchain
	IDXGIFactory4* m_factory4;
	result = CreateDXGIFactory2(0, __uuidof(IDXGIFactory4), (void**)&m_factory4);
	if (FAILED(result))
	{
		std::cout << "Failed to create DXGI factory.. " << std::endl;
		return false;
	}

	IDXGISwapChain1* swapChain1;	//temporary swapchain which is recasted
	DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
	swapChainDesc.Width = width;
	swapChainDesc.Height = height;
	swapChainDesc.BufferCount = 2;
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapChainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING;
	swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	swapChainDesc.SampleDesc.Count = 1;
	swapChainDesc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;
	swapChainDesc.Scaling = DXGI_SCALING_NONE;

	result = m_factory4->CreateSwapChainForHwnd(m_commandQueue, hwnd, &swapChainDesc, nullptr, nullptr, &swapChain1);
	if (FAILED(result))
	{
		std::cout << "Failed to create DXGI swapchain.. " << std::endl;
		return false;
	}
	
	swapChain1->QueryInterface<IDXGISwapChain3>(&m_swapChain);
	//Create command allocators and command list
	for (int i = 0; i < 2; i++)
	{
		result = m_device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, __uuidof(ID3D12CommandAllocator), (void**)&m_commandAllocators[i]);
		if (FAILED(result))
		{
			std::cout << "Failed on creating command allocator " << i << std::endl;
			return false;
		}
	}
	
	result = m_device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_commandAllocators[0], nullptr, __uuidof(ID3D12GraphicsCommandList), (void**)&m_commandList);
	if (FAILED(result))
		return false;

	//Creating rendertargets

	//First - descriptor heap for the rendertargets
	D3D12_DESCRIPTOR_HEAP_DESC rtvDescriptorHeapDesc = {};
	rtvDescriptorHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	rtvDescriptorHeapDesc.NumDescriptors = 2;
	rtvDescriptorHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;

	result = m_device->CreateDescriptorHeap(&rtvDescriptorHeapDesc, __uuidof(ID3D12DescriptorHeap), (void**)&m_renderTargetDescriptorHeap);
	if (FAILED(result))
	{
		std::cout << "Failed to create descriptor heap to hold rendertargets.. " << std::endl;
		return false;
	}

	m_renderTargetDescriptorSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);	//Have to get the size of a descriptor from the device to not screw everything
	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvDescriptorHandle(m_renderTargetDescriptorHeap->GetCPUDescriptorHandleForHeapStart());

	//Now time to fill out descriptor heap with rendertargets
	for (int i = 0; i < 2; i++)
	{
		result = m_swapChain->GetBuffer(i, __uuidof(ID3D12Resource), (void**)&m_renderTargets[i]);
		if (FAILED(result))
		{
			std::cout << "Failed to retrieve one of the swapchain buffers.. " << std::endl;
			return false;
		}

		m_device->CreateRenderTargetView(m_renderTargets[i], nullptr, rtvDescriptorHandle);
		rtvDescriptorHandle.Offset(1, m_renderTargetDescriptorSize);	//Offset so next RTV can be put in
	}

	//Now making a depth-stencil view
	D3D12_DESCRIPTOR_HEAP_DESC dsvDescriptorHeapDesc = {};
	dsvDescriptorHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	dsvDescriptorHeapDesc.NumDescriptors = 1;
	dsvDescriptorHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;

	result = m_device->CreateDescriptorHeap(&dsvDescriptorHeapDesc, __uuidof(ID3D12DescriptorHeap), (void**)&m_depthStencilDescriptorHeap);
	if (FAILED(result))
	{
		std::cout << "Failed to create descriptor heap to hold depth-stencil views.. " << std::endl;
		return false;
	}

	D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
	dsvDesc.Format = DXGI_FORMAT_D32_FLOAT;
	dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;

	D3D12_CLEAR_VALUE depthOptimizedClearValue = {};
	depthOptimizedClearValue.Format = dsvDesc.Format;
	depthOptimizedClearValue.DepthStencil.Depth = 1.0f;

	//Creating a depth stencil texture using some d3dx defaults
	result = m_device->CreateCommittedResource(&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT), D3D12_HEAP_FLAG_NONE, &CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_D32_FLOAT, width, height, 1, 0, 1, 0, D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL), D3D12_RESOURCE_STATE_DEPTH_WRITE, &depthOptimizedClearValue, __uuidof(ID3D12Resource), (void**)&m_depthStencilBuffer);
	if (FAILED(result))
	{
		std::cout << "Failed to create depth buffer.. " << std::endl;
		return false;
	}

	m_device->CreateDepthStencilView(m_depthStencilBuffer, &dsvDesc, m_depthStencilDescriptorHeap->GetCPUDescriptorHandleForHeapStart());

	//Finally, create fences for synchronization
	for (int i = 0; i < 2; i++)
	{
		result = m_device->CreateFence(0, D3D12_FENCE_FLAG_NONE, __uuidof(ID3D12Fence), (void**)&m_fences[i]);
		if (FAILED(result))
		{
			std::cout << "Failed to create D3D Fence " << i << std::endl;
			return false;
		}

		m_fenceValues[i] = 0;
	}

	fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
	if (!fenceEvent)
		return false;

	m_viewport.Width = width;
	m_viewport.Height = height;
	m_viewport.MinDepth = 0;
	m_viewport.MaxDepth = 1;
	m_viewport.TopLeftX = 0;
	m_viewport.TopLeftY = 0;
	
	m_scissorRect.right = width;
	m_scissorRect.bottom = height;
	m_scissorRect.left = 0;
	m_scissorRect.top = 0;
	return true;
}

void D3D::destroy()
{
	//todo
}

bool D3D::submitCommandList(ID3D12GraphicsCommandList* commandList)
{
	m_commandQueue->ExecuteCommandLists(1, (ID3D12CommandList**)&commandList);
	m_commandQueue->Signal(m_fences[this->getCurrentBuffer()], m_fenceValues[this->getCurrentBuffer()]);
	int currentBuffer = this->getCurrentBuffer();
	if (m_fences[currentBuffer]->GetCompletedValue() < m_fenceValues[currentBuffer])
	{
		HRESULT result = m_fences[currentBuffer]->SetEventOnCompletion(m_fenceValues[currentBuffer], fenceEvent);
		if (FAILED(result))
			return false;
		WaitForSingleObject(fenceEvent, INFINITE);
	}
	m_fenceValues[currentBuffer]++;

	return true;
}

bool D3D::executeAndSynchronize()
{
	m_commandList->Close();
	m_commandQueue->ExecuteCommandLists(1, (ID3D12CommandList**)&m_commandList);
	int currentBuffer = this->getCurrentBuffer();
	m_fenceValues[currentBuffer]++;
	m_commandQueue->Signal(m_fences[currentBuffer], m_fenceValues[currentBuffer]);
	return true;
}

bool D3D::synchronizeAndReset()
{
	int currentBuffer = this->getCurrentBuffer();
	if (m_fences[currentBuffer]->GetCompletedValue() < m_fenceValues[currentBuffer])
	{
		HRESULT result = m_fences[currentBuffer]->SetEventOnCompletion(m_fenceValues[currentBuffer], fenceEvent);
		if (FAILED(result))
			return false;
		WaitForSingleObject(fenceEvent, INFINITE);
	}
	m_fenceValues[currentBuffer]++;

	m_commandAllocators[currentBuffer]->Reset();
	m_commandList->Reset(m_commandAllocators[currentBuffer], nullptr);
	return true;
}

bool D3D::executeAndPresent(bool vsync)
{
	m_commandList->Close();
	m_commandQueue->ExecuteCommandLists(1, (ID3D12CommandList**)&m_commandList);
	m_commandQueue->Signal(m_fences[this->getCurrentBuffer()], m_fenceValues[this->getCurrentBuffer()]);
	HRESULT result = m_swapChain->Present((int)vsync, 0);
	if (FAILED(result))
	{
		std::cout << "DXGI Present Error : " << result << std::endl;
		std::cout << m_device->GetDeviceRemovedReason() << std::endl;
		return false;
	}
	return true;
}

void D3D::beginRenderPass(float r, float g, float b, float a)
{
	int currentBuffer = this->getCurrentBuffer();
	float clearColor[4] = { r,g,b,a };

	m_commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_renderTargets[this->getCurrentBuffer()], D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));
	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvDescriptorHandle(m_renderTargetDescriptorHeap->GetCPUDescriptorHandleForHeapStart(), currentBuffer, m_renderTargetDescriptorSize);
	CD3DX12_CPU_DESCRIPTOR_HANDLE dsvDescriptorHandle(m_depthStencilDescriptorHeap->GetCPUDescriptorHandleForHeapStart());

	m_commandList->OMSetRenderTargets(1, &rtvDescriptorHandle, FALSE, &dsvDescriptorHandle);
	m_commandList->ClearRenderTargetView(rtvDescriptorHandle, clearColor, 0, nullptr);
	m_commandList->ClearDepthStencilView(dsvDescriptorHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

	m_commandList->RSSetViewports(1, &m_viewport);
	m_commandList->RSSetScissorRects(1, &m_scissorRect);
	
}

void D3D::endRenderPass()
{
	m_commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_renderTargets[this->getCurrentBuffer()], D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));
}

void D3D::bindAllResourceHeaps(ID3D12DescriptorHeap** descriptorHeaps, int numDescriptorHeaps)
{
	m_commandList->SetDescriptorHeaps(numDescriptorHeaps, descriptorHeaps);
}

int D3D::getCurrentBuffer()
{
	return m_swapChain->GetCurrentBackBufferIndex();
}

ID3D12Device* D3D::getDevice()
{
	return m_device;
}

ID3D12GraphicsCommandList* D3D::getCommandList()
{
	return m_commandList;
}

void D3DContext::Register(D3D& d3d)
{
	m_current = &d3d;

}

D3D* D3DContext::getCurrent()
{
	return m_current;
}

D3D* D3DContext::m_current = nullptr;
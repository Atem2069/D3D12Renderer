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
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	swapChainDesc.Scaling = DXGI_SCALING_NONE;

	result = m_factory4->CreateSwapChainForHwnd(m_commandQueue, hwnd, &swapChainDesc, nullptr, nullptr, &swapChain1);
	if (FAILED(result))
	{
		std::cout << "Failed to create DXGI swapchain.. " << std::endl;
		return false;
	}
	
	m_swapChain = static_cast<IDXGISwapChain3*>(swapChain1);

	return true;
}

void D3D::destroy()
{
	//todo
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
	return true;
}

bool D3D::executeAndPresent()
{
	m_commandList->Close();
	m_commandQueue->ExecuteCommandLists(1, (ID3D12CommandList**)&m_commandList);
	m_commandQueue->Signal(m_fences[this->getCurrentBuffer()], m_fenceValues[this->getCurrentBuffer()]);
	m_swapChain->Present(0, 0);
	return true;
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
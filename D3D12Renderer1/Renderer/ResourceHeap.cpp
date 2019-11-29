#include "ResourceHeap.h"

bool ResourceHeap::init(int numDescriptors)
{
	HRESULT result;

	for (int i = 0; i < 2; i++)
	{
		D3D12_DESCRIPTOR_HEAP_DESC dhDesc = {};
		dhDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
		dhDesc.NodeMask = 0;
		dhDesc.NumDescriptors = numDescriptors;
		dhDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
		result = D3DContext::getCurrent()->getDevice()->CreateDescriptorHeap(&dhDesc, __uuidof(ID3D12DescriptorHeap), (void**)&m_descriptorHeaps[i]);
		if (FAILED(result))
		{
			std::cout << "Failed to create descriptor heap " << i << std::endl;
			return false;
		}
	}

	return true;
}


void ResourceHeap::destroy()
{
	//todo
}

void ResourceHeap::bindDescriptorTable(int rootParameterIndex, int baseDescriptorIndex)
{
	CD3DX12_GPU_DESCRIPTOR_HANDLE gpuDescriptorHandle(m_descriptorHeaps[D3DContext::getCurrent()->getCurrentBuffer()]->GetGPUDescriptorHandleForHeapStart());
	if (baseDescriptorIndex)
	{
		UINT increment = D3DContext::getCurrent()->getDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		gpuDescriptorHandle.Offset(baseDescriptorIndex, increment);
	}
	D3DContext::getCurrent()->getCurrentCommandList().m_commandList->SetGraphicsRootDescriptorTable(rootParameterIndex, gpuDescriptorHandle);
}

void ResourceHeap::bindDescriptorTable(int rootParameterIndex, int baseDescriptorIndex, int descriptorHeapIndex)
{
	CD3DX12_GPU_DESCRIPTOR_HANDLE gpuDescriptorHandle(m_descriptorHeaps[descriptorHeapIndex]->GetGPUDescriptorHandleForHeapStart());
	if (baseDescriptorIndex)
	{
		UINT increment = D3DContext::getCurrent()->getDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		gpuDescriptorHandle.Offset(baseDescriptorIndex, increment);
	}

	D3DContext::getCurrent()->getCurrentCommandList().m_commandList->SetGraphicsRootDescriptorTable(rootParameterIndex, gpuDescriptorHandle);
}

ID3D12DescriptorHeap* ResourceHeap::getCurrent()
{
	return m_descriptorHeaps[D3DContext::getCurrent()->getCurrentBuffer()];
}

ID3D12DescriptorHeap* ResourceHeap::getCurrent(int index)
{
	return m_descriptorHeaps[index];
}
#include "ResourceHeap.h"

bool ResourceHeap::init(int numDescriptors, int numHeapsToCreate, D3D12_DESCRIPTOR_HEAP_TYPE descriptorHeapType)
{
	HRESULT result;
	assert(numHeapsToCreate <= 2);
	m_numHeaps = numHeapsToCreate;
	m_descriptorHeapType = descriptorHeapType;
	for (int i = 0; i < numHeapsToCreate; i++)
	{
		D3D12_DESCRIPTOR_HEAP_DESC dhDesc = {};
		dhDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
		dhDesc.NodeMask = 0;
		dhDesc.NumDescriptors = numDescriptors;
		dhDesc.Type = m_descriptorHeapType;
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

void ResourceHeap::bindDescriptorTable(int rootParameterIndex, int baseDescriptorIndex, int descriptorHeapIndex)
{
	assert((descriptorHeapIndex + 1) <= 2);
	CD3DX12_GPU_DESCRIPTOR_HANDLE gpuDescriptorHandle(m_descriptorHeaps[descriptorHeapIndex]->GetGPUDescriptorHandleForHeapStart());
	if (baseDescriptorIndex)
	{
		UINT increment = D3DContext::getCurrent()->getDevice()->GetDescriptorHandleIncrementSize(m_descriptorHeapType);
		gpuDescriptorHandle.Offset(baseDescriptorIndex, increment);
	}
	D3DContext::getCurrent()->getCurrentCommandList().m_commandList->SetGraphicsRootDescriptorTable(rootParameterIndex, gpuDescriptorHandle);
}

ID3D12DescriptorHeap* ResourceHeap::getHeap(int index)
{
	return m_descriptorHeaps[index];
}

D3D12_GPU_DESCRIPTOR_HANDLE ResourceHeap::getResourceView(int descriptorHandle, int descriptorHeapHandle)
{
	CD3DX12_GPU_DESCRIPTOR_HANDLE gpuDescriptorHandle(m_descriptorHeaps[descriptorHeapHandle]->GetGPUDescriptorHandleForHeapStart());
	if (descriptorHandle)
	{
		UINT increment = D3DContext::getCurrent()->getDevice()->GetDescriptorHandleIncrementSize(m_descriptorHeapType);
		gpuDescriptorHandle.Offset(descriptorHandle, increment);
	}
	return gpuDescriptorHandle;
}
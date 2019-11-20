#include "ConstantBuffer.h"

bool ConstantBuffer::init(void* data, size_t dataSize, ResourceHeap& resourceHeap)
{
	HRESULT result;
	m_descriptorLocation = resourceHeap.numBoundDescriptors;
	for (int i = 0; i < 2; i++)
	{
		UINT uploadHeapSize = ((dataSize + (1024 * 64) - 1) / (1024 * 64)) * (1024 * 64);	//Round up to a multiple of 65536, as d3d12 likes 64k alignment for uploads
		//Create upload heaps for const buffer
		result = D3DContext::getCurrent()->getDevice()->CreateCommittedResource(&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD), D3D12_HEAP_FLAG_NONE, &CD3DX12_RESOURCE_DESC::Buffer(uploadHeapSize), D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, __uuidof(ID3D12Resource), (void**)&m_constantBufferUploadHeaps[i]);
		if (FAILED(result))
		{
			std::cout << "Failed to create constant buffer upload heap " << i << std::endl;
			return false;
		}

		CD3DX12_CPU_DESCRIPTOR_HANDLE resourceHeapHandle(resourceHeap.getCurrent(i)->GetCPUDescriptorHandleForHeapStart());
		if (resourceHeap.numBoundDescriptors > 0)
		{
			UINT increment = D3DContext::getCurrent()->getDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
			resourceHeapHandle.Offset(resourceHeap.numBoundDescriptors, increment);	//Maybe this is right?
		}

		D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
		cbvDesc.SizeInBytes = (dataSize + 255) & ~255;
		cbvDesc.BufferLocation = m_constantBufferUploadHeaps[i]->GetGPUVirtualAddress();
		D3DContext::getCurrent()->getDevice()->CreateConstantBufferView(&cbvDesc, resourceHeapHandle);
		CD3DX12_RANGE mapRange(0, 0);
		result = m_constantBufferUploadHeaps[i]->Map(0, &mapRange, (void**)&m_constantBufferGPUAddresses[i]);
		if (FAILED(result))
		{
			std::cout << "Failed to map constant buffer " << i << std::endl;
			return false;
		}
		memcpy(m_constantBufferGPUAddresses[i], data, dataSize);
	}

	resourceHeap.numBoundDescriptors += 1;	//We added a CBV to each resource heap frame, so we increment the counter.

	return true;
}

void ConstantBuffer::destroy()
{
	//todo
}

void ConstantBuffer::update(void* data, size_t dataSize)
{
	memcpy(m_constantBufferGPUAddresses[D3DContext::getCurrent()->getCurrentBuffer()], data, dataSize);
}

void ConstantBuffer::bind(int rootParameterIndex)
{
	D3DContext::getCurrent()->getCommandList()->SetGraphicsRootConstantBufferView(rootParameterIndex, m_constantBufferUploadHeaps[D3DContext::getCurrent()->getCurrentBuffer()]->GetGPUVirtualAddress());
}

int ConstantBuffer::getDescriptorLocation()
{
	return m_descriptorLocation;
}
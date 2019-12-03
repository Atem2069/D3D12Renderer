#include "ConstantBuffer.h"

bool ConstantBuffer::init(void* data, size_t dataSize)
{
	HRESULT result;
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

		CD3DX12_RANGE mapRange(0, 0);
		result = m_constantBufferUploadHeaps[i]->Map(0, &mapRange, (void**)&m_constantBufferGPUAddresses[i]);
		if (FAILED(result))
		{
			std::cout << "Failed to map constant buffer " << i << std::endl;
			return false;
		}
		memcpy(m_constantBufferGPUAddresses[i], data, dataSize);
	}


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
	D3DContext::getCurrent()->getCurrentCommandList().m_commandList->SetGraphicsRootConstantBufferView(rootParameterIndex, m_constantBufferUploadHeaps[D3DContext::getCurrent()->getCurrentBuffer()]->GetGPUVirtualAddress());
}

void ConstantBuffer::bind(int rootParameterIndex, CommandList commandList)
{
	commandList.m_commandList->SetGraphicsRootConstantBufferView(rootParameterIndex, m_constantBufferUploadHeaps[D3DContext::getCurrent()->getCurrentBuffer()]->GetGPUVirtualAddress());
}

int ConstantBuffer::getDescriptorLocation()
{
	return m_descriptorLocation;
}
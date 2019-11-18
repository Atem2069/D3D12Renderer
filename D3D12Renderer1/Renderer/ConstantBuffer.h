#pragma once

#include "BaseRenderer.h"
#include "ResourceHeap.h"

class ConstantBuffer
{
public:
	bool init(void* data, size_t dataSize, ResourceHeap& resourceHeap);
	void destroy();


	void update(void* data, size_t dataSize);

	int getDescriptorLocation();
private:
	int m_descriptorLocation;
	ID3D12Resource* m_constantBufferUploadHeaps[2];
	UINT8* m_constantBufferGPUAddresses[2];
};
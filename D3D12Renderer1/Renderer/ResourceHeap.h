#pragma once

#include "BaseRenderer.h"

class ResourceHeap
{
public:
	bool init(int numDescriptors, int numHeapsToCreate);
	void destroy();

	void bindDescriptorTable(int rootParameterIndex, int baseDescriptorIndex, int descriptorHeapIndex);
	int numBoundDescriptors = 0;	//Whichever method uses the resource heap should probably increment this handle to not screw everything up

	D3D12_GPU_DESCRIPTOR_HANDLE getResourceView(int descriptorIndex, int descriptorHeapIndex);

	ID3D12DescriptorHeap* getHeap(int index);
private:
	int m_numHeaps = 0;
	ID3D12DescriptorHeap* m_descriptorHeaps[2];
};
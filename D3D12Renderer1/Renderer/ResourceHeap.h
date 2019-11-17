#pragma once

#include "BaseRenderer.h"

class ResourceHeap
{
public:
	bool init(int numDescriptors);
	void destroy();

	void bindDescriptorTable(int rootParameterIndex, int baseDescriptorIndex);

	int numBoundDescriptors = 0;	//Whichever method uses the resource heap should probably increment this handle to not screw everything up

	ID3D12DescriptorHeap* getCurrent();
	ID3D12DescriptorHeap* getCurrent(int index);
private:
	ID3D12DescriptorHeap* m_descriptorHeaps[2];
};
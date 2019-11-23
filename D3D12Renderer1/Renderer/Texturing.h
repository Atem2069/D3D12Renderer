#pragma once

#include "BaseRenderer.h"
#include "ResourceHeap.h"
#include <stb_image.h>
class Texture2D
{
public:
	bool init(std::string texturePath, ResourceHeap& resourceHeap);
	void destroy();

	void bind(int rootParameterIndex);
	int m_descriptorOffset;
private:
	D3D12_GPU_VIRTUAL_ADDRESS m_textureAddress;
	ID3D12Resource* m_textureStorageHeap;
	ID3D12Resource* m_textureUploadHeap;

	void unpackRGBToRGBA(int width, int height, unsigned char * input, unsigned char * output);
};
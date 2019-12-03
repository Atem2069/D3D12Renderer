#pragma once
#define NOMINMAX
#include "BaseRenderer.h"
#include "ResourceHeap.h"
#include <stb_image.h>
class Texture2D
{
public:
	bool init(std::string texturePath, ResourceHeap& resourceHeap);
	void destroy();

	int m_descriptorOffset;

	static bool generateMipmaps(ID3D12Resource* resource, int numMipMaps, int width, int height);
private:
	ID3D12Resource* m_textureStorageHeap;
	ID3D12Resource* m_textureUploadHeap;

	void unpackRGBToRGBA(int width, int height, unsigned char * input, unsigned char * output);
};
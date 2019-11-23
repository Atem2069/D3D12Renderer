#include "Texturing.h"

bool Texture2D::init(std::string texturePath, ResourceHeap& resourceHeap)
{
	HRESULT result;

	int width, height, channels;
	unsigned char * imgData = stbi_load(texturePath.c_str(), &width, &height, &channels, 0);
	if (!imgData)
	{
		std::cout << "Failed to load image.. " << std::endl;
		return false;
	}

	DXGI_FORMAT format = DXGI_FORMAT_R8G8B8A8_UNORM;
	unsigned char * rgbaImageData = (unsigned char*)malloc(width*height * 4);
	if (channels == 3)
		this->unpackRGBToRGBA(width, height, imgData, rgbaImageData);

	
	D3D12_RESOURCE_DESC tex2DDesc = {};
	tex2DDesc.Alignment = 0;
	tex2DDesc.DepthOrArraySize = 1;
	tex2DDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	tex2DDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
	tex2DDesc.Format = format;
	tex2DDesc.Width = width;
	tex2DDesc.Height = height;
	tex2DDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	tex2DDesc.MipLevels = 1;
	tex2DDesc.SampleDesc.Count = 1;;
	
	result = D3DContext::getCurrent()->getDevice()->CreateCommittedResource(&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT), D3D12_HEAP_FLAG_NONE, &tex2DDesc, D3D12_RESOURCE_STATE_COPY_DEST, nullptr, __uuidof(ID3D12Resource), (void**)&m_textureStorageHeap);
	if (FAILED(result))
	{
		std::cout << "Failed to create Texture storage heap.. " << std::endl;
		return false;
	}

	UINT64 textureUploadBufferSize;
	D3DContext::getCurrent()->getDevice()->GetCopyableFootprints(&tex2DDesc, 0, 1, 0, nullptr, nullptr, nullptr, &textureUploadBufferSize);

	result = D3DContext::getCurrent()->getDevice()->CreateCommittedResource(&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD), D3D12_HEAP_FLAG_NONE, &CD3DX12_RESOURCE_DESC::Buffer(textureUploadBufferSize), D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, __uuidof(ID3D12Resource), (void**)&m_textureUploadHeap);
	if (FAILED(result))
	{
		std::cout << "Failed to create Texture upload heap.. " << std::endl;
		return false;
	}

	D3D12_SUBRESOURCE_DATA textureData = {};
	if (channels == 3)
		textureData.pData = rgbaImageData;
	else
		textureData.pData = imgData;
	textureData.RowPitch = width * 4;
	textureData.SlicePitch = 0;

	UpdateSubresources(D3DContext::getCurrent()->getCommandList(), m_textureStorageHeap, m_textureUploadHeap, 0, 0, 1, &textureData);
	D3DContext::getCurrent()->getCommandList()->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_textureStorageHeap, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE));
	m_textureAddress = m_textureStorageHeap->GetGPUVirtualAddress();

	stbi_image_free(imgData);
	delete[] rgbaImageData;

	CD3DX12_CPU_DESCRIPTOR_HANDLE resourceHeapHandle(resourceHeap.getCurrent(0)->GetCPUDescriptorHandleForHeapStart());
	if (resourceHeap.numBoundDescriptors > 0)
	{
		UINT increment = D3DContext::getCurrent()->getDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		resourceHeapHandle.Offset(resourceHeap.numBoundDescriptors, increment);	//Maybe this is right?
	}

	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Format = format;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MipLevels = 1;
	D3DContext::getCurrent()->getDevice()->CreateShaderResourceView(m_textureStorageHeap, &srvDesc, resourceHeapHandle);
	m_descriptorOffset = resourceHeap.numBoundDescriptors;
	resourceHeap.numBoundDescriptors++;
	return true;
}

void Texture2D::destroy()
{
	if (m_textureUploadHeap)
		m_textureUploadHeap->Release();
	if (m_textureStorageHeap)
		m_textureStorageHeap->Release();
}

void Texture2D::bind(int rootParameterIndex)
{
	D3DContext::getCurrent()->getCommandList()->SetGraphicsRootShaderResourceView(rootParameterIndex, m_textureAddress);
}

void Texture2D::unpackRGBToRGBA(int width, int height, unsigned char * input, unsigned char * output)	//(un)safe method of unpacking RGB to RGBA, feat char pointers
{

	for (int i = 0, j = 0; i < width*height * 4; i += 4, j += 3)
	{
		//the most basic copy in the entire world
		output[i] = input[j];
		output[i + 1] = input[j + 1];
		output[i + 2] = input[j + 2];
		output[i + 3] = 0xFFFFFFFF;	//255?
	}

}
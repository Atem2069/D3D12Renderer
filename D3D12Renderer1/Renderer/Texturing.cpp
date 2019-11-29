#include "Texturing.h"
#include <algorithm>
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
	tex2DDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
	tex2DDesc.Format = format;
	tex2DDesc.Width = width;
	tex2DDesc.Height = height;
	tex2DDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	tex2DDesc.MipLevels = (floor(log2(std::max(width, height)))) + 1;
	tex2DDesc.SampleDesc.Count = 1;;

	//std::cout << tex2DDesc.MipLevels << std::endl;
	
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

	UpdateSubresources(D3DContext::getCurrent()->getCurrentCommandList().m_commandList, m_textureStorageHeap, m_textureUploadHeap, 0, 0, 1, &textureData);
	D3DContext::getCurrent()->getCurrentCommandList().m_commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_textureStorageHeap, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE));

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
	srvDesc.Texture2D.MipLevels = tex2DDesc.MipLevels;
	srvDesc.Texture2D.MostDetailedMip = 0;
	D3DContext::getCurrent()->getDevice()->CreateShaderResourceView(m_textureStorageHeap, &srvDesc, resourceHeapHandle);
	m_descriptorOffset = resourceHeap.numBoundDescriptors;
	resourceHeap.numBoundDescriptors++;

	D3DContext::getCurrent()->submitCurrentlySetCommandList();

	if (!Texture2D::generateMipmaps(m_textureStorageHeap, tex2DDesc.MipLevels, width, height))
	{
		std::cout << "Mipmap generation failed on " << texturePath << std::endl;
		return false;
	}

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
	D3DContext::getCurrent()->getCurrentCommandList().m_commandList->SetGraphicsRootShaderResourceView(rootParameterIndex, m_textureAddress);
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

bool Texture2D::generateMipmaps(ID3D12Resource* resource, int numMipMaps, int width, int height)
{
	HRESULT result;
	//Union used for shader constants
	struct DWParam
	{
		DWParam(FLOAT f) : Float(f) {}
		DWParam(UINT u) : Uint(u) {}

		void operator= (FLOAT f) { Float = f; }
		void operator= (UINT u) { Uint = u; }

		union
		{
			FLOAT Float;
			UINT Uint;
		};
	};

	UINT32 requiredHeapSize = numMipMaps - 1;

	CD3DX12_DESCRIPTOR_RANGE srvCbvRanges[2];
	CD3DX12_ROOT_PARAMETER rootParameters[3];

	srvCbvRanges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 0);
	srvCbvRanges[1].Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 0, 0);
	rootParameters[0].InitAsConstants(2, 0);
	rootParameters[1].InitAsDescriptorTable(1, &srvCbvRanges[0]);
	rootParameters[2].InitAsDescriptorTable(1, &srvCbvRanges[1]);

	D3D12_STATIC_SAMPLER_DESC samplerDesc = {};
	samplerDesc.Filter = D3D12_FILTER_MIN_MAG_LINEAR_MIP_POINT;
	samplerDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	samplerDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	samplerDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	samplerDesc.MipLODBias = 0.0f;
	samplerDesc.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
	samplerDesc.MinLOD = 0.0f;
	samplerDesc.MaxLOD = D3D12_FLOAT32_MAX;
	samplerDesc.MaxAnisotropy = 0;
	samplerDesc.BorderColor = D3D12_STATIC_BORDER_COLOR_OPAQUE_BLACK;
	samplerDesc.ShaderRegister = 0;
	samplerDesc.RegisterSpace = 0;
	samplerDesc.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

	ID3DBlob* signature;;
	ID3DBlob* error;

	CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc;
	rootSignatureDesc.Init(_countof(rootParameters), rootParameters, 1, &samplerDesc, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);
	result = D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error);
	if (FAILED(result))
	{
		std::cout << "Failed to serialize root signature.. " << std::endl;
		std::cout << (char*)error->GetBufferPointer();
		return false;
	}
	ID3D12RootSignature* mipmapRootSignature;
	result = D3DContext::getCurrent()->getDevice()->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), __uuidof(ID3D12RootSignature), (void**)&mipmapRootSignature);
	if (FAILED(result))
	{
		std::cout << "Failed to create root signature.. " << std::endl;
		return false;
	}

	D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
	heapDesc.NumDescriptors = 2 * requiredHeapSize;
	heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	ID3D12DescriptorHeap* descriptorHeap;
	D3DContext::getCurrent()->getDevice()->CreateDescriptorHeap(&heapDesc, __uuidof(ID3D12DescriptorHeap), (void**)&descriptorHeap);

	UINT descriptorSize = D3DContext::getCurrent()->getDevice()->GetDescriptorHandleIncrementSize(heapDesc.Type);

	ID3DBlob* shaderbytecode;
	result = D3DCompileFromFile(L"Shaders\\generateMipsCompute.hlsl", nullptr, nullptr, "GenerateMipMaps", "cs_5_0", 0, 0, &shaderbytecode, &error);
	if (FAILED(result))
	{
		std::cout << "Failed to compile compute shader.. " << std::endl;
		std::cout << (char*)error->GetBufferPointer();
	}
	D3D12_SHADER_BYTECODE computeBytecode = {};
	computeBytecode.BytecodeLength = shaderbytecode->GetBufferSize();
	computeBytecode.pShaderBytecode = shaderbytecode->GetBufferPointer();

	D3D12_COMPUTE_PIPELINE_STATE_DESC psoDesc = {};
	psoDesc.pRootSignature = mipmapRootSignature;
	psoDesc.CS = computeBytecode;
	ID3D12PipelineState* psoMipMaps;
	result = D3DContext::getCurrent()->getDevice()->CreateComputePipelineState(&psoDesc, __uuidof(ID3D12PipelineState), (void**)&psoMipMaps);
	if (FAILED(result))
	{
		std::cout << "Failed to create compute PSO.. " << std::endl;
		return false;
	}

	D3D12_SHADER_RESOURCE_VIEW_DESC srcTextureSRVDesc = {};
	srcTextureSRVDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srcTextureSRVDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;

	D3D12_UNORDERED_ACCESS_VIEW_DESC destTextureUAVDesc = {};
	destTextureUAVDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;


	//CPU handle for the first descriptor on the descriptor heap, used to fill the heap
	CD3DX12_CPU_DESCRIPTOR_HANDLE currentCPUHandle(descriptorHeap->GetCPUDescriptorHandleForHeapStart(), 0, descriptorSize);

	//GPU handle for the first descriptor on the descriptor heap, used to initialize the descriptor tables
	CD3DX12_GPU_DESCRIPTOR_HANDLE currentGPUHandle(descriptorHeap->GetGPUDescriptorHandleForHeapStart(), 0, descriptorSize);

	CommandList cmdList = D3DContext::getCurrent()->createCommandList();

	cmdList.m_commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(resource, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS));
	for (uint32_t TopMip = 0; TopMip < numMipMaps - 1; TopMip++)
	{
		//Command list SHOULD have all staging changes submitted from synchronizing and resetting just prior, so this should be fine
		cmdList.m_commandList->SetComputeRootSignature(mipmapRootSignature);
		cmdList.m_commandList->SetPipelineState(psoMipMaps);
		cmdList.m_commandList->SetDescriptorHeaps(1, &descriptorHeap);

		uint32_t dstWidth = std::max(width >> (TopMip + 1), 1);
		uint32_t dstHeight = std::max(height >> (TopMip + 1), 1);

		srcTextureSRVDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		srcTextureSRVDesc.Texture2D.MipLevels = 1;
		srcTextureSRVDesc.Texture2D.MostDetailedMip = TopMip;

		D3DContext::getCurrent()->getDevice()->CreateShaderResourceView(resource, &srcTextureSRVDesc, currentCPUHandle);
		currentCPUHandle.Offset(1, descriptorSize);

		destTextureUAVDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		destTextureUAVDesc.Texture2D.MipSlice = TopMip + 1;

		D3DContext::getCurrent()->getDevice()->CreateUnorderedAccessView(resource, nullptr, &destTextureUAVDesc, currentCPUHandle);
		currentCPUHandle.Offset(1, descriptorSize);
		cmdList.m_commandList->SetComputeRoot32BitConstant(0, DWParam(1.0f / dstWidth).Uint, 0);
		cmdList.m_commandList->SetComputeRoot32BitConstant(0, DWParam(1.0f / dstHeight).Uint, 1);

		cmdList.m_commandList->SetComputeRootDescriptorTable(1, currentGPUHandle);
		currentGPUHandle.Offset(1, descriptorSize);
		cmdList.m_commandList->SetComputeRootDescriptorTable(2, currentGPUHandle);
		currentGPUHandle.Offset(1, descriptorSize);

		cmdList.m_commandList->Dispatch(std::max(dstWidth / 8, 1u), std::max(dstHeight / 8, 1u), 1);

		D3DContext::getCurrent()->submitCommandList(cmdList);

	}
	cmdList.m_commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(resource, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE));
	D3DContext::getCurrent()->submitCommandList(cmdList);
	descriptorHeap->Release();
	cmdList.m_commandList->Release();
	cmdList.m_commandAllocators[0]->Release();
	return true;
}
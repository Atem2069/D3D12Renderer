#pragma once

#include "BaseRenderer.h"
#include <string>

class RenderPipeline
{
public:
	//Will be extended in the future--don't exactly know how to design this for now so it can process basic geometry with no need for constant buffers or anything
	bool initBasic(std::string vertexShaderPath, std::string pixelShaderPath);

	//Initializes with descriptor tables
	bool initWithDescriptorTables(std::string vertexShaderPath, std::string pixelShaderPath, D3D12_DESCRIPTOR_RANGE* descriptorRangesVTX, int numDescriptorRangesVTX, D3D12_DESCRIPTOR_RANGE* descriptorRangesPIX, int numDescriptorRangesPIX);
	
	void destroy();

	void bind();

	int getVertexRangeBinding();
	int getPixelRangeBinding();
private:
	int m_vertexRangeBinding=-1,m_pixelRangeBinding=-1;
	ID3D12RootSignature* m_rootSignature;
	ID3D12PipelineState* m_pipelineState;
};
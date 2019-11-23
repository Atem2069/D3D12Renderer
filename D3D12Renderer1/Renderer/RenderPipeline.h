#pragma once

#include "BaseRenderer.h"
#include <string>

class RenderPipeline
{
public:
	//Will be extended in the future--don't exactly know how to design this for now so it can process basic geometry with no need for constant buffers or anything
	bool initBasic(std::string vertexShaderPath, std::string pixelShaderPath);

	//Initializes with custom root parameters
	bool initWithRootParameters(std::string vertexShaderPath, std::string pixelShaderPath, D3D12_ROOT_PARAMETER* rootParameters, int numRootParameters, D3D12_STATIC_SAMPLER_DESC* m_samplers, int numSamplers);
	void destroy();

	void bind();

	int getVertexRangeBinding();
	int getPixelRangeBinding();
private:
	int m_vertexRangeBinding=-1,m_pixelRangeBinding=-1;
	ID3D12RootSignature* m_rootSignature;
	ID3D12PipelineState* m_pipelineState;
};
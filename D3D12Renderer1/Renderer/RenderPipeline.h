#pragma once

#include "BaseRenderer.h"
#include <string>

class RenderPipeline
{
public:
	//Will be extended in the future--don't exactly know how to design this for now so it can process basic geometry with no need for constant buffers or anything
	bool initBasic(std::string vertexShaderPath, std::string pixelShaderPath);

	void destroy();

	void bind();
private:
	ID3D12RootSignature* m_rootSignature;
	ID3D12PipelineState* m_pipelineState;
};
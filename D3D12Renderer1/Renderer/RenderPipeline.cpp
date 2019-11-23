#include "RenderPipeline.h"

bool RenderPipeline::initBasic(std::string vertexPath, std::string pixelPath)
{
	CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc = {};
	rootSignatureDesc.Init(0, nullptr, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);
	ID3DBlob* signature;
	HRESULT result = D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, nullptr);
	if (FAILED(result))
	{
		std::cout << "Failed to serialize root signature.. " << std::endl;
		return false;
	}

	result = D3DContext::getCurrent()->getDevice()->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), __uuidof(ID3D12RootSignature), (void**)&m_rootSignature);
	if (FAILED(result))
	{
		std::cout << "Failed to create root signature.. " << std::endl;
		return false;
	}

	//Creating shaders
	std::wstring verttemp = std::wstring(vertexPath.begin(), vertexPath.end());
	LPCWSTR vertPath = verttemp.c_str();

	ID3DBlob* vertexBytecode, *error;
	result = D3DCompileFromFile(vertPath, nullptr, nullptr, "main", "vs_5_0", 0, 0, &vertexBytecode, &error);
	if (FAILED(result))
	{
		std::cout << "Failed to compile vertex shader.. " << std::endl;
		OutputDebugStringA((LPCSTR)error->GetBufferPointer());
		return false;
	}
	
	D3D12_SHADER_BYTECODE vertexShader = {};
	vertexShader.pShaderBytecode = vertexBytecode->GetBufferPointer();
	vertexShader.BytecodeLength = vertexBytecode->GetBufferSize();

	std::wstring pixeltemp = std::wstring(pixelPath.begin(), pixelPath.end());
	LPCWSTR pixPath = pixeltemp.c_str();

	ID3DBlob* pixelBytecode;
	result = D3DCompileFromFile(pixPath, nullptr, nullptr, "main", "ps_5_0", 0, 0, &pixelBytecode, &error);
	if (FAILED(result))
	{
		std::cout << "Failed to compile pixel shader.. " << std::endl;
		OutputDebugStringA((LPCSTR)error->GetBufferPointer());
		return false;
	}

	D3D12_SHADER_BYTECODE pixelShader = {};
	pixelShader.pShaderBytecode = pixelBytecode->GetBufferPointer();
	pixelShader.BytecodeLength = pixelBytecode->GetBufferSize();

	D3D12_INPUT_ELEMENT_DESC inputLayouts[1];
	inputLayouts[0].AlignedByteOffset = 0;
	inputLayouts[0].Format = DXGI_FORMAT_R32G32B32_FLOAT;
	inputLayouts[0].InputSlot = 0;
	inputLayouts[0].InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
	inputLayouts[0].InstanceDataStepRate = 0;
	inputLayouts[0].SemanticIndex = 0;
	inputLayouts[0].SemanticName = "POSITION";

	D3D12_INPUT_LAYOUT_DESC inputLayoutDesc = {};
	inputLayoutDesc.NumElements = 1;
	inputLayoutDesc.pInputElementDescs = inputLayouts;

	D3D12_RASTERIZER_DESC rasterizerStateDesc = {};
	rasterizerStateDesc.AntialiasedLineEnable = FALSE;
	rasterizerStateDesc.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;
	rasterizerStateDesc.CullMode = D3D12_CULL_MODE_BACK;
	rasterizerStateDesc.DepthBias = 0;
	rasterizerStateDesc.DepthBiasClamp = 0;
	rasterizerStateDesc.DepthClipEnable = FALSE;
	rasterizerStateDesc.FillMode = D3D12_FILL_MODE_SOLID;
	rasterizerStateDesc.ForcedSampleCount = 0;
	rasterizerStateDesc.FrontCounterClockwise = FALSE;
	rasterizerStateDesc.MultisampleEnable = FALSE;
	rasterizerStateDesc.SlopeScaledDepthBias = 0;

	D3D12_GRAPHICS_PIPELINE_STATE_DESC pipelineStateDesc = {};
	pipelineStateDesc.VS = vertexShader;
	pipelineStateDesc.PS = pixelShader;
	pipelineStateDesc.pRootSignature = m_rootSignature;
	pipelineStateDesc.InputLayout = inputLayoutDesc;
	pipelineStateDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	pipelineStateDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
	pipelineStateDesc.RasterizerState = rasterizerStateDesc;
	pipelineStateDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
	pipelineStateDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
	pipelineStateDesc.NumRenderTargets = 1;
	pipelineStateDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	pipelineStateDesc.SampleDesc.Count = 1;
	pipelineStateDesc.SampleMask = 0xFFFFFFFF;

	result = D3DContext::getCurrent()->getDevice()->CreateGraphicsPipelineState(&pipelineStateDesc, __uuidof(ID3D12PipelineState), (void**)&m_pipelineState);
	if (FAILED(result))
	{
		std::cout << "Failed to create graphics pipeline state.. " << std::endl;
		return false;
	}

	return true;
}

bool RenderPipeline::initWithRootParameters(std::string vertexPath, std::string pixelPath, D3D12_ROOT_PARAMETER* rootParameters, int numRootParameters, D3D12_STATIC_SAMPLER_DESC* m_samplers, int numSamplers)
{
	HRESULT result;;
	CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc = {};
	rootSignatureDesc.Init(numRootParameters, rootParameters, numSamplers, m_samplers, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

	ID3DBlob* signature;
	result = D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, nullptr);
	if (FAILED(result))
	{
		std::cout << "Failed to serialize root signature.. " << std::endl;
		return false;
	}

	result = D3DContext::getCurrent()->getDevice()->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), __uuidof(ID3D12RootSignature), (void**)&m_rootSignature);
	if (FAILED(result))
	{
		std::cout << "Failed to create root signature.. " << std::endl;
		return false;
	}

	//Creating shaders
	std::wstring verttemp = std::wstring(vertexPath.begin(), vertexPath.end());
	LPCWSTR vertPath = verttemp.c_str();

	ID3DBlob* vertexBytecode, *error;
	result = D3DCompileFromFile(vertPath, nullptr, nullptr, "main", "vs_5_0", 0, 0, &vertexBytecode, &error);
	if (FAILED(result))
	{
		std::cout << "Failed to compile vertex shader.. " << std::endl;
		OutputDebugStringA((LPCSTR)error->GetBufferPointer());
		return false;
	}

	D3D12_SHADER_BYTECODE vertexShader = {};
	vertexShader.pShaderBytecode = vertexBytecode->GetBufferPointer();
	vertexShader.BytecodeLength = vertexBytecode->GetBufferSize();

	std::wstring pixeltemp = std::wstring(pixelPath.begin(), pixelPath.end());
	LPCWSTR pixPath = pixeltemp.c_str();

	ID3DBlob* pixelBytecode;
	result = D3DCompileFromFile(pixPath, nullptr, nullptr, "main", "ps_5_0", 0, 0, &pixelBytecode, &error);
	if (FAILED(result))
	{
		std::cout << "Failed to compile pixel shader.. " << std::endl;
		OutputDebugStringA((LPCSTR)error->GetBufferPointer());
		return false;
	}

	D3D12_SHADER_BYTECODE pixelShader = {};
	pixelShader.pShaderBytecode = pixelBytecode->GetBufferPointer();
	pixelShader.BytecodeLength = pixelBytecode->GetBufferSize();

	D3D12_INPUT_ELEMENT_DESC inputLayouts[3];
	inputLayouts[0].AlignedByteOffset = 0;
	inputLayouts[0].Format = DXGI_FORMAT_R32G32B32_FLOAT;
	inputLayouts[0].InputSlot = 0;
	inputLayouts[0].InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
	inputLayouts[0].InstanceDataStepRate = 0;
	inputLayouts[0].SemanticIndex = 0;
	inputLayouts[0].SemanticName = "POSITION";

	inputLayouts[1].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
	inputLayouts[1].Format = DXGI_FORMAT_R32G32B32_FLOAT;
	inputLayouts[1].InputSlot = 0;
	inputLayouts[1].InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
	inputLayouts[1].InstanceDataStepRate = 0;
	inputLayouts[1].SemanticIndex = 0;
	inputLayouts[1].SemanticName = "NORMAL";

	inputLayouts[2].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
	inputLayouts[2].Format = DXGI_FORMAT_R32G32_FLOAT;
	inputLayouts[2].InputSlot = 0;
	inputLayouts[2].InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
	inputLayouts[2].InstanceDataStepRate = 0;
	inputLayouts[2].SemanticIndex = 0;
	inputLayouts[2].SemanticName = "TEXCOORD";

	D3D12_INPUT_LAYOUT_DESC inputLayoutDesc = {};
	inputLayoutDesc.NumElements = 3;
	inputLayoutDesc.pInputElementDescs = inputLayouts;

	D3D12_RASTERIZER_DESC rasterizerStateDesc = {};
	rasterizerStateDesc.AntialiasedLineEnable = FALSE;
	rasterizerStateDesc.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;
	rasterizerStateDesc.CullMode = D3D12_CULL_MODE_BACK;
	rasterizerStateDesc.DepthBias = 0;
	rasterizerStateDesc.DepthBiasClamp = 0;
	rasterizerStateDesc.DepthClipEnable = TRUE;
	rasterizerStateDesc.FillMode = D3D12_FILL_MODE_SOLID;
	rasterizerStateDesc.ForcedSampleCount = 0;
	rasterizerStateDesc.FrontCounterClockwise = FALSE;
	rasterizerStateDesc.MultisampleEnable = FALSE;
	rasterizerStateDesc.SlopeScaledDepthBias = 0;

	D3D12_GRAPHICS_PIPELINE_STATE_DESC pipelineStateDesc = {};
	pipelineStateDesc.VS = vertexShader;
	pipelineStateDesc.PS = pixelShader;
	pipelineStateDesc.pRootSignature = m_rootSignature;
	pipelineStateDesc.InputLayout = inputLayoutDesc;
	pipelineStateDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	pipelineStateDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
	pipelineStateDesc.RasterizerState = rasterizerStateDesc;
	pipelineStateDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
	pipelineStateDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
	pipelineStateDesc.NumRenderTargets = 1;
	pipelineStateDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	pipelineStateDesc.SampleDesc.Count = 1;
	pipelineStateDesc.SampleMask = 0xFFFFFFFF;

	result = D3DContext::getCurrent()->getDevice()->CreateGraphicsPipelineState(&pipelineStateDesc, __uuidof(ID3D12PipelineState), (void**)&m_pipelineState);
	if (FAILED(result))
	{
		std::cout << "Failed to create graphics pipeline state.. " << std::endl;
		return false;
	}

	return true;
}

void RenderPipeline::destroy()
{
	//todo
}

void RenderPipeline::bind()
{
	D3DContext::getCurrent()->getCommandList()->SetPipelineState(m_pipelineState);
	D3DContext::getCurrent()->getCommandList()->SetGraphicsRootSignature(m_rootSignature);
	D3DContext::getCurrent()->getCommandList()->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
}

int RenderPipeline::getVertexRangeBinding()
{
	return m_vertexRangeBinding;
}

int RenderPipeline::getPixelRangeBinding()
{
	return m_pixelRangeBinding;
}
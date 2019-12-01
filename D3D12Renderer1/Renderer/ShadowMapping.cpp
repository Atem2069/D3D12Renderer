#include "ShadowMapping.h"

bool DirectionalShadowMap::init(float width, float height, float orthoWidth, float orthoHeight, XMFLOAT4 lightDirection, ResourceHeap& objectsResourceHeap)
{
	HRESULT result;
	std::string vertShaderPath = R"(Shaders\Shadows\vertexShader.hlsl)";
	std::wstring vertexShaderPath(vertShaderPath.begin(), vertShaderPath.end());
	m_width = width; m_height = height;

	//Start by creating root signature
	CD3DX12_ROOT_DESCRIPTOR rootDescriptorCamera;
	rootDescriptorCamera.Init(0, 0);
	D3D12_ROOT_PARAMETER baseRootParameter = {};
	baseRootParameter.ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	baseRootParameter.Descriptor = rootDescriptorCamera;

	CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc;
	rootSignatureDesc.Init(1, &baseRootParameter, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);
	ID3DBlob* signature, *error;
	result = D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error);
	if (FAILED(result))
	{
		std::cout << "Failed to serialize root signature.. " << std::endl;
		std::cout << (char*)error->GetBufferPointer();
		return false;
	}

	result = D3DContext::getCurrent()->getDevice()->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), __uuidof(ID3D12RootSignature), (void**)&m_depthOnlyRootSignature);
	if (FAILED(result))
	{
		std::cout << "Failed to create root signature.. " << std::endl;
		return false;
	}

	ID3DBlob* vertexBytecode;

	result = D3DCompileFromFile(vertexShaderPath.c_str(), nullptr, nullptr, "main", "vs_5_0", 0, 0, &vertexBytecode, &error);
	if (FAILED(result))
	{
		std::cout << "Failed to compile vertex shader.. " << std::endl;
		std::cout << (char*)error->GetBufferPointer();
		return false;
	}

	D3D12_SHADER_BYTECODE vertexShaderBytecode = {};
	vertexShaderBytecode.pShaderBytecode = vertexBytecode->GetBufferPointer();
	vertexShaderBytecode.BytecodeLength = vertexBytecode->GetBufferSize();

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
	rasterizerStateDesc.DepthClipEnable = FALSE;
	rasterizerStateDesc.FillMode = D3D12_FILL_MODE_SOLID;
	rasterizerStateDesc.ForcedSampleCount = 0;
	rasterizerStateDesc.FrontCounterClockwise = FALSE;
	rasterizerStateDesc.MultisampleEnable = FALSE;
	rasterizerStateDesc.SlopeScaledDepthBias = 0;

	D3D12_DEPTH_STENCIL_DESC depthstencildesc;
	depthstencildesc.DepthEnable = true;
	depthstencildesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
	depthstencildesc.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
	depthstencildesc.StencilEnable = false;
	depthstencildesc.StencilReadMask = D3D12_DEFAULT_STENCIL_READ_MASK;
	depthstencildesc.StencilWriteMask = D3D12_DEFAULT_STENCIL_WRITE_MASK;
	depthstencildesc.FrontFace.StencilFailOp = D3D12_STENCIL_OP_KEEP;
	depthstencildesc.FrontFace.StencilDepthFailOp = D3D12_STENCIL_OP_KEEP;
	depthstencildesc.FrontFace.StencilPassOp = D3D12_STENCIL_OP_KEEP;
	depthstencildesc.FrontFace.StencilFunc = D3D12_COMPARISON_FUNC_ALWAYS;
	depthstencildesc.BackFace.StencilFailOp = D3D12_STENCIL_OP_KEEP;
	depthstencildesc.BackFace.StencilDepthFailOp = D3D12_STENCIL_OP_KEEP;
	depthstencildesc.BackFace.StencilPassOp = D3D12_STENCIL_OP_KEEP;
	depthstencildesc.BackFace.StencilFunc = D3D12_COMPARISON_FUNC_ALWAYS;

	D3D12_GRAPHICS_PIPELINE_STATE_DESC pipelineStateDesc = {};
	pipelineStateDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	pipelineStateDesc.DepthStencilState = depthstencildesc;
	pipelineStateDesc.RasterizerState = rasterizerStateDesc;
	pipelineStateDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
	pipelineStateDesc.VS = vertexShaderBytecode;
	pipelineStateDesc.InputLayout = inputLayoutDesc;
	pipelineStateDesc.NumRenderTargets = 0;
	pipelineStateDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	pipelineStateDesc.pRootSignature = m_depthOnlyRootSignature;
	pipelineStateDesc.SampleDesc.Count = 1;
	pipelineStateDesc.SampleMask = 0xFFFFFFFF;
	
	result = D3DContext::getCurrent()->getDevice()->CreateGraphicsPipelineState(&pipelineStateDesc, __uuidof(ID3D12PipelineState), (void**)&m_depthOnlyPipelineState);
	if (FAILED(result))
	{
		std::cout << "Failed to create pipeline state object.. " << std::endl;
		return false;
	}

	m_lightCamera = {};
	m_lightCamera.m_projection = XMMatrixOrthographicLH(orthoWidth, orthoHeight, 1.0f, 10000.0f);
	m_lightCamera.m_view = XMMatrixLookAtLH(XMVectorNegate(XMLoadFloat4(&lightDirection)), XMLoadFloat4(&lightDirection), XMVectorSet(0, 1, 0, 1));

	if (!m_cameraUploadBuffer.init(&m_lightCamera, sizeof(LightSpaceCamera)))
		return false;

	D3D12_DESCRIPTOR_HEAP_DESC dsvDescriptorHeapDesc = {};
	dsvDescriptorHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	dsvDescriptorHeapDesc.NumDescriptors = 1;
	dsvDescriptorHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;

	result = D3DContext::getCurrent()->getDevice()->CreateDescriptorHeap(&dsvDescriptorHeapDesc, __uuidof(ID3D12DescriptorHeap), (void**)&m_dsvDescriptorHeap);
	if (FAILED(result))
	{
		std::cout << "Failed to create descriptor heap to hold DSV.. " << std::endl;
		return false;
	}
	D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
	dsvDesc.Format = DXGI_FORMAT_D32_FLOAT;
	dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;

	D3D12_CLEAR_VALUE depthOptimizedClearValue = {};
	depthOptimizedClearValue.Format = dsvDesc.Format;
	depthOptimizedClearValue.DepthStencil.Depth = 1.0f;

	result = D3DContext::getCurrent()->getDevice()->CreateCommittedResource(&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT), D3D12_HEAP_FLAG_NONE, &CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_D32_FLOAT, m_width, m_height,1,0,1,0,D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL), D3D12_RESOURCE_STATE_DEPTH_WRITE, &depthOptimizedClearValue, __uuidof(ID3D12Resource), (void**)&m_shadowMapTex);
	if (FAILED(result))
	{
		std::cout << "Failed to create depth texture.. " << std::endl;
		return false;
	}

	//Create DSV
	D3DContext::getCurrent()->getDevice()->CreateDepthStencilView(m_shadowMapTex, &dsvDesc, m_dsvDescriptorHeap->GetCPUDescriptorHandleForHeapStart());

	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Format = DXGI_FORMAT_R32_FLOAT;
	srvDesc.Texture2D.MipLevels = 1;
	
	CD3DX12_CPU_DESCRIPTOR_HANDLE resourceHeapDescriptorHandle(objectsResourceHeap.getCurrent(0)->GetCPUDescriptorHandleForHeapStart());
	if (objectsResourceHeap.numBoundDescriptors > 0)
	{
		UINT64 increment = D3DContext::getCurrent()->getDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		resourceHeapDescriptorHandle.Offset(objectsResourceHeap.numBoundDescriptors, increment);
	}
	m_descriptorOffset = objectsResourceHeap.numBoundDescriptors;
	objectsResourceHeap.numBoundDescriptors++;
	
	D3DContext::getCurrent()->getCurrentCommandList().m_commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_shadowMapTex, D3D12_RESOURCE_STATE_DEPTH_WRITE, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE));

	D3DContext::getCurrent()->getDevice()->CreateShaderResourceView(m_shadowMapTex, &srvDesc, resourceHeapDescriptorHandle);

	m_viewport = {};
	m_viewport.Width = m_width;
	m_viewport.Height = m_height;
	m_viewport.TopLeftX = 0;
	m_viewport.TopLeftY = 0;
	m_viewport.MinDepth = 0.0f;
	m_viewport.MaxDepth = 1.0f;

	m_scissorRect = {};
	m_scissorRect.right = m_width;
	m_scissorRect.bottom = m_height;
	m_scissorRect.left = 0;
	m_scissorRect.top = 0;

	return true;
}

void DirectionalShadowMap::destroy()
{
	//todo
}

void DirectionalShadowMap::beginFrame(XMFLOAT4 lightDirection)
{
	m_lightCamera.m_view = XMMatrixLookAtLH(XMVectorNegate(XMLoadFloat4(&lightDirection)), XMLoadFloat4(&lightDirection), XMVectorSet(0, 1, 0, 1));
	m_cameraUploadBuffer.update(&m_lightCamera, sizeof(LightSpaceCamera));

	D3DContext::getCurrent()->getCurrentCommandList().m_commandList->SetPipelineState(m_depthOnlyPipelineState);
	D3DContext::getCurrent()->getCurrentCommandList().m_commandList->SetGraphicsRootSignature(m_depthOnlyRootSignature);
	D3DContext::getCurrent()->getCurrentCommandList().m_commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	D3DContext::getCurrent()->getCurrentCommandList().m_commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_shadowMapTex, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_DEPTH_WRITE));
	CD3DX12_CPU_DESCRIPTOR_HANDLE dsvDescriptorHandle(m_dsvDescriptorHeap->GetCPUDescriptorHandleForHeapStart());

	D3DContext::getCurrent()->getCurrentCommandList().m_commandList->OMSetRenderTargets(0, nullptr, FALSE, &dsvDescriptorHandle);
	D3DContext::getCurrent()->getCurrentCommandList().m_commandList->ClearDepthStencilView(dsvDescriptorHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

	D3DContext::getCurrent()->getCurrentCommandList().m_commandList->RSSetViewports(1, &m_viewport);
	D3DContext::getCurrent()->getCurrentCommandList().m_commandList->RSSetScissorRects(1, &m_scissorRect);

	m_cameraUploadBuffer.bind(0);
}

void DirectionalShadowMap::endFrame()
{
	D3DContext::getCurrent()->getCurrentCommandList().m_commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_shadowMapTex, D3D12_RESOURCE_STATE_DEPTH_WRITE, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE));
}

void DirectionalShadowMap::beginFrame(XMFLOAT4 lightDirection, CommandList commandList)
{
	m_lightCamera.m_view = XMMatrixLookAtLH(XMVectorNegate(XMLoadFloat4(&lightDirection)), XMLoadFloat4(&lightDirection), XMVectorSet(0, 1, 0, 1));
	m_cameraUploadBuffer.update(&m_lightCamera, sizeof(LightSpaceCamera));

	commandList.m_commandList->SetPipelineState(m_depthOnlyPipelineState);
	commandList.m_commandList->SetGraphicsRootSignature(m_depthOnlyRootSignature);
	commandList.m_commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	commandList.m_commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_shadowMapTex, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_DEPTH_WRITE));
	CD3DX12_CPU_DESCRIPTOR_HANDLE dsvDescriptorHandle(m_dsvDescriptorHeap->GetCPUDescriptorHandleForHeapStart());

	commandList.m_commandList->OMSetRenderTargets(0, nullptr, FALSE, &dsvDescriptorHandle);
	commandList.m_commandList->ClearDepthStencilView(dsvDescriptorHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

	commandList.m_commandList->RSSetViewports(1, &m_viewport);
	commandList.m_commandList->RSSetScissorRects(1, &m_scissorRect);

	m_cameraUploadBuffer.bind(0,commandList);
}

void DirectionalShadowMap::endFrame(CommandList commandList)
{
	D3DContext::getCurrent()->getCurrentCommandList().m_commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_shadowMapTex, D3D12_RESOURCE_STATE_DEPTH_WRITE, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE));
}

void DirectionalShadowMap::bindTexture(int rootParameterIndex, ResourceHeap& resourceHeap)
{
	resourceHeap.bindDescriptorTable(rootParameterIndex, m_descriptorOffset,0);
}

void DirectionalShadowMap::bindCamera(int binding)
{
	m_cameraUploadBuffer.bind(binding);
}
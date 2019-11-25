#pragma once

#include "BaseRenderer.h"
#include "ConstantBuffer.h"
#include "ResourceHeap.h"

struct LightSpaceCamera
{
	XMMATRIX m_projection;
	XMMATRIX m_view;
};

class DirectionalShadowMap
{
public:
	bool init(float width, float height, float orthoWidth, float orthoHeight, XMFLOAT4 lightDirection, ResourceHeap& objectsResourceHeap);
	void destroy();

	void beginFrame();
	void endFrame();
private:
	ID3D12DescriptorHeap* m_dsvDescriptorHeap;
	ID3D12Resource* m_shadowMapTex;
	float m_width, m_height;
	D3D12_VIEWPORT m_viewport;

	LightSpaceCamera m_lightCamera;
	ConstantBuffer m_cameraUploadBuffer;

	ID3D12RootSignature* m_depthOnlyRootSignature;
	ID3D12PipelineState* m_depthOnlyPipelineState;
};
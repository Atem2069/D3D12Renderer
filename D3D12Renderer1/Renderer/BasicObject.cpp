#include "BasicObject.h"

bool BasicObject::init(Vertex* vertices, int noVertices)
{
	HRESULT result;
	m_noVertices = noVertices;

	result = D3DContext::getCurrent()->getDevice()->CreateCommittedResource(&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT), D3D12_HEAP_FLAG_NONE, &CD3DX12_RESOURCE_DESC::Buffer(noVertices * sizeof(Vertex)), D3D12_RESOURCE_STATE_COPY_DEST, nullptr, __uuidof(ID3D12Resource), (void**)&m_vertexBuffer);
	if (FAILED(result))
	{
		std::cout << "Failed to create vertex buffer.. " << std::endl;
		return false;
	}

	result = D3DContext::getCurrent()->getDevice()->CreateCommittedResource(&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD), D3D12_HEAP_FLAG_NONE, &CD3DX12_RESOURCE_DESC::Buffer(noVertices * sizeof(Vertex)), D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, __uuidof(ID3D12Resource), (void**)&m_uploadBuffer);
	if (FAILED(result))
	{
		std::cout << "Failed to create vertex upload buffer.. " << std::endl;
		return false;
	}

	D3D12_SUBRESOURCE_DATA vertexData = {};
	vertexData.pData = vertices;
	vertexData.RowPitch = noVertices * sizeof(Vertex);
	vertexData.SlicePitch = noVertices * sizeof(Vertex);

	UpdateSubresources(D3DContext::getCurrent()->getCommandList(), m_vertexBuffer, m_uploadBuffer, 0, 0, 1, &vertexData);

	D3DContext::getCurrent()->getCommandList()->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_vertexBuffer, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER));

	m_vertexBufferView.BufferLocation = m_vertexBuffer->GetGPUVirtualAddress();
	m_vertexBufferView.SizeInBytes = noVertices * sizeof(Vertex);
	m_vertexBufferView.StrideInBytes = sizeof(Vertex);

	return true;
}

void BasicObject::destroy()
{
	//todo
}

void BasicObject::draw()
{
	D3DContext::getCurrent()->getCommandList()->IASetVertexBuffers(0, 1, &m_vertexBufferView);
	D3DContext::getCurrent()->getCommandList()->DrawInstanced(m_noVertices, 1, 0, 0);
	if (!m_intermediateBufferDestroyed)
	{
		m_uploadBuffer->Release();
		m_intermediateBufferDestroyed = true;
	}
}
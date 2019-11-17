#pragma once

#include "BaseRenderer.h"

class BasicObject
{
public:
	bool init(Vertex* vertices, int noVertices);
	void destroy();

	void draw();
private:
	D3D12_VERTEX_BUFFER_VIEW m_vertexBufferView;
	ID3D12Resource* m_vertexBuffer;
	ID3D12Resource* m_uploadBuffer;
	int m_noVertices;
	bool m_intermediateBufferDestroyed = false;
};
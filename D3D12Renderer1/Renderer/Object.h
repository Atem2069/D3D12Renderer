#pragma once

#include "BaseRenderer.h"

#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>

#include <string>
#include <vector>
struct Mesh
{
	int m_baseVertexLocation;
	int m_baseIndexLocation;
	int m_numIndices;
};

class Object
{
public:
	bool init(std::string filePath);
	void destroy();

	void draw();

private:
	//VBO
	D3D12_VERTEX_BUFFER_VIEW m_vertexBufferView;
	ID3D12Resource* m_vertexBufferHeap;
	ID3D12Resource* m_vertexBufferUploadHeap;

	//IBO
	D3D12_INDEX_BUFFER_VIEW m_indexBufferView;
	ID3D12Resource* m_indexBufferHeap;
	ID3D12Resource* m_indexBufferUploadHeap;
	std::vector<Mesh> m_meshes;
};
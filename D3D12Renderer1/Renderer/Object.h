#pragma once

#include "BaseRenderer.h"
#include "Texturing.h"

#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>

#include <string>
#include <vector>

struct Material
{
	bool m_albedoTextureLoaded = false;
	Texture2D m_albedoTexture;
	int m_materialIndex;
};

struct Mesh
{
	int m_baseVertexLocation;
	int m_baseIndexLocation;
	int m_numIndices;
	Material m_material;
};

class Object
{
public:
	bool init(std::string filePath, ResourceHeap& textureHeap);
	void destroy();

	void draw();
	void draw(CommandList commandList);
	void draw(ResourceHeap& resourceHeap);

	int m_faces = 0;

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
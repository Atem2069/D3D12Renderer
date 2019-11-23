#include "Object.h"

bool Object::init(std::string filePath, ResourceHeap& textureHeap)
{
	std::string dir = filePath.substr(0, filePath.find_last_of(R"(\)")) + R"(\)";
	HRESULT result;

	Assimp::Importer m_importer;
	const aiScene* m_scene = m_importer.ReadFile(filePath, aiProcess_Triangulate | aiProcess_ConvertToLeftHanded | aiProcess_GenSmoothNormals);
	if (!m_scene || m_scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !m_scene->mRootNode)
	{
		std::cout << "Assimp scene failed to import.. " << std::endl;
		std::cout << m_importer.GetErrorString() << std::endl;
		return false;
	}

	std::vector<Vertex> m_vertices;
	std::vector<unsigned int> m_indices;
	int globalVertexBase = 0, globalIndexBase = 0;	//state tracking

	for (int i = 0; i < m_scene->mNumMeshes; i++)
	{
		Mesh tempMesh = {};
		aiMesh* aiMesh = m_scene->mMeshes[i];

		for (unsigned int j = 0; j < aiMesh->mNumVertices; j++)
		{
			Vertex tempVertex = {};
			tempVertex.position.x = aiMesh->mVertices[j].x;
			tempVertex.position.y = aiMesh->mVertices[j].y;
			tempVertex.position.z = aiMesh->mVertices[j].z;

			tempVertex.normal.x = aiMesh->mNormals[j].x;
			tempVertex.normal.y = aiMesh->mNormals[j].y;
			tempVertex.normal.z = aiMesh->mNormals[j].z;

			if (aiMesh->mTextureCoords[0])
			{
				tempVertex.texcoord.x = aiMesh->mTextureCoords[0][j].x;
				tempVertex.texcoord.y = aiMesh->mTextureCoords[0][j].y;
			}

			m_vertices.push_back(tempVertex);
		}

		//Calc indices
		int noIndices = 0;
		for (unsigned int j = 0; j < aiMesh->mNumFaces; j++)
		{
			aiFace face = aiMesh->mFaces[j];
			for (unsigned int k = 0; k < face.mNumIndices; k++)
				m_indices.push_back(face.mIndices[k]);
			noIndices += face.mNumIndices;
			m_faces++;
		}

		tempMesh.m_baseVertexLocation = globalVertexBase;
		globalVertexBase += aiMesh->mNumVertices;

		tempMesh.m_baseIndexLocation = globalIndexBase;
		globalIndexBase += noIndices;

		tempMesh.m_numIndices = noIndices;

		//Texturing
		if (aiMesh->mMaterialIndex > 0)
		{
			aiMaterial* material = m_scene->mMaterials[aiMesh->mMaterialIndex];
			tempMesh.m_material.m_materialIndex = aiMesh->mMaterialIndex;

			if (material->GetTextureCount(aiTextureType_DIFFUSE) > 0)
			{
				bool loadTexture = true;
				for (int j = 0; j < i; j++)
				{
					if (m_meshes[j].m_material.m_materialIndex == tempMesh.m_material.m_materialIndex)
					{
						loadTexture = false;
						tempMesh.m_material.m_albedoTexture = m_meshes[j].m_material.m_albedoTexture;
					}
				}

				if (loadTexture)
				{
					aiString texPath;
					material->GetTexture(aiTextureType_DIFFUSE, 0, &texPath);
					std::string texFile = texPath.C_Str();
					std::string texturePath = dir + texFile;
					tempMesh.m_material.m_albedoTexture.init(texturePath, textureHeap);
				}

				tempMesh.m_material.m_albedoTextureLoaded = true;
			}
		}

		m_meshes.push_back(tempMesh);
	}

	//Create buffers now that we have meshes sorted and big buffers created

	result = D3DContext::getCurrent()->getDevice()->CreateCommittedResource(&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT), D3D12_HEAP_FLAG_NONE, &CD3DX12_RESOURCE_DESC::Buffer(m_vertices.size() * sizeof(Vertex)), D3D12_RESOURCE_STATE_COPY_DEST, nullptr, __uuidof(ID3D12Resource), (void**)&m_vertexBufferHeap);
	if (FAILED(result))
	{
		std::cout << "Failed to create Vertex Buffer storage heap.. " << std::endl;
		return false;
	}

	result = D3DContext::getCurrent()->getDevice()->CreateCommittedResource(&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD), D3D12_HEAP_FLAG_NONE, &CD3DX12_RESOURCE_DESC::Buffer(m_vertices.size() * sizeof(Vertex)), D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, __uuidof(ID3D12Resource), (void**)&m_vertexBufferUploadHeap);
	if (FAILED(result))
	{
		std::cout << "Failed to create Vertex Buffer upload heap.. " << std::endl;
		return false;
	}

	D3D12_SUBRESOURCE_DATA vertexData = {};
	vertexData.pData = &m_vertices[0];
	vertexData.RowPitch = m_vertices.size() * sizeof(Vertex);
	vertexData.SlicePitch = vertexData.RowPitch;

	UpdateSubresources(D3DContext::getCurrent()->getCommandList(), m_vertexBufferHeap, m_vertexBufferUploadHeap, 0, 0, 1, &vertexData);

	D3DContext::getCurrent()->getCommandList()->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_vertexBufferHeap, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER));

	m_vertexBufferView = {};
	m_vertexBufferView.BufferLocation = m_vertexBufferHeap->GetGPUVirtualAddress();
	m_vertexBufferView.StrideInBytes = sizeof(Vertex);
	m_vertexBufferView.SizeInBytes = m_vertices.size() * sizeof(Vertex);

	//Creating index buffer, similar way to vertex buffer

	result = D3DContext::getCurrent()->getDevice()->CreateCommittedResource(&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT), D3D12_HEAP_FLAG_NONE, &CD3DX12_RESOURCE_DESC::Buffer(m_indices.size() * sizeof(unsigned int)), D3D12_RESOURCE_STATE_COPY_DEST, nullptr, __uuidof(ID3D12Resource), (void**)&m_indexBufferHeap);
	if (FAILED(result))
	{
		std::cout << "Failed to create Index Buffer storage heap.. " << std::endl;
		return false;
	}

	result = D3DContext::getCurrent()->getDevice()->CreateCommittedResource(&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD), D3D12_HEAP_FLAG_NONE, &CD3DX12_RESOURCE_DESC::Buffer(m_indices.size() * sizeof(unsigned int)), D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, __uuidof(ID3D12Resource), (void**)&m_indexBufferUploadHeap);
	if (FAILED(result))
	{
		std::cout << "Failed to create Index Buffer upload heap.. " << std::endl;
		return false;
	}

	D3D12_SUBRESOURCE_DATA indexData = {};
	indexData.pData = &m_indices[0];
	indexData.RowPitch = m_indices.size() * sizeof(unsigned int);
	indexData.SlicePitch = indexData.RowPitch;

	UpdateSubresources(D3DContext::getCurrent()->getCommandList(), m_indexBufferHeap, m_indexBufferUploadHeap, 0, 0, 1, &indexData);

	D3DContext::getCurrent()->getCommandList()->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_indexBufferHeap, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_INDEX_BUFFER));

	m_indexBufferView = {};
	m_indexBufferView.BufferLocation = m_indexBufferHeap->GetGPUVirtualAddress();
	m_indexBufferView.Format = DXGI_FORMAT_R32_UINT;
	m_indexBufferView.SizeInBytes = m_indices.size() * sizeof(unsigned int);

	return true;
}

void Object::destroy()
{
	//todo lol
}

void Object::draw(ResourceHeap& textureHeap)
{
	D3DContext::getCurrent()->getCommandList()->IASetVertexBuffers(0, 1, &m_vertexBufferView);
	D3DContext::getCurrent()->getCommandList()->IASetIndexBuffer(&m_indexBufferView);
	for (int i = 0; i < m_meshes.size(); i++)
	{
		if(m_meshes[i].m_material.m_albedoTextureLoaded)
			textureHeap.bindDescriptorTable(2, m_meshes[i].m_material.m_albedoTexture.m_descriptorOffset,0);
		D3DContext::getCurrent()->getCommandList()->DrawIndexedInstanced(m_meshes[i].m_numIndices, 1, m_meshes[i].m_baseIndexLocation, m_meshes[i].m_baseVertexLocation, 0);
	}
}
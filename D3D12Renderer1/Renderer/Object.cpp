#include "Object.h"

bool Object::init(std::string filePath)
{
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

void Object::draw()
{
	D3DContext::getCurrent()->getCommandList()->IASetVertexBuffers(0, 1, &m_vertexBufferView);
	D3DContext::getCurrent()->getCommandList()->IASetIndexBuffer(&m_indexBufferView);
	for (int i = 0; i < m_meshes.size(); i++)
		D3DContext::getCurrent()->getCommandList()->DrawIndexedInstanced(m_meshes[i].m_numIndices, 1, m_meshes[i].m_baseIndexLocation, m_meshes[i].m_baseVertexLocation, 0);
}
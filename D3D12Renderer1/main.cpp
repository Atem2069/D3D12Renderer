#include<iostream>
#define NOMINMAX
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>

#include "Renderer/BaseRenderer.h"
#include "Renderer/RenderPipeline.h"
#include "Renderer/BasicObject.h"
#include "Renderer/Object.h"
#include "Renderer/ResourceHeap.h"
#include "Renderer/ConstantBuffer.h"


constexpr int Width = 800;
constexpr int Height = 800;

struct ColorConstBuffer
{
	XMFLOAT4 color;
};

struct VertexConstBuffer
{
	XMFLOAT4 scale;
};

int main()
{
	if (!glfwInit())
	{
		std::cout << "Failed to init GLFW" << std::endl;
		return -1;
	}

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
	GLFWwindow* m_window = glfwCreateWindow(Width, Height, "Direct3D 12", nullptr, nullptr);
	if (!m_window)
	{
		std::cout << "Failed to create GLFW window.. " << std::endl;
		return -1;
	}

	HWND hwnd = glfwGetWin32Window(m_window);

	D3D m_context;
	if (!m_context.init(Width, Height, hwnd))
		return -1;
	D3DContext::Register(m_context);

	D3D12_DESCRIPTOR_RANGE pixColorDescriptor[1] = {};
	pixColorDescriptor[0].BaseShaderRegister = 0;
	pixColorDescriptor[0].NumDescriptors = 1;
	pixColorDescriptor[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
	pixColorDescriptor[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
	pixColorDescriptor[0].RegisterSpace = 0;

	D3D12_DESCRIPTOR_RANGE vtxDescriptor[1] = {};
	vtxDescriptor[0].BaseShaderRegister = 0;
	vtxDescriptor[0].NumDescriptors = 1;
	vtxDescriptor[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
	vtxDescriptor[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
	vtxDescriptor[0].RegisterSpace = 0;

	RenderPipeline m_pipeline;
	if(!m_pipeline.initWithDescriptorTables(R"(Shaders\basicVertex.hlsl)",R"(Shaders\basicPixel.hlsl)",vtxDescriptor,1,pixColorDescriptor,1))
		return -1;

	Vertex vertices[3];
	vertices[0].position = XMFLOAT3(-0.5f, -0.5f, 0.0f);
	vertices[1].position = XMFLOAT3(0.0f, 0.5f, 0.0f);
	vertices[2].position = XMFLOAT3(0.5f, -0.5f, 0.0f);
	BasicObject m_basicObject;
	if (!m_basicObject.init(vertices, 3))
		return -1;

	Object m_object;
	if (!m_object.init(R"(Models\teapot.obj)"))
		return -1;

	ResourceHeap m_baseResourceHeap;
	if (!m_baseResourceHeap.init(2))
		return -1;


	ColorConstBuffer col = {};
	col.color = XMFLOAT4(1, 1, 1,1);

	ConstantBuffer m_constantBuffer;
	if (!m_constantBuffer.init(&col, sizeof(ColorConstBuffer), m_baseResourceHeap))
		return -1;

	VertexConstBuffer vtxCbuffer = {};
	vtxCbuffer.scale = XMFLOAT4(1, 1, 1, 1);

	ConstantBuffer m_vertexConstantBuffer;
	if (!m_vertexConstantBuffer.init(&vtxCbuffer, sizeof(VertexConstBuffer), m_baseResourceHeap))
		return -1;

	D3DContext::getCurrent()->executeAndSynchronize();	//Execute staging changes to CMD list.

	double lastTime = 0, currentTime = glfwGetTime(), deltaTime = 1;
	while (!glfwWindowShouldClose(m_window))
	{
		glfwPollEvents();

		D3DContext::getCurrent()->synchronizeAndReset();

		//cpu updates
		col.color.x = sin(glfwGetTime());
		m_constantBuffer.update(&col, sizeof(ColorConstBuffer));

		D3DContext::getCurrent()->beginRenderPass(0.564f, 0.8f, 0.976f, 1.f);
	
		m_pipeline.bind();
		ID3D12DescriptorHeap* baseResHeap = m_baseResourceHeap.getCurrent();
		D3DContext::getCurrent()->bindAllResourceHeaps(&baseResHeap, 1);
		m_baseResourceHeap.bindDescriptorTable(0, m_vertexConstantBuffer.getDescriptorLocation());
		m_baseResourceHeap.bindDescriptorTable(1, m_constantBuffer.getDescriptorLocation());
		m_object.draw();

		D3DContext::getCurrent()->endRenderPass();

		if (!D3DContext::getCurrent()->executeAndPresent(false))
			glfwSetWindowShouldClose(m_window, GLFW_TRUE);

		//Calculate deltatime
		deltaTime = currentTime - lastTime;
		lastTime = currentTime;
		currentTime = glfwGetTime();
		glfwSetWindowTitle(m_window, std::to_string(1 / deltaTime).c_str());
	}
	return 0;
}
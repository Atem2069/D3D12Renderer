#include<iostream>
#define NOMINMAX
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>

#include "Renderer/BaseRenderer.h"
#include "Renderer/RenderPipeline.h"
#include "Renderer/BasicObject.h"
#include "Renderer/ResourceHeap.h"
#include "Renderer/ConstantBuffer.h"


constexpr int Width = 500;
constexpr int Height = 500;

struct ColorConstBuffer
{
	XMFLOAT3 color;
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

	RenderPipeline m_pipeline;
	if(!m_pipeline.initWithDescriptorTables(R"(Shaders\basicVertex.hlsl)",R"(Shaders\basicPixel.hlsl)",nullptr,0,pixColorDescriptor,1))
		return -1;

	Vertex vertices[3];
	vertices[0].position = XMFLOAT3(-0.5f, -0.5f, 0.0f);
	vertices[1].position = XMFLOAT3(0.0f, 0.5f, 0.0f);
	vertices[2].position = XMFLOAT3(0.5f, -0.5f, 0.0f);
	BasicObject m_basicObject;
	if (!m_basicObject.init(vertices, 3))
		return false;

	ResourceHeap m_baseResourceHeap;
	if (!m_baseResourceHeap.init(1))
		return -1;


	ColorConstBuffer col = {};
	col.color = XMFLOAT3(1, 1, 1);

	ConstantBuffer m_constantBuffer;
	if (!m_constantBuffer.init(&col, sizeof(ColorConstBuffer), m_baseResourceHeap))
		return -1;


	D3DContext::getCurrent()->executeAndSynchronize();	//Execute staging changes to CMD list.

	double lastTime = 0, currentTime = glfwGetTime(), deltaTime = 1;
	while (!glfwWindowShouldClose(m_window))
	{
		glfwPollEvents();

		D3DContext::getCurrent()->synchronizeAndReset();

		D3DContext::getCurrent()->beginRenderPass(0.564f, 0.8f, 0.976f, 1.f);
	
		m_pipeline.bind();
		ID3D12DescriptorHeap* baseResHeap = m_baseResourceHeap.getCurrent();
		D3DContext::getCurrent()->bindAllResourceHeaps(&baseResHeap, 1);
		m_baseResourceHeap.bindDescriptorTable(0, 0);
		m_basicObject.draw();

		D3DContext::getCurrent()->endRenderPass();

		if (!D3DContext::getCurrent()->executeAndPresent(true))
			glfwSetWindowShouldClose(m_window, GLFW_TRUE);

		//Calculate deltatime
		deltaTime = currentTime - lastTime;
		lastTime = currentTime;
		currentTime = glfwGetTime();
		glfwSetWindowTitle(m_window, std::to_string(1 / deltaTime).c_str());
	}
	return 0;
}
#include<iostream>
#define NOMINMAX
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>

#include "Renderer/BaseRenderer.h"
#include "Renderer/RenderPipeline.h"
#include "Renderer/BasicObject.h"

constexpr int Width = 500;
constexpr int Height = 500;


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

	RenderPipeline m_pipeline;
	if (!m_pipeline.initBasic(R"(Shaders\basicVertex.hlsl)", R"(Shaders\basicPixel.hlsl)"))
		return -1;

	Vertex vertices[3];
	vertices[0].position = XMFLOAT3(-0.5f, -0.5f, 0.0f);
	vertices[1].position = XMFLOAT3(0.0f, 0.5f, 0.0f);
	vertices[2].position = XMFLOAT3(0.5f, -0.5f, 0.0f);
	BasicObject m_basicObject;
	if (!m_basicObject.init(vertices, 3))
		return false;

	D3DContext::getCurrent()->executeAndSynchronize();

	double lastTime = 0, currentTime = glfwGetTime(), deltaTime = 1;
	while (!glfwWindowShouldClose(m_window))
	{
		glfwPollEvents();

		D3DContext::getCurrent()->synchronizeAndReset();

		D3DContext::getCurrent()->beginRenderPass(0.564f, 0.8f, 0.976f, 1.f);
		m_pipeline.bind();
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
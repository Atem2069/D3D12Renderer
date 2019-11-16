#include<iostream>
#define NOMINMAX
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>

#include "Renderer/BaseRenderer.h"
#include "Renderer/RenderPipeline.h"

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

	D3DContext::getCurrent()->executeAndSynchronize();
	while (!glfwWindowShouldClose(m_window))
	{
		D3DContext::getCurrent()->synchronizeAndReset();

		D3DContext::getCurrent()->beginRenderPass(1, 0, 0, 1);
		m_pipeline.bind();
		D3DContext::getCurrent()->endRenderPass();

		D3DContext::getCurrent()->executeAndPresent();
		glfwPollEvents();
	}
	return 0;
}
#include<iostream>
#include <cmath>
#define NOMINMAX
#define GLFW_EXPOSE_NATIVE_WIN32
#pragma warning(disable:4996)
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>

#include "Renderer/BaseRenderer.h"
#include "Renderer/RenderPipeline.h"
#include "Renderer/BasicObject.h"
#include "Renderer/Object.h"
#include "Renderer/ResourceHeap.h"
#include "Renderer/ConstantBuffer.h"


constexpr int Width = 1600;
constexpr int Height = 900;

struct BasicCamera
{
	XMMATRIX projection;
	XMMATRIX view;
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

	D3D12_DESCRIPTOR_RANGE cameraDescriptorRange = {};
	cameraDescriptorRange.BaseShaderRegister = 0;
	cameraDescriptorRange.NumDescriptors = 1;
	cameraDescriptorRange.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
	cameraDescriptorRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
	cameraDescriptorRange.RegisterSpace = 0;

	RenderPipeline m_pipeline;
	if(!m_pipeline.initWithDescriptorTables(R"(Shaders\basicVertex.hlsl)",R"(Shaders\basicPixel.hlsl)",&cameraDescriptorRange,1,nullptr,0))
		return -1;

	Object m_object;
	if (!m_object.init(R"(Models\sponza\sponza.obj)"))
		return -1;

	ResourceHeap m_baseResourceHeap;
	if (!m_baseResourceHeap.init(64))
		return -1;

	BasicCamera m_basicCamera = {};
	XMVECTOR cameraPosition = XMVectorSet(0, 10, -15, 1);
	XMVECTOR cameraEye = XMVectorSet(0, 0, 1, 1);
	m_basicCamera.projection = XMMatrixPerspectiveFovLH(XMConvertToRadians(60.0f), (float)Width / (float)Height, 1.0f, 10000.0f);
	m_basicCamera.view = XMMatrixLookAtLH(cameraPosition, cameraPosition + cameraEye, XMVectorSet(0, 1, 0, 1));

	ConstantBuffer m_cameraBuffer;;
	if (!m_cameraBuffer.init(&m_basicCamera, sizeof(BasicCamera), m_baseResourceHeap))
		return -1;

	D3DContext::getCurrent()->executeAndSynchronize();	//Execute staging changes to CMD list.

	double lastTime = 0, currentTime = glfwGetTime(), deltaTime = 1;
	float pitch = 0, yaw = 0;
	while (!glfwWindowShouldClose(m_window))
	{
		glfwPollEvents();

		D3DContext::getCurrent()->synchronizeAndReset();

		D3DContext::getCurrent()->beginRenderPass(0.564f, 0.8f, 0.976f, 1.f);

		//CPU update
		m_basicCamera.view = XMMatrixLookAtLH(cameraPosition, cameraPosition + cameraEye, XMVectorSet(0, 1, 0, 1));
		m_cameraBuffer.update(&m_basicCamera, sizeof(BasicCamera));
	
		m_pipeline.bind();
		ID3D12DescriptorHeap* baseResHeap = m_baseResourceHeap.getCurrent();
		D3DContext::getCurrent()->bindAllResourceHeaps(&baseResHeap, 1);
		m_baseResourceHeap.bindDescriptorTable(m_pipeline.getVertexRangeBinding(), 0);
		m_object.draw();

		D3DContext::getCurrent()->endRenderPass();

		if (!D3DContext::getCurrent()->executeAndPresent(false))
			glfwSetWindowShouldClose(m_window, GLFW_TRUE);

		if (glfwGetKey(m_window, GLFW_KEY_W) == GLFW_PRESS)
			cameraPosition += cameraEye * 250.0f * deltaTime;
		if (glfwGetKey(m_window, GLFW_KEY_S) == GLFW_PRESS)
			cameraPosition -= cameraEye * 250.0f * deltaTime;
		if (glfwGetKey(m_window, GLFW_KEY_A) == GLFW_PRESS)
			cameraPosition += XMVector3Cross(cameraEye, XMVectorSet(0, 1, 0, 1)) * 250.0f * deltaTime;
		if (glfwGetKey(m_window, GLFW_KEY_D) == GLFW_PRESS)
			cameraPosition -= XMVector3Cross(cameraEye, XMVectorSet(0, 1, 0, 1)) * 250.0f * deltaTime;
		if (glfwGetKey(m_window, GLFW_KEY_SPACE))
			cameraPosition += XMVectorSet(0, 1, 0, 1) * 250.0f * deltaTime;
		if (glfwGetKey(m_window, GLFW_KEY_LEFT_SHIFT))
			cameraPosition -= XMVectorSet(0, 1, 0, 1) * 250.0f * deltaTime;
		if (glfwGetKey(m_window, GLFW_KEY_LEFT) == GLFW_PRESS)
			yaw += 0.5f * deltaTime * 250.0f;
		if (glfwGetKey(m_window, GLFW_KEY_RIGHT) == GLFW_PRESS)
			yaw -= 0.5f * deltaTime * 250.0f;
		if (glfwGetKey(m_window, GLFW_KEY_UP) == GLFW_PRESS)
			pitch += 0.5f * deltaTime * 250.0f;
		if (glfwGetKey(m_window, GLFW_KEY_DOWN) == GLFW_PRESS)
			pitch -= 0.5f * deltaTime * 250.0f;

		cameraEye = XMVectorSet(XMScalarCos(XMConvertToRadians(pitch)) * XMScalarCos(XMConvertToRadians(yaw)), XMScalarSin(XMConvertToRadians(pitch)), XMScalarCos(XMConvertToRadians(pitch)) * XMScalarSin(XMConvertToRadians(yaw)),1);
		XMVector3Normalize(cameraEye);
		//Calculate deltatime
		deltaTime = currentTime - lastTime;
		lastTime = currentTime;
		currentTime = glfwGetTime();
		char fps[10];
		_itoa(1 / deltaTime, fps, 10);
		glfwSetWindowTitle(m_window, fps);
	}
	return 0;
}
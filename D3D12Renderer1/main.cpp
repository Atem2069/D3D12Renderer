#include<iostream>
#include <cmath>
#define NOMINMAX
#define GLFW_EXPOSE_NATIVE_WIN32
#define STB_IMAGE_IMPLEMENTATION
#pragma warning(disable:4996)
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>

#include "Renderer/BaseRenderer.h"
#include "Renderer/RenderPipeline.h"
#include "Renderer/BasicObject.h"
#include "Renderer/Object.h"
#include "Renderer/ResourceHeap.h"
#include "Renderer/ConstantBuffer.h"

#include <imgui.h>
#include <imgui_impl_dx12.h>
#include <imgui_impl_glfw.h>


constexpr int Width = 1600;
constexpr int Height = 900;

struct BasicCamera
{
	XMMATRIX projection;
	XMMATRIX view;
};

struct LightBuffer
{
	XMFLOAT4 lightDirection;
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

	ImGui::CreateContext();
	ImGui::StyleColorsClassic();

	ResourceHeap m_imguiResourceHeap;
	if (!m_imguiResourceHeap.init(64))
		return -1;

	CD3DX12_CPU_DESCRIPTOR_HANDLE baseResHeapDescHandle(m_imguiResourceHeap.getCurrent(0)->GetCPUDescriptorHandleForHeapStart());
	baseResHeapDescHandle.Offset(m_imguiResourceHeap.numBoundDescriptors, D3DContext::getCurrent()->getDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV));

	CD3DX12_GPU_DESCRIPTOR_HANDLE baseResHeapDescGPUHandle(m_imguiResourceHeap.getCurrent(0)->GetGPUDescriptorHandleForHeapStart());;
	baseResHeapDescGPUHandle.Offset(m_imguiResourceHeap.numBoundDescriptors, D3DContext::getCurrent()->getDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV));
	ImGui_ImplGlfw_InitForVulkan(m_window, true);
	ImGui_ImplDX12_Init(D3DContext::getCurrent()->getDevice(), 2, DXGI_FORMAT_R8G8B8A8_UNORM, baseResHeapDescHandle, baseResHeapDescGPUHandle);

	D3D12_ROOT_PARAMETER m_rootParameters[3] = {};
	m_rootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	m_rootParameters[0].Descriptor = CD3DX12_ROOT_DESCRIPTOR(0, 0);
	m_rootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;

	m_rootParameters[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	m_rootParameters[1].Descriptor = CD3DX12_ROOT_DESCRIPTOR(0, 0);
	m_rootParameters[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

	D3D12_DESCRIPTOR_RANGE texRange = {};
	texRange.NumDescriptors = 1;
	texRange.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
	texRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	texRange.RegisterSpace = 0;
	texRange.BaseShaderRegister = 0;

	D3D12_ROOT_DESCRIPTOR_TABLE texTable = {};
	texTable.NumDescriptorRanges = 1;
	texTable.pDescriptorRanges = &texRange;

	m_rootParameters[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	m_rootParameters[2].DescriptorTable = texTable;
	m_rootParameters[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

	D3D12_STATIC_SAMPLER_DESC m_samplerDesc = {};
	m_samplerDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	m_samplerDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	m_samplerDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	m_samplerDesc.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
	m_samplerDesc.MaxAnisotropy = 16;
	m_samplerDesc.ShaderRegister = 0;
	m_samplerDesc.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
	

	RenderPipeline m_pipeline;
	if(!m_pipeline.initWithRootParameters(R"(Shaders\basicVertex.hlsl)",R"(Shaders\basicPixel.hlsl)",m_rootParameters,3,&m_samplerDesc,1))
		return -1;

	ResourceHeap m_objectsResourceHeap;
	if (!m_objectsResourceHeap.init(1024))
		return -1;

	Object m_object;
	if (!m_object.init(R"(Models\sanmiguel\san-miguel-low-poly.obj)", m_objectsResourceHeap))
		return -1;
	Object m_object2;
	if (!m_object2.init(R"(Models\nanosuit\nanosuit.obj)", m_objectsResourceHeap))
		return -1;
	BasicCamera m_basicCamera = {};
	XMVECTOR cameraPosition = XMVectorSet(0, 0, 0, 1);
	XMVECTOR cameraEye = XMVectorSet(0, 0, 1, 1);
	m_basicCamera.projection = XMMatrixPerspectiveFovLH(XMConvertToRadians(60.0f), (float)Width / (float)Height, 1.0f, 10000.0f);
	m_basicCamera.view = XMMatrixLookAtLH(cameraPosition, cameraPosition + cameraEye, XMVectorSet(0, 1, 0, 1));

	ConstantBuffer m_cameraBuffer;;
	if (!m_cameraBuffer.init(&m_basicCamera, sizeof(BasicCamera)))
		return -1;

	LightBuffer m_light = {};
	m_light.lightDirection = XMFLOAT4(-250, -3000, 0, 0);
	ConstantBuffer m_lightBuffer;
	if (!m_lightBuffer.init(&m_light, sizeof(LightBuffer)))
		return -1;

	D3DContext::getCurrent()->executeAndSynchronize();	//Execute staging changes to CMD list.

	double lastTime = 0, currentTime = glfwGetTime(), deltaTime = 1;
	float pitch = 0, yaw = 0;
	float cameraSpeed = 25.0f;
	while (!glfwWindowShouldClose(m_window))
	{
		glfwPollEvents();

		D3DContext::getCurrent()->synchronizeAndReset();

		ImGui_ImplDX12_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();

		ImGui::Begin("Renderer Info-Direct3D 12");
		{
			ImGui::Text("Current FPS:: %.2f", 1 / deltaTime);
			ImGui::Text("Current Face Count:: %d", m_object.m_faces + m_object2.m_faces);
		}
		ImGui::End();
		
		ImGui::Begin("Renderer Properties");
		{
			ImGui::DragFloat3("Light direction", (float*)&m_light.lightDirection);
		}
		ImGui::End();

		D3DContext::getCurrent()->beginRenderPass(0.564f, 0.8f, 0.976f, 1.f);

		//CPU update
		m_basicCamera.view = XMMatrixLookAtLH(cameraPosition, cameraPosition + cameraEye, XMVectorSet(0, 1, 0, 1));
		m_cameraBuffer.update(&m_basicCamera, sizeof(BasicCamera));
		m_lightBuffer.update(&m_light, sizeof(LightBuffer));
	
		m_pipeline.bind();
		ID3D12DescriptorHeap* objResHeaps[1] = { m_objectsResourceHeap.getCurrent(0) };
		D3DContext::getCurrent()->bindAllResourceHeaps(objResHeaps, 1);
		m_cameraBuffer.bind(0);
		m_lightBuffer.bind(1);
		m_object.draw(m_objectsResourceHeap);
		m_object2.draw(m_objectsResourceHeap);
		ID3D12DescriptorHeap* baseResHeaps[1] = { m_imguiResourceHeap.getCurrent(0) };
		D3DContext::getCurrent()->bindAllResourceHeaps(baseResHeaps, 1);
		ImGui::Render();
		ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), D3DContext::getCurrent()->getCommandList());

		D3DContext::getCurrent()->endRenderPass();
		if (!D3DContext::getCurrent()->executeAndPresent(false))
			glfwSetWindowShouldClose(m_window, GLFW_TRUE);

		if (glfwGetKey(m_window, GLFW_KEY_W) == GLFW_PRESS)
			cameraPosition += cameraEye * cameraSpeed * deltaTime;
		if (glfwGetKey(m_window, GLFW_KEY_S) == GLFW_PRESS)
			cameraPosition -= cameraEye * cameraSpeed * deltaTime;
		if (glfwGetKey(m_window, GLFW_KEY_A) == GLFW_PRESS)
			cameraPosition += XMVector3Cross(cameraEye, XMVectorSet(0, 1, 0, 1)) * cameraSpeed * deltaTime;
		if (glfwGetKey(m_window, GLFW_KEY_D) == GLFW_PRESS)
			cameraPosition -= XMVector3Cross(cameraEye, XMVectorSet(0, 1, 0, 1)) * cameraSpeed * deltaTime;
		if (glfwGetKey(m_window, GLFW_KEY_SPACE))
			cameraPosition += XMVectorSet(0, 1, 0, 1) * cameraSpeed * deltaTime;
		if (glfwGetKey(m_window, GLFW_KEY_LEFT_SHIFT))
			cameraPosition -= XMVectorSet(0, 1, 0, 1) * cameraSpeed * deltaTime;
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
	}
	return 0;
}
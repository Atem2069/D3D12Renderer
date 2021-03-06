#define NOMINMAX
#include<iostream>
#include <cmath>
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
#include "Renderer/ShadowMapping.h"

#include <imgui.h>
#include <imgui_impl_dx12.h>
#include <imgui_impl_glfw.h>


constexpr int Width = 1600;
constexpr int Height = 900;

struct BasicCamera
{
	XMMATRIX projection;
	XMMATRIX view;
	XMVECTOR position;
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

	//Creating command lists for use
	CommandList m_commandLists[3];
	for (int i = 0; i < sizeof(m_commandLists) / sizeof(CommandList); i++)
		m_commandLists[i] = D3DContext::getCurrent()->createCommandList();

	D3DContext::getCurrent()->setCurrentCommandList(m_commandLists[1]);

	ImGui::CreateContext();
	ImGui::StyleColorsClassic();

	ResourceHeap m_imguiResourceHeap;
	if (!m_imguiResourceHeap.init(64,1,D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV))
		return -1;

	CD3DX12_CPU_DESCRIPTOR_HANDLE baseResHeapDescHandle(m_imguiResourceHeap.getHeap(0)->GetCPUDescriptorHandleForHeapStart());
	baseResHeapDescHandle.Offset(m_imguiResourceHeap.numBoundDescriptors, D3DContext::getCurrent()->getDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV));

	CD3DX12_GPU_DESCRIPTOR_HANDLE baseResHeapDescGPUHandle(m_imguiResourceHeap.getHeap(0)->GetGPUDescriptorHandleForHeapStart());;
	baseResHeapDescGPUHandle.Offset(m_imguiResourceHeap.numBoundDescriptors, D3DContext::getCurrent()->getDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV));
	ImGui_ImplGlfw_InitForVulkan(m_window, true);
	ImGui_ImplDX12_Init(D3DContext::getCurrent()->getDevice(), 2, DXGI_FORMAT_R8G8B8A8_UNORM, baseResHeapDescHandle, baseResHeapDescGPUHandle);


	//root parameters all need cleaning up. 

	D3D12_DESCRIPTOR_RANGE texRange = {};
	texRange.NumDescriptors = 1;
	texRange.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
	texRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	texRange.RegisterSpace = 0;
	texRange.BaseShaderRegister = 0;

	D3D12_DESCRIPTOR_RANGE shadowTexRange = {};
	shadowTexRange.NumDescriptors = 1;;
	shadowTexRange.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
	shadowTexRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	shadowTexRange.RegisterSpace = 0;
	shadowTexRange.BaseShaderRegister = 1;

	D3D12_ROOT_DESCRIPTOR_TABLE texTable = {};
	texTable.NumDescriptorRanges = 1;
	texTable.pDescriptorRanges = &texRange;

	D3D12_ROOT_PARAMETER m_rootParameters[5] = {};
	m_rootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	m_rootParameters[0].Descriptor = CD3DX12_ROOT_DESCRIPTOR(0, 0);
	m_rootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;

	m_rootParameters[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	m_rootParameters[1].Descriptor = CD3DX12_ROOT_DESCRIPTOR(0, 0);
	m_rootParameters[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

	m_rootParameters[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	m_rootParameters[2].DescriptorTable = texTable;
	m_rootParameters[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

	m_rootParameters[3].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	m_rootParameters[3].DescriptorTable = CD3DX12_ROOT_DESCRIPTOR_TABLE(1, &shadowTexRange);
	m_rootParameters[3].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

	m_rootParameters[4].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	m_rootParameters[4].Descriptor = CD3DX12_ROOT_DESCRIPTOR(1, 0);
	m_rootParameters[4].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;

	D3D12_STATIC_SAMPLER_DESC m_samplerDesc[2] = {};
	m_samplerDesc[0].AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	m_samplerDesc[0].AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	m_samplerDesc[0].AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	m_samplerDesc[0].Filter = D3D12_FILTER_ANISOTROPIC;
	m_samplerDesc[0].MaxAnisotropy = 16;
	m_samplerDesc[0].ShaderRegister = 0;
	m_samplerDesc[0].MinLOD = 0;
	m_samplerDesc[0].MaxLOD = D3D12_FLOAT32_MAX;
	m_samplerDesc[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

	m_samplerDesc[1].AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	m_samplerDesc[1].AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	m_samplerDesc[1].AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	m_samplerDesc[1].Filter = D3D12_FILTER_COMPARISON_MIN_MAG_MIP_LINEAR;
	m_samplerDesc[1].ComparisonFunc = D3D12_COMPARISON_FUNC_GREATER;
	m_samplerDesc[1].MinLOD = 0;
	m_samplerDesc[1].MaxLOD = 0;
	m_samplerDesc[1].MaxAnisotropy = 0;
	m_samplerDesc[1].ShaderRegister = 1;
	m_samplerDesc[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
	

	RenderPipeline m_pipeline;
	if(!m_pipeline.initWithRootParameters(R"(Shaders\basicVertex.hlsl)",R"(Shaders\basicPixel.hlsl)",m_rootParameters,5,m_samplerDesc,2))
		return -1;

	ResourceHeap m_objectsResourceHeap;
	if (!m_objectsResourceHeap.init(65535,1,D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV))
		return -1;


	Object m_object;
	if (!m_object.init(R"(Models\sponza\sponza.obj)", m_objectsResourceHeap))
		return -1;
	Object m_object2;
	if (!m_object2.init(R"(Models\nanosuit\nanosuit.obj)", m_objectsResourceHeap))
		return -1;
	BasicCamera m_basicCamera = {};
	XMVECTOR cameraPosition = XMVectorSet(0, 0, 0, 1);
	XMVECTOR cameraEye = XMVectorSet(0, 0, 1, 1);
	XMVECTOR cameraUp = XMVectorSet(0, 1, 0, 1);
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

	DirectionalShadowMap m_directionalShadowMap;
	if (!m_directionalShadowMap.init(4096, 4096, 2048, 2048, m_light.lightDirection, m_objectsResourceHeap))
		return -1;

	D3DContext::getCurrent()->submitCurrentlySetCommandList();
	double lastTime = 0, currentTime = glfwGetTime(), deltaTime = 1;
	double cpuLastTime = 0, cpuCurrentTime = glfwGetTime(), cpuDeltaTime = 0;
	double gpuLastTime = 0, gpuCurrentTime = glfwGetTime(), gpuDeltaTime = 0;
	float pitch = 0, yaw = 0;
	float cameraSpeed = 250.0f;
	XMFLOAT4 temp_imguiPosition;

	while (!glfwWindowShouldClose(m_window))
	{
		glfwPollEvents();

		gpuCurrentTime = glfwGetTime();
		gpuDeltaTime = gpuCurrentTime - gpuLastTime;
		cpuLastTime = glfwGetTime();
		ImGui_ImplDX12_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();

		ImGui::Begin("Renderer Info-Direct3D 12");
		{
			ImGui::Text("Current FPS:: %.2f", 1 / deltaTime);
			ImGui::Text("CPU Time :: %.1f ms GPU Time :: %.1f ms", cpuDeltaTime*1000, gpuDeltaTime*1000);
			ImGui::Text("Current Face Count:: %d", m_object.m_faces + m_object2.m_faces);
		}
		ImGui::End();
		
		ImGui::Begin("Renderer Properties");
		{
			XMStoreFloat4(&temp_imguiPosition, cameraPosition);
			ImGui::Text("Position: {%.1f, %.1f, %.1f}", temp_imguiPosition.x, temp_imguiPosition.y, temp_imguiPosition.z);
			ImGui::DragFloat3("Light direction", (float*)&m_light.lightDirection);
		}
		ImGui::End();

		D3DContext::getCurrent()->setCurrentCommandList(m_commandLists[0]);
		//Shadowmap pass
		m_directionalShadowMap.beginFrame(m_light.lightDirection);
		m_object.draw();
		m_object2.draw();
		m_directionalShadowMap.endFrame();
		D3DContext::getCurrent()->setCurrentCommandList(m_commandLists[1]);

		D3DContext::getCurrent()->beginRenderPass(0.564f, 0.8f, 0.976f, 1.f);

		//CPU update
		m_basicCamera.view = XMMatrixLookAtLH(cameraPosition, cameraPosition + cameraEye, cameraUp);
		m_basicCamera.position = cameraPosition;
		m_cameraBuffer.update(&m_basicCamera, sizeof(BasicCamera));
		m_lightBuffer.update(&m_light, sizeof(LightBuffer));
	
		ID3D12DescriptorHeap* objResHeaps[1] = { m_objectsResourceHeap.getHeap(0) };
		D3DContext::getCurrent()->bindAllResourceHeaps(objResHeaps, 1);

		m_pipeline.bind();
		m_cameraBuffer.bind(0);
		m_lightBuffer.bind(1);
		m_directionalShadowMap.bindCamera(4);
		m_directionalShadowMap.bindTexture(3, m_objectsResourceHeap);
		m_object.draw(m_objectsResourceHeap);
		m_object2.draw(m_objectsResourceHeap);

		ID3D12DescriptorHeap* baseResHeaps[1] = { m_imguiResourceHeap.getHeap(0) };
		D3DContext::getCurrent()->bindAllResourceHeaps(baseResHeaps, 1);
		ImGui::Render();
		ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), D3DContext::getCurrent()->getCurrentCommandList().m_commandList);

		D3DContext::getCurrent()->endRenderPass();

		if (glfwGetKey(m_window, GLFW_KEY_W) == GLFW_PRESS)
			cameraPosition += cameraEye * cameraSpeed * deltaTime;
		if (glfwGetKey(m_window, GLFW_KEY_S) == GLFW_PRESS)
			cameraPosition -= cameraEye * cameraSpeed * deltaTime;
		if (glfwGetKey(m_window, GLFW_KEY_A) == GLFW_PRESS)
			cameraPosition += XMVector3Normalize(XMVector3Cross(cameraEye, cameraUp)) * cameraSpeed * deltaTime;
		if (glfwGetKey(m_window, GLFW_KEY_D) == GLFW_PRESS)
			cameraPosition -= XMVector3Normalize(XMVector3Cross(cameraEye, cameraUp)) * cameraSpeed * deltaTime;
		if (glfwGetKey(m_window, GLFW_KEY_SPACE))
			cameraPosition += cameraUp * cameraSpeed * deltaTime;
		if (glfwGetKey(m_window, GLFW_KEY_LEFT_SHIFT))
			cameraPosition -= cameraUp * cameraSpeed * deltaTime;
		if (glfwGetKey(m_window, GLFW_KEY_LEFT) == GLFW_PRESS)
			yaw += 0.5f * deltaTime * 250.0f;
		if (glfwGetKey(m_window, GLFW_KEY_RIGHT) == GLFW_PRESS)
			yaw -= 0.5f * deltaTime * 250.0f;
		if (glfwGetKey(m_window, GLFW_KEY_UP) == GLFW_PRESS)
			pitch += 0.5f * deltaTime * 250.0f;
		if (glfwGetKey(m_window, GLFW_KEY_DOWN) == GLFW_PRESS)
			pitch -= 0.5f * deltaTime * 250.0f;

		cameraEye = XMVectorSet(XMScalarCos(XMConvertToRadians(pitch)) * XMScalarCos(XMConvertToRadians(yaw)), XMScalarSin(XMConvertToRadians(pitch)), XMScalarCos(XMConvertToRadians(pitch)) * XMScalarSin(XMConvertToRadians(yaw)), 1);
		XMVector3Normalize(cameraEye);

		cpuCurrentTime = glfwGetTime();
		cpuDeltaTime = cpuCurrentTime - cpuLastTime;

		gpuLastTime = glfwGetTime();
		//if (!D3DContext::getCurrent()->executeAndPresent(false))
		//	glfwSetWindowShouldClose(m_window, GLFW_TRUE);
		D3DContext::getCurrent()->presentWithCommandLists(m_commandLists, 2);
		//Calculate deltatime
		deltaTime = currentTime - lastTime;
		lastTime = currentTime;
		currentTime = glfwGetTime();
	}
	return 0;
}
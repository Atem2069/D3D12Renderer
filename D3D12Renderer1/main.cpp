#include<iostream>
#define NOMINMAX
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>

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

	while (!glfwWindowShouldClose(m_window))
	{
		glfwPollEvents();
	}
	return 0;
}
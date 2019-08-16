#include <glm/gtc/matrix_transform.hpp>

#include <engine/model.h>
#include <engine/common.h>
#include <engine/strings.h>
#include <engine/utils.h>
#include <engine/framebuffer.h>

#include <array>
#include <time.h>
#include <iostream>

struct PointLight
{
	glm::vec3 _color;
	float _radius;
	glm::vec3 _position;
	unsigned int : 32;
};

void framebufferSizeCallback(GLFWwindow*, int, int);
void cursorPosCallback(GLFWwindow*, double, double);
void scrollCallback(GLFWwindow*, double, double);

void initPointers();
void deletePointers();
void copyLightDataToGPU();
void genOutputTexture();

phoenix::Camera* camera;
phoenix::Utils* utils;
phoenix::Framebuffer* gBuffer;
GLFWwindow* window;

float lastX = static_cast<float>(phoenix::SCREEN_WIDTH) / 2.0f;
float lastY = static_cast<float>(phoenix::SCREEN_HEIGHT) / 2.0f;

bool calibratedCursor = false;

std::array<PointLight*, 4096> pointLights;
unsigned int lightsBuffer, output;

int main()
{
	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	window = glfwCreateWindow(phoenix::SCREEN_WIDTH, phoenix::SCREEN_HEIGHT, "Tiled Deferred Shading", nullptr, nullptr);
	if (!window)
	{
		std::cerr << phoenix::GLFW_CREATE_WINDOW_ERROR;
		glfwTerminate();
		return -1;
	}
	glfwMakeContextCurrent(window);
	glfwSetFramebufferSizeCallback(window, framebufferSizeCallback);
	glfwSetCursorPosCallback(window, cursorPosCallback);
	glfwSetScrollCallback(window, scrollCallback);
	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
	{
		std::cerr << phoenix::GLAD_LOAD_GL_LOADER_ERROR;
		return -1;
	}

	glEnable(GL_DEPTH_TEST);

	initPointers();

	copyLightDataToGPU();

	phoenix::Shader gBufferPassShader("../Resources/Shaders/tiled_deferred_shading/g_buffer_pass.vs", "../Resources/Shaders/tiled_deferred_shading/g_buffer_pass.fs");
	phoenix::Shader cullLightsShader("../Resources/Shaders/tiled_deferred_shading/cull_lights.comp");
	cullLightsShader.use();
	cullLightsShader.setInt(phoenix::G_NORMAL_MAP, 0);
	cullLightsShader.setInt(phoenix::G_ALBEDO_SPECULAR_MAP, 1);
	cullLightsShader.setInt(phoenix::G_OUTPUT, 2);
	phoenix::Shader renderPassShader("../Resources/Shaders/tiled_deferred_shading/render_pass.vs", "../Resources/Shaders/tiled_deferred_shading/render_pass.fs");
	renderPassShader.use();
	renderPassShader.setInt(phoenix::G_OUTPUT, 0);

	phoenix::Model sponza("../Resources/Objects/sponza/sponza.obj");

	glBindFramebuffer(GL_FRAMEBUFFER, gBuffer->_FBO);
	unsigned int albedoSpecularMap = gBuffer->genAttachment(GL_RGBA32F, GL_RGBA, GL_FLOAT);
	GLenum bufs[2] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1 };
	glDrawBuffers(2, bufs);

	genOutputTexture();

	while (!glfwWindowShouldClose(window))
	{
		utils->_projection = glm::perspective(glm::radians(camera->_FOV), static_cast<float>(phoenix::SCREEN_WIDTH) / phoenix::SCREEN_HEIGHT, phoenix::PERSPECTIVE_NEAR_PLANE, phoenix::PERSPECTIVE_FAR_PLANE);
		utils->_view = camera->getViewMatrix();

		float currentFrame = glfwGetTime();
		utils->_deltaTime = currentFrame - utils->_lastTimestamp;
		utils->_lastTimestamp = currentFrame;

		utils->processInput(window, camera);

		glClearColor(0.1f, 0.1f, 0.1f, 1.0f);

		glBindFramebuffer(GL_FRAMEBUFFER, gBuffer->_FBO);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		gBufferPassShader.use();
		glm::mat4 world = glm::mat4(1.0f);
		world = glm::scale(world, glm::vec3(0.02f));
		gBufferPassShader.setMat4(phoenix::G_WVP, utils->_projection * utils->_view * world);
		gBufferPassShader.setMat3(phoenix::G_NORMAL_MATRIX, glm::transpose(glm::inverse(glm::mat3(world))));
		sponza.render(gBufferPassShader);

		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		cullLightsShader.use();
		cullLightsShader.setMat4(phoenix::G_PROJECTION_MATRIX, utils->_projection);
		cullLightsShader.setMat4(phoenix::G_VIEW_MATRIX, utils->_view);
		cullLightsShader.setMat4("gInverseVP", glm::inverse(utils->_projection * utils->_view));
		cullLightsShader.setVec3(phoenix::G_VIEW_POS, camera->_position);
		glBindImageTexture(0, gBuffer->_FBO, 0, GL_FALSE, 0, GL_READ_ONLY, GL_RGBA32F);
		glBindImageTexture(1, albedoSpecularMap, 0, GL_FALSE, 0, GL_READ_ONLY, GL_RGBA32F);
		glBindImageTexture(2, output, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, lightsBuffer);
		glDispatchCompute(160, 90, 1);

		renderPassShader.use();
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, output);
		utils->renderQuad(renderPassShader);

		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	deletePointers();
	glfwTerminate();
	return 0;
}

void framebufferSizeCallback(GLFWwindow* window, int width, int height)
{
	glViewport(0, 0, width, height);
}

void cursorPosCallback(GLFWwindow* window, double x, double y)
{
	if (!calibratedCursor)
	{
		lastX = x;
		lastY = y;
		calibratedCursor = true;
	}

	float xOffset = x - lastX;
	float yOffset = lastY - y; // Reversed for y-coordinates

	lastX = x;
	lastY = y;

	camera->processMouseMovement(xOffset, yOffset);
}

void scrollCallback(GLFWwindow* window, double xOffset, double yOffset)
{
	camera->processMouseScroll(yOffset);
}

void initPointers()
{
	srand(time(nullptr));
	for (size_t i = 0; i < pointLights.size(); ++i)
	{
		pointLights[i] = new PointLight();
		float r = (rand() % 100) / 200.0f + 0.5f;
		float g = (rand() % 100) / 200.0f + 0.5f;
		float b = (rand() % 100) / 200.0f + 0.5f;
		pointLights[i]->_color = glm::vec3(r, g, b);
		pointLights[i]->_radius = 3.0f;
		float x = (rand() % 100) / 100.0f * 82.0f - 43.0f;
		float y = (rand() % 100) / 100.0f * 32.0f - 3.0f;
		float z = (rand() % 100) / 100.0f * 52.0f - 26.0f;
		pointLights[i]->_position = glm::vec3(x, y, z);
	}
	gBuffer = new phoenix::Framebuffer(phoenix::SCREEN_WIDTH, phoenix::SCREEN_HEIGHT, false, true);
	utils = new phoenix::Utils();
	camera = new phoenix::Camera();
}

void deletePointers()
{
	if (camera)
	{
		delete camera;
	}
	if (utils)
	{
		delete utils;
	}
	if (gBuffer)
	{
		delete gBuffer;
	}
	for (size_t i = 0; i < pointLights.size(); ++i)
	{
		if (pointLights[i])
		{
			delete pointLights[i];
		}
	}
}

void copyLightDataToGPU()
{
	glGenBuffers(1, &lightsBuffer);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, lightsBuffer);
	glBufferData(GL_SHADER_STORAGE_BUFFER, pointLights.size() * sizeof(PointLight), nullptr, GL_STATIC_DRAW);

	glBindBuffer(GL_ARRAY_BUFFER, lightsBuffer);
	PointLight* ptr = reinterpret_cast<PointLight*>(glMapBufferRange(GL_ARRAY_BUFFER, 0, pointLights.size() * sizeof(PointLight), GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT));
	for (int i = 0; i < pointLights.size(); ++i)
	{
		ptr[i]._color = pointLights[i]->_color;
		ptr[i]._radius = pointLights[i]->_radius;
		ptr[i]._position = pointLights[i]->_position;
	}
	glUnmapBuffer(GL_ARRAY_BUFFER);
}

void genOutputTexture()
{
	glGenTextures(1, &output);
	glBindTexture(GL_TEXTURE_2D, output);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, phoenix::SCREEN_WIDTH, phoenix::SCREEN_HEIGHT, 0, GL_RGBA, GL_FLOAT, nullptr);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
}
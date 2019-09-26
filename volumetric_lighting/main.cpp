#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/gtc/matrix_transform.hpp>

#include <engine/camera.h>
#include <engine/model.h>
#include <engine/common.h>
#include <engine/strings.h>
#include <engine/utils.h>
#include <engine/framebuffer.h>
#include <engine/light.h>

#include <array>
#include <time.h>
#include <iostream>

void framebufferSizeCallback(GLFWwindow*, int, int);
void cursorPosCallback(GLFWwindow*, double, double);
void scrollCallback(GLFWwindow*, double, double);

void initPointers();
void deletePointers();

phoenix::Camera* camera;
phoenix::Utils* utils;
phoenix::Framebuffer* gBuffer;
GLFWwindow* window;

float lastX = static_cast<float>(phoenix::SCREEN_WIDTH) / 2.0f;
float lastY = static_cast<float>(phoenix::SCREEN_HEIGHT) / 2.0f;

bool calibratedCursor = false;

phoenix::DirectLight directLight;

int main()
{
	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	window = glfwCreateWindow(phoenix::SCREEN_WIDTH, phoenix::SCREEN_HEIGHT, "Volumetric Lighting", nullptr, nullptr);
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

	phoenix::Shader gBufferPassShader("../Resources/Shaders/god_rays/g_buffer_pass.vs", "../Resources/Shaders/god_rays/g_buffer_pass.fs");
	phoenix::Shader renderPassShader("../Resources/Shaders/god_rays/render_pass.vs", "../Resources/Shaders/god_rays/render_pass.fs");
	renderPassShader.use();
	renderPassShader.setInt(phoenix::G_POSITION_MAP, 0);
	renderPassShader.setInt(phoenix::G_NORMAL_MAP, 1);
	renderPassShader.setInt(phoenix::G_ALBEDO_SPECULAR_MAP, 2);

	phoenix::Model sponza("../Resources/Objects/sponza/sponza.obj");

	glBindFramebuffer(GL_FRAMEBUFFER, gBuffer->_FBO);
	unsigned int normalMap = gBuffer->genAttachment(GL_RGB16F, GL_RGB, GL_FLOAT);
	unsigned int albedoSpecularMap = gBuffer->genAttachment(GL_RGBA, GL_RGBA, GL_UNSIGNED_BYTE);
	GLenum bufs[3] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2 };
	glDrawBuffers(3, bufs);

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
		gBufferPassShader.setMat4(phoenix::G_VP, utils->_projection * utils->_view);
		gBufferPassShader.setMat4(phoenix::G_VIEW_MATRIX, utils->_view);
		glm::mat4 world = glm::mat4(1.0f);
		world = glm::scale(world, glm::vec3(0.02f));
		gBufferPassShader.setMat4(phoenix::G_WORLD_MATRIX, world);
		gBufferPassShader.setMat3(phoenix::G_NORMAL_MATRIX, glm::transpose(glm::inverse(glm::mat3(world))));
		sponza.render(gBufferPassShader);

		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		renderPassShader.use();
		renderPassShader.setVec3(phoenix::G_VIEW_POS, camera->_position);
		renderPassShader.setMat4(phoenix::G_INVERSE_VIEW_MATRIX, glm::inverse(utils->_view));
		directLight.setUniforms(renderPassShader);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, gBuffer->_textureID);
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, normalMap);
		glActiveTexture(GL_TEXTURE2);
		glBindTexture(GL_TEXTURE_2D, albedoSpecularMap);
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
	gBuffer = new phoenix::Framebuffer(phoenix::SCREEN_WIDTH, phoenix::SCREEN_HEIGHT);
	utils = new phoenix::Utils();
	camera = new phoenix::Camera();
}

void deletePointers()
{
	delete camera;
	delete utils;
	delete gBuffer;
}
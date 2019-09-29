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

// Light attenuation factors
const float LINEAR = 0.7f;
const float QUADRATIC = 1.8f;

phoenix::Camera* camera;
phoenix::Utils* utils;
phoenix::Framebuffer* gBuffer;
GLFWwindow* window;

float lastX = static_cast<float>(phoenix::SCREEN_WIDTH) / 2.0f;
float lastY = static_cast<float>(phoenix::SCREEN_HEIGHT) / 2.0f;

bool calibratedCursor = false;

std::array<phoenix::PointLight, 32> pointLights;

int main()
{
	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	window = glfwCreateWindow(phoenix::SCREEN_WIDTH, phoenix::SCREEN_HEIGHT, "Screen Space Reflections", nullptr, nullptr);
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

	srand(time(nullptr));
	for (size_t i = 0; i < pointLights.size(); ++i)
	{
		float x = (rand() % 100) / 100.0f * 6.0f - 3.0f;
		float y = (rand() % 100) / 100.0f * 6.0f - 4.0f;
		float z = (rand() % 100) / 100.0f * 6.0f - 3.0f;
		pointLights[i]._position = glm::vec3(x, y, z);
		float r = (rand() % 100) / 200.0f + 0.5f;
		float g = (rand() % 100) / 200.0f + 0.5f;
		float b = (rand() % 100) / 200.0f + 0.5f;
		pointLights[i]._color = glm::vec3(r, g, b);
		pointLights[i]._attenuation._linear = LINEAR;
		pointLights[i]._attenuation._quadratic = QUADRATIC;
	}

	unsigned int floorDiffuseTexture = phoenix::Utils::loadTexture("../Resources/Textures/ssr/1_col.png");
	unsigned int floorSpecularTexture = phoenix::Utils::loadTexture("../Resources/Textures/ssr/1_spec.png");

	phoenix::Shader gBufferPassShader("../Resources/Shaders/screen_space_reflections/g_buffer_pass.vs", "../Resources/Shaders/screen_space_reflections/g_buffer_pass.fs");
	phoenix::Shader lightingPassShader("../Resources/Shaders/screen_space_reflections/render_quad.vs", "../Resources/Shaders/screen_space_reflections/lighting_pass.fs");
	lightingPassShader.use();
	lightingPassShader.setInt(phoenix::G_POSITION_MAP, 0);
	lightingPassShader.setInt(phoenix::G_NORMAL_MAP, 1);
	lightingPassShader.setInt(phoenix::G_ALBEDO_SPECULAR_MAP, 2);
	phoenix::Shader renderPassShader("../Resources/Shaders/screen_space_reflections/render_quad.vs", "../Resources/Shaders/screen_space_reflections/render_pass.fs");
	renderPassShader.use();
	renderPassShader.setInt(phoenix::G_POSITION_MAP, 0);
	renderPassShader.setInt(phoenix::G_NORMAL_MAP, 1);
	renderPassShader.setInt(phoenix::G_ALBEDO_SPECULAR_MAP, 2);
	renderPassShader.setInt(phoenix::G_PREVIOUS_FRAME_MAP, 3);
	renderPassShader.setInt(phoenix::G_METALLIC_MAP, 4);

	phoenix::Model sponza("../Resources/Objects/sponza/sponza.obj");

	glBindFramebuffer(GL_FRAMEBUFFER, gBuffer->_FBO);
	unsigned int normalMap = gBuffer->genAttachment(GL_RGB16F, GL_RGB, GL_FLOAT);
	unsigned int albedoSpecularMap = gBuffer->genAttachment(GL_RGBA, GL_RGBA, GL_UNSIGNED_BYTE);
	unsigned int previousFrameMap = gBuffer->genAttachment(GL_RGBA, GL_RGBA, GL_UNSIGNED_BYTE);
	unsigned int metallicMap = gBuffer->genAttachment(GL_RED, GL_RED, GL_UNSIGNED_BYTE);
	GLenum bufs[5] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2, GL_COLOR_ATTACHMENT3, GL_COLOR_ATTACHMENT4 };
	glDrawBuffers(5, bufs);

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
		gBufferPassShader.setFloat(phoenix::G_METALNESS, 0.0f);
		gBufferPassShader.setMat4(phoenix::G_VP, utils->_projection * utils->_view);
		gBufferPassShader.setMat4(phoenix::G_VIEW_MATRIX, utils->_view);
		glm::mat4 world = glm::mat4(1.0f);
		world = glm::scale(world, glm::vec3(phoenix::OBJECT_SCALE));
		gBufferPassShader.setMat4(phoenix::G_WORLD_MATRIX, world);
		gBufferPassShader.setMat3(phoenix::G_NORMAL_MATRIX, glm::transpose(glm::inverse(glm::mat3(world))));
		sponza.render(gBufferPassShader);
		gBufferPassShader.setFloat(phoenix::G_METALNESS, 1.0f);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, floorDiffuseTexture);
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, floorSpecularTexture);
		utils->renderPlane(gBufferPassShader, glm::vec3(0.0f, 0.5f, 0.0f));

		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		glBindFramebuffer(GL_FRAMEBUFFER, gBuffer->_FBO);
		lightingPassShader.use();
		lightingPassShader.setVec3(phoenix::G_VIEW_POS, camera->_position);
		lightingPassShader.setMat4(phoenix::G_INVERSE_VIEW_MATRIX, glm::inverse(utils->_view));
		for (size_t i = 0; i < pointLights.size(); ++i)
		{
			pointLights[i].setUniforms(lightingPassShader, i);
		}
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, gBuffer->_textureID);
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, normalMap);
		glActiveTexture(GL_TEXTURE2);
		glBindTexture(GL_TEXTURE_2D, albedoSpecularMap);
		utils->renderQuad(lightingPassShader);

		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		renderPassShader.use();
		renderPassShader.setMat4(phoenix::G_INVERSE_VIEW_MATRIX, glm::inverse(utils->_view));
		renderPassShader.setMat4(phoenix::G_PROJECTION_MATRIX, utils->_projection);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, gBuffer->_textureID);
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, normalMap);
		glActiveTexture(GL_TEXTURE2);
		glBindTexture(GL_TEXTURE_2D, albedoSpecularMap);
		glActiveTexture(GL_TEXTURE3);
		glBindTexture(GL_TEXTURE_2D, previousFrameMap);
		glActiveTexture(GL_TEXTURE4);
		glBindTexture(GL_TEXTURE_2D, metallicMap);
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
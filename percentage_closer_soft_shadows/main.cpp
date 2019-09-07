#include <glm/gtc/matrix_transform.hpp>

#include <engine/shadow_common.h>
#include <engine/strings.h>
#include <engine/common.h>
#include <engine/framebuffer.h>

#include <array>
#include <time.h>
#include <iostream>

void framebufferSizeCallback(GLFWwindow*, int, int);
void cursorPosCallback(GLFWwindow*, double, double);
void scrollCallback(GLFWwindow*, double, double);

void generateRandom3DTexture();
void execShadowMapPass(const phoenix::Shader&, phoenix::Model&);
void execRenderPass(const phoenix::Shader&, phoenix::Model&);
void initPointers();
void deletePointers();

phoenix::Camera* camera;
phoenix::Utils* utils;
phoenix::Framebuffer* renderTargets;
phoenix::ShadowCommon* shadowCommon;

float lastX = static_cast<float>(phoenix::SCREEN_WIDTH) / 2.0f;
float lastY = static_cast<float>(phoenix::SCREEN_HEIGHT) / 2.0f;

bool calibratedCursor = false;

// References to various textures
unsigned int anglesTexture, normalMap, fluxMap, shadowMap;

int main()
{
	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	GLFWwindow* window = glfwCreateWindow(phoenix::SCREEN_WIDTH, phoenix::SCREEN_HEIGHT, "Percentage-Closer Soft Shadows", nullptr, nullptr);
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

	shadowCommon->_floorTexture = phoenix::Utils::loadTexture("../Resources/Textures/shadow_mapping/wood.png");
	shadowCommon->_objectTexture = phoenix::Utils::loadTexture("../Resources/Objects/dragon/albedo.png");
	shadowCommon->_altObjTexture = phoenix::Utils::loadTexture("../Resources/Objects/dragon/blue.png");
	// Generate volume textures and fill them with the cosines and sines of random rotation angles for PCSS
	generateRandom3DTexture();

	phoenix::Shader renderPassShader("../Resources/Shaders/shadow_mapping/render_pass.vs", "../Resources/Shaders/shadow_mapping/render_pass.fs");
	renderPassShader.use();
	renderPassShader.setInt(phoenix::G_DIFFUSE_TEXTURE, 0);
	renderPassShader.setInt(phoenix::G_POSITION_MAP, 1);
	renderPassShader.setInt(phoenix::G_NORMAL_MAP, 2);
	renderPassShader.setInt(phoenix::G_FLUX_MAP, 3);
	renderPassShader.setInt(phoenix::G_SHADOW_MAP, 4);
	renderPassShader.setInt(phoenix::G_ANGLES_TEXTURE, 5);
	renderPassShader.setFloat(phoenix::G_AMBIENT_FACTOR, phoenix::AMBIENT_FACTOR);
	renderPassShader.setFloat(phoenix::G_SPECULAR_FACTOR, phoenix::SPECULAR_FACTOR);
	renderPassShader.setFloat(phoenix::G_CALIBRATED_LIGHT_SIZE, phoenix::CALIBRATED_LIGHT_SIZE);
	renderPassShader.setVec3(phoenix::G_LIGHT_COLOR, phoenix::LIGHT_COLOR);
	phoenix::Shader shadowMapPassShader("../Resources/Shaders/shadow_mapping/shadow_map_pass.vs", "../Resources/Shaders/shadow_mapping/shadow_map_pass.fs");
	shadowMapPassShader.setInt(phoenix::G_DIFFUSE_TEXTURE, 0);
	phoenix::Shader renderQuadShader("../Resources/Shaders/shadow_mapping/render_quad.vs", "../Resources/Shaders/shadow_mapping/render_quad.fs");
	renderQuadShader.use();
	renderQuadShader.setInt(phoenix::G_RENDER_TARGET, 0);
	phoenix::Shader debugLinesShader("../Resources/Shaders/shadow_mapping/debug_lines.vs", "../Resources/Shaders/shadow_mapping/debug_lines.fs");

	phoenix::Model dragon("../Resources/Objects/dragon/dragon.obj");

	glBindFramebuffer(GL_FRAMEBUFFER, renderTargets->_FBO);
	normalMap = renderTargets->genAttachment(GL_RGB16F, GL_RGB, GL_FLOAT, true);
	fluxMap = renderTargets->genAttachment(GL_RGB16F, GL_RGB, GL_FLOAT, true);
	shadowMap = renderTargets->genAttachment(GL_R16F, GL_RED, GL_FLOAT, true);
	GLenum bufs[4] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2, GL_COLOR_ATTACHMENT3 };
	glDrawBuffers(4, bufs);

	while (!glfwWindowShouldClose(window))
	{
		utils->_projection = glm::perspective(glm::radians(camera->_FOV), static_cast<float>(phoenix::SCREEN_WIDTH) / phoenix::SCREEN_HEIGHT, phoenix::PERSPECTIVE_NEAR_PLANE, phoenix::PERSPECTIVE_FAR_PLANE);
		utils->_view = camera->getViewMatrix();

		float currentTime = glfwGetTime();
		shadowCommon->_deltaTime = currentTime - shadowCommon->_lastTimestamp;
		shadowCommon->_lastTimestamp = currentTime;

		shadowCommon->processInput(window, camera, true);

		glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		// Shadow map pass
		execShadowMapPass(shadowMapPassShader, dragon);

		glViewport(0, 0, phoenix::SCREEN_WIDTH, phoenix::SCREEN_HEIGHT);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		// Render pass
		if (shadowCommon->_renderMode == 1)
		{
			utils->renderQuad(renderQuadShader, renderTargets->_textureID);
		}
		else if (shadowCommon->_renderMode == 2)
		{
			utils->renderQuad(renderQuadShader, normalMap);
		}
		else if (shadowCommon->_renderMode == 3)
		{
			utils->renderQuad(renderQuadShader, fluxMap);
		}
		else if (shadowCommon->_renderMode == 4)
		{
			utils->renderQuad(renderQuadShader, shadowMap);
		}
		else
		{
			execRenderPass(renderPassShader, dragon);
			shadowCommon->renderDebugLines(debugLinesShader, utils);
		}

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

void generateRandom3DTexture()
{
	std::array<std::array<std::array<glm::vec2, 32>, 32>, 32> randomAngles;
	const int RESOLUTION = 32;
	srand(time(nullptr));
	for (size_t i = 0; i < RESOLUTION; ++i)
	{
		for (size_t j = 0; j < RESOLUTION; ++j)
		{
			for (size_t k = 0; k < RESOLUTION; ++k)
			{
				float randomAngle = static_cast<float>(rand()) / RAND_MAX * 2 * glm::pi<float>();
				randomAngles[i][j][k] = glm::vec2(glm::cos(randomAngle) * 0.5f + 0.5f, glm::sin(randomAngle) * 0.5f + 0.5f);
			}
		}
	}

	glGenTextures(1, &anglesTexture);
	glBindTexture(GL_TEXTURE_3D, anglesTexture);
	glTexImage3D(GL_TEXTURE_3D, 0, GL_RG16F, RESOLUTION, RESOLUTION, RESOLUTION, 0, GL_RG, GL_FLOAT, &randomAngles);

	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
}

void execShadowMapPass(const phoenix::Shader& shader, phoenix::Model& object)
{
	shadowCommon->setLightSpaceVP(shader, shadowCommon->_lightPos, phoenix::TARGET);
	glViewport(0, 0, phoenix::SHADOW_MAP_WIDTH, phoenix::SHADOW_MAP_HEIGHT);
	glBindFramebuffer(GL_FRAMEBUFFER, renderTargets->_FBO);

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	shadowCommon->renderScene(utils, shader, object);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void execRenderPass(const phoenix::Shader& shader, phoenix::Model& object)
{
	shadowCommon->setUniforms(shader, camera);
	shadowCommon->setLightSpaceVP(shader, shadowCommon->_lightPos, phoenix::TARGET);

	shadowCommon->changeColorTexture(shadowCommon->_floorTexture);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, renderTargets->_textureID);
	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, normalMap);
	glActiveTexture(GL_TEXTURE3);
	glBindTexture(GL_TEXTURE_2D, fluxMap);
	glActiveTexture(GL_TEXTURE4);
	glBindTexture(GL_TEXTURE_2D, shadowMap);
	glActiveTexture(GL_TEXTURE5);
	glBindTexture(GL_TEXTURE_3D, anglesTexture);

	shadowCommon->renderScene(utils, shader, object);
}

void initPointers()
{
	shadowCommon = new phoenix::ShadowCommon();
	renderTargets = new phoenix::Framebuffer(phoenix::SHADOW_MAP_WIDTH, phoenix::SHADOW_MAP_HEIGHT, true);
	utils = new phoenix::Utils();
	camera = new phoenix::Camera();
}

void deletePointers()
{
	delete camera;
	delete utils;
	delete renderTargets;
	delete shadowCommon;
}
#include <glm/gtc/matrix_transform.hpp>

#include <engine/shadow_common.h>
#include <engine/strings.h>
#include <engine/common.h>

#include <array>
#include <iostream>

void framebufferSizeCallback(GLFWwindow*, int, int);
void cursorPosCallback(GLFWwindow*, double, double);
void scrollCallback(GLFWwindow*, double, double);

void setupRenderToTexture();
void execShadowMapPass(const phoenix::Shader&, const phoenix::Shader&, phoenix::Model&);
void execRenderPass(const phoenix::Shader&, phoenix::Model&);
void initPointers();
void deletePointers();

const float TEXTURE_SIZE = phoenix::SHADOW_MAP_WIDTH;
const float SAMPLE_RADIUS = 3.0f;
const float CORRECTION_FACTOR = 0.0f;

phoenix::Camera* camera;
phoenix::Utils* utils;
phoenix::ShadowCommon* shadowCommon;

float lastX = static_cast<float>(phoenix::SCREEN_WIDTH) / 2.0f;
float lastY = static_cast<float>(phoenix::SCREEN_HEIGHT) / 2.0f;

bool calibratedCursor = false;

unsigned int FBO;
std::array<unsigned int, 2> shadowMaps;

int main()
{
	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	GLFWwindow* window = glfwCreateWindow(phoenix::SCREEN_WIDTH, phoenix::SCREEN_HEIGHT, "Variance Shadow Mapping", nullptr, nullptr);
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
	setupRenderToTexture();

	phoenix::Shader renderPassShader("../Resources/Shaders/shadow_mapping/render_pass.vs", "../Resources/Shaders/shadow_mapping/vsm_render_pass.fs");
	renderPassShader.use();
	renderPassShader.setInt(phoenix::G_DIFFUSE_TEXTURE, 0);
	renderPassShader.setInt(phoenix::G_SHADOW_MAP, 1);
	renderPassShader.setVec3(phoenix::G_LIGHT_COLOR, phoenix::LIGHT_COLOR);
	renderPassShader.setFloat(phoenix::G_AMBIENT_FACTOR, phoenix::AMBIENT_FACTOR);
	renderPassShader.setFloat(phoenix::G_SPECULAR_FACTOR, phoenix::SPECULAR_FACTOR);
	renderPassShader.setFloat(phoenix::G_CORRECTION_FACTOR, CORRECTION_FACTOR);
	phoenix::Shader shadowMapPassShader("../Resources/Shaders/shadow_mapping/vsm_shadow_map_pass.vs", "../Resources/Shaders/shadow_mapping/vsm_shadow_map_pass.fs");
	phoenix::Shader renderQuadShader("../Resources/Shaders/shadow_mapping/render_quad.vs", "../Resources/Shaders/shadow_mapping/z_buffer.fs");
	renderQuadShader.use();
	renderQuadShader.setInt(phoenix::G_DEPTH_MAP, 0);
	phoenix::Shader blurShader("../Resources/Shaders/shadow_mapping/render_quad.vs", "../Resources/Shaders/shadow_mapping/blur.fs");
	blurShader.use();
	blurShader.setInt(phoenix::G_DEPTH_MAP, 0);
	blurShader.setFloat("gResolution", TEXTURE_SIZE);
	blurShader.setFloat("gRadius", SAMPLE_RADIUS);
	phoenix::Shader debugLinesShader("../Resources/Shaders/shadow_mapping/debug_lines.vs", "../Resources/Shaders/shadow_mapping/debug_lines.fs");

	phoenix::Model dragon("../Resources/Objects/dragon/dragon.obj");

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
		execShadowMapPass(shadowMapPassShader, blurShader, dragon);

		glViewport(0, 0, phoenix::SCREEN_WIDTH, phoenix::SCREEN_HEIGHT);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		// Render pass
		if (!shadowCommon->_renderMode || shadowCommon->_renderMode > shadowMaps.size())
		{
			execRenderPass(renderPassShader, dragon);
			shadowCommon->renderDebugLines(debugLinesShader, utils);
		}
		else
		{
			utils->renderQuad(renderQuadShader, shadowMaps[shadowCommon->_renderMode - 1]);
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

void setupRenderToTexture()
{
	glGenTextures(shadowMaps.size(), &shadowMaps[0]);
	for (size_t i = 0; i < shadowMaps.size(); ++i)
	{
		glBindTexture(GL_TEXTURE_2D, shadowMaps[i]);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, phoenix::SHADOW_MAP_WIDTH, phoenix::SHADOW_MAP_HEIGHT, 0, GL_RGBA, GL_FLOAT, nullptr);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
		glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, phoenix::BORDER_COLOR);
	}

	// Attach the render target to the FBO so we can write to it
	glGenFramebuffers(1, &FBO);
	glBindFramebuffer(GL_FRAMEBUFFER, FBO);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, shadowMaps[0], 0);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, shadowMaps[1], 0);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void execShadowMapPass(const phoenix::Shader& shadowMapPassShader, const phoenix::Shader& blurShader, phoenix::Model& object)
{
	shadowCommon->setLightSpaceVP(shadowMapPassShader, shadowCommon->_lightPos, phoenix::TARGET);
	glViewport(0, 0, phoenix::SHADOW_MAP_WIDTH, phoenix::SHADOW_MAP_HEIGHT);
	glBindFramebuffer(GL_FRAMEBUFFER, FBO);

	glDrawBuffer(GL_COLOR_ATTACHMENT0);
	shadowCommon->renderScene(utils, shadowMapPassShader, object);

	blurShader.use();

	glDrawBuffer(GL_COLOR_ATTACHMENT1);
	blurShader.setVec2(phoenix::G_DIR, glm::vec2(1.0f, 0.0f));
	utils->renderQuad(blurShader, shadowMaps[0]);

	glDrawBuffer(GL_COLOR_ATTACHMENT0);
	blurShader.setVec2(phoenix::G_DIR, glm::vec2(0.0f, 1.0f));
	utils->renderQuad(blurShader, shadowMaps[1]);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void execRenderPass(const phoenix::Shader& shader, phoenix::Model& object)
{
	shadowCommon->setUniforms(shader, camera);
	shadowCommon->setLightSpaceVP(shader, shadowCommon->_lightPos, phoenix::TARGET);

	shadowCommon->changeColorTexture(shadowCommon->_floorTexture);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, shadowMaps[0]);

	shadowCommon->renderScene(utils, shader, object);
}

void initPointers()
{
	shadowCommon = new phoenix::ShadowCommon();
	utils = new phoenix::Utils();
	camera = new phoenix::Camera();
}

void deletePointers()
{
	delete camera;
	delete utils;
	delete shadowCommon;
}
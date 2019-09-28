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
void setInputs(const phoenix::Shader&, bool);
void execShadowMapPass(const phoenix::Shader&, phoenix::Model&);
void execRenderPass(const phoenix::Shader&, const phoenix::Shader&, phoenix::Model&);
void initPointers();
void deletePointers();

// Parameters for our hair model
const glm::vec3 TRANSLATION = glm::vec3(0.0f, -4.5f, 0.0f), SCALE = glm::vec3(0.1f);
const float ROTATION = -90.0f;

phoenix::Camera* camera;
phoenix::Utils* utils;
phoenix::Framebuffer* renderTargets;
phoenix::ShadowCommon* shadowCommon;
GLFWwindow* window;

float lastX = static_cast<float>(phoenix::SCREEN_WIDTH) / 2.0f;
float lastY = static_cast<float>(phoenix::SCREEN_HEIGHT) / 2.0f;

bool calibratedCursor = false;

unsigned int anglesTexture, normalMap, ambientOcclusionMap, specularMap, alphaTexture, shiftTexture, noiseTexture;

int main()
{
	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	window = glfwCreateWindow(phoenix::SCREEN_WIDTH, phoenix::SCREEN_HEIGHT, "Hair Rendering", nullptr, nullptr);
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
	shadowCommon->_objectTexture = phoenix::Utils::loadTexture("../Resources/Objects/hair/hair_diff_black.jpg");
	normalMap = phoenix::Utils::loadTexture("../Resources/Objects/hair/hair_normal.png");
	ambientOcclusionMap = phoenix::Utils::loadTexture("../Resources/Objects/hair/hair_ao.png");
	specularMap = phoenix::Utils::loadTexture("../Resources/Objects/hair/hair_spec.jpg");
	alphaTexture = phoenix::Utils::loadTexture("../Resources/Objects/hair/hair_op.psd");
	shiftTexture = phoenix::Utils::loadTexture("../Resources/Objects/hair/hair_shift.png");
	noiseTexture = phoenix::Utils::loadTexture("../Resources/Objects/hair/GlitterTexture.png");
	// Generate volume textures and fill them with the cosines and sines of random rotation angles for PCSS
	generateRandom3DTexture();

	phoenix::Shader floorShader("../Resources/Shaders/hair/floor.vs", "../Resources/Shaders/hair/floor.fs");
	floorShader.use();
	floorShader.setInt(phoenix::G_DIFFUSE_TEXTURE, 0);
	floorShader.setInt(phoenix::G_SHADOW_MAP, 1);
	floorShader.setInt(phoenix::G_ANGLES_TEXTURE, 2);
	floorShader.setFloat(phoenix::G_AMBIENT_FACTOR, phoenix::AMBIENT_FACTOR);
	floorShader.setFloat(phoenix::G_SPECULAR_FACTOR, phoenix::SPECULAR_FACTOR);
	floorShader.setFloat(phoenix::G_CALIBRATED_LIGHT_SIZE, phoenix::CALIBRATED_LIGHT_SIZE);
	floorShader.setVec3(phoenix::G_LIGHT_COLOR, phoenix::LIGHT_COLOR);
	phoenix::Shader hairShader("../Resources/Shaders/hair/hair.vs", "../Resources/Shaders/hair/hair.fs");
	hairShader.use();
	hairShader.setInt("gBaseTexture", 0);
	hairShader.setInt(phoenix::G_NORMAL_MAP, 3);
	hairShader.setInt(phoenix::G_AO_MAP, 4);
	hairShader.setInt(phoenix::G_SPECULAR_MAP, 5);
	hairShader.setInt("gAlphaTexture", 6);
	hairShader.setInt("gShiftTexture", 7);
	hairShader.setInt("gNoiseTexture", 8);
	hairShader.setFloat(phoenix::G_AMBIENT_FACTOR, 0.8f);
	hairShader.setFloat(phoenix::G_SPECULAR_FACTOR, phoenix::SPECULAR_FACTOR);
	hairShader.setVec3(phoenix::G_LIGHT_COLOR, phoenix::LIGHT_COLOR);
	phoenix::Shader shadowMapPassShader("../Resources/Shaders/hair/shadow_map_pass.vs", "../Resources/Shaders/hair/shadow_map_pass.fs");
	phoenix::Shader renderQuadShader("../Resources/Shaders/hair/render_quad.vs", "../Resources/Shaders/hair/render_quad.fs");
	renderQuadShader.use();
	renderQuadShader.setInt(phoenix::G_RENDER_TARGET, 0);
	phoenix::Shader debugLinesShader("../Resources/Shaders/hair/debug_lines.vs", "../Resources/Shaders/hair/debug_lines.fs");

	phoenix::Model hair("../Resources/Objects/hair/hair2_black_obj.obj");

	glBindFramebuffer(GL_FRAMEBUFFER, renderTargets->_FBO);
	glDrawBuffer(GL_COLOR_ATTACHMENT0);

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
		execShadowMapPass(shadowMapPassShader, hair);

		glViewport(0, 0, phoenix::SCREEN_WIDTH, phoenix::SCREEN_HEIGHT);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		// Render pass
		if (shadowCommon->_renderMode == 1)
		{
			utils->renderQuad(renderQuadShader, renderTargets->_textureID);
		}
		else
		{
			execRenderPass(floorShader, hairShader, hair);
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

void setInputs(const phoenix::Shader& shader, bool useObjTexture)
{
	shadowCommon->setUniforms(shader, camera);
	shadowCommon->setLightSpaceVP(shader, shadowCommon->_lightPos, phoenix::TARGET);

	shadowCommon->changeColorTexture(useObjTexture ? shadowCommon->_objectTexture : shadowCommon->_floorTexture);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, renderTargets->_textureID);
	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_3D, anglesTexture);
	glActiveTexture(GL_TEXTURE3);
	glBindTexture(GL_TEXTURE_2D, normalMap);
	glActiveTexture(GL_TEXTURE4);
	glBindTexture(GL_TEXTURE_2D, ambientOcclusionMap);
	glActiveTexture(GL_TEXTURE5);
	glBindTexture(GL_TEXTURE_2D, specularMap);
	glActiveTexture(GL_TEXTURE6);
	glBindTexture(GL_TEXTURE_2D, alphaTexture);
	glActiveTexture(GL_TEXTURE7);
	glBindTexture(GL_TEXTURE_2D, shiftTexture);
	glActiveTexture(GL_TEXTURE8);
	glBindTexture(GL_TEXTURE_2D, noiseTexture);
}

void execShadowMapPass(const phoenix::Shader& shader, phoenix::Model& object)
{
	shadowCommon->setLightSpaceVP(shader, shadowCommon->_lightPos, phoenix::TARGET);
	glViewport(0, 0, phoenix::SHADOW_MAP_WIDTH, phoenix::SHADOW_MAP_HEIGHT);
	glBindFramebuffer(GL_FRAMEBUFFER, renderTargets->_FBO);

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	utils->renderPlane(shader);
	shadowCommon->renderObject(utils, shader, object, TRANSLATION, ROTATION, SCALE);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void execRenderPass(const phoenix::Shader& floorShader, const phoenix::Shader& hairShader, phoenix::Model& object)
{
	setInputs(floorShader, false);
	utils->renderPlane(floorShader);
	setInputs(hairShader, true);
	shadowCommon->renderObject(utils, hairShader, object, TRANSLATION, ROTATION, SCALE);
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
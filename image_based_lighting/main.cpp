#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/gtc/matrix_transform.hpp>

#include <engine/stb_image.h>
#include <engine/camera.h>
#include <engine/model.h>
#include <engine/utils.h>
#include <engine/strings.h>
#include <engine/common.h>

#include <array>
#include <iostream>

void framebufferSizeCallback(GLFWwindow*, int, int);
void cursorPosCallback(GLFWwindow*, double, double);
void scrollCallback(GLFWwindow*, double, double);

void processInput();
void renderSphere();
void renderCube();
void initPointers();
void deletePointers();

void set2DTexture(unsigned int*);
void set2DTextureParams();
void setZBufferMemoryAttachment(int);
void unbindFBOAndZBufferAttachment();

void setupFramebuffer();
unsigned int loadHDRTexture(char const*);
unsigned int generateCubemapTexture(int, bool);
void setInputTexture(const phoenix::Shader&, unsigned int, bool);
void renderToCubemap(const phoenix::Shader&, unsigned int, int, int);
unsigned int integrateBRDF(const phoenix::Shader&);
void resetViewportToFramebufferSize();

// Renderbuffer and BRDF LUT resolution
const int RESOLUTION = 512;
const int IRRADIANCE_MAP_RES = 32, PREFILTERED_ENV_MAP_RES = 128;
const unsigned int NUM_FACES = 6, NUM_MIP_LEVELS = 5;

// Uniform names
const std::string G_VP = "gVP";
const std::string G_ENV_MAP = "gEnvMap";

phoenix::Camera* camera;
phoenix::Utils* utils;
GLFWwindow* window;

float lastX = static_cast<float>(phoenix::SCREEN_WIDTH) / 2.0f;
float lastY = static_cast<float>(phoenix::SCREEN_HEIGHT) / 2.0f;

bool calibratedCursor = false;
bool renderBRDFIntegrationMap = false;

// Meshes
unsigned int sphereVAO = 0, cubeVAO = 0;
// Must store this for subsequent draw calls
unsigned int numIndices;
unsigned int renderMode = 0;
unsigned int FBO, RBO;

const std::array<glm::vec3, 4> LIGHT_POSITIONS{ {
	glm::vec3(-10.0f, 10.0f, 10.0f),
	glm::vec3(10.0f, 10.0f, 10.0f),
	glm::vec3(-10.0f, -10.0f, 10.0f),
	glm::vec3(10.0f, -10.0f, 10.0f),
} };
const std::array<glm::vec3, 4> LIGHT_COLORS{ {
	glm::vec3(300.0f, 300.0f, 300.0f),
	glm::vec3(300.0f, 300.0f, 300.0f),
	glm::vec3(300.0f, 300.0f, 300.0f),
	glm::vec3(300.0f, 300.0f, 300.0f)
} };
const glm::mat4 CUBEMAP_PROJ = glm::perspective(glm::radians(90.0f), 1.0f, phoenix::PERSPECTIVE_NEAR_PLANE, phoenix::PERSPECTIVE_FAR_PLANE / 10.0f);
const std::array<glm::mat4, 6> CUBEMAP_VIEWS{ {
	glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(1.0f,  0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
	glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(-1.0f, 0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
	glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f,  1.0f,  0.0f), glm::vec3(0.0f,  0.0f,  1.0f)),
	glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f,  0.0f), glm::vec3(0.0f,  0.0f, -1.0f)),
	glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f,  0.0f,  1.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
	glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f,  0.0f, -1.0f), glm::vec3(0.0f, -1.0f,  0.0f))
} };

int main()
{
	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
	glfwWindowHint(GLFW_SAMPLES, 4);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	window = glfwCreateWindow(phoenix::SCREEN_WIDTH, phoenix::SCREEN_HEIGHT, "Image-Based Lighting", nullptr, nullptr);
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
	glDepthFunc(GL_LEQUAL);
	glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);

	initPointers();

	phoenix::Shader renderShader("../Resources/Shaders/pbr/render.vs", "../Resources/Shaders/pbr/render.fs");
	renderShader.use();
	renderShader.setInt("gIrradianceMap", 0);
	renderShader.setInt("gPrefilteredEnvMap", 1);
	renderShader.setInt("gBRDFIntegrationMap", 2);
	renderShader.setInt("gAlbedoMap", 3);
	renderShader.setInt("gNormalMap", 4);
	renderShader.setInt("gMetallicMap", 5);
	renderShader.setInt("gRoughnessMap", 6);
	renderShader.setInt("gAOMap", 7);
	for (size_t i = 0; i < LIGHT_POSITIONS.size(); ++i)
	{
		renderShader.setVec3("gLightPositions[" + std::to_string(i) + "]", LIGHT_POSITIONS[i]);
		renderShader.setVec3("gLightColors[" + std::to_string(i) + "]", LIGHT_COLORS[i]);
	}
	phoenix::Shader equirectangularToCubemapShader("../Resources/Shaders/pbr/precompute.vs", "../Resources/Shaders/pbr/equirectangular_to_cubemap.fs");
	equirectangularToCubemapShader.use();
	equirectangularToCubemapShader.setInt(G_ENV_MAP, 0);
	phoenix::Shader irradianceMapShader("../Resources/Shaders/pbr/precompute.vs", "../Resources/Shaders/pbr/irradiance_map.fs");
	irradianceMapShader.use();
	irradianceMapShader.setInt(G_ENV_MAP, 0);
	phoenix::Shader prefilterEnvMapShader("../Resources/Shaders/pbr/precompute.vs", "../Resources/Shaders/pbr/prefilter_env_map.fs");
	prefilterEnvMapShader.use();
	prefilterEnvMapShader.setInt(G_ENV_MAP, 0);
	prefilterEnvMapShader.setFloat("gSpecConvTexWidth", PREFILTERED_ENV_MAP_RES);
	phoenix::Shader integrateBRDFShader("../Resources/Shaders/pbr/integrateBRDF.vs", "../Resources/Shaders/pbr/integrateBRDF.fs");
	phoenix::Shader skyboxShader("../Resources/Shaders/pbr/skybox.vs", "../Resources/Shaders/pbr/skybox.fs");
	skyboxShader.use();
	skyboxShader.setInt(G_ENV_MAP, 0);

	// PBR Textures
	unsigned int ironAlbedoMap = utils->loadTexture("../Resources/Textures/pbr/rusted_iron/rustediron2_basecolor.png");
	unsigned int ironNormalMap = utils->loadTexture("../Resources/Textures/pbr/rusted_iron/rustediron2_normal.png");
	unsigned int ironMetallicMap = utils->loadTexture("../Resources/Textures/pbr/rusted_iron/rustediron2_metallic.png");
	unsigned int ironRoughnessMap = utils->loadTexture("../Resources/Textures/pbr/rusted_iron/rustediron2_roughness.png");
	unsigned int defaultAOMap = utils->loadTexture("../Resources/Textures/pbr/ao.png");

	unsigned int goldAlbedoMap = utils->loadTexture("../Resources/Textures/pbr/gold/gold-scuffed_basecolor.png");
	unsigned int goldNormalMap = utils->loadTexture("../Resources/Textures/pbr/gold/gold-scuffed_normal.png");
	unsigned int goldMetallicMap = utils->loadTexture("../Resources/Textures/pbr/gold/gold-scuffed_metallic.png");
	unsigned int goldRoughnessMap = utils->loadTexture("../Resources/Textures/pbr/gold/gold-scuffed_roughness.png");

	unsigned int woodAlbedoMap = utils->loadTexture("../Resources/Textures/pbr/wood/bamboo-wood-semigloss-albedo.png");
	unsigned int woodNormalMap = utils->loadTexture("../Resources/Textures/pbr/wood/bamboo-wood-semigloss-normal.png");
	unsigned int woodMetallicMap = utils->loadTexture("../Resources/Textures/pbr/wood/bamboo-wood-semigloss-metal.png");
	unsigned int woodRoughnessMap = utils->loadTexture("../Resources/Textures/pbr/wood/bamboo-wood-semigloss-roughness.png");
	unsigned int woodAOMap = utils->loadTexture("../Resources/Textures/pbr/wood/bamboo-wood-semigloss-ao.png");

	unsigned int plasticAlbedoMap = utils->loadTexture("../Resources/Textures/pbr/plastic/scuffed-plastic4-alb.png");
	unsigned int plasticNormalMap = utils->loadTexture("../Resources/Textures/pbr/plastic/scuffed-plastic-normal.png");
	unsigned int plasticMetallicMap = utils->loadTexture("../Resources/Textures/pbr/plastic/scuffed-plastic-metal.png");
	unsigned int plasticRoughnessMap = utils->loadTexture("../Resources/Textures/pbr/plastic/scuffed-plastic-rough.png");
	unsigned int plasticAOMap = utils->loadTexture("../Resources/Textures/pbr/plastic/scuffed-plastic-ao.png");

	unsigned int marbleAlbedoMap = utils->loadTexture("../Resources/Textures/pbr/marble/granitesmooth1-albedo2.png");
	unsigned int marbleNormalMap = utils->loadTexture("../Resources/Textures/pbr/marble/granitesmooth1-normal2.png");
	unsigned int marbleMetallicMap = utils->loadTexture("../Resources/Textures/pbr/marble/granitesmooth1-metalness.png");
	unsigned int marbleRoughnessMap = utils->loadTexture("../Resources/Textures/pbr/marble/granitesmooth1-roughness3.png");

	unsigned int gunAlbedoMap = utils->loadTexture("../Resources/Objects/gun/Textures/Cerberus_A.tga");
	unsigned int gunNormalMap = utils->loadTexture("../Resources/Objects/gun/Textures/Cerberus_N.tga");
	unsigned int gunMetallicMap = utils->loadTexture("../Resources/Objects/gun/Textures/Cerberus_M.tga");
	unsigned int gunRoughnessMap = utils->loadTexture("../Resources/Objects/gun/Textures/Cerberus_R.tga");
	unsigned int gunAOMap = utils->loadTexture("../Resources/Objects/gun/Textures/Cerberus_AO.tga");

	phoenix::Model gun("../Resources/Objects/gun/Cerberus_LP.FBX");

	unsigned int skullAlbedoMap = utils->loadTexture("../Resources/Objects/skull/RealTime_M_low1_BaseColor.png");
	unsigned int skullNormalMap = utils->loadTexture("../Resources/Objects/skull/Normal.png");
	unsigned int skullMetallicMap = utils->loadTexture("../Resources/Objects/skull/Metallic.png");
	unsigned int skullRoughnessMap = utils->loadTexture("../Resources/Objects/skull/Roughness.png");
	unsigned int skullAOMap = utils->loadTexture("../Resources/Objects/skull/AO.png");

	phoenix::Model skull("../Resources/Objects/skull/Skull_Low_res.obj");

	unsigned int equirectangularEnvMap = loadHDRTexture("../Resources/Textures/hdr/Newport_Loft_Ref.hdr");

	setupFramebuffer();

	// We're going to map the original HDR texture onto its cubemap equivalent for our calculations
	unsigned int envMap = generateCubemapTexture(RESOLUTION, true);
	setInputTexture(equirectangularToCubemapShader, equirectangularEnvMap, false);
	renderToCubemap(equirectangularToCubemapShader, envMap, RESOLUTION, 0);
	// This is to address a particular artifact
	glBindTexture(GL_TEXTURE_CUBE_MAP, envMap);
	glGenerateMipmap(GL_TEXTURE_CUBE_MAP);

	// Precompute an irradiance map that contains diffuse radiance data resulting from the Lambertian diffuse BRDF
	unsigned int irradianceMap = generateCubemapTexture(IRRADIANCE_MAP_RES, false);
	setInputTexture(irradianceMapShader, envMap, true);
	renderToCubemap(irradianceMapShader, irradianceMap, IRRADIANCE_MAP_RES, 0);

	// Now precompute the first of two prefiltered textures that collectively describe the specular radiance thanks
	// to Karis' split sum approximation. The basic idea here is to blur our environment map with corresponding
	// roughness factors, such that the higher mipmap levels store convolutions of increasing roughness
	unsigned int prefilteredEnvMap = generateCubemapTexture(PREFILTERED_ENV_MAP_RES, true);
	glGenerateMipmap(GL_TEXTURE_CUBE_MAP);
	setInputTexture(prefilterEnvMapShader, envMap, true);
	for (size_t i = 0; i < NUM_MIP_LEVELS; ++i)
	{
		prefilterEnvMapShader.setFloat("gRoughness", static_cast<float>(i) / (NUM_MIP_LEVELS - 1));
		renderToCubemap(prefilterEnvMapShader, prefilteredEnvMap, PREFILTERED_ENV_MAP_RES * pow(0.5f, i), i);
	}

	// We will also want to bake the specular BRDF integral part of the split sum in a 2D LUT
	unsigned int BRDFIntegrationMap = integrateBRDF(integrateBRDFShader);

	resetViewportToFramebufferSize();

	while (!glfwWindowShouldClose(window))
	{
		utils->_projection = glm::perspective(glm::radians(camera->_FOV), static_cast<float>(phoenix::SCREEN_WIDTH) / phoenix::SCREEN_HEIGHT, phoenix::PERSPECTIVE_NEAR_PLANE, phoenix::PERSPECTIVE_FAR_PLANE);
		utils->_view = camera->getViewMatrix();

		float currentFrame = glfwGetTime();
		utils->_deltaTime = currentFrame - utils->_lastTimestamp;
		utils->_lastTimestamp = currentFrame;

		processInput();

		glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		renderShader.use();
		renderShader.setVec3(phoenix::G_VIEW_POS, camera->_position);

		// Bind precomputed IBL data
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_CUBE_MAP, irradianceMap);
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_CUBE_MAP, prefilteredEnvMap);
		glActiveTexture(GL_TEXTURE2);
		glBindTexture(GL_TEXTURE_2D, BRDFIntegrationMap);

		// Render scene
		glActiveTexture(GL_TEXTURE3);
		glBindTexture(GL_TEXTURE_2D, ironAlbedoMap);
		glActiveTexture(GL_TEXTURE4);
		glBindTexture(GL_TEXTURE_2D, ironNormalMap);
		glActiveTexture(GL_TEXTURE5);
		glBindTexture(GL_TEXTURE_2D, ironMetallicMap);
		glActiveTexture(GL_TEXTURE6);
		glBindTexture(GL_TEXTURE_2D, ironRoughnessMap);
		glActiveTexture(GL_TEXTURE7);
		glBindTexture(GL_TEXTURE_2D, defaultAOMap);

		glm::mat4 world = glm::mat4(1.0f);
		world = glm::translate(world, glm::vec3(-5.0, 0.0, 2.0));
		renderShader.setMat4(phoenix::G_WORLD_MATRIX, world);
		renderShader.setMat4(phoenix::G_WVP, utils->_projection * utils->_view * world);
		renderShader.setMat3(phoenix::G_NORMAL_MATRIX, glm::mat3(world));
		renderSphere();

		glActiveTexture(GL_TEXTURE3);
		glBindTexture(GL_TEXTURE_2D, goldAlbedoMap);
		glActiveTexture(GL_TEXTURE4);
		glBindTexture(GL_TEXTURE_2D, goldNormalMap);
		glActiveTexture(GL_TEXTURE5);
		glBindTexture(GL_TEXTURE_2D, goldMetallicMap);
		glActiveTexture(GL_TEXTURE6);
		glBindTexture(GL_TEXTURE_2D, goldRoughnessMap);
		glActiveTexture(GL_TEXTURE7);
		glBindTexture(GL_TEXTURE_2D, defaultAOMap);

		world = glm::mat4(1.0f);
		world = glm::translate(world, glm::vec3(-3.0, 0.0, 2.0));
		renderShader.setMat4(phoenix::G_WORLD_MATRIX, world);
		renderShader.setMat4(phoenix::G_WVP, utils->_projection * utils->_view * world);
		renderShader.setMat3(phoenix::G_NORMAL_MATRIX, glm::mat3(world));
		renderSphere();

		glActiveTexture(GL_TEXTURE3);
		glBindTexture(GL_TEXTURE_2D, woodAlbedoMap);
		glActiveTexture(GL_TEXTURE4);
		glBindTexture(GL_TEXTURE_2D, woodNormalMap);
		glActiveTexture(GL_TEXTURE5);
		glBindTexture(GL_TEXTURE_2D, woodMetallicMap);
		glActiveTexture(GL_TEXTURE6);
		glBindTexture(GL_TEXTURE_2D, woodRoughnessMap);
		glActiveTexture(GL_TEXTURE7);
		glBindTexture(GL_TEXTURE_2D, woodAOMap);

		world = glm::mat4(1.0f);
		world = glm::translate(world, glm::vec3(-1.0, 0.0, 2.0));
		renderShader.setMat4(phoenix::G_WORLD_MATRIX, world);
		renderShader.setMat4(phoenix::G_WVP, utils->_projection * utils->_view * world);
		renderShader.setMat3(phoenix::G_NORMAL_MATRIX, glm::mat3(world));
		renderSphere();

		glActiveTexture(GL_TEXTURE3);
		glBindTexture(GL_TEXTURE_2D, plasticAlbedoMap);
		glActiveTexture(GL_TEXTURE4);
		glBindTexture(GL_TEXTURE_2D, plasticNormalMap);
		glActiveTexture(GL_TEXTURE5);
		glBindTexture(GL_TEXTURE_2D, plasticMetallicMap);
		glActiveTexture(GL_TEXTURE6);
		glBindTexture(GL_TEXTURE_2D, plasticRoughnessMap);
		glActiveTexture(GL_TEXTURE7);
		glBindTexture(GL_TEXTURE_2D, plasticAOMap);

		world = glm::mat4(1.0f);
		world = glm::translate(world, glm::vec3(1.0, 0.0, 2.0));
		renderShader.setMat4(phoenix::G_WORLD_MATRIX, world);
		renderShader.setMat4(phoenix::G_WVP, utils->_projection * utils->_view * world);
		renderShader.setMat3(phoenix::G_NORMAL_MATRIX, glm::mat3(world));
		renderSphere();

		glActiveTexture(GL_TEXTURE3);
		glBindTexture(GL_TEXTURE_2D, marbleAlbedoMap);
		glActiveTexture(GL_TEXTURE4);
		glBindTexture(GL_TEXTURE_2D, marbleNormalMap);
		glActiveTexture(GL_TEXTURE5);
		glBindTexture(GL_TEXTURE_2D, marbleMetallicMap);
		glActiveTexture(GL_TEXTURE6);
		glBindTexture(GL_TEXTURE_2D, marbleRoughnessMap);
		glActiveTexture(GL_TEXTURE7);
		glBindTexture(GL_TEXTURE_2D, defaultAOMap);

		world = glm::mat4(1.0f);
		world = glm::translate(world, glm::vec3(3.0, 0.0, 2.0));
		renderShader.setMat4(phoenix::G_WORLD_MATRIX, world);
		renderShader.setMat4(phoenix::G_WVP, utils->_projection * utils->_view * world);
		renderShader.setMat3(phoenix::G_NORMAL_MATRIX, glm::mat3(world));
		renderSphere();

		glActiveTexture(GL_TEXTURE3);
		glBindTexture(GL_TEXTURE_2D, gunAlbedoMap);
		glActiveTexture(GL_TEXTURE4);
		glBindTexture(GL_TEXTURE_2D, gunNormalMap);
		glActiveTexture(GL_TEXTURE5);
		glBindTexture(GL_TEXTURE_2D, gunMetallicMap);
		glActiveTexture(GL_TEXTURE6);
		glBindTexture(GL_TEXTURE_2D, gunRoughnessMap);
		glActiveTexture(GL_TEXTURE7);
		glBindTexture(GL_TEXTURE_2D, gunAOMap);

		world = glm::mat4(1.0f);
		world = glm::translate(world, glm::vec3(0.0, 0.0, -7.0));
		world = glm::scale(world, glm::vec3(0.05, 0.05, 0.05));
		world = glm::rotate(world, glm::radians(-90.0f), glm::vec3(1.0, 0.0, 0.0));
		renderShader.setMat4(phoenix::G_WORLD_MATRIX, world);
		renderShader.setMat4(phoenix::G_WVP, utils->_projection * utils->_view * world);
		renderShader.setMat3(phoenix::G_NORMAL_MATRIX, glm::mat3(world));
		gun.render();

		glActiveTexture(GL_TEXTURE3);
		glBindTexture(GL_TEXTURE_2D, skullAlbedoMap);
		glActiveTexture(GL_TEXTURE4);
		glBindTexture(GL_TEXTURE_2D, skullNormalMap);
		glActiveTexture(GL_TEXTURE5);
		glBindTexture(GL_TEXTURE_2D, skullMetallicMap);
		glActiveTexture(GL_TEXTURE6);
		glBindTexture(GL_TEXTURE_2D, skullRoughnessMap);
		glActiveTexture(GL_TEXTURE7);
		glBindTexture(GL_TEXTURE_2D, skullAOMap);

		world = glm::mat4(1.0f);
		world = glm::translate(world, glm::vec3(0.0, 0.0, 6.0));
		world = glm::scale(world, glm::vec3(2.0, 2.0, 2.0));
		world = glm::rotate(world, glm::radians(180.0f), glm::vec3(0.0, 1.0, 0.0));
		renderShader.setMat4(phoenix::G_WORLD_MATRIX, world);
		renderShader.setMat4(phoenix::G_WVP, utils->_projection * utils->_view * world);
		renderShader.setMat3(phoenix::G_NORMAL_MATRIX, glm::mat3(world));
		skull.render();

		skyboxShader.use();
		skyboxShader.setMat4(G_VP, utils->_projection * glm::mat4(glm::mat3(utils->_view)));
		glActiveTexture(GL_TEXTURE0);
		if (renderMode == 0)
		{
			glBindTexture(GL_TEXTURE_CUBE_MAP, envMap);
		}
		else if (renderMode == 1)
		{
			glBindTexture(GL_TEXTURE_CUBE_MAP, irradianceMap);
		}
		else
		{
			glBindTexture(GL_TEXTURE_CUBE_MAP, prefilteredEnvMap);
		}
		renderCube();

		if (renderBRDFIntegrationMap)
		{
			utils->renderQuad(integrateBRDFShader, 0);
		}

		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	deletePointers();
	glfwTerminate();
	return 0;
}

void processInput()
{
	if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
	{
		glfwSetWindowShouldClose(window, true);
	}

	if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
	{
		camera->processKeyPress(phoenix::FORWARD, utils->_deltaTime);
	}
	if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
	{
		camera->processKeyPress(phoenix::BACKWARD, utils->_deltaTime);
	}
	if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
	{
		camera->processKeyPress(phoenix::LEFT, utils->_deltaTime);
	}
	if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
	{
		camera->processKeyPress(phoenix::RIGHT, utils->_deltaTime);
	}

	if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS)
	{
		renderMode = 0;
		renderBRDFIntegrationMap = false;
	}
	if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS)
	{
		renderBRDFIntegrationMap = true;
	}

	if (glfwGetKey(window, GLFW_KEY_1) == GLFW_PRESS)
	{
		renderMode = 1;
	}
	if (glfwGetKey(window, GLFW_KEY_2) == GLFW_PRESS)
	{
		renderMode = 2;
	}
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

void renderSphere()
{
	if (sphereVAO == 0)
	{
		glGenVertexArrays(1, &sphereVAO);
		unsigned int sphereVBO, sphereEBO;
		glGenBuffers(1, &sphereVBO);
		glGenBuffers(1, &sphereEBO);

		std::vector<glm::vec3> positions;
		std::vector<glm::vec2> texCoords;
		std::vector<glm::vec3> normals;
		std::vector<float> vertices;
		std::vector<unsigned int> indices;

		const int NUM_SEGMENTS = 64;
		for (size_t j = 0; j <= NUM_SEGMENTS; ++j)
		{
			for (size_t i = 0; i <= NUM_SEGMENTS; ++i)
			{
				float u = static_cast<float>(i) / NUM_SEGMENTS;
				float v = static_cast<float>(j) / NUM_SEGMENTS;
				float x = cos(u * 2.0f * glm::pi<float>()) * sin(v * glm::pi<float>());
				float y = cos(v * glm::pi<float>());
				float z = sin(u * 2.0f * glm::pi<float>()) * sin(v * glm::pi<float>());

				positions.push_back(glm::vec3(x, y, z));
				texCoords.push_back(glm::vec2(u, v));
				normals.push_back(glm::vec3(x, y, z));
			}
		}

		for (size_t i = 0; i < positions.size(); ++i)
		{
			vertices.push_back(positions[i].x);
			vertices.push_back(positions[i].y);
			vertices.push_back(positions[i].z);
			if (texCoords.size() > 0)
			{
				vertices.push_back(texCoords[i].x);
				vertices.push_back(texCoords[i].y);
			}
			if (normals.size() > 0)
			{
				vertices.push_back(normals[i].x);
				vertices.push_back(normals[i].y);
				vertices.push_back(normals[i].z);
			}
		}

		bool oddRow = false;
		for (size_t j = 0; j < NUM_SEGMENTS; ++j)
		{
			if (!oddRow)
			{
				for (size_t i = 0; i <= NUM_SEGMENTS; ++i)
				{
					indices.push_back(j * (NUM_SEGMENTS + 1) + i);
					indices.push_back((j + 1) * (NUM_SEGMENTS + 1) + i);
				}
			}
			else
			{
				for (int i = NUM_SEGMENTS; i >= 0; --i)
				{
					indices.push_back((j + 1) * (NUM_SEGMENTS + 1) + i);
					indices.push_back(j * (NUM_SEGMENTS + 1) + i);
				}
			}
			oddRow = !oddRow;
		}
		numIndices = indices.size();

		glBindVertexArray(sphereVAO);

		glBindBuffer(GL_ARRAY_BUFFER, sphereVBO);
		glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), &vertices[0], GL_STATIC_DRAW);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, sphereEBO);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), &indices[0], GL_STATIC_DRAW);

		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
		glEnableVertexAttribArray(2);
		glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(5 * sizeof(float)));
		glBindVertexArray(0);
	}

	glBindVertexArray(sphereVAO);
	glDrawElements(GL_TRIANGLE_STRIP, numIndices, GL_UNSIGNED_INT, 0);
}

void renderCube()
{
	if (cubeVAO == 0)
	{
		float vertices[] = {
			// Back face
			-1.0f, -1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 0.0f, 0.0f, // Bottom-left
			 1.0f,  1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 1.0f, 1.0f, // Top-right
			 1.0f, -1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 1.0f, 0.0f, // Bottom-right
			 1.0f,  1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 1.0f, 1.0f, // Top-right
			-1.0f, -1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 0.0f, 0.0f, // Bottom-left
			-1.0f,  1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 0.0f, 1.0f, // Top-left
			// Front face
			-1.0f, -1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f, 0.0f, // Bottom-left
			 1.0f, -1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f, 0.0f, // Bottom-right
			 1.0f,  1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f, 1.0f, // Top-right
			 1.0f,  1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f, 1.0f, // Top-right
			-1.0f,  1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f, 1.0f, // Top-left
			-1.0f, -1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f, 0.0f, // Bottom-left
			// Left face
			-1.0f,  1.0f,  1.0f, -1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // Top-right
			-1.0f,  1.0f, -1.0f, -1.0f,  0.0f,  0.0f, 1.0f, 1.0f, // Top-left
			-1.0f, -1.0f, -1.0f, -1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // Bottom-left
			-1.0f, -1.0f, -1.0f, -1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // Bottom-left
			-1.0f, -1.0f,  1.0f, -1.0f,  0.0f,  0.0f, 0.0f, 0.0f, // Bottom-right
			-1.0f,  1.0f,  1.0f, -1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // Top-right
			// Right face
			 1.0f,  1.0f,  1.0f,  1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // Top-left
			 1.0f, -1.0f, -1.0f,  1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // Bottom-right
			 1.0f,  1.0f, -1.0f,  1.0f,  0.0f,  0.0f, 1.0f, 1.0f, // Top-right
			 1.0f, -1.0f, -1.0f,  1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // Bottom-right
			 1.0f,  1.0f,  1.0f,  1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // Top-left
			 1.0f, -1.0f,  1.0f,  1.0f,  0.0f,  0.0f, 0.0f, 0.0f, // Bottom-left
			// Bottom face
			-1.0f, -1.0f, -1.0f,  0.0f, -1.0f,  0.0f, 0.0f, 1.0f, // Top-right
			 1.0f, -1.0f, -1.0f,  0.0f, -1.0f,  0.0f, 1.0f, 1.0f, // Top-left
			 1.0f, -1.0f,  1.0f,  0.0f, -1.0f,  0.0f, 1.0f, 0.0f, // Bottom-left
			 1.0f, -1.0f,  1.0f,  0.0f, -1.0f,  0.0f, 1.0f, 0.0f, // Bottom-left
			-1.0f, -1.0f,  1.0f,  0.0f, -1.0f,  0.0f, 0.0f, 0.0f, // Bottom-right
			-1.0f, -1.0f, -1.0f,  0.0f, -1.0f,  0.0f, 0.0f, 1.0f, // Top-right
			// Top face
			-1.0f,  1.0f, -1.0f,  0.0f,  1.0f,  0.0f, 0.0f, 1.0f, // Top-left
			 1.0f,  1.0f , 1.0f,  0.0f,  1.0f,  0.0f, 1.0f, 0.0f, // Bottom-right
			 1.0f,  1.0f, -1.0f,  0.0f,  1.0f,  0.0f, 1.0f, 1.0f, // Top-right
			 1.0f,  1.0f,  1.0f,  0.0f,  1.0f,  0.0f, 1.0f, 0.0f, // Bottom-right
			-1.0f,  1.0f, -1.0f,  0.0f,  1.0f,  0.0f, 0.0f, 1.0f, // Top-left
			-1.0f,  1.0f,  1.0f,  0.0f,  1.0f,  0.0f, 0.0f, 0.0f  // Bottom-left 
		};

		glGenVertexArrays(1, &cubeVAO);
		unsigned int cubeVBO = 0;
		glGenBuffers(1, &cubeVBO);

		glBindBuffer(GL_ARRAY_BUFFER, cubeVBO);
		glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

		glBindVertexArray(cubeVAO);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
		glEnableVertexAttribArray(2);
		glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glBindVertexArray(0);
	}

	glBindVertexArray(cubeVAO);
	glDrawArrays(GL_TRIANGLES, 0, 36);
	glBindVertexArray(0);
}

void initPointers()
{
	utils = new phoenix::Utils();
	camera = new phoenix::Camera();
}

void deletePointers()
{
	delete camera;
	delete utils;
}

void set2DTexture(unsigned int* textureID)
{
	glGenTextures(1, textureID);
	glBindTexture(GL_TEXTURE_2D, *textureID);
}

void set2DTextureParams()
{
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
}

void setZBufferMemoryAttachment(int resolution)
{
	glBindFramebuffer(GL_FRAMEBUFFER, FBO);
	glBindRenderbuffer(GL_RENDERBUFFER, RBO);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT32, resolution, resolution);
}

void unbindFBOAndZBufferAttachment()
{
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glBindRenderbuffer(GL_RENDERBUFFER, 0);
}

void setupFramebuffer()
{
	glGenFramebuffers(1, &FBO);
	glGenRenderbuffers(1, &RBO);

	// Since we don't care about reading from the depth buffer, we can contain its data in a more efficient, non-readable attachment
	setZBufferMemoryAttachment(RESOLUTION);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, RBO);

	unbindFBOAndZBufferAttachment();
}

unsigned int loadHDRTexture(char const* filename)
{
	unsigned int textureID = -1;

	stbi_set_flip_vertically_on_load(true);
	int width, height, n;
	float* data = stbi_loadf(filename, &width, &height, &n, 0);
	if (data)
	{
		set2DTexture(&textureID);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, width, height, 0, GL_RGB, GL_FLOAT, data);
		set2DTextureParams();
	}
	else
	{
		std::cerr << "Failed to load file: " << filename << "\n";
	}
	stbi_image_free(data);

	return textureID;
}

unsigned int generateCubemapTexture(int resolution, bool useMipmap)
{
	unsigned int textureID;

	glGenTextures(1, &textureID);
	glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);
	for (size_t i = 0; i < NUM_FACES; ++i)
	{
		glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB16F, resolution, resolution, 0, GL_RGB, GL_FLOAT, nullptr);
	}

	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, useMipmap ? GL_LINEAR_MIPMAP_LINEAR : GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	return textureID;
}

void setInputTexture(const phoenix::Shader& shader, unsigned int texture, bool isCubemap)
{
	shader.use();
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(isCubemap ? GL_TEXTURE_CUBE_MAP : GL_TEXTURE_2D, texture);
}

void renderToCubemap(const phoenix::Shader& shader, unsigned int texture, int resolution, int level)
{
	setZBufferMemoryAttachment(resolution);
	glViewport(0, 0, resolution, resolution);
	for (size_t i = 0; i < NUM_FACES; ++i)
	{
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, texture, level);

		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		shader.setMat4(G_VP, CUBEMAP_PROJ * CUBEMAP_VIEWS[i]);
		renderCube();
	}
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

unsigned int integrateBRDF(const phoenix::Shader& shader)
{
	unsigned int textureID;

	set2DTexture(&textureID);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RG16F, RESOLUTION, RESOLUTION, 0, GL_RG, GL_FLOAT, 0);
	set2DTextureParams();

	setZBufferMemoryAttachment(RESOLUTION);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, textureID, 0);

	glViewport(0, 0, RESOLUTION, RESOLUTION);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	utils->renderQuad(shader, 0);

	unbindFBOAndZBufferAttachment();

	return textureID;
}

void resetViewportToFramebufferSize()
{
	int width, height;
	glfwGetFramebufferSize(window, &width, &height);
	glViewport(0, 0, width, height);
}
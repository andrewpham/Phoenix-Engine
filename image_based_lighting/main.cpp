#include <glm/gtc/matrix_transform.hpp>

#include <engine/stb_image.h>
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
void initPointers();
void deletePointers();

void set2DTexture(unsigned int*);
void set2DTextureParams();
void setZBufferMemoryAttachment(int);
void unbindFBOAndZBufferAttachment();

void setupFramebuffer();
unsigned int loadHDRTexture(char const*);
unsigned int generateCubemapTexture(int, bool, bool = false);
void setInputTexture(const phoenix::Shader&, unsigned int, bool);
void renderToCubemap(const phoenix::Shader&, unsigned int, int, int);
// For writing to spherical harmonic coefficient render targets
void renderToCubemaps(const phoenix::Shader&);
void allocSHTextures();
void genMipmaps();
void genSHCoefficients(const phoenix::Shader&, const phoenix::Shader&, unsigned int);
unsigned int integrateBRDF(const phoenix::Shader&);
void resetViewportToFramebufferSize();

// Environment map, BRDF LUT, and spherical harmonic coefficient render target resolution
const int RESOLUTION = 512;
const int IRRADIANCE_MAP_RES = 32, PREFILTERED_ENV_MAP_RES = 128;
const unsigned int NUM_FACES = 6, NUM_MIP_LEVELS = 5;

const std::string G_ENV_MAP = "gEnvMap";

phoenix::Camera* camera;
phoenix::Utils* utils;
GLFWwindow* window;

float lastX = static_cast<float>(phoenix::SCREEN_WIDTH) / 2.0f;
float lastY = static_cast<float>(phoenix::SCREEN_HEIGHT) / 2.0f;

bool calibratedCursor = false;
bool renderBRDFIntegrationMap = false;

unsigned int renderMode = 0, irradianceMapLightingMode = 0;
unsigned int FBO, RBO;
// Spherical harmonic coefficients representations
unsigned int L00, L1_1, L10, L11, L2_2, L2_1, L20, L21, L22;

const std::array<glm::vec3, 8> LIGHT_POSITIONS{ {
	glm::vec3(-10.0f, 10.0f, 10.0f),
	glm::vec3(10.0f, 10.0f, 10.0f),
	glm::vec3(-10.0f, -10.0f, 10.0f),
	glm::vec3(10.0f, -10.0f, 10.0f)
} };
const std::array<glm::vec3, 8> LIGHT_COLORS{ {
	glm::vec3(300.0f, 300.0f, 300.0f),
	glm::vec3(300.0f, 300.0f, 300.0f),
	glm::vec3(300.0f, 300.0f, 300.0f),
	glm::vec3(300.0f, 300.0f, 300.0f)
} };
const glm::mat4 CUBEMAP_PROJ = glm::perspective(glm::radians(90.0f), 1.0f, phoenix::PERSPECTIVE_NEAR_PLANE, phoenix::PERSPECTIVE_FAR_PLANE / 10.0f);
const std::array<glm::mat4, 6> CUBEMAP_VIEWS{ {
	glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f)),
	glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(-1.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f)),
	glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f)),
	glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f), glm::vec3(0.0f, 0.0f, -1.0f)),
	glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f), glm::vec3(0.0f, -1.0f, 0.0f)),
	glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, -1.0f), glm::vec3(0.0f, -1.0f, 0.0f))
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
	renderShader.setInt("gL00", 0);
	renderShader.setInt("gL1_1", 1);
	renderShader.setInt("gL10", 2);
	renderShader.setInt("gL11", 3);
	renderShader.setInt("gL2_2", 4);
	renderShader.setInt("gL2_1", 5);
	renderShader.setInt("gL20", 6);
	renderShader.setInt("gL21", 7);
	renderShader.setInt("gL22", 8);
	renderShader.setInt("gIrradianceMap", 9);
	renderShader.setInt("gPrefilteredEnvMap", 10);
	renderShader.setInt("gBRDFIntegrationMap", 11);
	renderShader.setInt("gAlbedoMap", 12);
	renderShader.setInt(phoenix::G_NORMAL_MAP, 13);
	renderShader.setInt("gMetallicMap", 14);
	renderShader.setInt("gRoughnessMap", 15);
	renderShader.setInt("gAOMap", 16);
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
	phoenix::Shader genSHCoefficientsShader("../Resources/Shaders/pbr/precompute.vs", "../Resources/Shaders/pbr/gen_sh_coefficients.fs");
	genSHCoefficientsShader.use();
	genSHCoefficientsShader.setInt(G_ENV_MAP, 0);
	// Extra pass for the L22 coefficient render target
	phoenix::Shader genSHCoefficientShader("../Resources/Shaders/pbr/precompute.vs", "../Resources/Shaders/pbr/gen_sh_coefficient.fs");
	genSHCoefficientShader.use();
	genSHCoefficientShader.setInt(G_ENV_MAP, 0);
	phoenix::Shader prefilterEnvMapShader("../Resources/Shaders/pbr/precompute.vs", "../Resources/Shaders/pbr/prefilter_env_map.fs");
	prefilterEnvMapShader.use();
	prefilterEnvMapShader.setInt(G_ENV_MAP, 0);
	prefilterEnvMapShader.setFloat("gSpecConvTexWidth", PREFILTERED_ENV_MAP_RES);
	phoenix::Shader integrateBRDFShader("../Resources/Shaders/pbr/integrateBRDF.vs", "../Resources/Shaders/pbr/integrateBRDF.fs");
	phoenix::Shader skyboxShader("../Resources/Shaders/pbr/skybox.vs", "../Resources/Shaders/pbr/skybox.fs");
	skyboxShader.use();
	skyboxShader.setInt("gSampler0", 0);
	skyboxShader.setInt("gSampler1", 1);
	skyboxShader.setInt("gSampler2", 2);
	skyboxShader.setInt("gSampler3", 3);
	skyboxShader.setInt("gSampler4", 4);
	skyboxShader.setInt("gSampler5", 5);
	skyboxShader.setInt("gSampler6", 6);
	skyboxShader.setInt("gSampler7", 7);
	skyboxShader.setInt("gSampler8", 8);

	// PBR Textures
	unsigned int ironAlbedoMap = phoenix::Utils::loadTexture("../Resources/Textures/pbr/rusted_iron/rustediron2_basecolor.png");
	unsigned int ironNormalMap = phoenix::Utils::loadTexture("../Resources/Textures/pbr/rusted_iron/rustediron2_normal.png");
	unsigned int ironMetallicMap = phoenix::Utils::loadTexture("../Resources/Textures/pbr/rusted_iron/rustediron2_metallic.png");
	unsigned int ironRoughnessMap = phoenix::Utils::loadTexture("../Resources/Textures/pbr/rusted_iron/rustediron2_roughness.png");
	unsigned int defaultAOMap = phoenix::Utils::loadTexture("../Resources/Textures/pbr/ao.png");

	unsigned int goldAlbedoMap = phoenix::Utils::loadTexture("../Resources/Textures/pbr/gold/gold-scuffed_basecolor.png");
	unsigned int goldNormalMap = phoenix::Utils::loadTexture("../Resources/Textures/pbr/gold/gold-scuffed_normal.png");
	unsigned int goldMetallicMap = phoenix::Utils::loadTexture("../Resources/Textures/pbr/gold/gold-scuffed_metallic.png");
	unsigned int goldRoughnessMap = phoenix::Utils::loadTexture("../Resources/Textures/pbr/gold/gold-scuffed_roughness.png");

	unsigned int woodAlbedoMap = phoenix::Utils::loadTexture("../Resources/Textures/pbr/wood/bamboo-wood-semigloss-albedo.png");
	unsigned int woodNormalMap = phoenix::Utils::loadTexture("../Resources/Textures/pbr/wood/bamboo-wood-semigloss-normal.png");
	unsigned int woodMetallicMap = phoenix::Utils::loadTexture("../Resources/Textures/pbr/wood/bamboo-wood-semigloss-metal.png");
	unsigned int woodRoughnessMap = phoenix::Utils::loadTexture("../Resources/Textures/pbr/wood/bamboo-wood-semigloss-roughness.png");
	unsigned int woodAOMap = phoenix::Utils::loadTexture("../Resources/Textures/pbr/wood/bamboo-wood-semigloss-ao.png");

	unsigned int plasticAlbedoMap = phoenix::Utils::loadTexture("../Resources/Textures/pbr/plastic/scuffed-plastic4-alb.png");
	unsigned int plasticNormalMap = phoenix::Utils::loadTexture("../Resources/Textures/pbr/plastic/scuffed-plastic-normal.png");
	unsigned int plasticMetallicMap = phoenix::Utils::loadTexture("../Resources/Textures/pbr/plastic/scuffed-plastic-metal.png");
	unsigned int plasticRoughnessMap = phoenix::Utils::loadTexture("../Resources/Textures/pbr/plastic/scuffed-plastic-rough.png");
	unsigned int plasticAOMap = phoenix::Utils::loadTexture("../Resources/Textures/pbr/plastic/scuffed-plastic-ao.png");

	unsigned int marbleAlbedoMap = phoenix::Utils::loadTexture("../Resources/Textures/pbr/marble/granitesmooth1-albedo2.png");
	unsigned int marbleNormalMap = phoenix::Utils::loadTexture("../Resources/Textures/pbr/marble/granitesmooth1-normal2.png");
	unsigned int marbleMetallicMap = phoenix::Utils::loadTexture("../Resources/Textures/pbr/marble/granitesmooth1-metalness.png");
	unsigned int marbleRoughnessMap = phoenix::Utils::loadTexture("../Resources/Textures/pbr/marble/granitesmooth1-roughness3.png");

	unsigned int gunAlbedoMap = phoenix::Utils::loadTexture("../Resources/Objects/gun/Textures/Cerberus_A.tga");
	unsigned int gunNormalMap = phoenix::Utils::loadTexture("../Resources/Objects/gun/Textures/Cerberus_N.tga");
	unsigned int gunMetallicMap = phoenix::Utils::loadTexture("../Resources/Objects/gun/Textures/Cerberus_M.tga");
	unsigned int gunRoughnessMap = phoenix::Utils::loadTexture("../Resources/Objects/gun/Textures/Cerberus_R.tga");
	unsigned int gunAOMap = phoenix::Utils::loadTexture("../Resources/Objects/gun/Textures/Cerberus_AO.tga");

	phoenix::Model gun("../Resources/Objects/gun/Cerberus_LP.FBX");

	unsigned int skullAlbedoMap = phoenix::Utils::loadTexture("../Resources/Objects/skull/RealTime_M_low1_BaseColor.png");
	unsigned int skullNormalMap = phoenix::Utils::loadTexture("../Resources/Objects/skull/Normal.png");
	unsigned int skullMetallicMap = phoenix::Utils::loadTexture("../Resources/Objects/skull/Metallic.png");
	unsigned int skullRoughnessMap = phoenix::Utils::loadTexture("../Resources/Objects/skull/Roughness.png");
	unsigned int skullAOMap = phoenix::Utils::loadTexture("../Resources/Objects/skull/AO.png");

	phoenix::Model skull("../Resources/Objects/skull/Skull_Low_res.obj");

	unsigned int equirectangularEnvMap = loadHDRTexture("../Resources/Textures/pbr/Ice_Lake_Ref.hdr");

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

	genSHCoefficients(genSHCoefficientsShader, genSHCoefficientShader, envMap);

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
		renderShader.setInt("gIrradianceMapLightingMode", irradianceMapLightingMode);

		// Bind precomputed IBL data
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_CUBE_MAP, L00);
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_CUBE_MAP, L1_1);
		glActiveTexture(GL_TEXTURE2);
		glBindTexture(GL_TEXTURE_CUBE_MAP, L10);
		glActiveTexture(GL_TEXTURE3);
		glBindTexture(GL_TEXTURE_CUBE_MAP, L11);
		glActiveTexture(GL_TEXTURE4);
		glBindTexture(GL_TEXTURE_CUBE_MAP, L2_2);
		glActiveTexture(GL_TEXTURE5);
		glBindTexture(GL_TEXTURE_CUBE_MAP, L2_1);
		glActiveTexture(GL_TEXTURE6);
		glBindTexture(GL_TEXTURE_CUBE_MAP, L20);
		glActiveTexture(GL_TEXTURE7);
		glBindTexture(GL_TEXTURE_CUBE_MAP, L21);
		glActiveTexture(GL_TEXTURE8);
		glBindTexture(GL_TEXTURE_CUBE_MAP, L22);
		glActiveTexture(GL_TEXTURE9);
		glBindTexture(GL_TEXTURE_CUBE_MAP, irradianceMap);
		glActiveTexture(GL_TEXTURE10);
		glBindTexture(GL_TEXTURE_CUBE_MAP, prefilteredEnvMap);
		glActiveTexture(GL_TEXTURE11);
		glBindTexture(GL_TEXTURE_2D, BRDFIntegrationMap);

		// Render scene
		glActiveTexture(GL_TEXTURE12);
		glBindTexture(GL_TEXTURE_2D, ironAlbedoMap);
		glActiveTexture(GL_TEXTURE13);
		glBindTexture(GL_TEXTURE_2D, ironNormalMap);
		glActiveTexture(GL_TEXTURE14);
		glBindTexture(GL_TEXTURE_2D, ironMetallicMap);
		glActiveTexture(GL_TEXTURE15);
		glBindTexture(GL_TEXTURE_2D, ironRoughnessMap);
		glActiveTexture(GL_TEXTURE16);
		glBindTexture(GL_TEXTURE_2D, defaultAOMap);

		glm::mat4 world = glm::mat4(1.0f);
		world = glm::translate(world, glm::vec3(-5.0, 0.0, 2.0));
		renderShader.setMat4(phoenix::G_WORLD_MATRIX, world);
		renderShader.setMat4(phoenix::G_WVP, utils->_projection * utils->_view * world);
		renderShader.setMat3(phoenix::G_NORMAL_MATRIX, glm::mat3(world));
		utils->renderSphere();

		glActiveTexture(GL_TEXTURE12);
		glBindTexture(GL_TEXTURE_2D, goldAlbedoMap);
		glActiveTexture(GL_TEXTURE13);
		glBindTexture(GL_TEXTURE_2D, goldNormalMap);
		glActiveTexture(GL_TEXTURE14);
		glBindTexture(GL_TEXTURE_2D, goldMetallicMap);
		glActiveTexture(GL_TEXTURE15);
		glBindTexture(GL_TEXTURE_2D, goldRoughnessMap);
		glActiveTexture(GL_TEXTURE16);
		glBindTexture(GL_TEXTURE_2D, defaultAOMap);

		world = glm::mat4(1.0f);
		world = glm::translate(world, glm::vec3(-3.0, 0.0, 2.0));
		renderShader.setMat4(phoenix::G_WORLD_MATRIX, world);
		renderShader.setMat4(phoenix::G_WVP, utils->_projection * utils->_view * world);
		renderShader.setMat3(phoenix::G_NORMAL_MATRIX, glm::mat3(world));
		utils->renderSphere();

		glActiveTexture(GL_TEXTURE12);
		glBindTexture(GL_TEXTURE_2D, woodAlbedoMap);
		glActiveTexture(GL_TEXTURE13);
		glBindTexture(GL_TEXTURE_2D, woodNormalMap);
		glActiveTexture(GL_TEXTURE14);
		glBindTexture(GL_TEXTURE_2D, woodMetallicMap);
		glActiveTexture(GL_TEXTURE15);
		glBindTexture(GL_TEXTURE_2D, woodRoughnessMap);
		glActiveTexture(GL_TEXTURE16);
		glBindTexture(GL_TEXTURE_2D, woodAOMap);

		world = glm::mat4(1.0f);
		world = glm::translate(world, glm::vec3(-1.0, 0.0, 2.0));
		renderShader.setMat4(phoenix::G_WORLD_MATRIX, world);
		renderShader.setMat4(phoenix::G_WVP, utils->_projection * utils->_view * world);
		renderShader.setMat3(phoenix::G_NORMAL_MATRIX, glm::mat3(world));
		utils->renderSphere();

		glActiveTexture(GL_TEXTURE12);
		glBindTexture(GL_TEXTURE_2D, plasticAlbedoMap);
		glActiveTexture(GL_TEXTURE13);
		glBindTexture(GL_TEXTURE_2D, plasticNormalMap);
		glActiveTexture(GL_TEXTURE14);
		glBindTexture(GL_TEXTURE_2D, plasticMetallicMap);
		glActiveTexture(GL_TEXTURE15);
		glBindTexture(GL_TEXTURE_2D, plasticRoughnessMap);
		glActiveTexture(GL_TEXTURE16);
		glBindTexture(GL_TEXTURE_2D, plasticAOMap);

		world = glm::mat4(1.0f);
		world = glm::translate(world, glm::vec3(1.0, 0.0, 2.0));
		renderShader.setMat4(phoenix::G_WORLD_MATRIX, world);
		renderShader.setMat4(phoenix::G_WVP, utils->_projection * utils->_view * world);
		renderShader.setMat3(phoenix::G_NORMAL_MATRIX, glm::mat3(world));
		utils->renderSphere();

		glActiveTexture(GL_TEXTURE12);
		glBindTexture(GL_TEXTURE_2D, marbleAlbedoMap);
		glActiveTexture(GL_TEXTURE13);
		glBindTexture(GL_TEXTURE_2D, marbleNormalMap);
		glActiveTexture(GL_TEXTURE14);
		glBindTexture(GL_TEXTURE_2D, marbleMetallicMap);
		glActiveTexture(GL_TEXTURE15);
		glBindTexture(GL_TEXTURE_2D, marbleRoughnessMap);
		glActiveTexture(GL_TEXTURE16);
		glBindTexture(GL_TEXTURE_2D, defaultAOMap);

		world = glm::mat4(1.0f);
		world = glm::translate(world, glm::vec3(3.0, 0.0, 2.0));
		renderShader.setMat4(phoenix::G_WORLD_MATRIX, world);
		renderShader.setMat4(phoenix::G_WVP, utils->_projection * utils->_view * world);
		renderShader.setMat3(phoenix::G_NORMAL_MATRIX, glm::mat3(world));
		utils->renderSphere();

		glActiveTexture(GL_TEXTURE12);
		glBindTexture(GL_TEXTURE_2D, gunAlbedoMap);
		glActiveTexture(GL_TEXTURE13);
		glBindTexture(GL_TEXTURE_2D, gunNormalMap);
		glActiveTexture(GL_TEXTURE14);
		glBindTexture(GL_TEXTURE_2D, gunMetallicMap);
		glActiveTexture(GL_TEXTURE15);
		glBindTexture(GL_TEXTURE_2D, gunRoughnessMap);
		glActiveTexture(GL_TEXTURE16);
		glBindTexture(GL_TEXTURE_2D, gunAOMap);

		world = glm::mat4(1.0f);
		world = glm::translate(world, glm::vec3(0.0, 0.0, -7.0));
		world = glm::scale(world, glm::vec3(0.05, 0.05, 0.05));
		world = glm::rotate(world, glm::radians(-90.0f), glm::vec3(1.0, 0.0, 0.0));
		renderShader.setMat4(phoenix::G_WORLD_MATRIX, world);
		renderShader.setMat4(phoenix::G_WVP, utils->_projection * utils->_view * world);
		renderShader.setMat3(phoenix::G_NORMAL_MATRIX, glm::mat3(world));
		gun.render();

		glActiveTexture(GL_TEXTURE12);
		glBindTexture(GL_TEXTURE_2D, skullAlbedoMap);
		glActiveTexture(GL_TEXTURE13);
		glBindTexture(GL_TEXTURE_2D, skullNormalMap);
		glActiveTexture(GL_TEXTURE14);
		glBindTexture(GL_TEXTURE_2D, skullMetallicMap);
		glActiveTexture(GL_TEXTURE15);
		glBindTexture(GL_TEXTURE_2D, skullRoughnessMap);
		glActiveTexture(GL_TEXTURE16);
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
		skyboxShader.setMat4(phoenix::G_VP, utils->_projection * glm::mat4(glm::mat3(utils->_view)));
		skyboxShader.setInt(phoenix::G_RENDER_MODE, renderMode);
		glActiveTexture(GL_TEXTURE0);
		if (renderMode == 0)
		{
			glBindTexture(GL_TEXTURE_CUBE_MAP, envMap);
		}
		else if (renderMode == 1)
		{
			glBindTexture(GL_TEXTURE_CUBE_MAP, irradianceMap);
		}
		else if (renderMode == 2)
		{
			glBindTexture(GL_TEXTURE_CUBE_MAP, prefilteredEnvMap);
		}
		else
		{
			glBindTexture(GL_TEXTURE_CUBE_MAP, L00);
			glActiveTexture(GL_TEXTURE1);
			glBindTexture(GL_TEXTURE_CUBE_MAP, L1_1);
			glActiveTexture(GL_TEXTURE2);
			glBindTexture(GL_TEXTURE_CUBE_MAP, L10);
			glActiveTexture(GL_TEXTURE3);
			glBindTexture(GL_TEXTURE_CUBE_MAP, L11);
			glActiveTexture(GL_TEXTURE4);
			glBindTexture(GL_TEXTURE_CUBE_MAP, L2_2);
			glActiveTexture(GL_TEXTURE5);
			glBindTexture(GL_TEXTURE_CUBE_MAP, L2_1);
			glActiveTexture(GL_TEXTURE6);
			glBindTexture(GL_TEXTURE_CUBE_MAP, L20);
			glActiveTexture(GL_TEXTURE7);
			glBindTexture(GL_TEXTURE_CUBE_MAP, L21);
			glActiveTexture(GL_TEXTURE8);
			glBindTexture(GL_TEXTURE_CUBE_MAP, L22);
		}
		utils->renderCube();

		if (renderBRDFIntegrationMap)
		{
			utils->renderQuad(integrateBRDFShader);
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
	if (glfwGetKey(window, GLFW_KEY_3) == GLFW_PRESS)
	{
		renderMode = 3;
	}

	if (glfwGetKey(window, GLFW_KEY_O) == GLFW_PRESS)
	{
		irradianceMapLightingMode = 0;
	}
	if (glfwGetKey(window, GLFW_KEY_P) == GLFW_PRESS)
	{
		irradianceMapLightingMode = 1;
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

unsigned int generateCubemapTexture(int resolution, bool useMipmap, bool highRes)
{
	unsigned int textureID;

	glGenTextures(1, &textureID);
	glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);
	for (size_t i = 0; i < NUM_FACES; ++i)
	{
		glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, highRes ? GL_RGB32F : GL_RGB16F, resolution, resolution, 0, GL_RGB, GL_FLOAT, nullptr);
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
		shader.setMat4(phoenix::G_VP, CUBEMAP_PROJ * CUBEMAP_VIEWS[i]);
		utils->renderCube();
	}
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void renderToCubemaps(const phoenix::Shader& shader)
{
	setZBufferMemoryAttachment(RESOLUTION);
	glViewport(0, 0, RESOLUTION, RESOLUTION);
	for (size_t i = 0; i < NUM_FACES; ++i)
	{
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, L00, 0);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, L1_1, 0);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, L10, 0);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT3, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, L11, 0);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT4, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, L2_2, 0);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT5, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, L2_1, 0);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT6, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, L20, 0);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT7, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, L21, 0);

		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		shader.setMat4(phoenix::G_VP, CUBEMAP_PROJ * CUBEMAP_VIEWS[i]);
		utils->renderCube();
	}
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void allocSHTextures()
{
	L00 = generateCubemapTexture(RESOLUTION, true, true);
	glGenerateMipmap(GL_TEXTURE_CUBE_MAP);
	L1_1 = generateCubemapTexture(RESOLUTION, true, true);
	glGenerateMipmap(GL_TEXTURE_CUBE_MAP);
	L10 = generateCubemapTexture(RESOLUTION, true, true);
	glGenerateMipmap(GL_TEXTURE_CUBE_MAP);
	L11 = generateCubemapTexture(RESOLUTION, true, true);
	glGenerateMipmap(GL_TEXTURE_CUBE_MAP);
	L2_2 = generateCubemapTexture(RESOLUTION, true, true);
	glGenerateMipmap(GL_TEXTURE_CUBE_MAP);
	L2_1 = generateCubemapTexture(RESOLUTION, true, true);
	glGenerateMipmap(GL_TEXTURE_CUBE_MAP);
	L20 = generateCubemapTexture(RESOLUTION, true, true);
	glGenerateMipmap(GL_TEXTURE_CUBE_MAP);
	L21 = generateCubemapTexture(RESOLUTION, true, true);
	glGenerateMipmap(GL_TEXTURE_CUBE_MAP);
	L22 = generateCubemapTexture(RESOLUTION, true, true);
	glGenerateMipmap(GL_TEXTURE_CUBE_MAP);
}

void genMipmaps()
{
	glBindTexture(GL_TEXTURE_CUBE_MAP, L00);
	glGenerateMipmap(GL_TEXTURE_CUBE_MAP);
	glBindTexture(GL_TEXTURE_CUBE_MAP, L1_1);
	glGenerateMipmap(GL_TEXTURE_CUBE_MAP);
	glBindTexture(GL_TEXTURE_CUBE_MAP, L10);
	glGenerateMipmap(GL_TEXTURE_CUBE_MAP);
	glBindTexture(GL_TEXTURE_CUBE_MAP, L11);
	glGenerateMipmap(GL_TEXTURE_CUBE_MAP);
	glBindTexture(GL_TEXTURE_CUBE_MAP, L2_2);
	glGenerateMipmap(GL_TEXTURE_CUBE_MAP);
	glBindTexture(GL_TEXTURE_CUBE_MAP, L2_1);
	glGenerateMipmap(GL_TEXTURE_CUBE_MAP);
	glBindTexture(GL_TEXTURE_CUBE_MAP, L20);
	glGenerateMipmap(GL_TEXTURE_CUBE_MAP);
	glBindTexture(GL_TEXTURE_CUBE_MAP, L21);
	glGenerateMipmap(GL_TEXTURE_CUBE_MAP);
	glBindTexture(GL_TEXTURE_CUBE_MAP, L22);
	glGenerateMipmap(GL_TEXTURE_CUBE_MAP);
}

// (Spherical Harmonics) Generate the 9 lighting coefficients for our environment
void genSHCoefficients(const phoenix::Shader& firstPassShader, const phoenix::Shader& secondPassShader, unsigned int texture)
{
	glBindFramebuffer(GL_FRAMEBUFFER, FBO);
	allocSHTextures();
	// Calculate SH coefficients via Monte Carlo integration on the input/environment map
	setInputTexture(firstPassShader, texture, true);
	renderToCubemaps(firstPassShader);
	// Do another pass for the remaining coefficient since we can only write to 8 framebuffer attachments in a single pass
	setInputTexture(secondPassShader, texture, true);
	renderToCubemap(secondPassShader, L22, RESOLUTION, 0);
	genMipmaps();
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
	utils->renderQuad(shader);

	unbindFBOAndZBufferAttachment();

	return textureID;
}

void resetViewportToFramebufferSize()
{
	int width, height;
	glfwGetFramebufferSize(window, &width, &height);
	glViewport(0, 0, width, height);
}
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
void execTextureSpaceInputsPass(const phoenix::Shader&, phoenix::Model&);
void execBlurPasses(const phoenix::Shader&, const phoenix::Shader&, const phoenix::Shader&, const phoenix::Shader&);
void execRenderPass(const phoenix::Shader&, const phoenix::Shader&, phoenix::Model&);
void initPointers();
void deletePointers();

const unsigned int NUM_BLUR_PASSES = 5;
// Parameters for our head model
const glm::vec3 TRANSLATION = glm::vec3(0.0f, 2.0f, 0.0f), SCALE = glm::vec3(6.5f);
const float ROTATION = -90.0f;

phoenix::Camera* camera;
phoenix::Utils* utils;
phoenix::Framebuffer* shadowMapRenderTarget;
phoenix::Framebuffer* textureSpaceInputs;
std::array<phoenix::Framebuffer*, 5> blurStretchPassesRenderTargets;
std::array<phoenix::Framebuffer*, 5> blurPassesRenderTargets;
phoenix::ShadowCommon* shadowCommon;

float lastX = static_cast<float>(phoenix::SCREEN_WIDTH) / 2.0f;
float lastY = static_cast<float>(phoenix::SCREEN_HEIGHT) / 2.0f;

bool calibratedCursor = false;

unsigned int anglesTexture, normalMap, irradianceMap, beckmannTexture, specularTexture;
std::array<unsigned int, 5> blurredIrradianceMaps;
std::array<unsigned int, 5> blurredStretchMaps;

int main()
{
	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	GLFWwindow* window = glfwCreateWindow(phoenix::SCREEN_WIDTH, phoenix::SCREEN_HEIGHT, "Skin Rendering", nullptr, nullptr);
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
	shadowCommon->_objectTexture = phoenix::Utils::loadTexture("../Resources/Objects/head/lambertian.jpg");
	normalMap = phoenix::Utils::loadTexture("../Resources/Objects/head/normal.png");
	beckmannTexture = phoenix::Utils::loadTexture("../Resources/Textures/skin/beckmannTex.jpg");
	specularTexture = phoenix::Utils::loadTexture("../Resources/Textures/skin/skin_spec.jpg");
	// Generate volume textures and fill them with the cosines and sines of random rotation angles for PCSS
	generateRandom3DTexture();

	phoenix::Shader floorShader("../Resources/Shaders/skin/render_pass.vs", "../Resources/Shaders/skin/floor.fs");
	floorShader.use();
	floorShader.setInt(phoenix::G_DIFFUSE_TEXTURE, 0);
	floorShader.setInt(phoenix::G_SHADOW_MAP, 1);
	floorShader.setInt(phoenix::G_ANGLES_TEXTURE, 2);
	floorShader.setFloat(phoenix::G_AMBIENT_FACTOR, phoenix::AMBIENT_FACTOR);
	floorShader.setFloat(phoenix::G_SPECULAR_FACTOR, phoenix::SPECULAR_FACTOR);
	floorShader.setFloat(phoenix::G_CALIBRATED_LIGHT_SIZE, phoenix::CALIBRATED_LIGHT_SIZE);
	floorShader.setVec3(phoenix::G_LIGHT_COLOR, phoenix::LIGHT_COLOR);
	phoenix::Shader headShader("../Resources/Shaders/skin/render_pass.vs", "../Resources/Shaders/skin/head.fs");
	headShader.use();
	headShader.setInt(phoenix::G_DIFFUSE_TEXTURE, 0);
	headShader.setInt(phoenix::G_SHADOW_MAP, 1); // Translucent shadow map
	headShader.setInt(phoenix::G_NORMAL_MAP, 3);
	headShader.setInt("gBeckmannTexture", 4);
	headShader.setInt(phoenix::G_STRETCH_MAP, 5);
	headShader.setInt("gSpecularTexture", 6);
	for (size_t i = 0; i < NUM_BLUR_PASSES; ++i)
	{
		headShader.setInt("gIrradianceMaps[" + std::to_string(i) + "]", i + 7);
	}
	headShader.setFloat(phoenix::G_AMBIENT_FACTOR, 0.4f);
	headShader.setFloat(phoenix::G_SPECULAR_FACTOR, 5.0f);
	headShader.setVec3(phoenix::G_LIGHT_COLOR, glm::vec3(1.0f));
	phoenix::Shader shadowMapPassShader("../Resources/Shaders/skin/shadow_map_pass.vs", "../Resources/Shaders/skin/shadow_map_pass.fs");
	phoenix::Shader textureSpaceInputsPassShader("../Resources/Shaders/skin/texture_space_inputs_pass.vs", "../Resources/Shaders/skin/texture_space_inputs_pass.fs");
	textureSpaceInputsPassShader.use();
	textureSpaceInputsPassShader.setInt(phoenix::G_DIFFUSE_TEXTURE, 0);
	phoenix::Shader convolveStretchUShader("../Resources/Shaders/skin/render_quad.vs", "../Resources/Shaders/skin/convolve_stretch_u.fs");
	convolveStretchUShader.use();
	convolveStretchUShader.setInt(phoenix::G_STRETCH_MAP, 0);
	phoenix::Shader convolveStretchVShader("../Resources/Shaders/skin/render_quad.vs", "../Resources/Shaders/skin/convolve_stretch_v.fs");
	convolveStretchVShader.use();
	convolveStretchVShader.setInt(phoenix::G_STRETCH_MAP, 0);
	phoenix::Shader convolveUShader("../Resources/Shaders/skin/render_quad.vs", "../Resources/Shaders/skin/convolve_u.fs");
	convolveUShader.use();
	convolveUShader.setInt(phoenix::G_IRRADIANCE_MAP, 0);
	convolveUShader.setInt(phoenix::G_STRETCH_MAP, 1);
	phoenix::Shader convolveVShader("../Resources/Shaders/skin/render_quad.vs", "../Resources/Shaders/skin/convolve_v.fs");
	convolveVShader.use();
	convolveVShader.setInt(phoenix::G_IRRADIANCE_MAP, 0);
	convolveVShader.setInt(phoenix::G_STRETCH_MAP, 1);
	phoenix::Shader renderQuadShader("../Resources/Shaders/skin/render_quad.vs", "../Resources/Shaders/skin/render_quad.fs");
	renderQuadShader.use();
	renderQuadShader.setInt(phoenix::G_RENDER_TARGET, 0);
	phoenix::Shader debugLinesShader("../Resources/Shaders/skin/debug_lines.vs", "../Resources/Shaders/skin/debug_lines.fs");

	phoenix::Model head("../Resources/Objects/head/head.OBJ");

	glBindFramebuffer(GL_FRAMEBUFFER, shadowMapRenderTarget->_FBO);
	glDrawBuffer(GL_COLOR_ATTACHMENT0);

	glBindFramebuffer(GL_FRAMEBUFFER, textureSpaceInputs->_FBO);
	irradianceMap = textureSpaceInputs->genAttachment(GL_RGBA32F, GL_RGBA, GL_FLOAT);
	GLenum bufs[2] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1 };
	glDrawBuffers(2, bufs);

	for (size_t i = 0; i < NUM_BLUR_PASSES; ++i)
	{
		glBindFramebuffer(GL_FRAMEBUFFER, blurStretchPassesRenderTargets[i]->_FBO);
		blurredStretchMaps[i] = blurStretchPassesRenderTargets[i]->genAttachment(GL_RGBA32F, GL_RGBA, GL_FLOAT);
		glDrawBuffers(2, bufs);
	}

	for (size_t i = 0; i < NUM_BLUR_PASSES; ++i)
	{
		glBindFramebuffer(GL_FRAMEBUFFER, blurPassesRenderTargets[i]->_FBO);
		blurredIrradianceMaps[i] = blurPassesRenderTargets[i]->genAttachment(GL_RGBA32F, GL_RGBA, GL_FLOAT);
		glDrawBuffers(2, bufs);
	}

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

		execShadowMapPass(shadowMapPassShader, head);
		execTextureSpaceInputsPass(textureSpaceInputsPassShader, head);
		execBlurPasses(convolveStretchUShader, convolveStretchVShader, convolveUShader, convolveVShader);

		glViewport(0, 0, phoenix::SCREEN_WIDTH, phoenix::SCREEN_HEIGHT);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		// Render pass
		if (shadowCommon->_renderMode == 1)
		{
			utils->renderQuad(renderQuadShader, irradianceMap);
		}
		else if (shadowCommon->_renderMode == 2)
		{
			utils->renderQuad(renderQuadShader, blurredIrradianceMaps[0]);
		}
		else if (shadowCommon->_renderMode == 3)
		{
			utils->renderQuad(renderQuadShader, blurredIrradianceMaps[1]);
		}
		else if (shadowCommon->_renderMode == 4)
		{
			utils->renderQuad(renderQuadShader, blurredIrradianceMaps[2]);
		}
		else if (shadowCommon->_renderMode == 5)
		{
			utils->renderQuad(renderQuadShader, blurredIrradianceMaps[3]);
		}
		else if (shadowCommon->_renderMode == 6)
		{
			utils->renderQuad(renderQuadShader, blurredIrradianceMaps[4]);
		}
		else
		{
			execRenderPass(floorShader, headShader, head);
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
	glBindTexture(GL_TEXTURE_2D, shadowMapRenderTarget->_textureID);
	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_3D, anglesTexture);
	glActiveTexture(GL_TEXTURE3);
	glBindTexture(GL_TEXTURE_2D, normalMap);
	glActiveTexture(GL_TEXTURE4);
	glBindTexture(GL_TEXTURE_2D, beckmannTexture);
	glActiveTexture(GL_TEXTURE5);
	glBindTexture(GL_TEXTURE_2D, textureSpaceInputs->_textureID);
	glActiveTexture(GL_TEXTURE6);
	glBindTexture(GL_TEXTURE_2D, specularTexture);
	for (size_t i = 0; i <= NUM_BLUR_PASSES; ++i)
	{
		glActiveTexture(GL_TEXTURE7 + i);
		if (i > 0)
		{
			glBindTexture(GL_TEXTURE_2D, blurredIrradianceMaps[i - 1]);
		}
		else
		{
			glBindTexture(GL_TEXTURE_2D, irradianceMap);
		}
	}
}

void execShadowMapPass(const phoenix::Shader& shader, phoenix::Model& object)
{
	shadowCommon->setLightSpaceVP(shader, shadowCommon->_lightPos, phoenix::TARGET);
	glViewport(0, 0, phoenix::HIGH_RES_WIDTH, phoenix::HIGH_RES_HEIGHT);
	glBindFramebuffer(GL_FRAMEBUFFER, shadowMapRenderTarget->_FBO);

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	utils->renderPlane(shader);
	shadowCommon->renderObject(utils, shader, object, TRANSLATION, ROTATION, SCALE);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void execTextureSpaceInputsPass(const phoenix::Shader& shader, phoenix::Model& object)
{
	glViewport(0, 0, phoenix::HIGH_RES_WIDTH, phoenix::HIGH_RES_HEIGHT);
	glBindFramebuffer(GL_FRAMEBUFFER, textureSpaceInputs->_FBO);

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	shadowCommon->changeColorTexture(shadowCommon->_objectTexture);
	shadowCommon->renderObject(utils, shader, object, TRANSLATION, ROTATION, SCALE);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void execBlurPasses(const phoenix::Shader& convolveStretchUShader, const phoenix::Shader& convolveStretchVShader, const phoenix::Shader& convolveUShader, const phoenix::Shader& convolveVShader)
{
	glViewport(0, 0, phoenix::HIGH_RES_WIDTH, phoenix::HIGH_RES_HEIGHT);
	for (size_t i = 0; i < NUM_BLUR_PASSES; ++i)
	{
		glBindFramebuffer(GL_FRAMEBUFFER, blurStretchPassesRenderTargets[i]->_FBO);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		convolveStretchUShader.use();
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, i > 0 ? blurredStretchMaps[i - 1] : textureSpaceInputs->_textureID);
		utils->renderQuad(convolveStretchUShader);

		convolveStretchVShader.use();
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, blurStretchPassesRenderTargets[i]->_textureID);
		utils->renderQuad(convolveStretchVShader);

		glBindFramebuffer(GL_FRAMEBUFFER, blurPassesRenderTargets[i]->_FBO);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		convolveUShader.use();
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, i > 0 ? blurredIrradianceMaps[i - 1] : irradianceMap);
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, blurredStretchMaps[i]);
		utils->renderQuad(convolveUShader);

		convolveVShader.use();
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, blurPassesRenderTargets[i]->_textureID);
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, blurredStretchMaps[i]);
		utils->renderQuad(convolveVShader);
	}
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void execRenderPass(const phoenix::Shader& floorShader, const phoenix::Shader& headShader, phoenix::Model& object)
{
	setInputs(floorShader, false);
	utils->renderPlane(floorShader);
	setInputs(headShader, true);
	shadowCommon->renderObject(utils, headShader, object, TRANSLATION, ROTATION, SCALE);
}

void initPointers()
{
	shadowCommon = new phoenix::ShadowCommon();
	shadowMapRenderTarget = new phoenix::Framebuffer(phoenix::HIGH_RES_WIDTH, phoenix::HIGH_RES_HEIGHT, true);
	textureSpaceInputs = new phoenix::Framebuffer(phoenix::HIGH_RES_WIDTH, phoenix::HIGH_RES_HEIGHT, false, true);
	for (size_t i = 0; i < NUM_BLUR_PASSES; ++i)
	{
		blurStretchPassesRenderTargets[i] = new phoenix::Framebuffer(phoenix::HIGH_RES_WIDTH, phoenix::HIGH_RES_HEIGHT, false, true);
	}
	for (size_t i = 0; i < NUM_BLUR_PASSES; ++i)
	{
		blurPassesRenderTargets[i] = new phoenix::Framebuffer(phoenix::HIGH_RES_WIDTH, phoenix::HIGH_RES_HEIGHT, false, true);
	}
	utils = new phoenix::Utils();
	camera = new phoenix::Camera();
}

void deletePointers()
{
	delete camera;
	delete utils;
	for (size_t i = 0; i < NUM_BLUR_PASSES; ++i)
	{
		delete blurPassesRenderTargets[i];
	}
	for (size_t i = 0; i < NUM_BLUR_PASSES; ++i)
	{
		delete blurStretchPassesRenderTargets[i];
	}
	delete textureSpaceInputs;
	delete shadowMapRenderTarget;
	delete shadowCommon;
}
#include <glm/gtc/matrix_transform.hpp>

#include <engine/shadow_common.h>
#include <engine/strings.h>
#include <engine/common.h>

#include <array>
#include <iostream>

struct BoundingBox
{
	float _l, _r, _b, _t, _n, _f;
};

void framebufferSizeCallback(GLFWwindow*, int, int);
void cursorPosCallback(GLFWwindow*, double, double);
void scrollCallback(GLFWwindow*, double, double);

void setupRenderToTexture();
void execShadowMapPass(const phoenix::Shader&, phoenix::Model&);
void execRenderPass(const phoenix::Shader&, phoenix::Model&);
void setLightSpaceVP(const phoenix::Shader&, unsigned int, bool);
void calcOrthoProjs();
void setClipSpaceCascadeEnds(const phoenix::Shader&);
void bindZBufferForWriting(unsigned int);
void bindZBufferForReading();
void initPointers();
void deletePointers();

const float CORRECTION_FACTOR = 80.0f;
const std::array<float, 4> CASCADE_ENDS{ {phoenix::PERSPECTIVE_NEAR_PLANE, 5.0f, 10.0f, phoenix::PERSPECTIVE_FAR_PLANE} };

phoenix::Camera* camera;
phoenix::Utils* utils;
phoenix::ShadowCommon* shadowCommon;
GLFWwindow* window;

float lastX = static_cast<float>(phoenix::SCREEN_WIDTH) / 2.0f;
float lastY = static_cast<float>(phoenix::SCREEN_HEIGHT) / 2.0f;

bool calibratedCursor = false;

unsigned int FBO;
std::array<unsigned int, 3> shadowMaps;
std::array<BoundingBox, 3> shadowOrthoProjInfo;

int main()
{
	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	window = glfwCreateWindow(phoenix::SCREEN_WIDTH, phoenix::SCREEN_HEIGHT, "Cascaded Exponential Shadow Mapping", nullptr, nullptr);
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

	phoenix::Shader renderPassShader("../Resources/Shaders/shadow_mapping/csm_render_pass.vs", "../Resources/Shaders/shadow_mapping/csm_render_pass.fs");
	renderPassShader.use();
	renderPassShader.setInt(phoenix::G_DIFFUSE_TEXTURE, 0);
	renderPassShader.setInt(phoenix::G_SHADOW_MAP + "[0]", 1);
	renderPassShader.setInt(phoenix::G_SHADOW_MAP + "[1]", 2);
	renderPassShader.setInt(phoenix::G_SHADOW_MAP + "[2]", 3);
	renderPassShader.setVec3(phoenix::G_LIGHT_COLOR, phoenix::LIGHT_COLOR);
	renderPassShader.setFloat(phoenix::G_AMBIENT_FACTOR, phoenix::AMBIENT_FACTOR);
	renderPassShader.setFloat(phoenix::G_SPECULAR_FACTOR, phoenix::SPECULAR_FACTOR);
	renderPassShader.setFloat(phoenix::G_CORRECTION_FACTOR, CORRECTION_FACTOR);
	phoenix::Shader shadowMapPassShader("../Resources/Shaders/shadow_mapping/csm_shadow_map_pass.vs", "../Resources/Shaders/shadow_mapping/csm_shadow_map_pass.fs");
	phoenix::Shader renderQuadShader("../Resources/Shaders/shadow_mapping/render_quad.vs", "../Resources/Shaders/shadow_mapping/z_buffer.fs");
	renderQuadShader.use();
	renderQuadShader.setInt(phoenix::G_DEPTH_MAP, 0);

	phoenix::Model dragon("../Resources/Objects/dragon/dragon.obj");

	while (!glfwWindowShouldClose(window))
	{
		utils->_projection = glm::perspectiveLH(glm::radians(camera->_FOV), static_cast<float>(phoenix::SCREEN_WIDTH) / phoenix::SCREEN_HEIGHT, phoenix::PERSPECTIVE_NEAR_PLANE, phoenix::PERSPECTIVE_FAR_PLANE);
		utils->_view = camera->getViewMatrix();

		float currentTime = glfwGetTime();
		shadowCommon->_deltaTime = currentTime - shadowCommon->_lastTimestamp;
		shadowCommon->_lastTimestamp = currentTime;

		shadowCommon->processInput(window, camera, false);

		glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		// Shadow map pass
		execShadowMapPass(shadowMapPassShader, dragon);

		glViewport(0, 0, phoenix::SCREEN_WIDTH, phoenix::SCREEN_HEIGHT);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		// Render pass
		if (!shadowCommon->_renderMode || shadowCommon->_renderMode > shadowMaps.size())
		{
			execRenderPass(renderPassShader, dragon);
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

	float xOffset = lastX - x;
	float yOffset = y - lastY;

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
		glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32, phoenix::SHADOW_MAP_WIDTH, phoenix::SHADOW_MAP_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_NONE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
		glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, phoenix::BORDER_COLOR);
	}

	// Attach the render targets to the FBO so we can write to them
	glGenFramebuffers(1, &FBO);
	glBindFramebuffer(GL_FRAMEBUFFER, FBO);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, shadowMaps[0], 0);
	glDrawBuffer(GL_NONE);
	glReadBuffer(GL_NONE);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void setLightSpaceVP(const phoenix::Shader& shader, unsigned int cascadeIndex, bool isRenderPass)
{
	shader.use();
	glm::mat4 lightProjection, lightView, lightSpaceVP;
	BoundingBox bb = shadowOrthoProjInfo[cascadeIndex];
	lightProjection = glm::orthoLH(bb._l, bb._r, bb._b, bb._t, bb._n, bb._f);
	lightView = glm::lookAtLH(shadowCommon->_lightPos, phoenix::TARGET, phoenix::UP);
	lightSpaceVP = lightProjection * lightView;
	if (isRenderPass)
	{
		shader.setMat4(phoenix::G_LIGHT_SPACE_VP + "[" + std::to_string(cascadeIndex) + "]", lightSpaceVP);
	}
	else
	{
		shader.setMat4(phoenix::G_LIGHT_SPACE_VP, lightSpaceVP);
	}
}

void execShadowMapPass(const phoenix::Shader& shader, phoenix::Model& object)
{
	calcOrthoProjs();
	glViewport(0, 0, phoenix::SHADOW_MAP_WIDTH, phoenix::SHADOW_MAP_HEIGHT);

	for (size_t i = 0; i < shadowMaps.size(); ++i)
	{
		bindZBufferForWriting(i);
		glClear(GL_DEPTH_BUFFER_BIT);

		setLightSpaceVP(shader, i, false);

		shadowCommon->renderScene(utils, shader, object);
	}

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void execRenderPass(const phoenix::Shader& shader, phoenix::Model& object)
{
	shadowCommon->setUniforms(shader, camera);
	setClipSpaceCascadeEnds(shader);
	for (size_t i = 0; i < shadowMaps.size(); ++i)
	{
		setLightSpaceVP(shader, i, true);
	}

	shadowCommon->changeColorTexture(shadowCommon->_floorTexture);
	bindZBufferForReading();

	shadowCommon->renderScene(utils, shader, object);
}

void calcOrthoProjs()
{
	glm::mat4 cameraInverse = glm::inverse(camera->getViewMatrix());
	glm::mat4 lightView = glm::lookAtLH(shadowCommon->_lightPos, phoenix::TARGET, phoenix::UP);

	float aspect = static_cast<float>(phoenix::SCREEN_HEIGHT) / phoenix::SCREEN_WIDTH;
	float tanHalfHFOV = glm::tan(glm::radians(camera->_FOV / 2.0f));
	float tanHalfVFOV = glm::tan(glm::radians(camera->_FOV * aspect / 2.0f));

	for (size_t i = 0; i < shadowMaps.size(); ++i)
	{
		float xn = CASCADE_ENDS[i] * tanHalfHFOV;
		float xf = CASCADE_ENDS[i + 1] * tanHalfHFOV;
		float yn = CASCADE_ENDS[i] * tanHalfVFOV;
		float yf = CASCADE_ENDS[i + 1] * tanHalfVFOV;

		std::array<glm::vec4, phoenix::NUM_FRUSTUM_CORNERS> frustumCorners{ {
			// Near plane
			glm::vec4(xn, yn, CASCADE_ENDS[i], 1.0f),
			glm::vec4(-xn, yn, CASCADE_ENDS[i], 1.0f),
			glm::vec4(xn, -yn, CASCADE_ENDS[i], 1.0f),
			glm::vec4(-xn, -yn, CASCADE_ENDS[i], 1.0f),

			// Far plane
			glm::vec4(xf, yf, CASCADE_ENDS[i + 1], 1.0f),
			glm::vec4(-xf, yf, CASCADE_ENDS[i + 1], 1.0f),
			glm::vec4(xf, -yf, CASCADE_ENDS[i + 1], 1.0f),
			glm::vec4(-xf, -yf, CASCADE_ENDS[i + 1], 1.0f)
		} };

		std::array<glm::vec4, phoenix::NUM_FRUSTUM_CORNERS> frustumCornersL;

		float minX = std::numeric_limits<float>::max();
		float maxX = std::numeric_limits<float>::min();
		float minY = std::numeric_limits<float>::max();
		float maxY = std::numeric_limits<float>::min();
		float minZ = std::numeric_limits<float>::max();
		float maxZ = std::numeric_limits<float>::min();

		for (size_t j = 0; j < phoenix::NUM_FRUSTUM_CORNERS; ++i)
		{
			// Transform the frustum coordinate from view to world space
			glm::vec4 vW = cameraInverse * frustumCorners[j];

			// Transform the frustum coordinate from world to light space
			frustumCornersL[j] = lightView * vW;

			minX = std::min(minX, frustumCornersL[j].x);
			maxX = std::max(maxX, frustumCornersL[j].x);
			minY = std::min(minY, frustumCornersL[j].y);
			maxY = std::max(maxY, frustumCornersL[j].y);
			minZ = std::min(minZ, frustumCornersL[j].z);
			maxZ = std::max(maxZ, frustumCornersL[j].z);
		}

		shadowOrthoProjInfo[i]._l = minX;
		shadowOrthoProjInfo[i]._r = maxX;
		shadowOrthoProjInfo[i]._b = minY;
		shadowOrthoProjInfo[i]._t = maxY;
		shadowOrthoProjInfo[i]._n = minZ;
		shadowOrthoProjInfo[i]._f = maxZ;
	}
}

void setClipSpaceCascadeEnds(const phoenix::Shader& shader)
{
	for (int i = 0; i < shadowMaps.size(); ++i)
	{
		glm::vec4 viewSpaceCascadeEnd(0.0f, 0.0f, CASCADE_ENDS[i + 1], 1.0f);
		glm::vec4 clipSpaceCascadeEnd = utils->_projection * viewSpaceCascadeEnd;
		shader.setFloat("gClipSpaceCascadeEnds[" + std::to_string(i) + "]", clipSpaceCascadeEnd.z);
	}
}

void bindZBufferForWriting(unsigned int cascadeIndex)
{
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, FBO);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, shadowMaps[cascadeIndex], 0);
}

void bindZBufferForReading()
{
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, shadowMaps[0]);

	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, shadowMaps[1]);

	glActiveTexture(GL_TEXTURE3);
	glBindTexture(GL_TEXTURE_2D, shadowMaps[2]);
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
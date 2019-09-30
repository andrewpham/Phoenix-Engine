#include <glm/gtc/matrix_transform.hpp>

#include <engine/common.h>
#include <engine/strings.h>
#include <engine/framebuffer.h>
#include <engine/light.h>
#include <engine/shadow_common.h>

#include <array>
#include <iostream>

void framebufferSizeCallback(GLFWwindow*, int, int);
void cursorPosCallback(GLFWwindow*, double, double);
void scrollCallback(GLFWwindow*, double, double);

void setLightSpaceVP(const phoenix::Shader&);
void renderObject(const phoenix::Shader&, phoenix::Model&);
void execShadowMapPass(const phoenix::Shader&, phoenix::Model&);
void execGeometryPass(const phoenix::Shader&, phoenix::Model&);
void execLightingPass(const phoenix::Shader&);
void execBlurPasses(const phoenix::Shader&);
void initPointers();
void deletePointers();

phoenix::Camera* camera;
phoenix::Utils* utils;
phoenix::Framebuffer* shadowMapRenderTarget;
phoenix::Framebuffer* gBuffer;
phoenix::Framebuffer* blurRenderTarget;
phoenix::ShadowCommon* shadowCommon;
GLFWwindow* window;

float lastX = static_cast<float>(phoenix::SCREEN_WIDTH) / 2.0f;
float lastY = static_cast<float>(phoenix::SCREEN_HEIGHT) / 2.0f;

bool calibratedCursor = false;

unsigned int normalMap, albedoSpecularMap, previousFrameMap;
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

	directLight._color = glm::vec3(0.98f, 0.8392f, 0.647f);

	phoenix::Shader shadowMapPassShader("../Resources/Shaders/god_rays/shadow_map_pass.vs", "../Resources/Shaders/god_rays/shadow_map_pass.fs");
	phoenix::Shader gBufferPassShader("../Resources/Shaders/god_rays/g_buffer_pass.vs", "../Resources/Shaders/god_rays/g_buffer_pass.fs");
	phoenix::Shader lightingPassShader("../Resources/Shaders/god_rays/render_quad.vs", "../Resources/Shaders/god_rays/lighting_pass.fs");
	lightingPassShader.use();
	lightingPassShader.setInt(phoenix::G_POSITION_MAP, 0);
	lightingPassShader.setInt(phoenix::G_NORMAL_MAP, 1);
	lightingPassShader.setInt(phoenix::G_ALBEDO_SPECULAR_MAP, 2);
	lightingPassShader.setInt(phoenix::G_SHADOW_MAP, 3);
	lightingPassShader.setFloat(phoenix::G_AMBIENT_FACTOR, 0.1f);
	phoenix::Shader blurShader("../Resources/Shaders/god_rays/render_quad.vs", "../Resources/Shaders/god_rays/blur.fs");
	blurShader.use();
	blurShader.setInt(phoenix::G_PREVIOUS_FRAME_MAP, 0);
	phoenix::Shader renderQuadShader("../Resources/Shaders/god_rays/render_quad.vs", "../Resources/Shaders/god_rays/render_quad.fs");
	renderQuadShader.use();
	renderQuadShader.setInt(phoenix::G_RENDER_TARGET, 0);

	phoenix::Model sponza("../Resources/Objects/sponza/sponza.obj");

	glBindFramebuffer(GL_FRAMEBUFFER, shadowMapRenderTarget->_FBO);
	glDrawBuffer(GL_COLOR_ATTACHMENT0);

	glBindFramebuffer(GL_FRAMEBUFFER, gBuffer->_FBO);
	normalMap = gBuffer->genAttachment(GL_RGB16F, GL_RGB, GL_FLOAT);
	albedoSpecularMap = gBuffer->genAttachment(GL_RGBA, GL_RGBA, GL_UNSIGNED_BYTE);
	previousFrameMap = gBuffer->genAttachment(GL_RGBA, GL_RGBA, GL_UNSIGNED_BYTE);
	GLenum bufs[4] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2, GL_COLOR_ATTACHMENT3 };
	glDrawBuffers(4, bufs);

	glBindFramebuffer(GL_FRAMEBUFFER, blurRenderTarget->_FBO);
	glDrawBuffer(GL_COLOR_ATTACHMENT0);

	while (!glfwWindowShouldClose(window))
	{
		utils->_projection = glm::perspectiveLH(glm::radians(camera->_FOV), static_cast<float>(phoenix::SCREEN_WIDTH) / phoenix::SCREEN_HEIGHT, phoenix::PERSPECTIVE_NEAR_PLANE, phoenix::PERSPECTIVE_FAR_PLANE);
		utils->_view = camera->getViewMatrix();

		float currentTime = glfwGetTime();
		shadowCommon->_deltaTime = currentTime - shadowCommon->_lastTimestamp;
		shadowCommon->_lastTimestamp = currentTime;

		shadowCommon->processInput(window, camera, false);

		glClearColor(1.0f, 1.0f, 1.0f, 1.0f);

		execShadowMapPass(shadowMapPassShader, sponza);

		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		if (shadowCommon->_renderMode == 1)
		{
			utils->renderQuad(renderQuadShader, shadowMapRenderTarget->_textureID);
		}
		else
		{
			execGeometryPass(gBufferPassShader, sponza);
			execLightingPass(lightingPassShader);
			execBlurPasses(blurShader);
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

void setLightSpaceVP(const phoenix::Shader& shader)
{
	shader.use();

	glm::vec3 lightViewForward = glm::normalize(directLight._direction);
	glm::vec3 lightViewRight = glm::normalize(glm::cross(phoenix::UP, lightViewForward));
	glm::vec3 lightViewUp = glm::normalize(glm::cross(lightViewForward, lightViewRight));
	glm::mat3 rotation;
	rotation[0] = lightViewForward;
	rotation[1] = lightViewRight;
	rotation[2] = lightViewUp;

	std::array<glm::vec3, phoenix::NUM_FRUSTUM_CORNERS> frustumCorners{ {
			glm::vec3(-phoenix::FRUSTUM_HALF_WIDTH, -phoenix::FRUSTUM_HALF_WIDTH, phoenix::FRUSTUM_HALF_WIDTH),
			glm::vec3(-phoenix::FRUSTUM_HALF_WIDTH, -phoenix::FRUSTUM_HALF_WIDTH, -phoenix::FRUSTUM_HALF_WIDTH),
			glm::vec3(-phoenix::FRUSTUM_HALF_WIDTH, phoenix::FRUSTUM_HALF_WIDTH, phoenix::FRUSTUM_HALF_WIDTH),
			glm::vec3(-phoenix::FRUSTUM_HALF_WIDTH, phoenix::FRUSTUM_HALF_WIDTH, -phoenix::FRUSTUM_HALF_WIDTH),
			glm::vec3(phoenix::FRUSTUM_HALF_WIDTH, -phoenix::FRUSTUM_HALF_WIDTH, phoenix::FRUSTUM_HALF_WIDTH),
			glm::vec3(phoenix::FRUSTUM_HALF_WIDTH, -phoenix::FRUSTUM_HALF_WIDTH, -phoenix::FRUSTUM_HALF_WIDTH),
			glm::vec3(phoenix::FRUSTUM_HALF_WIDTH, phoenix::FRUSTUM_HALF_WIDTH, phoenix::FRUSTUM_HALF_WIDTH),
			glm::vec3(phoenix::FRUSTUM_HALF_WIDTH, phoenix::FRUSTUM_HALF_WIDTH, -phoenix::FRUSTUM_HALF_WIDTH)
		} };

	for (size_t i = 0; i < phoenix::NUM_FRUSTUM_CORNERS; ++i)
	{
		frustumCorners[i] = frustumCorners[i] * rotation;
	}

	float minX = frustumCorners[0].x, minY = frustumCorners[0].y, minZ = frustumCorners[0].z, maxX = frustumCorners[0].x, maxY = frustumCorners[0].y, maxZ = frustumCorners[0].z;
	for (size_t i = 0; i < phoenix::NUM_FRUSTUM_CORNERS; ++i)
	{
		if (frustumCorners[i].x < minX)
		{
			minX = frustumCorners[i].x;
		}
		if (frustumCorners[i].x > maxX)
		{
			maxX = frustumCorners[i].x;
		}
		if (frustumCorners[i].y < minY)
		{
			minY = frustumCorners[i].y;
		}
		if (frustumCorners[i].y > maxY)
		{
			maxY = frustumCorners[i].y;
		}
		if (frustumCorners[i].z < minZ)
		{
			minZ = frustumCorners[i].z;
		}
		if (frustumCorners[i].z > maxZ)
		{
			maxZ = frustumCorners[i].z;
		}
	}

	glm::mat4 lightView(rotation);
	lightView[3][0] = -(minX + maxX) * 0.5f;
	lightView[3][1] = -(minY + maxY) * 0.5f;
	lightView[3][2] = -(minZ + maxZ) * 0.5f;
	lightView[0][3] = 0.0f;
	lightView[1][3] = 0.0f;
	lightView[2][3] = 0.0f;
	lightView[3][3] = 1.0f;

	glm::vec3 frustumExtents(maxX - minX, maxY - minY, maxZ - minZ);
	glm::vec3 halfExtents = frustumExtents * 0.5f;

	glm::mat4 lightSpaceVP = glm::orthoLH(-halfExtents.x, halfExtents.x, -halfExtents.y, halfExtents.y, -halfExtents.z, halfExtents.z) * lightView;
	shader.setMat4(phoenix::G_LIGHT_SPACE_VP, lightSpaceVP);
}

void renderObject(const phoenix::Shader& shader, phoenix::Model& object)
{
	glm::mat4 world = glm::mat4(1.0f);
	world = glm::scale(world, glm::vec3(phoenix::OBJECT_SCALE));
	shader.setMat4(phoenix::G_WORLD_MATRIX, world);
	shader.setMat3(phoenix::G_NORMAL_MATRIX, glm::transpose(glm::inverse(glm::mat3(world))));
	object.render(shader);
}

void execShadowMapPass(const phoenix::Shader& shader, phoenix::Model& object)
{
	glViewport(0, 0, phoenix::HIGH_RES_WIDTH, phoenix::HIGH_RES_HEIGHT);
	glBindFramebuffer(GL_FRAMEBUFFER, shadowMapRenderTarget->_FBO);

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	setLightSpaceVP(shader);
	renderObject(shader, object);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glViewport(0, 0, phoenix::SCREEN_WIDTH, phoenix::SCREEN_HEIGHT);
}

void execGeometryPass(const phoenix::Shader& shader, phoenix::Model& object)
{
	glBindFramebuffer(GL_FRAMEBUFFER, gBuffer->_FBO);

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	shader.use();
	shader.setMat4(phoenix::G_VP, utils->_projection * utils->_view);
	shader.setMat4(phoenix::G_VIEW_MATRIX, utils->_view);
	renderObject(shader, object);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void execLightingPass(const phoenix::Shader& shader)
{
	glBindFramebuffer(GL_FRAMEBUFFER, gBuffer->_FBO);

	setLightSpaceVP(shader);
	shader.setVec3(phoenix::G_VIEW_POS, camera->_position);
	shader.setMat4(phoenix::G_INVERSE_VIEW_MATRIX, glm::inverse(utils->_view));
	directLight.setUniforms(shader);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, gBuffer->_textureID);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, normalMap);
	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, albedoSpecularMap);
	glActiveTexture(GL_TEXTURE3);
	glBindTexture(GL_TEXTURE_2D, shadowMapRenderTarget->_textureID);
	utils->renderQuad(shader);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void execBlurPasses(const phoenix::Shader& blurShader)
{
	blurShader.use();

	glBindFramebuffer(GL_FRAMEBUFFER, blurRenderTarget->_FBO);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	blurShader.setVec2(phoenix::G_DIR, glm::vec2(1.0f, 0.0f));
	utils->renderQuad(blurShader, previousFrameMap);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	blurShader.setVec2(phoenix::G_DIR, glm::vec2(0.0f, 1.0f));
	utils->renderQuad(blurShader, blurRenderTarget->_textureID);
}

void initPointers()
{
	shadowCommon = new phoenix::ShadowCommon();
	blurRenderTarget = new phoenix::Framebuffer(phoenix::SCREEN_WIDTH, phoenix::SCREEN_HEIGHT);
	gBuffer = new phoenix::Framebuffer(phoenix::SCREEN_WIDTH, phoenix::SCREEN_HEIGHT);
	shadowMapRenderTarget = new phoenix::Framebuffer(phoenix::HIGH_RES_WIDTH, phoenix::HIGH_RES_HEIGHT, true);
	utils = new phoenix::Utils();
	camera = new phoenix::Camera();
}

void deletePointers()
{
	delete camera;
	delete utils;
	delete shadowMapRenderTarget;
	delete gBuffer;
	delete blurRenderTarget;
	delete shadowCommon;
}
#include <engine/shadow_common.h>
#include <engine/strings.h>
#include <engine/common.h>
#include <glm/gtc/matrix_transform.hpp>

namespace phoenix
{
	void ShadowCommon::processInput(GLFWwindow* window, Camera* camera, bool isRH)
	{
		if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
		{
			glfwSetWindowShouldClose(window, true);
		}

		if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
		{
			camera->processKeyPress(isRH ? FORWARD : BACKWARD, _deltaTime);
		}
		if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
		{
			camera->processKeyPress(isRH ? BACKWARD : FORWARD, _deltaTime);
		}
		if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
		{
			camera->processKeyPress(LEFT, _deltaTime);
		}
		if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
		{
			camera->processKeyPress(RIGHT, _deltaTime);
		}

		int dir = isRH ? 1 : -1;
		if (glfwGetKey(window, GLFW_KEY_I) == GLFW_PRESS)
		{
			_lightPos += glm::vec3(0.0f, 1.0f, 0.0f) * MOVEMENT_SPEED * _deltaTime;
		}
		if (glfwGetKey(window, GLFW_KEY_K) == GLFW_PRESS)
		{
			_lightPos += glm::vec3(0.0f, -1.0f, 0.0f) * MOVEMENT_SPEED * _deltaTime;
		}
		if (glfwGetKey(window, GLFW_KEY_J) == GLFW_PRESS)
		{
			_lightPos += glm::vec3(dir * -1.0f, 0.0f, 0.0f) * MOVEMENT_SPEED * _deltaTime;
		}
		if (glfwGetKey(window, GLFW_KEY_L) == GLFW_PRESS)
		{
			_lightPos += glm::vec3(dir * 1.0f, 0.0f, 0.0f) * MOVEMENT_SPEED * _deltaTime;
		}
		if (glfwGetKey(window, GLFW_KEY_Y) == GLFW_PRESS)
		{
			_lightPos += glm::vec3(0.0f, 0.0f, dir * -1.0f) * MOVEMENT_SPEED * _deltaTime;
		}
		if (glfwGetKey(window, GLFW_KEY_H) == GLFW_PRESS)
		{
			_lightPos += glm::vec3(0.0f, 0.0f, dir * 1.0f) * MOVEMENT_SPEED * _deltaTime;
		}

		if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS)
		{
			_renderMode = 0;
		}
		if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS)
		{
			_renderMode = 1;
		}

		if (glfwGetKey(window, GLFW_KEY_1) == GLFW_PRESS)
		{
			_renderMode = 1;
		}
		if (glfwGetKey(window, GLFW_KEY_2) == GLFW_PRESS)
		{
			_renderMode = 2;
		}
		if (glfwGetKey(window, GLFW_KEY_3) == GLFW_PRESS)
		{
			_renderMode = 3;
		}
		if (glfwGetKey(window, GLFW_KEY_4) == GLFW_PRESS)
		{
			_renderMode = 4;
		}
		if (glfwGetKey(window, GLFW_KEY_5) == GLFW_PRESS)
		{
			_renderMode = 5;
		}
		if (glfwGetKey(window, GLFW_KEY_6) == GLFW_PRESS)
		{
			_renderMode = 6;
		}
	}

	void ShadowCommon::changeColorTexture(unsigned int texture)
	{
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, texture);
	}

	void ShadowCommon::renderObject(const Utils* utils, const Shader& shader, Model& object, glm::vec3 translation, float rotation)
	{
		shader.use();
		glm::mat4 world = glm::mat4(1.0f);
		world = glm::translate(world, translation);
		world = glm::scale(world, OBJ_SCALE);
		world = glm::rotate(world, glm::radians(rotation), UP);
		shader.setMat4(G_WVP, utils->_projection * utils->_view * world);
		shader.setMat4(G_WORLD_MATRIX, world);
		shader.setMat3(G_NORMAL_MATRIX, glm::transpose(glm::inverse(glm::mat3(world))));
		object.render();
	}

	void ShadowCommon::renderScene(Utils* utils, const Shader& shader, Model& object)
	{
		utils->renderPlane(shader);
		changeColorTexture(_objectTexture);
		renderObject(utils, shader, object, glm::vec3(1.0f, 0.2f, 2.0f), 180.0f);
		renderObject(utils, shader, object, glm::vec3(1.0f, 0.2f, -3.0f), 180.0f);
		changeColorTexture(_altObjTexture);
		renderObject(utils, shader, object, glm::vec3(1.0f, 0.2f, -8.0f), 180.0f);
		renderObject(utils, shader, object, glm::vec3(-0.8f, 0.8f, 2.3f), 90.0f);
		changeColorTexture(_objectTexture);
		renderObject(utils, shader, object, glm::vec3(-3.5f, 1.8f, 2.0f), 0.0f);
	}

	void ShadowCommon::setUniforms(const Shader& shader, const Camera* camera)
	{
		shader.use();
		shader.setVec3(G_VIEW_POS, camera->_position);
		shader.setVec3("gLightPos", _lightPos);
		shader.setInt("gRenderMode", _renderMode);
	}

	void ShadowCommon::renderDebugLines(const Shader& shader, Utils* utils)
	{
		shader.use();
		glm::mat4 world = glm::mat4(1.0f);
		world = glm::translate(world, _lightPos);
		shader.setMat4(G_WVP, utils->_projection * utils->_view * world);

		// Visualize orthographic projection bounds
		glm::vec3 lightViewForward(glm::normalize(glm::vec3(0.0f) - _lightPos));
		glm::vec3 lightViewRight = glm::normalize(glm::cross(glm::vec3(0.0f, 1.0f, 0.0f), lightViewForward));
		glm::vec3 lightViewUp = glm::normalize(glm::cross(lightViewForward, lightViewRight));
		glm::mat4 lightView = glm::lookAt(_lightPos, TARGET, UP);
		glm::vec4 viewSpaceLightPos = lightView * glm::vec4(_lightPos, 1.0f);
		glm::vec4 viewSpaceNear = viewSpaceLightPos + glm::vec4(0.0f, 0.0f, NEAR_PLANE, 1.0f);
		glm::vec4 viewSpaceFar = viewSpaceLightPos + glm::vec4(0.0f, 0.0f, FAR_PLANE, 1.0f);
		glm::mat4 viewInverse = glm::inverse(lightView);
		float nearDistance = glm::distance(glm::vec3(viewInverse * viewSpaceNear), _lightPos);
		float farDistance = glm::distance(glm::vec3(viewInverse * viewSpaceFar), _lightPos);

		glm::vec3 p0(_lightPos + ORTHO_PROJ_HALF_WIDTH * lightViewRight + ORTHO_PROJ_HALF_WIDTH * lightViewUp + nearDistance * lightViewForward);
		glm::vec3 p1(_lightPos - ORTHO_PROJ_HALF_WIDTH * lightViewRight + ORTHO_PROJ_HALF_WIDTH * lightViewUp + nearDistance * lightViewForward);
		glm::vec3 p2(_lightPos - ORTHO_PROJ_HALF_WIDTH * lightViewRight - ORTHO_PROJ_HALF_WIDTH * lightViewUp + nearDistance * lightViewForward);
		glm::vec3 p3(_lightPos + ORTHO_PROJ_HALF_WIDTH * lightViewRight - ORTHO_PROJ_HALF_WIDTH * lightViewUp + nearDistance * lightViewForward);
		glm::vec3 p4(_lightPos + ORTHO_PROJ_HALF_WIDTH * lightViewRight + ORTHO_PROJ_HALF_WIDTH * lightViewUp + farDistance * lightViewForward);
		glm::vec3 p5(_lightPos - ORTHO_PROJ_HALF_WIDTH * lightViewRight + ORTHO_PROJ_HALF_WIDTH * lightViewUp + farDistance * lightViewForward);
		glm::vec3 p6(_lightPos - ORTHO_PROJ_HALF_WIDTH * lightViewRight - ORTHO_PROJ_HALF_WIDTH * lightViewUp + farDistance * lightViewForward);
		glm::vec3 p7(_lightPos + ORTHO_PROJ_HALF_WIDTH * lightViewRight - ORTHO_PROJ_HALF_WIDTH * lightViewUp + farDistance * lightViewForward);

		GLfloat vertices[] = {
			p0.x, p0.y, p0.z,
			p1.x, p1.y, p1.z,
			p1.x, p1.y, p1.z,
			p2.x, p2.y, p2.z,
			p2.x, p2.y, p2.z,
			p3.x, p3.y, p3.z,
			p3.x, p3.y, p3.z,
			p0.x, p0.y, p0.z,
			p4.x, p4.y, p4.z,
			p5.x, p5.y, p5.z,
			p5.x, p5.y, p5.z,
			p6.x, p6.y, p6.z,
			p6.x, p6.y, p6.z,
			p7.x, p7.y, p7.z,
			p7.x, p7.y, p7.z,
			p4.x, p4.y, p4.z,
			p0.x, p0.y, p0.z,
			p4.x, p4.y, p4.z,
			p1.x, p1.y, p1.z,
			p5.x, p5.y, p5.z,
			p2.x, p2.y, p2.z,
			p6.x, p6.y, p6.z,
			p3.x, p3.y, p3.z,
			p7.x, p7.y, p7.z
		};

		if (_debugLinesVAO == 0)
		{
			glGenVertexArrays(1, &_debugLinesVAO);
			glGenBuffers(1, &_debugLinesVBO);

			glBindBuffer(GL_ARRAY_BUFFER, _debugLinesVBO);
			glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

			glBindVertexArray(_debugLinesVAO);
			glEnableVertexAttribArray(0);
			glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
			glBindVertexArray(0);
		}
		else
		{
			glBindBuffer(GL_ARRAY_BUFFER, _debugLinesVBO);
			glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
		}

		glBindVertexArray(_debugLinesVAO);
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		glDrawArrays(GL_LINES, 0, 24);
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
		glBindVertexArray(0);
	}

	void ShadowCommon::setLightSpaceVP(const Shader& shader, const glm::vec3& lightPos, const glm::vec3& lightDirection)
	{
		shader.use();
		glm::mat4 lightProjection, lightView, lightSpaceVP;
		lightProjection = glm::ortho(-ORTHO_PROJ_HALF_WIDTH, ORTHO_PROJ_HALF_WIDTH, -ORTHO_PROJ_HALF_WIDTH, ORTHO_PROJ_HALF_WIDTH, NEAR_PLANE, FAR_PLANE);
		lightView = glm::lookAt(lightPos, lightDirection, UP);
		lightSpaceVP = lightProjection * lightView;
		shader.setMat4(G_LIGHT_SPACE_VP, lightSpaceVP);
	}
}
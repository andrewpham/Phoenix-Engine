#pragma once
#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <engine/mesh.h>
#include <engine/camera.h>

namespace phoenix
{
	class Utils
	{
	public:
		glm::mat4 _projection, _view;

		float _deltaTime = 0.0f, _lastTimestamp = 0.0f;

		void renderPlane(const Shader&, const glm::vec3& = glm::vec3(0.0f, 0.0f, 0.0f));
		void renderQuad(const Shader&, unsigned int = 0);
		void renderSphere();
		void renderCube();
		static Mesh* createQuad();
		static unsigned int loadTexture(char const*);
		virtual void processInput(GLFWwindow*, Camera*);

	private:
		unsigned int _planeVAO = 0, _quadVAO = 0, _sphereVAO = 0, _cubeVAO = 0, _numIndices;
	};
}
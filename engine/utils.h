#pragma once
#include <engine/shader.h>

namespace phoenix
{
	class Utils
	{
	public:
		glm::mat4 _projection;
		glm::mat4 _view;

		float _deltaTime = 0.0f;
		float _lastTimestamp = 0.0f;

		void renderPlane(const phoenix::Shader&);
		void renderQuad(const phoenix::Shader&, unsigned int);
		unsigned int loadTexture(char const*);

	private:
		unsigned int _planeVAO = 0;
		unsigned int _quadVAO = 0;
	};
}
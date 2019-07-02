#pragma once
#include <engine/shader.h>

namespace phoenix
{
	struct Light
	{
		glm::vec3 _color = glm::vec3(1.0f);
		float _intensity = 1.0f;
	};

	struct PointLight : public Light
	{
		glm::vec3 _position = glm::vec3(0.0f);
		struct
		{
			float _constant = 1.0f, _linear = 0.0f, _quadratic = 1.0f;
		} _attenuation;

		void setUniforms(const Shader&);
		void setUniforms(const Shader&, unsigned int);
	};
}
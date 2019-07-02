#pragma once
#include <engine/shader.h>

namespace phoenix
{
	struct Material
	{
		glm::vec3 _diffuseColor, _specularColor;
		float _specularReflectivity = 0.5f, _diffuseReflectivity = 1.0f, _emissivity = 0.0f, _aperture = 1.57f;

		Material(const glm::vec3 diffuseColor = glm::vec3(1.0f))
			: _diffuseColor(diffuseColor), _specularColor(diffuseColor) {}

		void setUniforms(const Shader&);

		static Material* defaultMaterial()
		{
			return new Material();
		}
		static Material* white()
		{
			return new Material(glm::vec3(0.97f, 0.97f, 0.97f));
		}
		static Material* red()
		{
			return new Material(glm::vec3(1.0f, 0.26f, 0.27f));
		}
		static Material* blue()
		{
			return new Material(glm::vec3(0.35f, 0.38f, 1.0f));
		}
	};
}
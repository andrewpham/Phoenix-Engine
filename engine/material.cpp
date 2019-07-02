#include <engine/material.h>

namespace phoenix
{
	void Material::setUniforms(const Shader& shader)
	{
		shader.use();

		shader.setVec3("gMaterial._diffuseColor", _diffuseColor);
		shader.setVec3("gMaterial._specularColor", _specularColor);

		shader.setFloat("gMaterial._specularReflectivity", _specularReflectivity);
		shader.setFloat("gMaterial._diffuseReflectivity", _diffuseReflectivity);
		shader.setFloat("gMaterial._emissivity", _emissivity);
		shader.setFloat("gMaterial._aperture", _aperture);
	}
}
#include <engine/light.h>

namespace phoenix
{
	void PointLight::setUniforms(const Shader& shader)
	{
		shader.setVec3("gPointLight._position", _position);
		shader.setVec3("gPointLight._light._color", _color);
		shader.setFloat("gPointLight._light._intensity", _intensity);
		shader.setFloat("gPointLight._attenuation._constant", _attenuation._constant);
		shader.setFloat("gPointLight._attenuation._linear", _attenuation._linear);
		shader.setFloat("gPointLight._attenuation._quadratic", _attenuation._quadratic);
	}

	void PointLight::setUniforms(const Shader& shader, unsigned int index)
	{
		shader.setVec3("gPointLights[" + std::to_string(index) + "]._position", _position);
		shader.setVec3("gPointLights[" + std::to_string(index) + "]._light._color", _color);
		shader.setFloat("gPointLights[" + std::to_string(index) + "]._light._intensity", _intensity);
		shader.setFloat("gPointLights[" + std::to_string(index) + "]._attenuation._constant", _attenuation._constant);
		shader.setFloat("gPointLights[" + std::to_string(index) + "]._attenuation._linear", _attenuation._linear);
		shader.setFloat("gPointLights[" + std::to_string(index) + "]._attenuation._quadratic", _attenuation._quadratic);
	}

	void DirectLight::setUniforms(const Shader& shader)
	{
		shader.setVec3("gDirectLight._direction", _direction);
		shader.setVec3("gDirectLight._light._color", _color);
		shader.setFloat("gDirectLight._light._intensity", _intensity);
	}
}
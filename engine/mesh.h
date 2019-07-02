#pragma once
#include <glm/glm.hpp>

#include <engine/shader.h>
#include <engine/material.h>

#include <vector>

namespace phoenix
{
	enum TextureType
	{
		NONE,
		DIFFUSE,
		SPECULAR,
		AMBIENT,
		HEIGHT,
		OPACITY
	};

	struct Vertex
	{
		glm::vec3 _position;
		glm::vec3 _normal;
		glm::vec2 _texCoords;
		glm::vec3 _tangent;
		glm::vec3 _bitangent;
	};

	struct Texture
	{
		unsigned int _ID;
		TextureType _textureType;
		std::string _key;
	};

	class Mesh
	{
	public:
		float _rotation = glm::radians(0.0f);
		glm::vec3 _translation = glm::vec3(0.0f), _rotationAxis = glm::vec3(0.0f, 1.0f, 0.0f), _scale = glm::vec3(1.0f);
		Material* _material = nullptr;

		Mesh(const std::vector<Vertex>&, const std::vector<unsigned int>&, const std::vector<Texture> & = {});

		void render();
		void render(const Shader&);

		~Mesh();

	private:
		unsigned int _VAO, _numIndices;
		std::vector<Texture> _textures;
	};
}
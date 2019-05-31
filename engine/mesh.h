#pragma once
#include <glm/glm.hpp>

#include <vector>

namespace phoenix
{
	struct Vertex
	{
		glm::vec3 _position;
		glm::vec3 _normal;
		glm::vec2 _texCoords;
		glm::vec3 _tangent;
		glm::vec3 _bitangent;
	};

	class Mesh
	{
	public:
		Mesh(const std::vector<Vertex>&, const std::vector<unsigned int>&);

		void render();

	private:
		unsigned int _VAO, _numIndices;
	};
}
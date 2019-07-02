#include <engine/mesh.h>
#include <engine/strings.h>
#include <glad/glad.h>

namespace phoenix
{
	Mesh::Mesh(const std::vector<Vertex>& vertices, const std::vector<unsigned int>& indices, const std::vector<Texture>& textures) : _numIndices(indices.size()), _textures(textures)
	{
		glGenVertexArrays(1, &_VAO);
		unsigned int VBO, EBO;
		glGenBuffers(1, &VBO);
		glGenBuffers(1, &EBO);

		glBindVertexArray(_VAO);

		glBindBuffer(GL_ARRAY_BUFFER, VBO);
		glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), &vertices[0], GL_STATIC_DRAW);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, _numIndices * sizeof(unsigned int), &indices[0], GL_STATIC_DRAW);

		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
		glEnableVertexAttribArray(2);
		glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, _normal));
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, _texCoords));
		glEnableVertexAttribArray(3);
		glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, _tangent));
		glEnableVertexAttribArray(4);
		glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, _bitangent));
		glBindVertexArray(0);
	}

	void Mesh::render()
	{
		glBindVertexArray(_VAO);
		glDrawElements(GL_TRIANGLES, _numIndices, GL_UNSIGNED_INT, 0);
		glBindVertexArray(0);
	}

	void Mesh::render(const Shader& shader)
	{
		unsigned int numDiffuseMaps = 0;
		unsigned int numSpecularMaps = 0;
		unsigned int numAmbientMaps = 0;
		unsigned int numBumpMaps = 0;
		unsigned int numOpacityMaps = 0;
		for (size_t i = 0; i < _textures.size(); ++i)
		{
			glActiveTexture(GL_TEXTURE0 + i);

			std::string name;
			switch (_textures[i]._textureType)
			{
			case DIFFUSE:
				name = G_DIFFUSE_MAP + std::to_string(numDiffuseMaps++);
				break;
			case SPECULAR:
				name = G_SPECULAR_MAP + std::to_string(numSpecularMaps++);
				break;
			case AMBIENT:
				name = G_AMBIENT_MAP + std::to_string(numAmbientMaps++);
				break;
			case HEIGHT:
				name = G_BUMP_MAP + std::to_string(numBumpMaps++);
				break;
			case OPACITY:
				name = G_OPACITY_MAP + std::to_string(numOpacityMaps++);
				break;
			}

			glUniform1i(glGetUniformLocation(shader._program, name.c_str()), i);
			glBindTexture(GL_TEXTURE_2D, _textures[i]._ID);
		}

		render();
	}

	Mesh::~Mesh()
	{
		glDeleteVertexArrays(1, &_VAO);
		if (_material)
		{
			delete _material;
		}
	}
}
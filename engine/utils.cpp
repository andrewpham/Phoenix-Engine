#include <engine/utils.h>
#include <engine/strings.h>
#include <engine/stb_image.h>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>
#include <vector>

namespace phoenix
{
	void Utils::renderPlane(const Shader& shader, const glm::vec3& translation)
	{
		shader.use();
		glm::mat4 world = glm::mat4(1.0f);
		world = glm::translate(world, translation);
		shader.setMat4(G_WVP, _projection * _view * world);
		shader.setMat4(G_WORLD_MATRIX, world);
		shader.setMat3(G_NORMAL_MATRIX, glm::transpose(glm::inverse(glm::mat3(world))));

		if (_planeVAO == 0)
		{
			float vertices[] = {
				// Positions            // Normals         // UVs
				 25.0f, -0.5f,  25.0f,  0.0f, 1.0f, 0.0f,  25.0f,  0.0f,
				-25.0f, -0.5f,  25.0f,  0.0f, 1.0f, 0.0f,   0.0f,  0.0f,
				-25.0f, -0.5f, -25.0f,  0.0f, 1.0f, 0.0f,   0.0f, 25.0f,

				 25.0f, -0.5f,  25.0f,  0.0f, 1.0f, 0.0f,  25.0f,  0.0f,
				-25.0f, -0.5f, -25.0f,  0.0f, 1.0f, 0.0f,   0.0f, 25.0f,
				 25.0f, -0.5f, -25.0f,  0.0f, 1.0f, 0.0f,  25.0f, 25.0f
			};

			glGenVertexArrays(1, &_planeVAO);
			unsigned int planeVBO;
			glGenBuffers(1, &planeVBO);

			glBindBuffer(GL_ARRAY_BUFFER, planeVBO);
			glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

			glBindVertexArray(_planeVAO);
			glEnableVertexAttribArray(0);
			glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
			glEnableVertexAttribArray(2);
			glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
			glEnableVertexAttribArray(1);
			glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
			glBindVertexArray(0);
		}

		glBindVertexArray(_planeVAO);
		glDrawArrays(GL_TRIANGLES, 0, 6);
		glBindVertexArray(0);
	}

	void Utils::renderQuad(const Shader& shader, unsigned int texture)
	{
		shader.use();

		if (texture)
		{
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, texture);
		}

		if (_quadVAO == 0)
		{
			float vertices[] = {
				// Positions        // UVs
				-1.0f,  1.0f, 0.0f, 0.0f, 1.0f,
				-1.0f, -1.0f, 0.0f, 0.0f, 0.0f,
				 1.0f,  1.0f, 0.0f, 1.0f, 1.0f,
				 1.0f, -1.0f, 0.0f, 1.0f, 0.0f,
			};

			glGenVertexArrays(1, &_quadVAO);
			unsigned int quadVBO;
			glGenBuffers(1, &quadVBO);

			glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
			glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

			glBindVertexArray(_quadVAO);
			glEnableVertexAttribArray(0);
			glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
			glEnableVertexAttribArray(1);
			glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
			glBindVertexArray(0);
		}

		glBindVertexArray(_quadVAO);
		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
		glBindVertexArray(0);
	}

	void Utils::renderSphere()
	{
		if (_sphereVAO == 0)
		{
			glGenVertexArrays(1, &_sphereVAO);
			unsigned int sphereVBO, sphereEBO;
			glGenBuffers(1, &sphereVBO);
			glGenBuffers(1, &sphereEBO);

			std::vector<glm::vec3> positions;
			std::vector<glm::vec2> texCoords;
			std::vector<glm::vec3> normals;
			std::vector<float> vertices;
			std::vector<unsigned int> indices;

			const int NUM_SEGMENTS = 64;
			for (size_t j = 0; j <= NUM_SEGMENTS; ++j)
			{
				for (size_t i = 0; i <= NUM_SEGMENTS; ++i)
				{
					float u = static_cast<float>(i) / NUM_SEGMENTS;
					float v = static_cast<float>(j) / NUM_SEGMENTS;
					float x = cos(u * 2.0f * glm::pi<float>()) * sin(v * glm::pi<float>());
					float y = cos(v * glm::pi<float>());
					float z = sin(u * 2.0f * glm::pi<float>()) * sin(v * glm::pi<float>());

					positions.push_back(glm::vec3(x, y, z));
					texCoords.push_back(glm::vec2(u, v));
					normals.push_back(glm::vec3(x, y, z));
				}
			}

			for (size_t i = 0; i < positions.size(); ++i)
			{
				vertices.push_back(positions[i].x);
				vertices.push_back(positions[i].y);
				vertices.push_back(positions[i].z);
				if (!texCoords.empty())
				{
					vertices.push_back(texCoords[i].x);
					vertices.push_back(texCoords[i].y);
				}
				if (!normals.empty())
				{
					vertices.push_back(normals[i].x);
					vertices.push_back(normals[i].y);
					vertices.push_back(normals[i].z);
				}
			}

			bool oddRow = false;
			for (size_t j = 0; j < NUM_SEGMENTS; ++j)
			{
				if (!oddRow)
				{
					for (size_t i = 0; i <= NUM_SEGMENTS; ++i)
					{
						indices.push_back(j * (NUM_SEGMENTS + 1) + i);
						indices.push_back((j + 1) * (NUM_SEGMENTS + 1) + i);
					}
				}
				else
				{
					for (int i = NUM_SEGMENTS; i >= 0; --i)
					{
						indices.push_back((j + 1) * (NUM_SEGMENTS + 1) + i);
						indices.push_back(j * (NUM_SEGMENTS + 1) + i);
					}
				}
				oddRow = !oddRow;
			}
			_numIndices = indices.size();

			glBindVertexArray(_sphereVAO);

			glBindBuffer(GL_ARRAY_BUFFER, sphereVBO);
			glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), &vertices[0], GL_STATIC_DRAW);
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, sphereEBO);
			glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), &indices[0], GL_STATIC_DRAW);

			glEnableVertexAttribArray(0);
			glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
			glEnableVertexAttribArray(1);
			glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
			glEnableVertexAttribArray(2);
			glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(5 * sizeof(float)));
			glBindVertexArray(0);
		}

		glBindVertexArray(_sphereVAO);
		glDrawElements(GL_TRIANGLE_STRIP, _numIndices, GL_UNSIGNED_INT, 0);
	}

	void Utils::renderCube()
	{
		if (_cubeVAO == 0)
		{
			float vertices[] = {
				// Back face
				-1.0f, -1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 0.0f, 0.0f, // Bottom-left
				 1.0f,  1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 1.0f, 1.0f, // Top-right
				 1.0f, -1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 1.0f, 0.0f, // Bottom-right
				 1.0f,  1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 1.0f, 1.0f, // Top-right
				-1.0f, -1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 0.0f, 0.0f, // Bottom-left
				-1.0f,  1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 0.0f, 1.0f, // Top-left
				// Front face
				-1.0f, -1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f, 0.0f, // Bottom-left
				 1.0f, -1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f, 0.0f, // Bottom-right
				 1.0f,  1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f, 1.0f, // Top-right
				 1.0f,  1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f, 1.0f, // Top-right
				-1.0f,  1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f, 1.0f, // Top-left
				-1.0f, -1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f, 0.0f, // Bottom-left
				// Left face
				-1.0f,  1.0f,  1.0f, -1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // Top-right
				-1.0f,  1.0f, -1.0f, -1.0f,  0.0f,  0.0f, 1.0f, 1.0f, // Top-left
				-1.0f, -1.0f, -1.0f, -1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // Bottom-left
				-1.0f, -1.0f, -1.0f, -1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // Bottom-left
				-1.0f, -1.0f,  1.0f, -1.0f,  0.0f,  0.0f, 0.0f, 0.0f, // Bottom-right
				-1.0f,  1.0f,  1.0f, -1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // Top-right
				// Right face
				 1.0f,  1.0f,  1.0f,  1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // Top-left
				 1.0f, -1.0f, -1.0f,  1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // Bottom-right
				 1.0f,  1.0f, -1.0f,  1.0f,  0.0f,  0.0f, 1.0f, 1.0f, // Top-right
				 1.0f, -1.0f, -1.0f,  1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // Bottom-right
				 1.0f,  1.0f,  1.0f,  1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // Top-left
				 1.0f, -1.0f,  1.0f,  1.0f,  0.0f,  0.0f, 0.0f, 0.0f, // Bottom-left
				// Bottom face
				-1.0f, -1.0f, -1.0f,  0.0f, -1.0f,  0.0f, 0.0f, 1.0f, // Top-right
				 1.0f, -1.0f, -1.0f,  0.0f, -1.0f,  0.0f, 1.0f, 1.0f, // Top-left
				 1.0f, -1.0f,  1.0f,  0.0f, -1.0f,  0.0f, 1.0f, 0.0f, // Bottom-left
				 1.0f, -1.0f,  1.0f,  0.0f, -1.0f,  0.0f, 1.0f, 0.0f, // Bottom-left
				-1.0f, -1.0f,  1.0f,  0.0f, -1.0f,  0.0f, 0.0f, 0.0f, // Bottom-right
				-1.0f, -1.0f, -1.0f,  0.0f, -1.0f,  0.0f, 0.0f, 1.0f, // Top-right
				// Top face
				-1.0f,  1.0f, -1.0f,  0.0f,  1.0f,  0.0f, 0.0f, 1.0f, // Top-left
				 1.0f,  1.0f , 1.0f,  0.0f,  1.0f,  0.0f, 1.0f, 0.0f, // Bottom-right
				 1.0f,  1.0f, -1.0f,  0.0f,  1.0f,  0.0f, 1.0f, 1.0f, // Top-right
				 1.0f,  1.0f,  1.0f,  0.0f,  1.0f,  0.0f, 1.0f, 0.0f, // Bottom-right
				-1.0f,  1.0f, -1.0f,  0.0f,  1.0f,  0.0f, 0.0f, 1.0f, // Top-left
				-1.0f,  1.0f,  1.0f,  0.0f,  1.0f,  0.0f, 0.0f, 0.0f  // Bottom-left
			};

			glGenVertexArrays(1, &_cubeVAO);
			unsigned int cubeVBO = 0;
			glGenBuffers(1, &cubeVBO);

			glBindBuffer(GL_ARRAY_BUFFER, cubeVBO);
			glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

			glBindVertexArray(_cubeVAO);
			glEnableVertexAttribArray(0);
			glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
			glEnableVertexAttribArray(1);
			glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
			glEnableVertexAttribArray(2);
			glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
			glBindBuffer(GL_ARRAY_BUFFER, 0);
			glBindVertexArray(0);
		}

		glBindVertexArray(_cubeVAO);
		glDrawArrays(GL_TRIANGLES, 0, 36);
		glBindVertexArray(0);
	}

	Mesh* Utils::createQuad()
	{
		std::vector<Vertex> vertices;
		Vertex v;

		v._position = { -1, -1, 1 };
		v._texCoords = { 0, 1 };
		v._normal = { 0, 0, 1 };
		vertices.emplace_back(v);

		v._position = { 1, -1, 1 };
		v._texCoords = { 0, 0 };
		v._normal = { 0, 0, 1 };
		vertices.emplace_back(v);

		v._position = { 1, 1, 1 };
		v._texCoords = { 1, 1 };
		v._normal = { 0, 0, 1 };
		vertices.emplace_back(v);

		v._position = { -1, 1, 1 };
		v._texCoords = { 1 , 0 };
		v._normal = { 0, 0, 1 };
		vertices.emplace_back(v);

		const std::vector<unsigned int> indices{ { 0, 1, 2, 0, 2, 3 } };

		return new Mesh(vertices, indices);
	}

	unsigned int Utils::loadTexture(char const* filename)
	{
		unsigned int textureID = -1;

		int width, height, n;
		unsigned char* data = stbi_load(filename, &width, &height, &n, 0);
		if (data)
		{
			GLenum format;
			if (n == 1)
			{
				format = GL_RED;
			}
			else if (n == 2)
			{
				format = GL_RG;
			}
			else if (n == 3)
			{
				format = GL_RGB;
			}
			else if (n == 4)
			{
				format = GL_RGBA;
			}

			glGenTextures(1, &textureID);
			glBindTexture(GL_TEXTURE_2D, textureID);
			glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
			glGenerateMipmap(GL_TEXTURE_2D);

			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		}
		else
		{
			std::cout << "Failed to load file: " << filename << "\n";
		}
		stbi_image_free(data);

		return textureID;
	}

	void Utils::processInput(GLFWwindow* window, Camera* camera, bool isRH)
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
			camera->processKeyPress(isRH? BACKWARD : FORWARD, _deltaTime);
		}
		if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
		{
			camera->processKeyPress(phoenix::LEFT, _deltaTime);
		}
		if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
		{
			camera->processKeyPress(phoenix::RIGHT, _deltaTime);
		}
	}
}
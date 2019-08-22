#include <glad/glad.h>
#include <glm/gtc/matrix_transform.hpp>

#include <engine/sh.h>
#include <engine/common.h>

namespace phoenix
{
	glm::vec3 mapUVSToN(float u, float v, size_t s, int resolution)
	{
		glm::vec3 N = glm::vec3(0.0f);

		switch (s)
		{
		case 0:
			N = glm::normalize(glm::vec3(1.0f, v, -u));
			break;
		case 1:
			N = glm::normalize(glm::vec3(-1.0f, v, u));
			break;
		case 2:
			N = glm::normalize(glm::vec3(u, 1.0f, -v));
			break;
		case 3:
			N = glm::normalize(glm::vec3(u, -1.0f, v));
			break;
		case 4:
			N = glm::normalize(glm::vec3(u, v, 1.0f));
			break;
		case 5:
			N = glm::normalize(glm::vec3(-u, v, -1.0f));
			break;
		}

		return N;
	}

	SH9 genSHCoefficients(const glm::vec3& N)
	{
		SH9 result;

		// Band 0
		result[0] = 0.282095f;

		// Band 1
		result[1] = 0.488603f * N.y;
		result[2] = 0.488603f * N.z;
		result[3] = 0.488603f * N.x;

		// Band 2
		result[4] = 1.092548f * N.x * N.y;
		result[5] = 1.092548f * N.y * N.z;
		result[6] = 0.315392f * (3.0f * N.z * N.z - 1.0f);
		result[7] = 1.092548f * N.x * N.z;
		result[8] = 0.546274f * (N.x * N.x - N.y * N.y);

		return result;
	}

	// L_l_m = Sum of All L(theta, phi) * Y_l_m(theta, phi) = Sum of All L(N) * Y_l_m(N)
	SH9Color genLightingCoefficientsForNormal(const glm::vec3& N, const glm::vec3& L)
	{
		SH9 SHCoefficients = genSHCoefficients(N);
		SH9Color result;
		for (size_t i = 0; i < result._coefficients.size(); ++i)
		{
			result[i] = L * SHCoefficients[i];
		}
		return result;
	}

	SH9Color genLightingCoefficients(unsigned int texture, int resolution)
	{
		SH9Color result;
		for (size_t i = 0; i < result._coefficients.size(); ++i)
		{
			result[i] = glm::vec3(0.0f);
		}

		float u, v, temp, weight, weightSum = 0.0f;
		size_t row, col;
		glm::vec3 L;

		float* img = new float[3 * resolution * resolution];
		glBindTexture(GL_TEXTURE_CUBE_MAP, texture);
		for (size_t s = 0; s < NUM_CUBEMAP_FACES; ++s)
		{
			glGetTexImage(GL_TEXTURE_CUBE_MAP_POSITIVE_X + s, 0, GL_RGB, GL_FLOAT, img);
			for (size_t y = 0; y < resolution; ++y)
			{
				for (size_t x = 0; x < resolution; ++x)
				{
					row = 3 * resolution * y;
					col = 3 * x;
					L = glm::vec3(img[row + col], img[row + col + 1], img[row + col + 2]);

					u = (x + 0.5f) / resolution;
					v = (y + 0.5f) / resolution;
					u = u * 2.0f - 1.0f;
					v = v * 2.0f - 1.0f;

					temp = 1.0f + u * u + v * v;
					weight = 4.0f / (sqrt(temp) * temp);

					glm::vec3 N = mapUVSToN(u, v, s, resolution);
					result += genLightingCoefficientsForNormal(N, L) * weight;
					weightSum += weight;
				}
			}
		}
		delete img;

		result *= 4.0f * glm::pi<float>() / weightSum;
		return result;
	}
}
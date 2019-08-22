#pragma once
#include <array>

namespace phoenix
{
	template<typename T>
	class SH
	{
	public:
		std::array<T, 9> _coefficients;

		T& operator[](size_t idx)
		{
			return _coefficients[idx];
		}

		SH& operator+=(const SH& other)
		{
			for (size_t i = 0; i < _coefficients.size(); ++i)
			{
				_coefficients[i] += other._coefficients[i];
			}
			return *this;
		}

		SH& operator*=(const float& scale)
		{
			for (size_t i = 0; i < _coefficients.size(); ++i)
			{
				_coefficients[i] *= scale;
			}
			return *this;
		}

		SH operator*(const float& scale) const
		{
			SH result;
			for (size_t i = 0; i < _coefficients.size(); ++i)
			{
				result._coefficients[i] = _coefficients[i] * scale;
			}
			return result;
		}
	};

	typedef SH<float> SH9;
	typedef SH<glm::vec3> SH9Color;

	glm::vec3 mapUVSToN(float, float, size_t, int);
	SH9 genSHCoefficients(const glm::vec3&);
	SH9Color genLightingCoefficientsForNormal(const glm::vec3&, const glm::vec3&);

	SH9Color genLightingCoefficients(unsigned int, int); // By sampling from the environment map
}


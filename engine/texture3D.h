#pragma once
#include <engine/shader.h>

#include <vector>
#include <array>

namespace phoenix
{
	struct Texture3D
	{
		unsigned int _textureID;

		Texture3D(const std::vector<float>&, int, int, int, bool);

		void bind(const Shader&, const std::string&, int);
		void clear(const std::array<float, 4>&);
	};
}
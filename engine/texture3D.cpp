#include <engine/texture3D.h>

namespace phoenix
{
	Texture3D::Texture3D(const std::vector<float>& data, int width, int height, int depth, bool generateMipmaps)
	{
		glGenTextures(1, &_textureID);
		glBindTexture(GL_TEXTURE_3D, _textureID);
		glTexStorage3D(GL_TEXTURE_3D, 7, GL_RGBA8, width, height, depth);
		glTexImage3D(GL_TEXTURE_3D, 0, GL_RGBA8, width, height, depth, 0, GL_RGBA, GL_FLOAT, &data[0]);
		if (generateMipmaps)
		{
			glGenerateMipmap(GL_TEXTURE_3D);
		}

		glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
		glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
		glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_BORDER);
		glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

		glBindTexture(GL_TEXTURE_3D, 0);
	}

	void Texture3D::bind(const Shader& shader, const std::string& name, int textureUnit)
	{
		glActiveTexture(GL_TEXTURE0 + textureUnit);
		glBindTexture(GL_TEXTURE_3D, _textureID);
		shader.setInt(name, textureUnit);
	}

	void Texture3D::clear(const std::array<float, 4>& clearColor)
	{
		int previousTextureID;
		glGetIntegerv(GL_TEXTURE_BINDING_3D, &previousTextureID);
		glBindTexture(GL_TEXTURE_3D, _textureID);
		glClearTexImage(_textureID, 0, GL_RGBA, GL_FLOAT, &clearColor);
		glBindTexture(GL_TEXTURE_3D, previousTextureID);
	}
}
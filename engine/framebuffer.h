#pragma once
#include <engine/shader.h>

namespace phoenix
{
	class Framebuffer
	{
	public:
		unsigned int _width, _height, _FBO, _textureID;

		Framebuffer(unsigned int, unsigned int, bool = false, bool = false);

		void bindTexture(const Shader&, const std::string&, int = GL_TEXTURE0);
		unsigned int genAttachment(GLenum, GLenum, GLenum, bool = false, GLenum = GL_NEAREST, GLenum = GL_NEAREST);

		~Framebuffer();

	private:
		unsigned int _RBO, _numAttachments = 0;
		int _previousFBO;

		void genColorMemoryAttachment(bool = false, bool = false);
		void setZBufferMemoryAttachment();
		void unbindFBOAndZBufferAttachment();
		void setupFramebuffer();
	};
}
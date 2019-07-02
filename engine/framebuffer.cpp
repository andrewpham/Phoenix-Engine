#include <engine/framebuffer.h>
#include <engine/strings.h>
#include <iostream>

namespace phoenix
{
	void Framebuffer::genColorMemoryAttachment()
	{
		glGenTextures(1, &_textureID);
		glBindTexture(GL_TEXTURE_2D, _textureID);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, _width, _height, 0, GL_RGBA, GL_FLOAT, nullptr);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	}

	void Framebuffer::setZBufferMemoryAttachment()
	{
		glBindFramebuffer(GL_FRAMEBUFFER, _FBO);
		glBindRenderbuffer(GL_RENDERBUFFER, _RBO);
		glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, _width, _height);
	}

	void Framebuffer::unbindFBOAndZBufferAttachment()
	{
		glBindFramebuffer(GL_FRAMEBUFFER, _previousFBO);
		glBindRenderbuffer(GL_RENDERBUFFER, 0);
		if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
		{
			std::cerr << phoenix::FRAMEBUFFER_INIT_ERROR;
		}
	}

	void Framebuffer::setupFramebuffer()
	{
		glGenFramebuffers(1, &_FBO); 
		glGenRenderbuffers(1, &_RBO);

		setZBufferMemoryAttachment();
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + _numAttachments++, GL_TEXTURE_2D, _textureID, 0);
		glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, _RBO);

		unbindFBOAndZBufferAttachment();
	}

	Framebuffer::Framebuffer(unsigned int width, unsigned int height) : _width(width), _height(height)
	{
		glGetIntegerv(GL_FRAMEBUFFER_BINDING, &_previousFBO);
		genColorMemoryAttachment();
		setupFramebuffer();
	}

	void Framebuffer::bindTexture(const Shader& shader, const std::string& name, int textureUnit)
	{
		glActiveTexture(GL_TEXTURE0 + textureUnit);
		glBindTexture(GL_TEXTURE_2D, _textureID);
		shader.setInt(name, textureUnit);
	}

	unsigned int Framebuffer::genAttachment(GLenum internalFormat, GLenum format, GLenum type, GLenum minFilter, GLenum magFilter)
	{
		unsigned int textureID;

		glGenTextures(1, &textureID);
		glBindTexture(GL_TEXTURE_2D, textureID);
		glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, _width, _height, 0, format, type, nullptr);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, minFilter);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, magFilter);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + _numAttachments++, GL_TEXTURE_2D, textureID, 0);

		return textureID;
	}

	Framebuffer::~Framebuffer()
	{
		glDeleteTextures(1, &_textureID);
		glDeleteFramebuffers(1, &_FBO);
	}
}
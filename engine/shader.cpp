#include <engine/shader.h>
#include <fstream>
#include <sstream>
#include <iostream>

namespace phoenix
{
	Shader::Shader(const char* vShaderFilename, const char* fShaderFilename)
	{
		std::string vShaderStr;
		std::string fShaderStr;
		std::ifstream vShaderFileStream;
		std::ifstream fShaderFileStream;
		vShaderFileStream.exceptions(std::ifstream::failbit | std::ifstream::badbit);
		fShaderFileStream.exceptions(std::ifstream::failbit | std::ifstream::badbit);

		try
		{
			vShaderFileStream.open(vShaderFilename);
			fShaderFileStream.open(fShaderFilename);

			std::stringstream vShaderStrStream, fShaderStrStream;
			vShaderStrStream << vShaderFileStream.rdbuf();
			fShaderStrStream << fShaderFileStream.rdbuf();

			vShaderFileStream.close();
			fShaderFileStream.close();

			vShaderStr = vShaderStrStream.str();
			fShaderStr = fShaderStrStream.str();
		}
		catch (std::ifstream::failure e)
		{
			std::cerr << "Exception opening/reading/closing file!\n";
		}

		const char* vShaderCode = vShaderStr.c_str();
		const char* fShaderCode = fShaderStr.c_str();

		unsigned int vShader, fShader;

		vShader = glCreateShader(GL_VERTEX_SHADER);
		glShaderSource(vShader, 1, &vShaderCode, nullptr);
		glCompileShader(vShader);
		checkCompileErrors(vShader, "vertex");

		fShader = glCreateShader(GL_FRAGMENT_SHADER);
		glShaderSource(fShader, 1, &fShaderCode, nullptr);
		glCompileShader(fShader);
		checkCompileErrors(fShader, "fragment");

		_program = glCreateProgram();
		glAttachShader(_program, vShader);
		glAttachShader(_program, fShader);
		glLinkProgram(_program);
		checkCompileErrors(_program, "program");

		glDeleteShader(vShader);
		glDeleteShader(fShader);
	}

	void Shader::checkCompileErrors(unsigned int shaderID, const std::string shaderType)
	{
		int success;
		char log[_MSG_LEN];

		if (shaderType != "program")
		{
			glGetShaderiv(shaderID, GL_COMPILE_STATUS, &success);
			if (!success)
			{
				glGetShaderInfoLog(shaderID, _MSG_LEN, nullptr, log);
				std::cerr << "Error compiling " << shaderType << " shader!\n" << log << "\n";
			}
		}
		else
		{
			glGetProgramiv(shaderID, GL_LINK_STATUS, &success);
			if (!success)
			{
				glGetProgramInfoLog(shaderID, _MSG_LEN, nullptr, log);
				std::cerr << "Error linking shaders!\n" << log << "\n";
			}
		}
	}
}